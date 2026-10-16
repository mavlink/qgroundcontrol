#include "GStreamer.h"

#include <algorithm>
#include <mutex>

#include <QtCore/QByteArrayList>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QVarLengthArray>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

#include "Fact.h"
#include "GStreamerEnvironment.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstScoped.h"
#include "GstVideoReceiver.h"
#include "HwBuffers/common/HwBuffers.h"
#if defined(QGC_HAS_ANY_GPU_PATH)
#include <rhi/qrhi.h>

#include "HwBuffers/common/QGCRhiCapture.h"
#endif
#include "QGCLoggingCategory.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/gstqgcvideosinkbin.h"

#include <gst/gst.h>

QGC_LOGGING_CATEGORY(GStreamerLog, "Video.GStreamer.GStreamer")

G_BEGIN_DECLS
#ifdef QGC_GST_STATIC_BUILD
// Generated from cmake/GStreamer/gst_static_plugins.c.in by qgc_gst_generate_static_init (StaticInit.cmake):
// registers every configured plugin and loads the static GIO modules (gioopenssl + the bundled CA bundle on mobile).
extern void gst_init_static_plugins(void);
#endif

GST_PLUGIN_STATIC_DECLARE(qgc);
G_END_DECLS

namespace GStreamer {

namespace {

void _registerPlugins()
{
    // GST_PLUGIN_STATIC_REGISTER / gst_init_static_plugins() are not idempotent: a second pass
    // re-registers GTypes ("cannot register existing type 'GstBaseQTMux'") and aborts, so run once.
    static std::once_flag s_pluginsRegistered;
    std::call_once(s_pluginsRegistered, [] {
#ifdef QGC_GST_STATIC_BUILD
        // The generated shim's registrations are registry-guarded: gst_init() may already have run it
        // (see cmake/GStreamer/gst_static_plugins.c.in).
        gst_init_static_plugins();
#endif
        GST_PLUGIN_STATIC_REGISTER(qgc);
    });
}

// plugin_init can fail silently; confirm the element factory is exposed so failures surface here
// rather than as a misleading "create returned nullptr" later. Common cause: iOS LTO / Android R8
// stripping the GST_ELEMENT_REGISTER side effect.
bool requireFactory(const char* name, const char* hint)
{
    const GstFactoryPtr factory = adoptFactory(gst_element_factory_find(name));
    if (!factory) {
        qCCritical(GStreamerLog) << name << "factory not found —" << hint;
        return false;
    }
    qCDebug(GStreamerLog) << name << "factory available";
    return true;
}

bool _verifyPlugins()
{
    GstRegistry* registry = gst_registry_get();
    if (!registry) {
        qCCritical(GStreamerLog) << "Failed to get GStreamer registry";
        return false;
    }

    qCDebug(GStreamerLog) << "Installed GStreamer plugins:";
    GStreamer::forEachPlugin(registry, [](GstPlugin* plugin) {
        qCDebug(GStreamerLog) << "  " << gst_plugin_get_name(plugin) << gst_plugin_get_version(plugin);
    });

    bool result = true;
    const auto hasPlugin = [registry](const char* name) {
        const GstObjectPtr plugin(GST_OBJECT(gst_registry_find_plugin(registry, name)));
        return plugin != nullptr;
    };
    // qgc plus GSTREAMER_RUNTIME_REQUIRED_PLUGINS (PluginPolicy.cmake), so a stripped registry fails loudly instead
    // of at the first stream attempt. Each entry lists, '|'-separated, the plugin sets that satisfy it, a set's
    // plugins joined by '+' (videoconvert+videoscale stand in for the fused videoconvertscale of 1.22+).
    static constexpr const char* kRequiredPlugins[] = {"qgc", QGC_GST_RUNTIME_REQUIRED_PLUGINS};
    for (const char* requirement : kRequiredPlugins) {
        const QList<QByteArray> alternatives = QByteArray(requirement).split('|');
        const bool satisfied = std::ranges::any_of(alternatives, [&hasPlugin](const QByteArray& alternative) {
            const QList<QByteArray> plugins = alternative.split('+');
            return std::ranges::all_of(
                plugins, [&hasPlugin](const QByteArray& plugin) { return hasPlugin(plugin.constData()); });
        });
        if (!satisfied) {
            qCCritical(GStreamerLog) << "Required GStreamer plugin not found:" << requirement;
            result = false;
        }
    }

    if (!result) {
        // Surface blacklisted plugins so a failure from a corrupt/incompatible plugin file
        // shows up here instead of looking like a missing-plugin problem.
        GStreamer::forEachPlugin(registry, [](GstPlugin* p) {
            const gchar* desc = gst_plugin_get_description(p);
            if (!desc || !g_str_has_prefix(desc, "BLACKLIST"))
                return;
            const gchar* filename = gst_plugin_get_filename(p);
            qCWarning(GStreamerLog) << "Blacklisted plugin:" << gst_plugin_get_name(p)
                                    << "file:" << (filename ? filename : "(null)");
        });

        // Path / scanner env vars belong to the environment layer that set them.
        Environment::logDiagnostics();
    }

    return result;
}

}  // anonymous namespace

Environment::ValidationResult prepareEnvironment()
{
    return Environment::prepareEnvironment();
}

namespace {

bool _initGstRuntime(const QStringList& args, const Environment::ValidationResult& env)
{
    if (!env.ok) {
        qCCritical(GStreamerLog) << "Invalid GStreamer environment configuration:" << env.error;
        return false;
    }

    // args is snapshotted on the GUI thread: QCoreApplication::arguments() is not thread-safe
    // and initialize() runs on a QtConcurrent pool thread.
    QByteArrayList argStorage;
    argStorage.reserve(args.size());
    for (const QString& arg : args) {
        argStorage.append(arg.toUtf8());
    }

    QVarLengthArray<char*, 16> argv;
    for (QByteArray& arg : argStorage) {
        argv.append(arg.data());
    }

    int argc = argv.size();
    char** argvPtr = argv.data();
    GError* error = nullptr;

    if (!gst_init_check(&argc, &argvPtr, &error)) {
        qCCritical(GStreamerLog) << "Failed to initialize GStreamer:" << (error ? error->message : "unknown error");
        g_clear_error(&error);
        return false;
    }

    return true;
}

}  // anonymous namespace

bool completeInit()
{
    if (!gst_is_initialized()) {
        qCCritical(GStreamerLog) << "completeInit called but gst_init() has not been called";
        return false;
    }

    GStreamer::configureDebugLogging();

    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);
    qCDebug(GStreamerLog) << "GStreamer initialized:" << major << "." << minor << "." << micro;

    if (major != GST_VERSION_MAJOR || minor != GST_VERSION_MINOR) {
        qCWarning(GStreamerLog) << "GStreamer version mismatch: built against" << GST_VERSION_MAJOR << "."
                                << GST_VERSION_MINOR << "but runtime is" << major << "." << minor << "." << micro;
    }

    _registerPlugins();

#ifdef Q_OS_IOS
    // Prefer applemedia-backed sources on iOS. Must run after _registerPlugins() (registry empty before).
    if (GstRegistry* reg = gst_registry_get()) {
        GStreamer::changeFeatureRank(reg, "filesrc", GST_RANK_SECONDARY);
        GStreamer::changeFeatureRank(reg, "giosrc", GST_RANK_SECONDARY - 1);
    }
#endif

    if (!_verifyPlugins()) {
        qCCritical(GStreamerLog) << "Plugin verification failed";
        return false;
    }

    GStreamer::logDecoderRanks();

    if (!requireFactory("qgcqvideosink", "sink bin will fail to construct")) {
        return false;
    }
    if (!requireFactory("qgcvideosinkbin",
                        "qgc plugin registered but element exposure failed. Likely link-time symbol "
                        "stripping (iOS LTO / Android R8) removed the GST_ELEMENT_REGISTER side effect; "
                        "add gstqgcelements.cc to a -force_load / keep rule.")) {
        return false;
    }

    if (GStreamer::didExternalPluginLoaderFail()) {
        qCCritical(GStreamerLog)
            << "GStreamer external plugin loader failed. Check GST_PLUGIN_SCANNER and bundled runtime paths.";
        return false;
    }

    return true;
}

bool initialize(const QStringList& arguments, const Environment::ValidationResult& envResult)
{
    GStreamer::resetExternalPluginLoaderFailure();
    GStreamer::redirectGLibLogging();

    // Suppress GStreamer's default stderr handler before gst_init_check() — raw ANSI codes
    // corrupt the terminal on macOS.
    gst_debug_remove_log_function(gst_debug_log_default);

    if (!_initGstRuntime(arguments, envResult)) {
        return false;
    }

    return completeInit();
}

// Video sink refcount protocol: createVideoSink returns one owned, non-floating ref, which the caller
// drops with releaseVideoSink. While decoding, the receiver holds its own ref and the pipeline bin
// takes another on gst_bin_add (dropped by gst_bin_remove). Keep the ref/unref sites balanced.
void* createVideoSink(const VideoSinkConfig& config)
{
    const gboolean disablePar = config.disablePixelAspectRatio ? TRUE : FALSE;
    const char* const conversion = config.conversionElement.isEmpty() ? nullptr : config.conversionElement.constData();

    const GstFactoryPtr factory = adoptFactory(gst_element_factory_find("qgcvideosinkbin"));
    if (!factory) {
        // completeInit verified this factory at startup; absence here means the registry changed underfoot.
        qCCritical(GStreamerLog) << "qgcvideosinkbin factory not found";
        return nullptr;
    }

#if defined(QGC_HAS_ANY_GPU_PATH)
    GstElement* videoSinkBin =
        gst_element_factory_create_full(factory.get(), "gpu-zerocopy", config.gpuZeroCopy ? TRUE : FALSE,
                                        "conversion-element", conversion, "disable-par", disablePar, NULL);
#else
    GstElement* videoSinkBin = gst_element_factory_create_full(factory.get(), "conversion-element", conversion,
                                                               "disable-par", disablePar, NULL);
#endif
    if (!videoSinkBin) {
        qCCritical(GStreamerLog) << "qgcvideosinkbin element creation failed";
        return nullptr;
    }
    // Sink the floating ref so adding the sink to a bin never consumes the caller's reference.
    return gst_object_ref_sink(videoSinkBin);
}

void releaseVideoSink(void* sink)
{
    if (!sink)
        return;
    GstElement* videoSink = GST_ELEMENT(sink);
    gst_clear_object(&videoSink);
}

VideoReceiver* createVideoReceiver(QObject* parent)
{
    return new GstVideoReceiver(parent);
}

bool setupQVideoSinkElement(void* sinkBin, QVideoSink* videoSink, QObject* controllerParent)
{
    if (!sinkBin || !videoSink || !controllerParent) {
        // controllerParent owns the controller's QObject lifetime — else it leaks (caller has no handle).
        qCWarning(GStreamerLog) << "setupQVideoSinkElement: null sinkBin, videoSink, or controllerParent";
        return false;
    }

    // Idempotent re-setup: tear down prior controllers so repeated startDecoding cycles
    // don't accumulate dangling ones.
    for (auto* c : QGCQVideoSinkController::controllersOf(controllerParent)) {
        c->setActive(false);
        // Stop the poll timer synchronously: deleteLater is deferred and the timer would otherwise
        // keep binding the same element while the new controller is being installed.
        c->prepareForRelease();
        c->deleteLater();
    }

    // Clear the GL/Vulkan bridges' exhausted-retry latches so a restart after Qt's RHI appears can
    // prime on the next NEED_CONTEXT.
    HwBuffers::onPipelineRestart();

    // Accessor returns a transfer-full ref; the guard releases it once the controller has taken
    // its own ref for deferred QObject teardown.
    const GstObjectPtr element(GST_OBJECT(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(sinkBin))));
    if (!element) {
        qCCritical(GStreamerLog) << "setupQVideoSinkElement: bin has no qgcqvideosink child";
        return false;
    }
    GstElement* const elementRaw = GST_ELEMENT(element.get());

    auto* controller = new QGCQVideoSinkController(elementRaw, controllerParent);

    // Route the initial install through the controller so its destroyed-sink QPointer guard
    // covers setup, not just later swaps.
    controller->setVideoSink(QPointer<QVideoSink>(videoSink));
    controller->setActive(true);

    return true;
}

void attachAppSink(QObject* receiver, void* sink, QQuickItem* widget)
{
    if (!sink || !widget || !receiver) {
        return;
    }

    auto* videoOutput = qobject_cast<QQuickVideoOutput*>(widget);
    if (!videoOutput) {
        qCWarning(GStreamerLog) << "Widget is not a VideoOutput, cannot connect qgcqvideosink";
        return;
    }

    QVideoSink* videoSink = videoOutput->videoSink();
    if (!setupQVideoSinkElement(sink, videoSink, receiver)) {
        qCWarning(GStreamerLog) << "setupQVideoSinkElement failed";
    }

    QGCQVideoSinkController::syncActiveToWindowVisibility(receiver, videoOutput);
}

void bindDebugLevelFact(Fact* fact, QObject* context)
{
    if (!fact || !context)
        return;
    QObject::connect(fact, &Fact::rawValueChanged, context,
                     [](const QVariant& value) { setDebugLevel(value.toInt()); });
}

static const char* graphicsApiName(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
        case QSGRendererInterface::Software:
            return "Software";
        case QSGRendererInterface::OpenGL:
            return "OpenGL";
        case QSGRendererInterface::Direct3D11:
            return "Direct3D11";
        case QSGRendererInterface::Direct3D12:
            return "Direct3D12";
        case QSGRendererInterface::Vulkan:
            return "Vulkan";
        case QSGRendererInterface::Metal:
            return "Metal";
        default:
            return "Unknown";
    }
}

// Zero-copy buffer family the resolved RHI backend enables, or "CPU" when no import path is compiled for it.
static const char* zeroCopyFamilyForApi(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
        case QSGRendererInterface::OpenGL:
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_DMABUF_GPU_PATH)
            return "GLMemory/DMABuf";
#else
            return "CPU";
#endif
        case QSGRendererInterface::Direct3D11:
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
            return "D3D11";
#else
            return "CPU";
#endif
        case QSGRendererInterface::Direct3D12:
            // GStreamer 1.28 can match adapter LUID but cannot wrap Qt's ID3D12Device; D3D12 zero-copy disabled until
            // same-device import is possible.
            return "CPU (D3D12 import disabled)";
        case QSGRendererInterface::Metal:
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
            return "IOSurface/VideoToolbox";
#else
            return "CPU";
#endif
        case QSGRendererInterface::Vulkan:
            // Vulkan import is dormant (foreign-VkDevice guard → CPU copy), so it never delivers zero-copy today.
            return "CPU (Vulkan import dormant)";
        default:
            return "CPU";
    }
}

void onMainWindowReady(QQuickWindow* window)
{
    HwBuffers::connectMainWindow(window);
    // Prefer the resolved backend (cachedRhi) over the configured API once the scene graph is up; QGCRhiCapture exists
    // only when a GPU path is compiled, hence the guard.
    QSGRendererInterface::GraphicsApi api = QQuickWindow::graphicsApi();
#if defined(QGC_HAS_ANY_GPU_PATH)
    if (QRhi* rhi = QGCRhiCapture::cachedRhi()) {
        switch (rhi->backend()) {
            case QRhi::OpenGLES2: api = QSGRendererInterface::OpenGL; break;
            case QRhi::D3D11:     api = QSGRendererInterface::Direct3D11; break;
            case QRhi::D3D12:     api = QSGRendererInterface::Direct3D12; break;
            case QRhi::Metal:     api = QSGRendererInterface::Metal; break;
            case QRhi::Vulkan:    api = QSGRendererInterface::Vulkan; break;
            default: break;
        }
    }
#endif
    qCInfo(GStreamerLog) << "Resolved RHI backend:" << graphicsApiName(api) << "→ zero-copy path:"
                         << zeroCopyFamilyForApi(api);
}

QList<VideoDecoderOptions> availableDecoderFamilies()
{
    QList<VideoDecoderOptions> families;
    forEachVideoDecoderFactory(GST_RANK_NONE, [&families](GstElementFactory* factory) {
        const VideoDecoderOptions family =
            decoderFamilyForFactory(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
        if ((family != ForceVideoDecoderDefault) && !families.contains(family)) {
            families.append(family);
        }
    });
    return families;
}

}  // namespace GStreamer
