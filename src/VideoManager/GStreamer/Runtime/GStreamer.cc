#include "GStreamer.h"

#include <algorithm>
#include <gst/gst.h>
#ifdef QGC_GST_STATIC_BUILD
#include <mutex>
#endif
#include <utility>

#include <QtCore/QByteArrayList>
#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QVarLengthArray>

#include "GStreamerDecoderRanks.h"
#include "GStreamerEnvironment.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GStreamerUdpSrcWorkaround.h"
#include "GstScoped.h"
#include "HwBuffers/HwBuffers.h"
#include "QGCLoggingCategory.h"
#include "gstqgc/GstQgcSinkPolicy.h"

QGC_LOGGING_CATEGORY(GStreamerLog, "Video.GStreamer.GStreamer")

#ifdef QGC_GST_STATIC_BUILD
G_BEGIN_DECLS
// Generated from cmake/GStreamer/gst_static_plugins.c.in by qgc_gst_generate_static_init (Mobile.cmake):
// registers every configured plugin and loads the static GIO modules (gioopenssl + the bundled CA bundle on mobile).
extern void gst_init_static_plugins(void);
G_END_DECLS
#endif

namespace GStreamer {

namespace {

#ifdef QGC_GST_STATIC_BUILD
void _registerPlugins()
{
    // gst_init_static_plugins() is not idempotent: a second pass re-registers GTypes ("cannot register existing type
    // 'GstBaseQTMux'") and aborts, so run once. The generated shim's registrations are registry-guarded: gst_init()
    // may already have run it (see cmake/GStreamer/gst_static_plugins.c.in).
    static std::once_flag s_pluginsRegistered;
    std::call_once(s_pluginsRegistered, [] { gst_init_static_plugins(); });
}
#endif

struct PluginFeature
{
    const char* name;
    bool required;
    const char* factories;  // space-separated; "a|b" is satisfied by either factory
};

// Checked per element, as Qt's qGstErrorMessageIfElementsNotAvailable does, so a missing optional plugin disables only
// its feature. Only the display core is required.
constexpr PluginFeature kPluginFeatures[] = {
    {"Video display", true, "decodebin3 tee queue valve capsfilter videoconvertscale|videoconvert"},
    {"RTP/UDP video", true, "udpsrc rtpjitterbuffer parsebin rtph264depay h264parse rtph265depay h265parse"},
    {"RTSP video", false, "rtspsrc"},
    {"MPEG-TS video", false, "tsdemux"},
    {"TCP video", false, "tcpclientsrc"},
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

    const QList<MissingPluginFeature> missing = missingPluginFeatures(hasElementFactory);
    if (missing.isEmpty()) {
        return true;
    }

    bool displayAvailable = true;
    for (const MissingPluginFeature& feature : missing) {
        const QString message = QStringLiteral("%1 unavailable, GStreamer elements not found: %2")
                                    .arg(feature.feature, feature.factories.join(QStringLiteral(", ")));
        if (feature.required) {
            qCCritical(GStreamerLog).noquote() << message;
            displayAvailable = false;
        } else {
            qCWarning(GStreamerLog).noquote() << message;
        }
    }

    // Surface blacklisted plugins so a failure from a corrupt/incompatible plugin file
    // shows up here instead of looking like a missing-plugin problem.
    GStreamer::forEachPlugin(registry, [](GstPlugin* p) {
        if (!GST_OBJECT_FLAG_IS_SET(p, GST_PLUGIN_FLAG_BLACKLISTED)) {
            return;
        }
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

namespace {

bool _initGstRuntime(const QStringList& args, const VideoBackend::EnvPrepResult& env)
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

bool completeInit(std::optional<int> debugLevel)
{
    if (!gst_is_initialized()) {
        qCCritical(GStreamerLog) << "completeInit called but gst_init() has not been called";
        return false;
    }

    GStreamer::configureDebugLogging(debugLevel);

    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);
    qCDebug(GStreamerLog) << "GStreamer initialized:" << major << "." << minor << "." << micro;

    // A newer runtime is ABI-compatible; an older one may lack API the build used.
    if (std::make_pair(major, minor) < std::make_pair(guint{GST_VERSION_MAJOR}, guint{GST_VERSION_MINOR})) {
        qCWarning(GStreamerLog) << "GStreamer runtime" << major << "." << minor << "." << micro << "is older than the"
                                << GST_VERSION_MAJOR << "." << GST_VERSION_MINOR << "QGC was built against";
    }

#ifdef QGC_GST_STATIC_BUILD
    _registerPlugins();
#endif

    if (!_verifyPluginFeatures()) {
        return false;
    }

    serializeUdpSrcUnlock();

    GStreamer::logDecoderRanks();

    return true;
}

bool initialize(const VideoBackend::InitParams& params)
{
    GStreamer::resetExternalPluginLoaderFailed();
    GStreamer::redirectGLibLogging();

    // Before gst_init_check(), so that what GStreamer logs while it initializes (the plugin scan) reaches QGC's log at
    // the persisted level. It also removes GStreamer's default stderr handler, unless the user asked for GStreamer's
    // own output, which gst_init_check() would otherwise keep; its raw ANSI codes corrupt the terminal on macOS.
    GStreamer::configureDebugLogging(params.debugLevel);

    if (!_initGstRuntime(params.arguments, params.environment)) {
        return false;
    }

    // gst_deinit() can't be undone, so it is opt-in, as Qt's QT_GSTREAMER_DEINIT: it lets the leaks tracer report.
    if (qEnvironmentVariableIntValue("QGC_GST_DEINIT") > 0) {
        qAddPostRoutine(_deinitGStreamer);
    }

    // The threshold in effect: the persisted level, unless a GStreamer option among the arguments (--gst-debug-level)
    // replaced it during gst_init_check(), which then stays.
    return completeInit(static_cast<int>(gst_debug_get_default_threshold()));
}

void onMainWindowReady(QQuickWindow* window)
{
    HwBuffers::connectMainWindow(window);
    qCInfo(GStreamerLog) << "Resolved RHI backend:" << GstQgc::toString(GstQgc::currentRenderTarget().backend);
}

}  // namespace GStreamer
