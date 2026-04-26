#include "GStreamer.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QVarLengthArray>
#include <atomic>
#include <mutex>

#include "GStreamerLogging.h"
#include "GstEnvironment.h"
#include "GstPluginRegistry.h"
#include "QGCLoggingCategory.h"

#include <gst/gst.h>

QGC_LOGGING_CATEGORY(GStreamerLog, "VideoManager.GStreamer.GStreamer")

#ifdef Q_OS_IOS
extern "C" {
void gst_ios_pre_init(void);
void gst_ios_post_init(void);
}
#endif

namespace GStreamer {

namespace {

std::atomic_bool s_available = false;

}  // namespace

void setDebugLevel(int level)
{
    if (!gst_is_initialized()) {
        return;
    }
    const int clamped = qBound(0, level, static_cast<int>(GST_LEVEL_MEMDUMP));
    gst_debug_set_default_threshold(static_cast<GstDebugLevel>(clamped));
    qCDebug(GStreamerLog) << "GStreamer debug threshold set to" << clamped;
}

namespace {

bool _initGstRuntime()
{
    QString envError;
    if (!GStreamer::envPathsValid(&envError)) {
        qCCritical(GStreamerLog) << "Invalid GStreamer environment configuration:" << envError;
        return false;
    }

    // Cache arguments on the stack — QCoreApplication::arguments() is not thread-safe,
    // but this runs early during init before concurrent access is possible.
    const QStringList args = QCoreApplication::arguments();
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

#ifdef Q_OS_IOS
    gst_ios_pre_init();
#endif

    if (!gst_init_check(&argc, &argvPtr, &error)) {
        qCCritical(GStreamerLog) << "Failed to initialize GStreamer:" << (error ? error->message : "unknown error");
        g_clear_error(&error);
        return false;
    }

#ifdef Q_OS_IOS
    gst_ios_post_init();
#endif

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

    GStreamer::registerPlugins();

    if (!GStreamer::verifyPlugins()) {
        qCCritical(GStreamerLog) << "Plugin verification failed";
        return false;
    }

    static constexpr const char* kRequiredFactories[] = {
        "parsebin",
        "queue",
        "h264parse",
        "h265parse",
        "mpegtsmux",
        "appsink",
    };
    for (const char* factoryName : kRequiredFactories) {
        GstElementFactory* factory = gst_element_factory_find(factoryName);
        if (!factory) {
            qCCritical(GStreamerLog) << "Required ingest-session factory not found:" << factoryName;
            return false;
        }
        gst_object_unref(factory);
    }

    if (GStreamer::didExternalPluginLoaderFail()) {
        qCCritical(GStreamerLog)
            << "GStreamer external plugin loader failed. Check GST_PLUGIN_SCANNER and bundled runtime paths.";
        return false;
    }

    s_available.store(true, std::memory_order_release);
    return true;
}

bool initialize()
{
    GStreamer::resetExternalPluginLoaderFailure();
    GStreamer::redirectGLibLogging();

    gst_debug_remove_log_function(gst_debug_log_default);

    if (!_initGstRuntime()) {
        return false;
    }

    return completeInit();
}

QFuture<bool> initAsync()
{
    // Guard against double-init — qputenv in prepareEnvironment is not thread-safe,
    // and gst_init must only be called once per process.
    static std::once_flag s_initOnce;
    static QFuture<bool> s_initFuture;

    std::call_once(s_initOnce, []() {
        prepareEnvironment();

        s_initFuture = QtConcurrent::run([]() -> bool {
            if (!initialize()) {
                s_available.store(false, std::memory_order_release);
                return false;
            }
            return true;
        });
    });

    return s_initFuture;
}

bool isAvailable()
{
    return s_available.load(std::memory_order_acquire);
}

}  // namespace GStreamer
