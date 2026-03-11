#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstVideoReceiver.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QStringList>
#include <QtQuick/QQuickItem>

#include <atomic>

#ifdef Q_OS_LINUX
#include <dlfcn.h>
#endif

#include <gst/gst.h>

G_BEGIN_DECLS
#ifdef QGC_GST_STATIC_BUILD
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(isomp4);
GST_PLUGIN_STATIC_DECLARE(libav);
GST_PLUGIN_STATIC_DECLARE(matroska);
GST_PLUGIN_STATIC_DECLARE(mpegtsdemux);
GST_PLUGIN_STATIC_DECLARE(opengl);
GST_PLUGIN_STATIC_DECLARE(openh264);
GST_PLUGIN_STATIC_DECLARE(playback);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(rtsp);
GST_PLUGIN_STATIC_DECLARE(sdpelem);
GST_PLUGIN_STATIC_DECLARE(tcp);
GST_PLUGIN_STATIC_DECLARE(typefindfunctions);
GST_PLUGIN_STATIC_DECLARE(udp);
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
GST_PLUGIN_STATIC_DECLARE(vpx);

#ifdef GST_PLUGIN_androidmedia_FOUND
GST_PLUGIN_STATIC_DECLARE(androidmedia);
#endif
#ifdef GST_PLUGIN_applemedia_FOUND
GST_PLUGIN_STATIC_DECLARE(applemedia);
#endif
#ifdef GST_PLUGIN_d3d_FOUND
GST_PLUGIN_STATIC_DECLARE(d3d);
#endif
#ifdef GST_PLUGIN_d3d11_FOUND
GST_PLUGIN_STATIC_DECLARE(d3d11);
#endif
#ifdef GST_PLUGIN_d3d12_FOUND
GST_PLUGIN_STATIC_DECLARE(d3d12);
#endif
#ifdef GST_PLUGIN_dav1d_FOUND
GST_PLUGIN_STATIC_DECLARE(dav1d);
#endif
#ifdef GST_PLUGIN_dxva_FOUND
GST_PLUGIN_STATIC_DECLARE(dxva);
#endif
#ifdef GST_PLUGIN_nvcodec_FOUND
GST_PLUGIN_STATIC_DECLARE(nvcodec);
#endif
#ifdef GST_PLUGIN_qsv_FOUND
GST_PLUGIN_STATIC_DECLARE(qsv);
#endif
#ifdef GST_PLUGIN_va_FOUND
GST_PLUGIN_STATIC_DECLARE(va);
#endif
#ifdef GST_PLUGIN_vulkan_FOUND
GST_PLUGIN_STATIC_DECLARE(vulkan);
#endif
#endif

GST_PLUGIN_STATIC_DECLARE(qml6);
GST_PLUGIN_STATIC_DECLARE(qgc);
G_END_DECLS

namespace GStreamer
{

static std::atomic<bool> s_envPathsValid{true};
static QMutex s_envPathsMutex;
static QString s_envPathsError;

void _registerPlugins()
{
#ifdef QGC_GST_STATIC_BUILD
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(isomp4);
    GST_PLUGIN_STATIC_REGISTER(libav);
    GST_PLUGIN_STATIC_REGISTER(matroska);
    GST_PLUGIN_STATIC_REGISTER(mpegtsdemux);
    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(openh264);
    GST_PLUGIN_STATIC_REGISTER(playback);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(rtsp);
    GST_PLUGIN_STATIC_REGISTER(sdpelem);
    GST_PLUGIN_STATIC_REGISTER(tcp);
    GST_PLUGIN_STATIC_REGISTER(typefindfunctions);
    GST_PLUGIN_STATIC_REGISTER(udp);
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
    GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
    GST_PLUGIN_STATIC_REGISTER(vpx);

#ifdef GST_PLUGIN_androidmedia_FOUND
    GST_PLUGIN_STATIC_REGISTER(androidmedia);
#endif
#ifdef GST_PLUGIN_applemedia_FOUND
    GST_PLUGIN_STATIC_REGISTER(applemedia);
#endif
#ifdef GST_PLUGIN_d3d_FOUND
    GST_PLUGIN_STATIC_REGISTER(d3d);
#endif
#ifdef GST_PLUGIN_d3d11_FOUND
    GST_PLUGIN_STATIC_REGISTER(d3d11);
#endif
#ifdef GST_PLUGIN_d3d12_FOUND
    GST_PLUGIN_STATIC_REGISTER(d3d12);
#endif
#ifdef GST_PLUGIN_dav1d_FOUND
    GST_PLUGIN_STATIC_REGISTER(dav1d);
#endif
#ifdef GST_PLUGIN_dxva_FOUND
    GST_PLUGIN_STATIC_REGISTER(dxva);
#endif
#ifdef GST_PLUGIN_nvcodec_FOUND
    GST_PLUGIN_STATIC_REGISTER(nvcodec);
#endif
#ifdef GST_PLUGIN_qsv_FOUND
    GST_PLUGIN_STATIC_REGISTER(qsv);
#endif
#ifdef GST_PLUGIN_va_FOUND
    GST_PLUGIN_STATIC_REGISTER(va);
#endif
#ifdef GST_PLUGIN_vulkan_FOUND
    GST_PLUGIN_STATIC_REGISTER(vulkan);
#endif
#endif

    GST_PLUGIN_STATIC_REGISTER(qml6);
    GST_PLUGIN_STATIC_REGISTER(qgc);
}

void _resetEnvValidation()
{
    const QMutexLocker locker(&s_envPathsMutex);
    s_envPathsError.clear();
    s_envPathsValid.store(true, std::memory_order_release);
}

void _setEnvValidationError(const QString &error)
{
    const QMutexLocker locker(&s_envPathsMutex);
    s_envPathsError = error;
    s_envPathsValid.store(false, std::memory_order_release);
    qCCritical(GStreamerLog) << error;
}

QString _cleanJoin(const QString &base, const QString &relative)
{
    return QDir::cleanPath(QDir(base).filePath(relative));
}

void _setGstEnv(const char *name, const QString &value)
{
    qputenv(name, value.toUtf8());
    qCDebug(GStreamerLog) << "  " << name << "=" << value;
}

void _unsetEnv(const char *name)
{
    if (qEnvironmentVariableIsSet(name)) {
        qunsetenv(name);
        qCDebug(GStreamerLog) << "  unset" << name;
    }
}

void _setGstEnvIfExists(const char *name, const QString &path)
{
    if (QFileInfo::exists(path)) {
        _setGstEnv(name, path);
    }
}

bool _isExecutableFile(const QString &path)
{
    const QFileInfo fileInfo(path);
    return fileInfo.exists() && fileInfo.isFile() && fileInfo.isExecutable();
}

QString _firstExistingPath(const QStringList &paths)
{
    for (const QString &path : paths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }

    return {};
}

QString _joinExistingPaths(const QStringList &paths)
{
    QStringList existing;
    existing.reserve(paths.size());

    for (const QString &path : paths) {
        if (QFileInfo::exists(path) && !existing.contains(path)) {
            existing.append(path);
        }
    }

    return existing.join(QDir::listSeparator());
}

void _clearManagedGstEnvVars()
{
    static constexpr const char *varsToUnset[] = {
        "GIO_EXTRA_MODULES",
        "GIO_MODULE_DIR",
        "GIO_USE_VFS",
        "GST_PTP_HELPER_1_0",
        "GST_PTP_HELPER",
        "GST_PLUGIN_SCANNER_1_0",
        "GST_PLUGIN_SCANNER",
        "GST_PLUGIN_SYSTEM_PATH_1_0",
        "GST_PLUGIN_SYSTEM_PATH",
        "GST_PLUGIN_PATH_1_0",
        "GST_PLUGIN_PATH",
        "PYTHONNOUSERSITE",
    };

    for (const char *name : varsToUnset) {
        _unsetEnv(name);
    }
}

void _setGstEnvIfExecutable(const char *name, const QString &path)
{
    if (_isExecutableFile(path)) {
        _setGstEnv(name, path);
    } else {
        _unsetEnv(name);
    }
}

static constexpr const char *kPythonEnvVars[] = {
    "PYTHONHOME",
    "PYTHONPATH",
    "VIRTUAL_ENV",
    "CONDA_PREFIX",
    "CONDA_DEFAULT_ENV",
    "PYTHONUSERBASE",
    "PYTHONNOUSERSITE",
};

QHash<QByteArray, QByteArray> s_savedPythonEnv;
bool s_pythonEnvSanitized = false;

void _sanitizePythonEnvForScanner()
{
    s_savedPythonEnv.clear();
    s_pythonEnvSanitized = true;

    for (const char *name : kPythonEnvVars) {
        if (qEnvironmentVariableIsSet(name)) {
            s_savedPythonEnv.insert(QByteArray(name), qgetenv(name));
        }
        _unsetEnv(name);
    }

    _setGstEnv("PYTHONNOUSERSITE", QStringLiteral("1"));
}

void _restorePythonEnv()
{
    if (!s_pythonEnvSanitized) {
        return;
    }
    s_pythonEnvSanitized = false;

    qCDebug(GStreamerLog) << "Restoring Python environment variables";

    for (const char *name : kPythonEnvVars) {
        const QByteArray key(name);
        if (s_savedPythonEnv.contains(key)) {
            qputenv(name, s_savedPythonEnv.value(key));
            qCDebug(GStreamerLog) << "  " << name << "=" << s_savedPythonEnv.value(key);
        } else {
            _unsetEnv(name);
        }
    }

    s_savedPythonEnv.clear();
}

void _applyGstEnvVars(const QString &pluginDir, const QString &gioModDir,
                      const QString &scannerPath, const QString &ptpPath)
{
    qCDebug(GStreamerLog) << "Applying GStreamer environment:";

    _clearManagedGstEnvVars();
    _sanitizePythonEnvForScanner();
    // Force fresh registry scan since we're redirecting plugin paths to bundled locations
    _setGstEnv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", QStringLiteral("no"));
    _setGstEnvIfExists("GIO_EXTRA_MODULES", gioModDir);
    _setGstEnvIfExecutable("GST_PTP_HELPER_1_0", ptpPath);
    _setGstEnvIfExecutable("GST_PTP_HELPER", ptpPath);
    _setGstEnvIfExecutable("GST_PLUGIN_SCANNER_1_0", scannerPath);
    _setGstEnvIfExecutable("GST_PLUGIN_SCANNER", scannerPath);
    _setGstEnv("GST_PLUGIN_SYSTEM_PATH_1_0", pluginDir);
    _setGstEnv("GST_PLUGIN_SYSTEM_PATH", pluginDir);
    _setGstEnv("GST_PLUGIN_PATH_1_0", pluginDir);
    _setGstEnv("GST_PLUGIN_PATH", pluginDir);
}

#if defined(Q_OS_LINUX)
bool _systemGioIsNew()
{
    // Probe the system GIO library on disk (not the in-process symbols) for
    // g_task_set_static_name, which was added in GIO 2.76. This mirrors
    // AppRun's `nm -D "$SYSTEM_GIO" | grep g_task_set_static_name` check.
    static constexpr const char *kGioSoPaths[] = {
        "/usr/lib/x86_64-linux-gnu/libgio-2.0.so.0",
        "/usr/lib/aarch64-linux-gnu/libgio-2.0.so.0",
        "/usr/lib64/libgio-2.0.so.0",
        "/usr/lib/libgio-2.0.so.0",
    };

    for (const char *path : kGioSoPaths) {
        void *handle = dlopen(path, RTLD_LAZY | RTLD_NOLOAD);
        if (!handle) {
            handle = dlopen(path, RTLD_LAZY);
        }
        if (!handle) {
            continue;
        }
        const bool found = (dlsym(handle, "g_task_set_static_name") != nullptr);
        dlclose(handle);
        return found;
    }

    return false;
}

void _applyGioCompatOverride(const QString &gioModDir)
{
    if (gioModDir.isEmpty()) {
        return;
    }

    // GIO 2.76+ requires bundled modules to be loaded via GIO_MODULE_DIR with
    // VFS forced to local, mirroring the AppImage launcher logic.
    if (_systemGioIsNew()) {
        _unsetEnv("GIO_EXTRA_MODULES");
        _setGstEnv("GIO_MODULE_DIR", gioModDir);
        _setGstEnv("GIO_USE_VFS", QStringLiteral("local"));
    }
}
#endif

void _warnIfScannerMissing(const QString &platformLabel, const QString &scannerPath)
{
    if (scannerPath.isEmpty()) {
        qCWarning(GStreamerLog) << "GStreamer:" << platformLabel
                                << "bundled gst-plugin-scanner not found; GStreamer will use in-process scanning";
    } else if (!_isExecutableFile(scannerPath)) {
        qCWarning(GStreamerLog) << "GStreamer:" << platformLabel
                                << "gst-plugin-scanner is not executable:" << scannerPath;
    }
}

bool _validateMacBundlePaths(const QString &bundleFrameworkRoot,
                             const QString &pluginDirs,
                             const QString &scannerPath)
{
    if (pluginDirs.isEmpty()) {
        _setEnvValidationError(QStringLiteral(
            "GStreamer: bundled macOS framework found but plugin directory is missing under %1")
            .arg(bundleFrameworkRoot));
        return false;
    }

    _warnIfScannerMissing(QStringLiteral("macOS framework"), scannerPath);
    return true;
}

bool _validateBundledDesktopPaths(const QString &platformLabel,
                                  const QString &pluginDirs,
                                  const QString &scannerPath)
{
    if (pluginDirs.isEmpty()) {
        _setEnvValidationError(QStringLiteral(
            "GStreamer: %1 bundled plugin directory is missing.")
            .arg(platformLabel));
        return false;
    }

    _warnIfScannerMissing(platformLabel, scannerPath);
    return true;
}

void _setGstEnvVars()
{
    _resetEnvValidation();

    const QString appDir = QCoreApplication::applicationDirPath();
    qCDebug(GStreamerLog) << "App directory:" << appDir;

#if defined(Q_OS_MACOS)
    const QString frameworkDir = _cleanJoin(appDir, "../Frameworks/GStreamer.framework");
    const QString rootDir = _firstExistingPath({
        _cleanJoin(frameworkDir, "Versions/1.0"),
        _cleanJoin(frameworkDir, "Versions/Current"),
        frameworkDir,
    });

#if defined(QGC_GST_MACOS_FRAMEWORK)
    // Framework builds prefer framework paths over app-relative paths
    const QString pluginDirs = _joinExistingPaths({
        _cleanJoin(rootDir, "lib/gstreamer-1.0"),
        _cleanJoin(appDir, "../lib/gstreamer-1.0"),
    });
    const QString gioMod = _firstExistingPath({
        _cleanJoin(rootDir, "lib/gio/modules"),
        _cleanJoin(appDir, "../lib/gio/modules"),
    });
#else
    // Non-framework (Homebrew) builds prefer app-relative paths
    const QString pluginDirs = _joinExistingPaths({
        _cleanJoin(appDir, "../lib/gstreamer-1.0"),
        _cleanJoin(rootDir, "lib/gstreamer-1.0"),
    });
    const QString gioMod = _firstExistingPath({
        _cleanJoin(appDir, "../lib/gio/modules"),
        _cleanJoin(rootDir, "lib/gio/modules"),
    });
#endif

    const QString scanner = _firstExistingPath({
        _cleanJoin(appDir, "../libexec/gstreamer-1.0/gst-plugin-scanner"),
        _cleanJoin(rootDir, "libexec/gstreamer-1.0/gst-plugin-scanner"),
    });
    const QString ptp = _firstExistingPath({
        _cleanJoin(appDir, "../libexec/gstreamer-1.0/gst-ptp-helper"),
        _cleanJoin(rootDir, "libexec/gstreamer-1.0/gst-ptp-helper"),
    });
    const bool hasBundledFramework = QFileInfo::exists(frameworkDir);

    const bool validBundlePaths = hasBundledFramework
        ? _validateMacBundlePaths(rootDir, pluginDirs, scanner)
        : !pluginDirs.isEmpty();

    if (!pluginDirs.isEmpty() && validBundlePaths) {
        _applyGstEnvVars(pluginDirs, gioMod, scanner, ptp);
    }

#if defined(QGC_GST_MACOS_FRAMEWORK)
    if (hasBundledFramework) {
        _setGstEnv("GTK_PATH", rootDir);
    }
#endif

#elif defined(Q_OS_WIN)
    const QString libDir = _cleanJoin(appDir, "../lib");
    const QString pluginDir = _cleanJoin(libDir, "gstreamer-1.0");
    const QString gioMod = _cleanJoin(libDir, "gio/modules");
    const QString libexecDir = _cleanJoin(appDir, "../libexec");
    const QString scanner = _cleanJoin(libexecDir, "gstreamer-1.0/gst-plugin-scanner.exe");
    const QString ptp = _cleanJoin(libexecDir, "gstreamer-1.0/gst-ptp-helper.exe");

    if (QFileInfo::exists(pluginDir)
        && _validateBundledDesktopPaths(QStringLiteral("Windows"), pluginDir, scanner)) {
        _applyGstEnvVars(pluginDir, gioMod, scanner, ptp);
    }

#elif defined(Q_OS_LINUX)
    // AppRun sets GStreamer env vars before launch (including GIO compatibility
    // logic). Only apply fallback paths when no external override is present.
    if (!qEnvironmentVariableIsSet("GST_PLUGIN_PATH_1_0")
        && !qEnvironmentVariableIsSet("GST_PLUGIN_PATH")
        && !qEnvironmentVariableIsSet("GST_PLUGIN_SYSTEM_PATH_1_0")
        && !qEnvironmentVariableIsSet("GST_PLUGIN_SYSTEM_PATH")) {
        const QString libDir = _cleanJoin(appDir, "../lib");
        const QString libexecDir = _cleanJoin(appDir, "../libexec");
        const QString pluginDir = _cleanJoin(libDir, "gstreamer-1.0");
        const QString gioMod = _cleanJoin(libDir, "gio/modules");
        const QString scanner = _firstExistingPath({
            _cleanJoin(libDir, "gstreamer1.0/gstreamer-1.0/gst-plugin-scanner"),
            _cleanJoin(libexecDir, "gstreamer-1.0/gst-plugin-scanner"),
        });
        const QString ptp = _firstExistingPath({
            _cleanJoin(libDir, "gstreamer1.0/gstreamer-1.0/gst-ptp-helper"),
            _cleanJoin(libexecDir, "gstreamer-1.0/gst-ptp-helper"),
        });

        if (QFileInfo::exists(pluginDir)
            && _validateBundledDesktopPaths(QStringLiteral("Linux"), pluginDir, scanner)) {
            _applyGstEnvVars(pluginDir, gioMod, scanner, ptp);
            _applyGioCompatOverride(gioMod);
        }
    }
#endif

}

bool _verifyPlugins()
{
    GstRegistry *registry = gst_registry_get();
    if (!registry) {
        qCCritical(GStreamerLog) << "Failed to get GStreamer registry";
        return false;
    }

    GList *plugins = gst_registry_get_plugin_list(registry);
    if (plugins) {
        qCDebug(GStreamerLog) << "Installed GStreamer plugins:";
        for (GList *node = plugins; node != nullptr; node = node->next) {
            GstPlugin *plugin = static_cast<GstPlugin*>(node->data);
            if (plugin) {
                qCDebug(GStreamerLog) << "  " << gst_plugin_get_name(plugin)
                                      << gst_plugin_get_version(plugin);
            }
        }
        gst_plugin_list_free(plugins);
    }

    bool result = true;
    static constexpr const char *requiredPlugins[] = {"qml6", "qgc", "coreelements"};
    for (const char *name : requiredPlugins) {
        GstPlugin *plugin = gst_registry_find_plugin(registry, name);
        if (!plugin) {
            qCCritical(GStreamerLog) << "Required QGC plugin not found:" << name;
            result = false;
            continue;
        }
        gst_clear_object(&plugin);
    }

    if (!result) {
        const QByteArray pluginPath = qEnvironmentVariableIsSet("GST_PLUGIN_PATH_1_0")
            ? qgetenv("GST_PLUGIN_PATH_1_0")
            : qgetenv("GST_PLUGIN_PATH");

        if (!pluginPath.isEmpty()) {
            qCCritical(GStreamerLog) << "Check GST_PLUGIN_PATH=" << pluginPath;
        } else {
            qCCritical(GStreamerLog) << "GST_PLUGIN_PATH is not set";
        }

        GList *allPlugins = gst_registry_get_plugin_list(registry);
        for (GList *node = allPlugins; node != nullptr; node = node->next) {
            GstPlugin *p = static_cast<GstPlugin*>(node->data);
            if (!p) continue;
            const gchar *desc = gst_plugin_get_description(p);
            const gchar *filename = gst_plugin_get_filename(p);
            if (desc && g_str_has_prefix(desc, "BLACKLIST")) {
                qCWarning(GStreamerLog) << "Blacklisted plugin:" << gst_plugin_get_name(p)
                                        << "file:" << (filename ? filename : "(null)");
            }
        }
        gst_plugin_list_free(allPlugins);

        static constexpr const char *envDiagnostics[] = {
            "GST_PLUGIN_PATH", "GST_PLUGIN_PATH_1_0",
            "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0",
            "GST_PLUGIN_SCANNER", "GST_PLUGIN_SCANNER_1_0",
            "GST_REGISTRY_REUSE_PLUGIN_SCANNER",
        };
        qCCritical(GStreamerLog) << "GStreamer environment diagnostics:";
        for (const char *var : envDiagnostics) {
            const QByteArray val = qgetenv(var);
            qCCritical(GStreamerLog) << "  " << var << "=" << (val.isEmpty() ? "(unset)" : val.constData());
        }
    }

    return result;
}


void prepareEnvironment()
{
    _setGstEnvVars();
}

bool _initGstRuntime()
{
    if (!s_envPathsValid.load(std::memory_order_acquire)) {
        const QMutexLocker locker(&s_envPathsMutex);
        qCCritical(GStreamerLog) << "Invalid GStreamer environment configuration:" << s_envPathsError;
        return false;
    }

    QByteArrayList argStorage;
    for (const QString &arg : QCoreApplication::arguments()) {
        argStorage.append(arg.toUtf8());
    }

    QVarLengthArray<char*, 16> argv;
    for (QByteArray &arg : argStorage) {
        argv.append(arg.data());
    }

    int argc = argv.size();
    char **argvPtr = argv.data();
    GError *error = nullptr;

    if (!gst_init_check(&argc, &argvPtr, &error)) {
        qCCritical(GStreamerLog) << "Failed to initialize GStreamer:"
                                  << (error ? error->message : "unknown error");
        g_clear_error(&error);
        _restorePythonEnv();
        return false;
    }

    _restorePythonEnv();
    return true;
}

bool completeInit()
{
    if (!gst_is_initialized()) {
        qCCritical(GStreamerLog) << "completeInit called but gst_init() has not been called";
        return false;
    }

    GStreamer::configureDebugLogging();

    gchar *version = gst_version_string();
    qCDebug(GStreamerLog) << "GStreamer initialized:" << version;
    g_free(version);

    _registerPlugins();

    if (!_verifyPlugins()) {
        qCCritical(GStreamerLog) << "Plugin verification failed";
        return false;
    }

    GStreamer::logDecoderRanks();
    GStreamer::setCodecPriorities(static_cast<GStreamer::VideoDecoderOptions>(
        SettingsManager::instance()->videoSettings()->forceVideoDecoder()->rawValue().toInt()));

    GstElement *sink = gst_element_factory_make("qml6glsink", nullptr);
    if (!sink) {
        qCCritical(GStreamerLog) << "Failed to create qml6glsink element";
        return false;
    }
    gst_clear_object(&sink);

    if (GStreamer::didExternalPluginLoaderFail()) {
        qCCritical(GStreamerLog)
            << "GStreamer external plugin loader failed. Check GST_PLUGIN_SCANNER and bundled runtime paths.";
        return false;
    }

    return true;
}

bool initialize()
{
    prepareEnvironment();

    GStreamer::resetExternalPluginLoaderFailure();
    GStreamer::redirectGLibLogging();
    GStreamer::configureDebugLogging();

    if (!_initGstRuntime()) {
        return false;
    }

    return completeInit();
}

void *createVideoSink(QQuickItem *widget, QObject * /*parent*/)
{
    GstElement *videoSinkBin = gst_element_factory_make("qgcvideosinkbin", NULL);
    if (videoSinkBin) {
        if (widget) {
            g_object_set(videoSinkBin, "widget", widget, NULL);
        }
    } else {
        qCCritical(GStreamerLog) << "gst_element_factory_make('qgcvideosinkbin') failed";
    }

    return videoSinkBin;
}

void releaseVideoSink(void *sink)
{
    if (!sink) return;
    GstElement *videoSink = GST_ELEMENT(sink);
    gst_clear_object(&videoSink);
}

VideoReceiver *createVideoReceiver(QObject *parent)
{
    return new GstVideoReceiver(parent);
}

} // namespace GStreamer
