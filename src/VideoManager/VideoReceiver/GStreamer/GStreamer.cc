#include "GStreamer.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#ifdef Q_OS_ANDROID
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#endif
#include <array>
#include <memory>
#include <mutex>
#include <utility>

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
#include "gstqgc/gstqgcqvideosink.h"
#include "gstqgc/gstqgcvideosinkbin.h"

#ifdef Q_OS_LINUX
#include <dlfcn.h>
#endif

#ifdef Q_OS_ANDROID
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

#include <gst/gst.h>

QGC_LOGGING_CATEGORY(GStreamerLog, "Video.GStreamer.GStreamer")

G_BEGIN_DECLS
#ifdef QGC_GST_STATIC_BUILD
// Generated from gst_static_plugins.c.in (Android/IOS.cmake on mobile, desktop static cmake):
// registers every configured plugin, and on mobile loads gioopenssl + the bundled CA bundle.
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
        // Per-plugin registers in the generated shim are registry-guarded, so plugins the Android SDK
        // gst_init() already pre-registered aren't re-added here.
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
    // Mirrors GSTREAMER_RUNTIME_REQUIRED_PLUGINS (PluginPolicy.cmake) plus qgc,
    // so a stripped registry fails loudly instead of at first stream attempt.
    static constexpr std::array<const char*, 12> kRequiredPlugins = {
        "qgc", "coreelements", "isomp4", "matroska", "multifile", "opengl",
        "playback", "rtp", "rtpmanager", "rtsp", "tcp", "udp",
    };
    for (const char* name : kRequiredPlugins) {
        if (!hasPlugin(name)) {
            qCCritical(GStreamerLog) << "Required GStreamer plugin not found:" << name;
            result = false;
        }
    }
    // GStreamer 1.22+ fuses videoconvert+videoscale into videoconvertscale; accept either.
    if (!hasPlugin("videoconvertscale") && !(hasPlugin("videoconvert") && hasPlugin("videoscale"))) {
        qCCritical(GStreamerLog) << "Required GStreamer plugin not found: videoconvertscale (or videoconvert+videoscale)";
        result = false;
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

#ifdef QGC_GST_BUILD_VERSION_MAJOR
    if (major != QGC_GST_BUILD_VERSION_MAJOR || minor != QGC_GST_BUILD_VERSION_MINOR) {
        qCWarning(GStreamerLog) << "GStreamer version mismatch: built against" << QGC_GST_BUILD_VERSION_MAJOR << "."
                                << QGC_GST_BUILD_VERSION_MINOR << "but runtime is" << major << "." << minor << "."
                                << micro;
    }
#endif

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

// Video sink refcount protocol: createVideoSink returns floating(1); ref on add to the pipeline,
// unref on removal; releaseVideoSink drops the last ref. Keep the ref/unref sites balanced.
void* createVideoSink(const VideoSinkConfig& config)
{
    // All bin tunables are construct-only — properties drive behavior, no env-var indirection.
    const gboolean disablePar = config.disablePixelAspectRatio ? TRUE : FALSE;
    const char* const conversion = config.conversionElement.isEmpty() ? nullptr : config.conversionElement.constData();

    const GstFactoryPtr factory = adoptFactory(gst_element_factory_find("qgcvideosinkbin"));
    if (!factory) {
        // completeInit verified this factory at startup; absence here means the registry changed underfoot.
        qCCritical(GStreamerLog) << "qgcvideosinkbin factory not found";
        return nullptr;
    }

#if defined(QGC_HAS_ANY_GPU_PATH)
    // gpu-zerocopy is construct-only on the bin; adapter reads it back from the bin so the two halves can't desync.
    GstElement* videoSinkBin =
        gst_element_factory_create_full(factory.get(), "gpu-zerocopy", config.gpuZeroCopy ? TRUE : FALSE,
                                        "conversion-element", conversion, "disable-par", disablePar, NULL);
#else
    GstElement* videoSinkBin = gst_element_factory_create_full(factory.get(), "conversion-element", conversion,
                                                               "disable-par", disablePar, NULL);
#endif
    if (!videoSinkBin) {
        qCCritical(GStreamerLog) << "qgcvideosinkbin element creation failed";
    }
    return videoSinkBin;
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

    // Clear the GL bridge's exhausted-retry latch so a restart after Qt's globalShareContext
    // appears can prime on the next NEED_CONTEXT. No-op when already primed.
    HwBuffers::onPipelineRestart();

    // Accessor returns a transfer-full ref; the guard releases it once the controller has taken
    // its own ref for deferred QObject teardown.
    const GstObjectPtr element(GST_OBJECT(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(sinkBin))));
    if (!element) {
        qCCritical(GStreamerLog) << "setupQVideoSinkElement: bin has no qgcqvideosink child";
        return false;
    }
    GstElement* const elementRaw = GST_ELEMENT(element.get());

#if defined(QGC_HAS_ANY_GPU_PATH)
    // Resolve the GPU context on the GUI thread and push it in, else gpu-zerocopy=TRUE negotiates
    // GPU caps but show_frame still memcpys. gpu-zerocopy is the bin's property, not the inner sink's.
    const gboolean gpuZeroCopy = gst_qgc_video_sink_bin_get_gpu_zerocopy(GST_ELEMENT(sinkBin));
    if (gpuZeroCopy) {
        gst_qgc_q_video_sink_set_hw_context(GST_QGC_Q_VIDEO_SINK(elementRaw), HwBuffers::makeAdapterContext(true));
    }
#endif

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
    // One walk of the decoder factory list (mirrors prioritizeByHardwareClass) classifies each
    // factory by name prefix into a VideoDecoderOptions family.
    static constexpr std::array<std::pair<VideoDecoderOptions, const char*>, 5> kFamilyPrefixes = {{
        {ForceVideoDecoderNVIDIA, "nv"},
        {ForceVideoDecoderVAAPI, "va"},
        {ForceVideoDecoderIntel, "qsv"},
        {ForceVideoDecoderVideoToolbox, "vtdec"},
        {ForceVideoDecoderVulkan, "vulkan"},
    }};

    QList<VideoDecoderOptions> families;
    const auto note = [&families](VideoDecoderOptions f) {
        if (!families.contains(f))
            families.append(f);
    };

    GList* decoderFactories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);
    for (GList* node = decoderFactories; node != nullptr; node = node->next) {
        GstElementFactory* factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory)
            continue;
        const gchar* name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
        if (!name)
            continue;

        if (g_str_has_prefix(name, "msdk")) {
            note(ForceVideoDecoderIntel);
            continue;
        }
        if (g_str_has_prefix(name, "d3d11") || g_str_has_prefix(name, "d3d12") || g_str_has_prefix(name, "dxva")) {
            note(ForceVideoDecoderDirectX3D);
            continue;
        }
        // Legacy gstreamer-vaapi (vaapi*) also matches "va" but is demoted to RANK_NONE and never bumped by
        // ForceVideoDecoderVAAPI (modern va* only); skip it so an unusable VAAPI option isn't kept.
        if (g_str_has_prefix(name, "vaapi")) {
            continue;
        }
        for (const auto& [family, prefix] : kFamilyPrefixes) {
            if (g_str_has_prefix(name, prefix)) {
                note(family);
                break;
            }
        }
    }
    gst_plugin_feature_list_free(decoderFactories);

    return families;
}

}  // namespace GStreamer
