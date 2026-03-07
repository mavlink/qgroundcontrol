#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "AppSettings.h"
#include "GstVideoReceiver.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtQuick/QQuickItem>

#include <atomic>

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

// Written by prepareEnvironment() on the main thread, read by _initGstRuntime()
// on a QtConcurrent worker thread. The atomic bool provides the happens-before
// guarantee; the mutex guards the non-atomic QString.
static std::atomic<bool> s_envPathsValid{true};
static QMutex s_envPathsMutex;
static QString s_envPathsError;

// Android and iOS platform plugins (e.g. androidmedia, applemedia) are
// registered by the generated gst_init_static_plugins() in
// gstreamer_android-1.0.c / gst_ios_init.m — not by this function.
// QGC_GST_STATIC_BUILD covers desktop static builds only.
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

    // qml6 and qgc are QGC-built plugins, always registered explicitly.
    // Platform plugins come from gst_init_static_plugins() (Android/iOS) or dynamic loading.
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

void _applyGstEnvVars(const QString &pluginDir, const QString &gioModDir,
                      const QString &scannerPath, const QString &ptpPath)
{
    qCDebug(GStreamerLog) << "Applying GStreamer environment:";

    _clearManagedGstEnvVars();
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

void _sanitizePythonEnvForScanner()
{
    // gst-plugin-scanner may initialize Python (gst-python). Virtualenv/conda
    // environment variables frequently point to interpreters without gi.
    static constexpr const char *varsToUnset[] = {
        "PYTHONHOME",
        "PYTHONPATH",
        "VIRTUAL_ENV",
        "CONDA_PREFIX",
        "CONDA_DEFAULT_ENV",
        "PYTHONUSERBASE",
    };

    for (const char *name : varsToUnset) {
        if (qEnvironmentVariableIsSet(name)) {
            qunsetenv(name);
            qCDebug(GStreamerLog) << "  unset" << name;
        }
    }

    // Keep scanner imports deterministic and avoid user-site packages.
    _setGstEnv("PYTHONNOUSERSITE", QStringLiteral("1"));
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

    if (scannerPath.isEmpty()) {
        _setEnvValidationError(QStringLiteral(
            "GStreamer: bundled macOS framework found but gst-plugin-scanner is missing under %1")
            .arg(bundleFrameworkRoot));
        return false;
    }

    if (!_isExecutableFile(scannerPath)) {
        _setEnvValidationError(QStringLiteral(
            "GStreamer: gst-plugin-scanner is not executable: %1")
            .arg(scannerPath));
        return false;
    }

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

    if (scannerPath.isEmpty()) {
        _setEnvValidationError(QStringLiteral(
            "GStreamer: %1 bundled gst-plugin-scanner is missing.")
            .arg(platformLabel));
        return false;
    }

    if (!_isExecutableFile(scannerPath)) {
        _setEnvValidationError(QStringLiteral(
            "GStreamer: %1 gst-plugin-scanner is not executable: %2")
            .arg(platformLabel, scannerPath));
        return false;
    }

    return true;
}

void _setGstEnvVars()
{
    _resetEnvValidation();
    _sanitizePythonEnvForScanner();

    const QString appDir = QCoreApplication::applicationDirPath();
    qCDebug(GStreamerLog) << "App directory:" << appDir;

#if defined(Q_OS_MACOS)
#if defined(QGC_GST_MACOS_FRAMEWORK)
    const QString frameworkDir = _cleanJoin(appDir, "../Frameworks/GStreamer.framework");
    QString rootDir = _firstExistingPath({
        _cleanJoin(frameworkDir, "Versions/1.0"),
        _cleanJoin(frameworkDir, "Versions/Current"),
        frameworkDir,
    });
    if (rootDir.isEmpty()) {
        rootDir = _cleanJoin(frameworkDir, "Versions/1.0");
    }
    const QString pluginDirs = _joinExistingPaths({
        _cleanJoin(rootDir, "lib/gstreamer-1.0"),
        _cleanJoin(appDir, "../lib/gstreamer-1.0"),
    });
    const QString gioMod = _firstExistingPath({
        _cleanJoin(rootDir, "lib/gio/modules"),
        _cleanJoin(appDir, "../lib/gio/modules"),
    });
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

    if (hasBundledFramework) {
        _setGstEnv("GTK_PATH", rootDir);
    }
#else
    const QString frameworkDir = _cleanJoin(appDir, "../Frameworks/GStreamer.framework");
    QString frameworkRoot = _firstExistingPath({
        _cleanJoin(frameworkDir, "Versions/1.0"),
        _cleanJoin(frameworkDir, "Versions/Current"),
        frameworkDir,
    });
    if (frameworkRoot.isEmpty()) {
        frameworkRoot = _cleanJoin(frameworkDir, "Versions/1.0");
    }
    const QString pluginDirs = _joinExistingPaths({
        _cleanJoin(appDir, "../lib/gstreamer-1.0"),
        _cleanJoin(frameworkRoot, "lib/gstreamer-1.0"),
    });
    const QString gioMod = _firstExistingPath({
        _cleanJoin(appDir, "../lib/gio/modules"),
        _cleanJoin(frameworkRoot, "lib/gio/modules"),
    });
    const QString scanner = _firstExistingPath({
        _cleanJoin(appDir, "../libexec/gstreamer-1.0/gst-plugin-scanner"),
        _cleanJoin(frameworkRoot, "libexec/gstreamer-1.0/gst-plugin-scanner"),
    });
    const QString ptp = _firstExistingPath({
        _cleanJoin(appDir, "../libexec/gstreamer-1.0/gst-ptp-helper"),
        _cleanJoin(frameworkRoot, "libexec/gstreamer-1.0/gst-ptp-helper"),
    });
    const bool hasBundledFramework = QFileInfo::exists(frameworkDir);

    bool validBundlePaths = true;
    if (!pluginDirs.isEmpty()) {
        validBundlePaths = _validateBundledDesktopPaths(QStringLiteral("macOS"), pluginDirs, scanner);
    }
    if (hasBundledFramework) {
        validBundlePaths = validBundlePaths && _validateMacBundlePaths(frameworkRoot, pluginDirs, scanner);
    }

    if (!pluginDirs.isEmpty() && validBundlePaths) {
        _applyGstEnvVars(pluginDirs, gioMod, scanner, ptp);
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
    // logic). Only apply fallback paths for non-AppImage builds.
    if (!qEnvironmentVariableIsSet("GST_PLUGIN_PATH_1_0")) {
        const QString libDir = _cleanJoin(appDir, "../lib");
        const QString pluginDir = _cleanJoin(libDir, "gstreamer-1.0");
        const QString gioMod = _cleanJoin(libDir, "gio/modules");
        const QString scanner = _cleanJoin(libDir, "gstreamer1.0/gstreamer-1.0/gst-plugin-scanner");
        const QString ptp = _cleanJoin(libDir, "gstreamer1.0/gstreamer-1.0/gst-ptp-helper");

        if (QFileInfo::exists(pluginDir)
            && _validateBundledDesktopPaths(QStringLiteral("Linux"), pluginDir, scanner)) {
            _applyGstEnvVars(pluginDir, gioMod, scanner, ptp);
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
        return false;
    }

    return true;
}

bool completeInit()
{
    if (!gst_is_initialized()) {
        qCCritical(GStreamerLog) << "completeInit called but gst_init() has not been called";
        return false;
    }

    _configureDebugLogging();

    gchar *version = gst_version_string();
    qCDebug(GStreamerLog) << "GStreamer initialized:" << version;
    g_free(version);

    _registerPlugins();

    if (!_verifyPlugins()) {
        qCCritical(GStreamerLog) << "Plugin verification failed";
        return false;
    }

    _logDecoderRanks();

    GstElementFactory *sinkFactory = gst_element_factory_find("qml6glsink");
    if (!sinkFactory) {
        qCCritical(GStreamerLog) << "qml6glsink factory not found";
        return false;
    }
    gst_object_unref(sinkFactory);

    GstElementFactory *playbinFactory = gst_element_factory_find("playbin");
    if (!playbinFactory) {
        qCCritical(GStreamerLog) << "playbin factory not found";
        return false;
    }
    gst_object_unref(playbinFactory);

    if (GStreamer::didExternalPluginLoaderFail()) {
        qCCritical(GStreamerLog)
            << "GStreamer external plugin loader failed. Check GST_PLUGIN_SCANNER and bundled runtime paths.";
        return false;
    }

    return true;
}

bool initialize()
{
    // Capture GLib errors emitted during gst_init_check(), including
    // external plugin loader failures.
    GStreamer::resetExternalPluginLoaderFailure();
    GStreamer::redirectGLibLogging();

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
