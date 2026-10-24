#include "GStreamer.h"

#include <algorithm>
#include <gst/base/gstbasesrc.h>
#include <mutex>
#include <utility>

#include <QtCore/QByteArrayList>
#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QVarLengthArray>
#include <QtMultimedia/QVideoSink>
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
#include <gst/gst.h>

#include "QGCLoggingCategory.h"
#include "QGCQVideoSinkController.h"
#include "SettingsManager.h"
#include "VideoBackend.h"
#include "VideoSettings.h"
#include "gstqgc/gstqgcvideosinkbin.h"

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

struct PluginFeature
{
    const char* name;
    bool required;
    const char* factories;  // space-separated; "a|b" is satisfied by either factory
};

// Checked per element, as Qt's qGstErrorMessageIfElementsNotAvailable does, so a missing optional plugin disables only
// its feature. Only the display core is required. The qgc elements are checked separately (requireFactory).
constexpr PluginFeature kPluginFeatures[] = {
    {"Video display", true, "decodebin3 tee queue valve capsfilter videoconvertscale|videoconvert"},
    {"RTP/UDP video", true, "udpsrc rtpjitterbuffer parsebin rtph264depay h264parse rtph265depay h265parse"},
    {"RTSP video", false, "rtspsrc"},
    {"MPEG-TS video", false, "tsdemux"},
    {"TCP video", false, "tcpclientsrc"},
    {"Video recording", false, "splitmuxsink"},
    {"MKV recording", false, "matroskamux"},
    {"MP4/MOV recording", false, "mp4mux qtmux"},
};

bool _verifyPluginFeatures()
{
    GstRegistry* registry = gst_registry_get();
    if (GStreamerLog().isDebugEnabled()) {
        qCDebug(GStreamerLog) << "Installed GStreamer plugins:";
        GStreamer::forEachPlugin(registry, [](GstPlugin* plugin) {
            qCDebug(GStreamerLog) << "  " << gst_plugin_get_name(plugin) << gst_plugin_get_version(plugin);
        });
    }

    const QList<MissingPluginFeature> missing = missingPluginFeatures(
        [](const char* name) { return static_cast<bool>(adoptFactory(gst_element_factory_find(name))); });
    if (missing.isEmpty()) {
        return true;
    }

    bool displayAvailable = true;
    for (const MissingPluginFeature& feature : missing) {
        if (feature.required) {
            qCCritical(GStreamerLog) << feature.feature
                                     << "unavailable, GStreamer elements not found:" << feature.factories;
            displayAvailable = false;
        } else {
            qCWarning(GStreamerLog) << feature.feature
                                    << "unavailable, GStreamer elements not found:" << feature.factories;
        }
    }

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

    if (!displayAvailable) {
        // Path / scanner env vars belong to the environment layer that set them.
        Environment::logDiagnostics();
    }

    return displayAvailable;
}

using BaseSrcUnlockFunc = gboolean (*)(GstBaseSrc*);

BaseSrcUnlockFunc s_udpSrcUnlock = nullptr;
BaseSrcUnlockFunc s_udpSrcUnlockStop = nullptr;
// Not the object lock: a distro build that backported the upstream fix already takes it inside these calls.
std::mutex s_udpSrcUnlockMutex;

gboolean _udpSrcUnlockSerialized(GstBaseSrc* src)
{
    const std::lock_guard lock(s_udpSrcUnlockMutex);
    return s_udpSrcUnlock(src);
}

gboolean _udpSrcUnlockStopSerialized(GstBaseSrc* src)
{
    const std::lock_guard lock(s_udpSrcUnlockMutex);
    return s_udpSrcUnlockStop(src);
}

// Before gst-plugins-good 1.24.10, udpsrc's unlock_stop() replaces its GCancellable on the streaming thread (basesrc
// defers it to the first create() after PLAYING) while unlock() cancels it from the thread leaving PLAYING. A pipeline
// stopped right after it started then cancels a freed GCancellable and corrupts the heap. Serialize the two, as the
// upstream fix does (gstreamer!7897).
void _serializeUdpSrcUnlock()
{
    static std::once_flag s_serialized;
    std::call_once(s_serialized, [] {
        const GstFactoryPtr factory = adoptFactory(gst_element_factory_find("udpsrc"));
        if (!factory || gst_plugin_feature_check_version(GST_PLUGIN_FEATURE(factory.get()), 1, 24, 10)) {
            return;
        }

        const GstFeaturePtr loaded = adoptFeature(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory.get())));
        const GType type =
            loaded ? gst_element_factory_get_element_type(GST_ELEMENT_FACTORY(loaded.get())) : G_TYPE_INVALID;
        if (!g_type_is_a(type, GST_TYPE_BASE_SRC)) {
            qCWarning(GStreamerLog) << "udpsrc failed to load; stopping a UDP stream right after it starts may crash";
            return;
        }

        // Never released, so the patched class outlives every udpsrc.
        auto* const klass = static_cast<GstBaseSrcClass*>(g_type_class_ref(type));
        s_udpSrcUnlock = klass->unlock;
        s_udpSrcUnlockStop = klass->unlock_stop;
        if (s_udpSrcUnlock && s_udpSrcUnlockStop) {
            klass->unlock = _udpSrcUnlockSerialized;
            klass->unlock_stop = _udpSrcUnlockStopSerialized;
            qCDebug(GStreamerLog) << "Serialized udpsrc unlock/unlock_stop (gst-plugins-good before 1.24.10)";
        }
    });
}

}  // anonymous namespace

QList<MissingPluginFeature> missingPluginFeatures(const std::function<bool(const char* factoryName)>& hasFactory)
{
    QList<MissingPluginFeature> missing;
    for (const PluginFeature& feature : kPluginFeatures) {
        QStringList absent;
        for (const QByteArray& entry : QByteArray(feature.factories).split(' ')) {
            const QList<QByteArray> alternatives = entry.split('|');
            if (std::ranges::none_of(alternatives,
                                     [&hasFactory](const QByteArray& name) { return hasFactory(name.constData()); })) {
                absent.append(QString::fromLatin1(entry));
            }
        }
        if (!absent.isEmpty()) {
            missing.append(
                {.feature = QString::fromLatin1(feature.name), .factories = absent, .required = feature.required});
        }
    }
    return missing;
}

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

// A Qt post routine runs in ~QCoreApplication, after QGCApplication::shutdown() has destroyed the receivers.
void _deinitGStreamer()
{
    gst_debug_remove_log_function(GStreamer::qtGstLog);
    gst_deinit();
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

    // A newer runtime is ABI-compatible; an older one may lack API the build used.
    if (std::make_pair(major, minor) < std::make_pair(guint{GST_VERSION_MAJOR}, guint{GST_VERSION_MINOR})) {
        qCWarning(GStreamerLog) << "GStreamer runtime" << major << "." << minor << "." << micro << "is older than the"
                                << GST_VERSION_MAJOR << "." << GST_VERSION_MINOR << "QGC was built against";
    }

    _registerPlugins();

    if (!_verifyPluginFeatures()) {
        return false;
    }

    _serializeUdpSrcUnlock();

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

    return true;
}

bool initialize(const QStringList& arguments, const Environment::ValidationResult& envResult)
{
    GStreamer::redirectGLibLogging();

    // Unless the user asked for GStreamer's own output, remove its default stderr handler before gst_init_check(),
    // which would otherwise keep it; its raw ANSI codes corrupt the terminal on macOS.
    if (!GStreamer::nativeDebugLogRequested()) {
        gst_debug_remove_log_function(gst_debug_log_default);
    }

    if (!_initGstRuntime(arguments, envResult)) {
        return false;
    }

    // gst_deinit() can't be undone, so it is opt-in, as Qt's QT_GSTREAMER_DEINIT: it lets the leaks tracer report.
    if (qEnvironmentVariableIntValue("QGC_GST_DEINIT") > 0) {
        qAddPostRoutine(_deinitGStreamer);
    }

    return completeInit();
}

// Video sink refcount protocol: createVideoSink returns one owned, non-floating ref, which the caller
// adopts. While decoding, the pipeline bin takes another on gst_bin_add (dropped by gst_bin_remove).
GstElement* createVideoSink(const VideoSinkConfig& config)
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
    return GST_ELEMENT(gst_object_ref_sink(videoSinkBin));
}

VideoSinkConfig videoSinkConfigFromSettings()
{
    VideoSettings* const settings = SettingsManager::instance()->videoSettings();
    const bool forceCpu = settings->forceCpuVideoPath()->rawValue().toBool();
    const bool softwareDecoder = settings->forceVideoDecoder()->rawValue().toInt() == ForceVideoDecoderSoftware;
    VideoSinkConfig config;
    config.conversionElement = settings->videoConversionElement()->rawValue().toString().toUtf8();
    config.disablePixelAspectRatio = settings->disablePixelAspectRatio()->rawValue().toBool();
    config.gpuZeroCopy = VideoBackend::gpuZeroCopyAllowedForCurrentGraphicsApi(forceCpu, softwareDecoder);
    return config;
}

VideoReceiver* createVideoReceiver(QObject* parent)
{
    return new GstVideoReceiver(parent);
}

QGCQVideoSinkController* setupQVideoSinkElement(GstElement* sinkBin, QVideoSink* videoSink, QObject* controllerParent)
{
    if (!sinkBin || !videoSink || !controllerParent) {
        // controllerParent owns the controller's QObject lifetime — else it leaks (caller has no handle).
        qCWarning(GStreamerLog) << "setupQVideoSinkElement: null sinkBin, videoSink, or controllerParent";
        return nullptr;
    }

    // Idempotent re-setup: tear down prior controllers so repeated setups don't accumulate dangling ones.
    for (auto* c : QGCQVideoSinkController::controllersOf(controllerParent)) {
        c->setActive(false);
        // Released now: deleteLater is deferred, and the old binding must not outlive the new one's installation.
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
        return nullptr;
    }
    GstElement* const elementRaw = GST_ELEMENT(element.get());

    auto* controller = new QGCQVideoSinkController(elementRaw, controllerParent);

    // Route the initial install through the controller so its destroyed-sink QPointer guard
    // covers setup, not just later swaps.
    controller->setVideoSink(QPointer<QVideoSink>(videoSink));
    controller->setActive(true);

    return controller;
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
