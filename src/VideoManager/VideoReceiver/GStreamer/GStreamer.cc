#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "AppSettings.h"
#include "QGCLoggingCategory.h"
#include "GstVideoReceiver.h"
#include "SettingsManager.h"
#include "VideoSettings.h"
#include "Fact.h"

#include "GstAppSinkAdapter.h"
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#  include "HwBuffers/GstGlContextBridge.h"
#endif

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtQuick/QQuickItem>

#include <atomic>

#ifdef Q_OS_LINUX
#include <dlfcn.h>
#endif

#include <gst/gst.h>

QGC_LOGGING_CATEGORY(GStreamerLog, "Video.GStreamer.GStreamer")
QGC_LOGGING_CATEGORY(GStreamerDecoderRanksLog, "Video.GStreamer.GStreamer.DecoderRanks")

#ifdef Q_OS_IOS
extern "C" {
void gst_ios_pre_init(void);
void gst_ios_post_init(void);
}
#endif

G_BEGIN_DECLS
#ifdef QGC_GST_STATIC_BUILD
GST_PLUGIN_STATIC_DECLARE(app);
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(isomp4);
GST_PLUGIN_STATIC_DECLARE(libav);
GST_PLUGIN_STATIC_DECLARE(matroska);
GST_PLUGIN_STATIC_DECLARE(mpegtsdemux);
GST_PLUGIN_STATIC_DECLARE(openh264);
GST_PLUGIN_STATIC_DECLARE(playback);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(rtsp);
GST_PLUGIN_STATIC_DECLARE(sdpelem);
GST_PLUGIN_STATIC_DECLARE(tcp);
GST_PLUGIN_STATIC_DECLARE(typefindfunctions);
GST_PLUGIN_STATIC_DECLARE(udp);
// gst 1.22 merged videoconvert+videoscale into videoconvertscale, but custom/embedded
// gst builds may keep the legacy split. FindQGCGStreamer.cmake exports GST_PLUGIN_<name>_FOUND
// so we declare against what the linker actually has, not a version assumption.
#ifdef GST_PLUGIN_videoconvertscale_FOUND
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
#endif
#ifdef GST_PLUGIN_videoconvert_FOUND
GST_PLUGIN_STATIC_DECLARE(videoconvert);
#endif
#ifdef GST_PLUGIN_videoscale_FOUND
GST_PLUGIN_STATIC_DECLARE(videoscale);
#endif
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

GST_PLUGIN_STATIC_DECLARE(qgc);
G_END_DECLS

namespace GStreamer
{

namespace {

static std::atomic<bool> s_envPathsValid{true};
static QMutex s_envPathsMutex;
static QString s_envPathsError;

void _registerPlugins()
{
#ifdef QGC_GST_STATIC_BUILD
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(isomp4);
    GST_PLUGIN_STATIC_REGISTER(libav);
    GST_PLUGIN_STATIC_REGISTER(matroska);
    GST_PLUGIN_STATIC_REGISTER(mpegtsdemux);
    GST_PLUGIN_STATIC_REGISTER(openh264);
    GST_PLUGIN_STATIC_REGISTER(playback);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(rtsp);
    GST_PLUGIN_STATIC_REGISTER(sdpelem);
    GST_PLUGIN_STATIC_REGISTER(tcp);
    GST_PLUGIN_STATIC_REGISTER(typefindfunctions);
    GST_PLUGIN_STATIC_REGISTER(udp);
#ifdef GST_PLUGIN_videoconvertscale_FOUND
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
#endif
#ifdef GST_PLUGIN_videoconvert_FOUND
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
#endif
#ifdef GST_PLUGIN_videoscale_FOUND
    GST_PLUGIN_STATIC_REGISTER(videoscale);
#endif
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

    GST_PLUGIN_STATIC_REGISTER(qgc);
}

void _resetEnvValidation()
{
    const QMutexLocker locker(&s_envPathsMutex);
    s_envPathsError.clear();
    s_envPathsValid.store(true, std::memory_order_release);
}

// Used by every platform branch except iOS.
[[maybe_unused]] QString _cleanJoin(const QString &base, const QString &relative)
{
    return QDir::cleanPath(QDir(base).filePath(relative));
}

[[maybe_unused]] void _setGstEnv(const char *name, const QString &value)
{
    qputenv(name, value.toUtf8());
    qCDebug(GStreamerLog) << "  " << name << "=" << value;
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)

void _setEnvValidationError(const QString &error)
{
    const QMutexLocker locker(&s_envPathsMutex);
    s_envPathsError = error;
    s_envPathsValid.store(false, std::memory_order_release);
    qCCritical(GStreamerLog) << error;
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

#if defined(Q_OS_MACOS)
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
#endif

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


void _sanitizePythonEnvForScanner()
{
    static constexpr const char *varsToUnset[] = {
        "PYTHONHOME",
        "PYTHONPATH",
        "VIRTUAL_ENV",
        "CONDA_PREFIX",
        "CONDA_DEFAULT_ENV",
        "PYTHONUSERBASE",
    };

    for (const char *name : varsToUnset) {
        _unsetEnv(name);
    }

    _setGstEnv("PYTHONNOUSERSITE", QStringLiteral("1"));
}

void _applyGstEnvVars(const QString &pluginDir, const QString &gioModDir,
                      const QString &scannerPath, const QString &ptpPath)
{
    qCDebug(GStreamerLog) << "Applying GStreamer environment:";

    _sanitizePythonEnvForScanner();
    _clearManagedGstEnvVars();
    _setGstEnv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", QStringLiteral("no"));
    _setGstEnv("GST_REGISTRY_FORK", QStringLiteral("no"));
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
    // Try the bare soname first — dlopen resolves it via ldconfig/LD_LIBRARY_PATH,
    // which works on NixOS, Guix, and other non-FHS distros.
    // Fall back to hardcoded paths for environments where the bare name fails.
    static constexpr const char *kGioSoPaths[] = {
        "libgio-2.0.so.0",
        "/usr/lib/x86_64-linux-gnu/libgio-2.0.so.0",
        "/usr/lib/aarch64-linux-gnu/libgio-2.0.so.0",
        "/usr/lib/arm-linux-gnueabihf/libgio-2.0.so.0",
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

#if defined(Q_OS_MACOS)
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
#endif

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

#endif // !Q_OS_ANDROID && !Q_OS_IOS

void _setGstEnvVars()
{
    _resetEnvValidation();

    const QString appDir = QCoreApplication::applicationDirPath();
    qCDebug(GStreamerLog) << "App directory:" << appDir;

#if defined(Q_OS_MACOS)
    const QString frameworkDir = _cleanJoin(appDir, "../Frameworks/GStreamer.framework");
    QString rootDir = _firstExistingPath({
        _cleanJoin(frameworkDir, "Versions/1.0"),
        _cleanJoin(frameworkDir, "Versions/Current"),
        frameworkDir,
    });
    if (rootDir.isEmpty()) {
        rootDir = _cleanJoin(frameworkDir, "Versions/1.0");
    }

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

    bool validBundlePaths = true;
    if (!pluginDirs.isEmpty()) {
        validBundlePaths = _validateBundledDesktopPaths(QStringLiteral("macOS"), pluginDirs, scanner);
    }
    if (hasBundledFramework) {
        validBundlePaths = validBundlePaths && _validateMacBundlePaths(rootDir, pluginDirs, scanner);
    }

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

        // Ensure the app's bin directory is on PATH so that child processes
        // (gst-plugin-scanner.exe) can locate GStreamer DLLs installed
        // alongside the main executable.
        const QByteArray curPath = qgetenv("PATH");
        const QByteArray binDir = QDir::toNativeSeparators(appDir).toUtf8();
        if (!curPath.split(';').contains(binDir)) {
            qputenv("PATH", binDir + ";" + curPath);
        }
    }

#elif defined(Q_OS_ANDROID)
    // Android uses static plugins — no GST_PLUGIN_PATH needed. But fontconfig
    // and TLS need env vars pointing to the app's files/cache dirs where
    // GStreamer.java copied fonts and certificates.
    {
        const QString filesDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

        if (!filesDir.isEmpty()) {
            _setGstEnv("HOME", filesDir);
            _setGstEnv("FONTCONFIG_PATH", _cleanJoin(filesDir, "fontconfig"));
            _setGstEnv("CA_CERTIFICATES", _cleanJoin(filesDir, "ssl/certs/ca-certificates.crt"));
            _setGstEnv("XDG_DATA_DIRS", filesDir);
            _setGstEnv("XDG_CONFIG_DIRS", filesDir);
            _setGstEnv("XDG_CONFIG_HOME", filesDir);
            _setGstEnv("XDG_DATA_HOME", filesDir);
        }

        if (!cacheDir.isEmpty()) {
            _setGstEnv("TMP", cacheDir);
            _setGstEnv("TEMP", cacheDir);
            _setGstEnv("TMPDIR", cacheDir);
            _setGstEnv("XDG_CACHE_HOME", cacheDir);
            _setGstEnv("XDG_RUNTIME_DIR", cacheDir);
            _setGstEnv("GST_REGISTRY", _cleanJoin(cacheDir, "registry.bin"));
        }

        _setGstEnv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", QStringLiteral("no"));
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
    // Mirror the install-time verification list so a stripped registry fails loudly here
    // instead of waiting for first stream attempt with a misleading "no source element".
    static constexpr const char *requiredPlugins[] = {
        "qgc", "coreelements", "playback", "rtp", "rtpmanager", "rtsp", "tcp", "udp",
    };
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

void _logDecoderRanks()
{
    GList *factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);

    if (!factories) {
        qCDebug(GStreamerDecoderRanksLog) << "No video decoder factories found";
        return;
    }

    factories = g_list_sort(factories, [](gconstpointer lhs, gconstpointer rhs) -> gint {
        const guint lhsRank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(lhs));
        const guint rhsRank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(rhs));
        if (lhsRank != rhsRank) {
            return (lhsRank > rhsRank) ? -1 : 1;
        }
        return g_strcmp0(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(lhs)),
                         gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(rhs)));
    });

    qCDebug(GStreamerDecoderRanksLog) << "Video decoder ranks:";
    for (GList *node = factories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(node->data);
        GstPluginFeature *feature = GST_PLUGIN_FEATURE(factory);
        const gchar *featureName = gst_plugin_feature_get_name(feature);
        const guint rank = gst_plugin_feature_get_rank(feature);
        const gchar *klass = gst_element_factory_get_klass(factory);
        const bool isHw = GStreamer::isHardwareDecoderFactory(factory);

        GstPlugin *plugin = gst_plugin_feature_get_plugin(feature);
        const gchar *pluginName = plugin ? gst_plugin_get_name(plugin) : "?";

        qCDebug(GStreamerDecoderRanksLog).noquote()
            << QStringLiteral("  [%1] %2/%3 rank=%4 (%5)")
                   .arg(isHw ? QStringLiteral("HW") : QStringLiteral("SW"),
                        QString::fromUtf8(pluginName),
                        QString::fromUtf8(featureName))
                   .arg(rank)
                   .arg(QString::fromUtf8(klass));

        if (plugin) {
            gst_object_unref(plugin);
        }
    }

    gst_plugin_feature_list_free(factories);
}

void _configureDebugLogging()
{
    gst_debug_remove_log_function(gst_debug_log_default);
    gst_debug_add_log_function(GStreamer::qtGstLog, nullptr, nullptr);

    if (!qEnvironmentVariableIsEmpty("GST_DEBUG")) {
        return;
    }

    QSettings settings;
    if (settings.contains(AppSettings::gstDebugLevelName)) {
        const int level = qBound(0, settings.value(AppSettings::gstDebugLevelName).toInt(),
                                 static_cast<int>(GST_LEVEL_MEMDUMP));
        gst_debug_set_default_threshold(static_cast<GstDebugLevel>(level));
    }
}

} // anonymous namespace

void setDebugLevel(int level)
{
    if (!gst_is_initialized()) {
        return;
    }
    const int clamped = qBound(0, level, static_cast<int>(GST_LEVEL_MEMDUMP));
    gst_debug_set_default_threshold(static_cast<GstDebugLevel>(clamped));
    qCDebug(GStreamerLog) << "GStreamer debug threshold set to" << clamped;
}

void prepareEnvironment()
{
    _setGstEnvVars();
}

namespace {

bool _initGstRuntime()
{
    if (!s_envPathsValid.load(std::memory_order_acquire)) {
        const QMutexLocker locker(&s_envPathsMutex);
        qCCritical(GStreamerLog) << "Invalid GStreamer environment configuration:" << s_envPathsError;
        return false;
    }

    // Cache arguments on the stack — QCoreApplication::arguments() is not thread-safe,
    // but this runs early during init before concurrent access is possible.
    const QStringList args = QCoreApplication::arguments();
    QByteArrayList argStorage;
    argStorage.reserve(args.size());
    for (const QString &arg : args) {
        argStorage.append(arg.toUtf8());
    }

    QVarLengthArray<char*, 16> argv;
    for (QByteArray &arg : argStorage) {
        argv.append(arg.data());
    }

    int argc = argv.size();
    char **argvPtr = argv.data();
    GError *error = nullptr;

#ifdef Q_OS_IOS
    gst_ios_pre_init();
#endif

    if (!gst_init_check(&argc, &argvPtr, &error)) {
        qCCritical(GStreamerLog) << "Failed to initialize GStreamer:"
                                  << (error ? error->message : "unknown error");
        g_clear_error(&error);
        return false;
    }

#ifdef Q_OS_IOS
    gst_ios_post_init();
#endif

    return true;
}

} // anonymous namespace

bool completeInit()
{
    if (!gst_is_initialized()) {
        qCCritical(GStreamerLog) << "completeInit called but gst_init() has not been called";
        return false;
    }

    _configureDebugLogging();

    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);
    qCDebug(GStreamerLog) << "GStreamer initialized:" << major << "." << minor << "." << micro;

#ifdef QGC_GST_BUILD_VERSION_MAJOR
    if (major != QGC_GST_BUILD_VERSION_MAJOR || minor != QGC_GST_BUILD_VERSION_MINOR) {
        qCWarning(GStreamerLog) << "GStreamer version mismatch: built against"
            << QGC_GST_BUILD_VERSION_MAJOR << "." << QGC_GST_BUILD_VERSION_MINOR
            << "but runtime is" << major << "." << minor << "." << micro;
    }
#endif

    _registerPlugins();

    if (!_verifyPlugins()) {
        qCCritical(GStreamerLog) << "Plugin verification failed";
        return false;
    }

    _logDecoderRanks();

    GstElementFactory *appsinkFactory = gst_element_factory_find("appsink");
    if (!appsinkFactory) {
        qCCritical(GStreamerLog) << "appsink factory not found — videoconvert→appsink path unavailable";
        return false;
    }
    qCDebug(GStreamerLog) << "appsink factory available (videoconvert → appsink → QVideoSink)";
    gst_object_unref(appsinkFactory);

    if (GStreamer::didExternalPluginLoaderFail()) {
        qCCritical(GStreamerLog)
            << "GStreamer external plugin loader failed. Check GST_PLUGIN_SCANNER and bundled runtime paths.";
        return false;
    }

    return true;
}

bool initialize()
{
    GStreamer::resetExternalPluginLoaderFailure();
    GStreamer::redirectGLibLogging();

    // Suppress GStreamer's default stderr debug handler before gst_init_check()
    // to prevent raw ANSI escape codes from corrupting the terminal on macOS.
    gst_debug_remove_log_function(gst_debug_log_default);

    if (!_initGstRuntime()) {
        return false;
    }

    return completeInit();
}

// Ownership protocol for the video sink element:
//   createVideoSink  — returns a floating-ref element (refcount conceptually 1).
//   startDecoding     — calls gst_object_ref (sinks float, refcount=1).
//   _ensureVideoSinkInPipeline — gst_object_ref (+1=2), gst_bin_add (+1=3).
//   _shutdownDecodingBranch   — gst_bin_remove (-1=2), gst_clear_object (-1=1).
//   releaseVideoSink  — gst_clear_object (-1=0, freed).
void *createVideoSink(QQuickItem * /*widget*/, QObject * /*parent*/)
{
    GstElement *videoSinkBin = nullptr;
    // All bin tunables are construct-only — properties drive behavior, no env-var indirection.
    VideoSettings *const vs = SettingsManager::instance()->videoSettings();
    const QByteArray conversionElement = vs->videoConversionElement()->rawValue().toString().toUtf8();
    const gboolean disablePar = vs->disablePixelAspectRatio()->rawValue().toBool() ? TRUE : FALSE;
#if defined(QGC_HAS_ANY_GPU_PATH)
    // gpu-zerocopy is construct-only on the bin; adapter reads it back from the bin so the two halves can't desync.
    // Bin defaults to gpu-zerocopy=FALSE — every GPU-capable platform must set it explicitly here or zero-copy stays off.
    const bool forceCpu = vs->forceCpuVideoPath()->rawValue().toBool();
    const bool swDecoder = vs->forceVideoDecoder()->rawValue().toInt()
                           == GStreamer::ForceVideoDecoderSoftware;
    const bool gpuZeroCopy = !forceCpu && !swDecoder;
    if (GstElementFactory *factory = gst_element_factory_find("qgcvideosinkbin")) {
        videoSinkBin = gst_element_factory_create_full(factory,
                                                       "gpu-zerocopy", gpuZeroCopy ? TRUE : FALSE,
                                                       "conversion-element",
                                                            conversionElement.isEmpty() ? nullptr : conversionElement.constData(),
                                                       "disable-par", disablePar,
                                                       NULL);
        gst_object_unref(factory);
    }
#else
    if (GstElementFactory *factory = gst_element_factory_find("qgcvideosinkbin")) {
        videoSinkBin = gst_element_factory_create_full(factory,
                                                       "conversion-element",
                                                            conversionElement.isEmpty() ? nullptr : conversionElement.constData(),
                                                       "disable-par", disablePar,
                                                       NULL);
        gst_object_unref(factory);
    }
#endif
    if (!videoSinkBin) {
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

bool setupAppSinkAdapter(void *sinkBin, QVideoSink *videoSink, QObject *adapterParent)
{
    if (!sinkBin || !videoSink || !adapterParent) {
        // adapterParent owns the adapter's QObject lifetime; without it the adapter
        // would leak on success since the caller has no handle to destroy it.
        qCWarning(GStreamerLog) << "setupAppSinkAdapter: null sinkBin, videoSink, or adapterParent";
        return false;
    }

    // Idempotent re-setup: tear down any previous adapter parented under this caller
    // before creating a new one, so repeated startDecoding cycles don't accumulate
    // dangling adapters under the same parent.
    const auto existing = adapterParent->findChildren<GstAppSinkAdapter *>(
        QString(), Qt::FindDirectChildrenOnly);
    for (GstAppSinkAdapter *old : existing) {
        // setActive(false) BEFORE teardown — teardown() nulls the sink under lock and
        // the empty-frame push would no-op. Order matters for the ghost-frame fix to land.
        old->setActive(false);
        old->teardown();
        old->deleteLater();
    }

    // Clear the GL bridge's exhausted-retry latch so a pipeline restart that occurs after Qt's
    // globalShareContext finally appeared can prime on the next NEED_CONTEXT. No-op when the
    // bridge is already primed (keeps cached display/context across restarts to avoid the
    // expensive re-discovery dance).
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    GstGlContextBridge::rearm();
#endif

    auto *adapter = new GstAppSinkAdapter(adapterParent);
    if (!adapter->setup(GST_ELEMENT(sinkBin), videoSink)) {
        qCCritical(GStreamerLog) << "GstAppSinkAdapter::setup() failed";
        adapter->deleteLater();
        return false;
    }

    // Hand the display refresh rate to the adapter so it can keep max-time bounded by the
    // panel's redraw budget; the adapter combines this with the negotiated stream framerate
    // on each caps change. setRefreshRate is a no-op when QScreen is unavailable
    // (headless/early boot) — bin's 33 ms default stays in effect.
    const qreal refreshHz = QGuiApplication::primaryScreen()
        ? QGuiApplication::primaryScreen()->refreshRate() : 0.0;
    if (refreshHz >= 1.0) {
        adapter->setRefreshRate(refreshHz);
    }

    // Opt-in OBS-style smoothing ring (default off). Read here so the streaming thread
    // never has to dip into the SettingsManager. Pass refreshHz so the tick paces with
    // the panel; the adapter falls back to 60 Hz when refreshHz is 0.
    if (SettingsManager::instance()->videoSettings()->frameSmoothingEnabled()->rawValue().toBool()) {
        adapter->setSmoothingEnabled(true, refreshHz);
    }
    // Connect latencyChanged so the adapter re-queries immediately on RTSP jitter-buffer reconfigures.
    if (auto *gstReceiver = qobject_cast<GstVideoReceiver *>(adapterParent)) {
        QObject::connect(gstReceiver, &GstVideoReceiver::latencyChanged,
                         adapter, &GstAppSinkAdapter::requestLatencyRefresh,
                         Qt::DirectConnection);
    }
    return true;
}

void setAppSinkAdaptersActive(QObject *adapterParent, bool active)
{
    if (!adapterParent) return;
    const auto adapters = adapterParent->findChildren<GstAppSinkAdapter *>(
        QString(), Qt::FindDirectChildrenOnly);
    for (GstAppSinkAdapter *a : adapters) {
        a->setActive(active);
    }
}

} // namespace GStreamer
