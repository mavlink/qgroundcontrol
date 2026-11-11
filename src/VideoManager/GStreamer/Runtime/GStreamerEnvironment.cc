#include "GStreamerEnvironment.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QSaveFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtCore/QSysInfo>
#include <QtCore/QThread>

#include "QGCLoggingCategory.h"

#ifdef Q_OS_ANDROID
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#endif

#include <gst/gst.h>

#ifdef Q_OS_WIN
#include <cstdlib>
#endif

QGC_LOGGING_CATEGORY(GStreamerEnvLog, "Video.GStreamer.Environment")

namespace {

using GStreamer::Environment::BundledRuntime;
using GStreamer::Environment::BundleLayout;
using GStreamer::Environment::DesktopInstall;

// Single source of truth for the managed plugin-path/scanner vars: keeps _clearManagedGstEnvVars
// and logDiagnostics from drifting. Each site layers its own extras (GIO/PTP, registry) on top.
constexpr const char* kManagedGstPathVars[] = {
    "GST_PLUGIN_PATH",    "GST_PLUGIN_PATH_1_0",    "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0",
    "GST_PLUGIN_SCANNER", "GST_PLUGIN_SCANNER_1_0",
};

BundledRuntime s_bundledRuntime;

// What this layer last set each variable to, so that preparing the environment again doesn't take its own values for
// the user's.
QHash<QByteArray, QString> s_managedEnvValues;

QString _cleanJoin(const QString& base, const QString& relative)
{
    return QDir::cleanPath(QDir(base).filePath(relative));
}

QStringList _cleanJoinAll(const QStringList& bases, const QString& relative)
{
    QStringList joined;
    joined.reserve(bases.size());
    for (const QString& base : bases) {
        joined.append(_cleanJoin(base, relative));
    }
    return joined;
}

void _setGstEnv(const char* name, const QString& value)
{
#ifdef Q_OS_WIN
    // GLib reads the wide environment; qputenv's _putenv_s would decode these UTF-8 bytes in the ANSI
    // code page and corrupt non-ASCII install paths. g_setenv sets the process environment and GLib's C runtime copy;
    // _wputenv_s sets QGC's, which qEnvironmentVariable reads and which is a separate (debug) runtime in Debug builds.
    (void) g_setenv(name, value.toUtf8().constData(), TRUE);
    (void) _wputenv_s(QString::fromLatin1(name).toStdWString().c_str(), value.toStdWString().c_str());
#else
    qputenv(name, value.toUtf8());
#endif
    s_managedEnvValues.insert(name, value);
    qCDebug(GStreamerEnvLog) << "  " << name << "=" << value;
}

bool _setByUser(const char* name)
{
    if (!qEnvironmentVariableIsSet(name)) {
        return false;
    }
    const auto managed = s_managedEnvValues.constFind(name);
    return (managed == s_managedEnvValues.constEnd()) || (qEnvironmentVariable(name) != managed.value());
}

QString _firstExistingPath(const QStringList& paths)
{
    for (const QString& path : paths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }

    return {};
}

// An AppImage is mounted, and a quarantined macOS app translocated, to a new random directory on every launch: name
// such an install after what stays put, the AppImage file or the translocated app's name.
QString _stableInstallPath(const QString& installPath)
{
    // Whatever an AppImage starts inherits APPIMAGE and APPDIR, so they only name an install inside the mount.
    QString appImage = qEnvironmentVariable("APPIMAGE");
    const QString mountDir = qEnvironmentVariable("APPDIR");
    if (!appImage.isEmpty() && !mountDir.isEmpty() &&
        installPath.startsWith(QDir::cleanPath(mountDir) + QLatin1Char('/'))) {
        return appImage;
    }

    static const QRegularExpression translocation(QStringLiteral("/AppTranslocation/[^/]+/"));
    return QString(installPath).replace(translocation, QStringLiteral("/AppTranslocation/"));
}

// GStreamer only rewrites a registry when the plugins change, so touch @p registry to mark it in use, and remove the
// registries next to it that no install has used for a month (an old GStreamer version, a replaced AppImage). Removing
// one that is still in use only costs its install a rescan.
void _pruneUnusedRegistries(const QString& registry)
{
    static constexpr int kUnusedRegistryDays = 30;

    const QDateTime now = QDateTime::currentDateTimeUtc();
    QFile current(registry);
    if (current.open(QIODevice::ReadWrite | QIODevice::ExistingOnly)) {
        (void) current.setFileTime(now, QFileDevice::FileModificationTime);
        current.close();
    }

    const QFileInfo registryInfo(registry);
    const QDateTime cutoff = now.addDays(-kUnusedRegistryDays);
    const QFileInfoList registries =
        registryInfo.absoluteDir().entryInfoList({QStringLiteral("gstreamer-*.bin")}, QDir::Files);
    for (const QFileInfo& other : registries) {
        if ((other.fileName() != registryInfo.fileName()) && (other.lastModified() < cutoff) &&
            QFile::remove(other.absoluteFilePath())) {
            qCDebug(GStreamerEnvLog) << "Removed unused GStreamer registry" << other.absoluteFilePath();
        }
    }
}

// A bundled runtime's plugins differ from the system GStreamer's, so sharing the default per-user registry would make
// each invalidate the other's plugin cache. The runtime version in the name keeps upgrades from reusing it, and the
// install hash keeps two installs of one version (stable and daily builds) from rescanning over each other's.
QString _usePrivateRegistry(const QString& cacheDir, const QString& installPath)
{
    if (cacheDir.isEmpty() || _setByUser("GST_REGISTRY_1_0") || _setByUser("GST_REGISTRY")) {
        return {};
    }
    if (!QDir().mkpath(cacheDir)) {
        qCWarning(GStreamerEnvLog) << "Cannot create the GStreamer registry directory" << cacheDir;
        return {};
    }

    guint major = 0;
    guint minor = 0;
    guint micro = 0;
    guint nano = 0;
    gst_version(&major, &minor, &micro, &nano);
    const QByteArray installHash =
        QCryptographicHash::hash(_stableInstallPath(installPath).toUtf8(), QCryptographicHash::Sha256).toHex().left(8);
    QString registry = _cleanJoin(cacheDir, QStringLiteral("gstreamer-%1.%2.%3-%4-%5.bin")
                                                .arg(major)
                                                .arg(minor)
                                                .arg(micro)
                                                .arg(QSysInfo::buildCpuArchitecture(), QLatin1String(installHash)));
    _setGstEnv("GST_REGISTRY", registry);
    _pruneUnusedRegistries(registry);
    return registry;
}

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
void _setTmpDirVars(const QString& dir)
{
    _setGstEnv("TMP", dir);
    _setGstEnv("TEMP", dir);
    _setGstEnv("TMPDIR", dir);
}

// Points CA_CERTIFICATES, which the static plugin init makes GIO's default TLS database (gst_static_plugins.c.in), at
// the bundled @p caBundle unless it is set.
void _setCaBundleIfUnset(const QString& caBundle)
{
    if (qEnvironmentVariableIsEmpty("CA_CERTIFICATES") && QFileInfo::exists(caBundle)) {
        _setGstEnv("CA_CERTIFICATES", caBundle);
    }
}
#endif

#ifdef Q_OS_ANDROID
// Extracts an APK asset to destPath, which outlives the APK: an updated app replaces what an earlier version left.
bool _extractApkAsset(const char* assetPath, const QString& destPath)
{
    QJniObject ctx = QNativeInterface::QAndroidApplication::context();
    if (!ctx.isValid()) {
        qCWarning(GStreamerEnvLog) << "Cannot resolve Android Context for asset extraction";
        return false;
    }

    QJniObject jAssetMgr = ctx.callObjectMethod("getAssets", "()Landroid/content/res/AssetManager;");
    if (!jAssetMgr.isValid()) {
        qCWarning(GStreamerEnvLog) << "Context.getAssets() returned null";
        return false;
    }

    QJniEnvironment env;
    AAssetManager* am = AAssetManager_fromJava(env.jniEnv(), jAssetMgr.object());
    if (!am) {
        qCWarning(GStreamerEnvLog) << "AAssetManager_fromJava failed";
        return false;
    }

    AAsset* asset = AAssetManager_open(am, assetPath, AASSET_MODE_BUFFER);
    if (!asset) {
        qCDebug(GStreamerEnvLog) << "APK asset not present:" << assetPath;
        return false;
    }
    const off_t assetLen = AAsset_getLength(asset);
    const void* const buf = AAsset_getBuffer(asset);
    const bool ok = buf && (assetLen > 0) &&
                    GStreamer::Environment::writeIfChanged(
                        destPath, QByteArrayView(static_cast<const char*>(buf), static_cast<qsizetype>(assetLen)));
    AAsset_close(asset);
    if (!ok) {
        qCWarning(GStreamerEnvLog) << "Failed writing asset" << assetPath << "to" << destPath;
    }
    return ok;
}
#endif

// QGC_GST_SYSTEM_PLUGINS: a Linux build with QGC_GST_BUNDLE_PLUGINS=OFF, packaged with the system's plugins, which it
// always uses as they are.
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(QGC_GST_SYSTEM_PLUGINS)

void _unsetEnv(const char* name)
{
    if (qEnvironmentVariableIsSet(name)) {
        qunsetenv(name);
        qCDebug(GStreamerEnvLog) << "  unset" << name;
    }
}

void _setGstEnvIfExists(const char* name, const QString& path)
{
    if (QFileInfo::exists(path)) {
        _setGstEnv(name, path);
    }
}

bool _isExecutableFile(const QString& path)
{
    const QFileInfo fileInfo(path);
    return fileInfo.exists() && fileInfo.isFile() && fileInfo.isExecutable();
}

QString _joinExistingPaths(const QStringList& paths)
{
    QStringList existing;
    existing.reserve(paths.size());

    for (const QString& path : paths) {
        if (QFileInfo::exists(path) && !existing.contains(path)) {
            existing.append(path);
        }
    }

    return existing.join(QDir::listSeparator());
}

void _clearManagedGstEnvVars()
{
    static constexpr const char* gioPtpVars[] = {
        "GIO_EXTRA_MODULES", "GIO_MODULE_DIR", "GIO_USE_VFS", "GST_PTP_HELPER_1_0", "GST_PTP_HELPER",
    };

    for (const char* name : gioPtpVars) {
        _unsetEnv(name);
    }
    for (const char* name : kManagedGstPathVars) {
        _unsetEnv(name);
    }
}

void _setGstEnvIfExecutable(const char* name, const QString& path)
{
    if (_isExecutableFile(path)) {
        _setGstEnv(name, path);
    } else {
        _unsetEnv(name);
    }
}

void _applyGstEnvVars(const QString& pluginDir, const QString& gioModDir, const QString& scannerPath,
                      const QString& ptpPath)
{
    qCDebug(GStreamerEnvLog) << "Applying GStreamer environment:";

    _clearManagedGstEnvVars();
    _setGstEnvIfExists("GIO_EXTRA_MODULES", gioModDir);
    _setGstEnvIfExecutable("GST_PTP_HELPER_1_0", ptpPath);
    _setGstEnvIfExecutable("GST_PTP_HELPER", ptpPath);
    _setGstEnvIfExecutable("GST_PLUGIN_SCANNER_1_0", scannerPath);
    _setGstEnvIfExecutable("GST_PLUGIN_SCANNER", scannerPath);
    // SYSTEM_PATH replaces the default system/user plugin dirs. GST_PLUGIN_PATH (cleared above) would
    // only rescan the same dir, so leaving it unset loads the identical plugin set.
    _setGstEnv("GST_PLUGIN_SYSTEM_PATH_1_0", pluginDir);
    _setGstEnv("GST_PLUGIN_SYSTEM_PATH", pluginDir);
}

#if defined(Q_OS_LINUX)
void _applyGioCompatOverride(const QString& gioModDir)
{
    if (gioModDir.isEmpty()) {
        return;
    }

    // GIO 2.76+ needs bundled modules via GIO_MODULE_DIR with VFS forced local (AppImage launcher logic).
    // The in-process GLib (the bundled one when present) is the GIO that loads these modules.
    if (glib_check_version(2, 76, 0) == nullptr) {
        _unsetEnv("GIO_EXTRA_MODULES");
        _setGstEnv("GIO_MODULE_DIR", gioModDir);
        _setGstEnv("GIO_USE_VFS", QStringLiteral("local"));
    }
}
#endif

void _warnIfScannerMissing(const QString& platformLabel, const QString& scannerPath)
{
    if (scannerPath.isEmpty()) {
        qCWarning(GStreamerEnvLog) << "GStreamer:" << platformLabel
                                   << "bundled gst-plugin-scanner not found; GStreamer will use in-process scanning";
    } else if (!_isExecutableFile(scannerPath)) {
        qCWarning(GStreamerEnvLog) << "GStreamer:" << platformLabel
                                   << "gst-plugin-scanner is not executable:" << scannerPath;
    }
}

// Points GStreamer at the bundled runtime. Returns false, leaving the environment untouched, when no
// bundled plugin directory exists (system GStreamer is used as-is).
bool _applyBundleLayout(const QString& platformLabel, const BundleLayout& layout)
{
    const QString pluginDirs = _joinExistingPaths(layout.pluginDirs);
    if (pluginDirs.isEmpty()) {
        return false;
    }

    const QString scanner = _firstExistingPath(layout.scannerCandidates);
    _warnIfScannerMissing(platformLabel, scanner);
    const QString gioModuleDir = _firstExistingPath(layout.gioModuleDirs);
    _applyGstEnvVars(pluginDirs, gioModuleDir, scanner, _firstExistingPath(layout.ptpCandidates));
    (void) _usePrivateRegistry(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), pluginDirs);
    s_bundledRuntime = {.pluginDirs = pluginDirs.split(QDir::listSeparator()), .gioModuleDir = gioModuleDir};
    return true;
}

#endif  // !Q_OS_ANDROID && !Q_OS_IOS && !QGC_GST_SYSTEM_PLUGINS

VideoBackend::EnvPrepResult _setGstEnvVars(const QString& appDir)
{
    VideoBackend::EnvPrepResult result;
    s_bundledRuntime = {};

    qCDebug(GStreamerEnvLog) << "App directory:" << appDir;

#if defined(Q_OS_MACOS)
    const QString frameworkDir = _cleanJoin(appDir, "../Frameworks/GStreamer.framework");
#if defined(QGC_GST_MACOS_FRAMEWORK)
    const BundleLayout layout = GStreamer::Environment::bundleLayout(DesktopInstall::MacOSFramework, appDir);
#else
    const BundleLayout layout = GStreamer::Environment::bundleLayout(DesktopInstall::MacOSFlat, appDir);
#endif
    const bool hasBundledFramework = QFileInfo::exists(frameworkDir);

    if (!_applyBundleLayout(QStringLiteral("macOS"), layout) && hasBundledFramework) {
        result.ok = false;
        result.error =
            QStringLiteral("GStreamer: bundled macOS framework found but plugin directory is missing under %1")
                .arg(frameworkDir);
        qCCritical(GStreamerEnvLog) << result.error;
    }

#elif defined(Q_OS_WIN)
    if (_applyBundleLayout(QStringLiteral("Windows"),
                           GStreamer::Environment::bundleLayout(DesktopInstall::Windows, appDir))) {
        // Put the app's bin dir on PATH so child processes (gst-plugin-scanner.exe) can locate
        // GStreamer DLLs installed alongside the main executable.
        const QString curPath = qEnvironmentVariable("PATH");
        const QString binDir = QDir::toNativeSeparators(appDir);
        // Windows paths are case-insensitive; compare accordingly to avoid duplicate prepends.
        if (!curPath.split(QDir::listSeparator()).contains(binDir, Qt::CaseInsensitive)) {
            _setGstEnv("PATH", binDir + QDir::listSeparator() + curPath);
        }
    }

#elif defined(Q_OS_IOS)
    // Static plugins — no GST_PLUGIN_PATH; bundle resources read-only, scratch in sandbox. CA
    // bundle (MACOSX_PACKAGE_LOCATION) is consumed by qgc_load_gio_modules_and_ca() at static init.
    {
        const QString resources = appDir;
        const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        const QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        const QString tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

        if (!tmp.isEmpty()) {
            _setTmpDirVars(tmp);
        }
        if (!cache.isEmpty()) {
            _setGstEnv("XDG_CACHE_HOME", cache);
            _setGstEnv("XDG_CONFIG_HOME", cache);
            _setGstEnv("GST_REGISTRY", _cleanJoin(cache, "registry.bin"));
        }
        // The sandbox forbids fork(), and the plugins are static, so there is no scanner to run.
        _setGstEnv("GST_REGISTRY_FORK", QStringLiteral("no"));
        // Tutorial 5 binds XDG_RUNTIME_DIR to the read-only bundle; libGStreamer never writes there on iOS.
        if (!resources.isEmpty()) {
            _setGstEnv("XDG_RUNTIME_DIR", resources);
            _setGstEnv("XDG_DATA_DIRS", resources);
            _setGstEnv("XDG_CONFIG_DIRS", resources);
            _setGstEnv("XDG_DATA_HOME", resources);
        }
        if (!docs.isEmpty()) {
            _setGstEnv("HOME", docs);
        }

        if (!resources.isEmpty()) {
            _setCaBundleIfUnset(_cleanJoin(resources, "ssl/certs/ca-certificates.crt"));
        }
    }

#elif defined(Q_OS_ANDROID)
    // Static plugins — no GST_PLUGIN_PATH; APK assets are read-only, so the CA bundle is extracted
    // to filesDir, and again when an update changes it (we run gst_init from C++, not Java's GStreamer.init).
    {
        const QString filesDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

        if (!filesDir.isEmpty()) {
            _setGstEnv("HOME", filesDir);
            _setGstEnv("FONTCONFIG_PATH", _cleanJoin(filesDir, "fontconfig"));
            const QString caBundle = _cleanJoin(filesDir, "ssl/certs/ca-certificates.crt");
            if (_extractApkAsset("ssl/certs/ca-certificates.crt", caBundle)) {
                _setCaBundleIfUnset(caBundle);
            }
            _setGstEnv("XDG_DATA_DIRS", filesDir);
            _setGstEnv("XDG_CONFIG_DIRS", filesDir);
            _setGstEnv("XDG_CONFIG_HOME", filesDir);
            _setGstEnv("XDG_DATA_HOME", filesDir);
        }

        if (!cacheDir.isEmpty()) {
            _setTmpDirVars(cacheDir);
            _setGstEnv("XDG_CACHE_HOME", cacheDir);
            _setGstEnv("XDG_RUNTIME_DIR", cacheDir);
            _setGstEnv("GST_REGISTRY", _cleanJoin(cacheDir, "registry.bin"));
        }

        // The plugins are static and the APK ships no scanner; an app process must not fork() into one.
        _setGstEnv("GST_REGISTRY_FORK", QStringLiteral("no"));
    }

#elif defined(Q_OS_LINUX) && !defined(QGC_GST_SYSTEM_PLUGINS)
    // An AppImage or other bundle ships its runtime under <prefix>/lib; a plugin path set by the user wins.
    if (!_setByUser("GST_PLUGIN_PATH_1_0") && !_setByUser("GST_PLUGIN_PATH") &&
        !_setByUser("GST_PLUGIN_SYSTEM_PATH_1_0") && !_setByUser("GST_PLUGIN_SYSTEM_PATH")) {
        const BundleLayout layout = GStreamer::Environment::bundleLayout(DesktopInstall::Linux, appDir);
        if (_applyBundleLayout(QStringLiteral("Linux"), layout)) {
            _applyGioCompatOverride(s_bundledRuntime.gioModuleDir);
        }
    }
#endif

    return result;
}

}  // anonymous namespace

namespace GStreamer::Environment {

BundleLayout bundleLayout(DesktopInstall install, const QString& appDir)
{
    switch (install) {
        case DesktopInstall::MacOSFramework:
        case DesktopInstall::MacOSFlat: {
            const QString frameworkDir = _cleanJoin(appDir, "../Frameworks/GStreamer.framework");
            QString rootDir = _firstExistingPath({
                _cleanJoin(frameworkDir, "Versions/1.0"),
                _cleanJoin(frameworkDir, "Versions/Current"),
                frameworkDir,
            });
            if (rootDir.isEmpty()) {
                rootDir = _cleanJoin(frameworkDir, "Versions/1.0");
            }
            const QString frameworkLib = _cleanJoin(rootDir, "lib");
            const QString appLib = _cleanJoin(appDir, "../lib");
            const QStringList libDirs = (install == DesktopInstall::MacOSFramework) ? QStringList{frameworkLib, appLib}
                                                                                    : QStringList{appLib, frameworkLib};
            const QStringList helperDirs = {_cleanJoin(appDir, "../libexec/gstreamer-1.0"),
                                            _cleanJoin(rootDir, "libexec/gstreamer-1.0")};
            return {
                .pluginDirs = _cleanJoinAll(libDirs, "gstreamer-1.0"),
                .gioModuleDirs = _cleanJoinAll(libDirs, "gio/modules"),
                .scannerCandidates = _cleanJoinAll(helperDirs, "gst-plugin-scanner"),
                .ptpCandidates = _cleanJoinAll(helperDirs, "gst-ptp-helper"),
            };
        }
        case DesktopInstall::Windows: {
            const QString libDir = _cleanJoin(appDir, "../lib");
            const QString helperDir = _cleanJoin(appDir, "../libexec/gstreamer-1.0");
            return {
                .pluginDirs = {_cleanJoin(libDir, "gstreamer-1.0")},
                .gioModuleDirs = {_cleanJoin(libDir, "gio/modules")},
                .scannerCandidates = {_cleanJoin(helperDir, "gst-plugin-scanner.exe")},
                .ptpCandidates = {_cleanJoin(helperDir, "gst-ptp-helper.exe")},
            };
        }
        case DesktopInstall::Linux: {
            const QString libDir = _cleanJoin(appDir, "../lib");
            const QStringList helperDirs = {_cleanJoin(libDir, "gstreamer1.0/gstreamer-1.0"),
                                            _cleanJoin(appDir, "../libexec/gstreamer-1.0")};
            return {
                .pluginDirs = {_cleanJoin(libDir, "gstreamer-1.0")},
                .gioModuleDirs = {_cleanJoin(libDir, "gio/modules")},
                .scannerCandidates = _cleanJoinAll(helperDirs, "gst-plugin-scanner"),
                .ptpCandidates = _cleanJoinAll(helperDirs, "gst-ptp-helper"),
            };
        }
    }
    return {};
}

VideoBackend::EnvPrepResult prepareEnvironment(const QString& appDir)
{
    // qputenv is not thread-safe against concurrent getenv; this must run on the GUI thread before
    // the QtConcurrent init worker spawns. Warn loudly rather than corrupt the environment silently.
    if (QCoreApplication::instance() && (QThread::currentThread() != QCoreApplication::instance()->thread())) {
        qCWarning(GStreamerEnvLog) << "prepareEnvironment() called off the GUI thread; qputenv is not thread-safe";
    }
    return _setGstEnvVars(appDir.isEmpty() ? QCoreApplication::applicationDirPath() : appDir);
}

QString usePrivateRegistry(const QString& cacheDir, const QString& installPath)
{
    return _usePrivateRegistry(cacheDir, installPath);
}

bool writeIfChanged(const QString& path, QByteArrayView content)
{
    QFile current(path);
    if (current.open(QIODevice::ReadOnly) && (current.size() == content.size()) && (current.readAll() == content)) {
        return true;
    }
    current.close();

    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        qCWarning(GStreamerEnvLog) << "Cannot create the directory of" << path;
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || (file.write(content.data(), content.size()) != content.size()) ||
        !file.commit()) {
        qCWarning(GStreamerEnvLog) << "Cannot write" << path << ":" << file.errorString();
        return false;
    }
    return true;
}

BundledRuntime bundledRuntime()
{
    return s_bundledRuntime;
}

void logDiagnostics()
{
    qCCritical(GStreamerEnvLog) << "GStreamer environment diagnostics:";
    const auto dumpVar = [](const char* var) {
        const QByteArray val = qgetenv(var);
        qCCritical(GStreamerEnvLog) << "  " << var << "=" << (val.isEmpty() ? "(unset)" : val.constData());
    };
    for (const char* var : kManagedGstPathVars) {
        dumpVar(var);
    }
    dumpVar("GST_REGISTRY_FORK");
    dumpVar("GST_REGISTRY_REUSE_PLUGIN_SCANNER");
    dumpVar("GST_REGISTRY");
}

}  // namespace GStreamer::Environment
