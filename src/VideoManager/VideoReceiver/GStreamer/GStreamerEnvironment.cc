#include "GStreamerEnvironment.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtCore/QSysInfo>
#include <QtCore/QThread>

#include "QGCLoggingCategory.h"

#ifdef Q_OS_ANDROID
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

#include <gst/gst.h>

QGC_LOGGING_CATEGORY(GStreamerEnvLog, "Video.GStreamer.Environment")

namespace {

using GStreamer::Environment::ValidationResult;

// Serializes the whole env mutation: qputenv/setenv aren't thread-safe vs gst_init's getenv.
static QMutex s_setEnvVarsMutex;

// Single source of truth for the managed plugin-path/scanner vars: keeps _clearManagedGstEnvVars
// and logDiagnostics from drifting. Each site layers its own extras (GIO/PTP, registry) on top.
constexpr const char* kManagedGstPathVars[] = {
    "GST_PLUGIN_PATH",    "GST_PLUGIN_PATH_1_0",    "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0",
    "GST_PLUGIN_SCANNER", "GST_PLUGIN_SCANNER_1_0",
};

QString _cleanJoin(const QString& base, const QString& relative)
{
    return QDir::cleanPath(QDir(base).filePath(relative));
}

void _setGstEnv(const char* name, const QString& value)
{
#ifdef Q_OS_WIN
    // GLib reads the wide environment; qputenv's _putenv_s would decode these UTF-8 bytes in the ANSI
    // code page and corrupt non-ASCII install paths.
    (void) g_setenv(name, value.toUtf8().constData(), TRUE);
#else
    qputenv(name, value.toUtf8());
#endif
    qCDebug(GStreamerEnvLog) << "  " << name << "=" << value;
}

// Set @p var to the first existing candidate, but only if unset (explicit user/system value wins).
// @p nativePath uses native separators + 8-bit encoding, required for OpenSSL SSL_CERT_FILE on Windows.
[[maybe_unused]] void _setCaBundleIfUnset(const char* var, const QStringList& candidates, bool nativePath = false)
{
    if (!qEnvironmentVariableIsEmpty(var)) {
        return;
    }
    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            if (nativePath) {
                const QByteArray encoded = QFile::encodeName(QDir::toNativeSeparators(path));
                qputenv(var, encoded);
                qCDebug(GStreamerEnvLog) << "  " << var << "=" << encoded;
            } else {
                _setGstEnv(var, path);
            }
            return;
        }
    }
}

// A bundled runtime's plugins differ from the system GStreamer's, so sharing the default per-user registry would make
// each invalidate the other's plugin cache; the runtime version in the name keeps upgrades from reusing it too.
QString _usePrivateRegistry(const QString& cacheDir)
{
    if (cacheDir.isEmpty() || qEnvironmentVariableIsSet("GST_REGISTRY_1_0") ||
        qEnvironmentVariableIsSet("GST_REGISTRY")) {
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
    const QString registry = _cleanJoin(cacheDir, QStringLiteral("gstreamer-%1.%2.%3-%4.bin")
                                                      .arg(major)
                                                      .arg(minor)
                                                      .arg(micro)
                                                      .arg(QSysInfo::buildCpuArchitecture()));
    _setGstEnv("GST_REGISTRY", registry);
    return registry;
}

[[maybe_unused]] void _setTmpDirVars(const QString& dir)
{
    _setGstEnv("TMP", dir);
    _setGstEnv("TEMP", dir);
    _setGstEnv("TMPDIR", dir);
}

#ifdef Q_OS_ANDROID
// Extract an APK asset to destPath; skip rewrite when sizes already match
// (asset bytes are immutable per APK build, so size match ⇒ same content).
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

    const QFileInfo destInfo(destPath);
    if (destInfo.exists() && destInfo.size() == assetLen) {
        AAsset_close(asset);
        return true;
    }

    QDir().mkpath(destInfo.absolutePath());
    QFile out(destPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(GStreamerEnvLog) << "Cannot open" << destPath << "for write:" << out.errorString();
        AAsset_close(asset);
        return false;
    }

    const void* buf = AAsset_getBuffer(asset);
    bool ok = false;
    if (buf && assetLen > 0) {
        ok = (out.write(static_cast<const char*>(buf), assetLen) == assetLen);
    }
    out.close();
    // close() flushes; a flush failure surfaces via error() after close.
    ok = ok && (out.error() == QFileDevice::NoError);
    AAsset_close(asset);
    if (!ok) {
        qCWarning(GStreamerEnvLog) << "Failed writing asset" << assetPath << "to" << destPath;
        QFile::remove(destPath);
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

QString _firstExistingPath(const QStringList& paths)
{
    for (const QString& path : paths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }

    return {};
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

QStringList _cleanJoinAll(const QStringList& bases, const QString& relative)
{
    QStringList joined;
    joined.reserve(bases.size());
    for (const QString& base : bases) {
        joined.append(_cleanJoin(base, relative));
    }
    return joined;
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

void _sanitizePythonEnvForScanner()
{
    static constexpr const char* varsToUnset[] = {
        "PYTHONHOME", "PYTHONPATH", "VIRTUAL_ENV", "CONDA_PREFIX", "CONDA_DEFAULT_ENV", "PYTHONUSERBASE",
    };

    for (const char* name : varsToUnset) {
        _unsetEnv(name);
    }

    _setGstEnv("PYTHONNOUSERSITE", QStringLiteral("1"));
}

void _applyGstEnvVars(const QString& pluginDir, const QString& gioModDir, const QString& scannerPath,
                      const QString& ptpPath)
{
    qCDebug(GStreamerEnvLog) << "Applying GStreamer environment:";

    _sanitizePythonEnvForScanner();
    _clearManagedGstEnvVars();
    _setGstEnv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", QStringLiteral("no"));
    _setGstEnv("GST_REGISTRY_FORK", QStringLiteral("no"));
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

// Candidate locations of a desktop bundle's GStreamer runtime, in priority order.
struct BundleLayout
{
    QStringList pluginDirs;         // every existing entry is searched
    QStringList gioModuleDirs;      // first existing entry wins
    QStringList scannerCandidates;  // first existing entry wins
    QStringList ptpCandidates;      // first existing entry wins
};

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
    _applyGstEnvVars(pluginDirs, _firstExistingPath(layout.gioModuleDirs), scanner,
                     _firstExistingPath(layout.ptpCandidates));
    (void) _usePrivateRegistry(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    return true;
}

#endif  // !Q_OS_ANDROID && !Q_OS_IOS && !QGC_GST_SYSTEM_PLUGINS

ValidationResult _setGstEnvVars()
{
    ValidationResult result;

    const QString appDir = QCoreApplication::applicationDirPath();
    qCDebug(GStreamerEnvLog) << "App directory:" << appDir;

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
    const QStringList libDirs = {_cleanJoin(rootDir, "lib"), _cleanJoin(appDir, "../lib")};
#else
    // Flat (non-framework) SDK builds prefer app-relative paths
    const QStringList libDirs = {_cleanJoin(appDir, "../lib"), _cleanJoin(rootDir, "lib")};
#endif
    const QStringList helperDirs = {_cleanJoin(appDir, "../libexec/gstreamer-1.0"),
                                    _cleanJoin(rootDir, "libexec/gstreamer-1.0")};
    const BundleLayout layout{
        .pluginDirs = _cleanJoinAll(libDirs, "gstreamer-1.0"),
        .gioModuleDirs = _cleanJoinAll(libDirs, "gio/modules"),
        .scannerCandidates = _cleanJoinAll(helperDirs, "gst-plugin-scanner"),
        .ptpCandidates = _cleanJoinAll(helperDirs, "gst-ptp-helper"),
    };
    const bool hasBundledFramework = QFileInfo::exists(frameworkDir);

    if (!_applyBundleLayout(QStringLiteral("macOS"), layout) && hasBundledFramework) {
        result.ok = false;
        result.error =
            QStringLiteral("GStreamer: bundled macOS framework found but plugin directory is missing under %1")
                .arg(rootDir);
        qCCritical(GStreamerEnvLog) << result.error;
    }

#if defined(QGC_GST_MACOS_FRAMEWORK)
    if (hasBundledFramework) {
        _setGstEnv("GTK_PATH", rootDir);
    }
#endif

    // libgioopenssl's compiled-in CA path is the Cerbero prefix; repoint OpenSSL at the
    // bundled file (framework Versions/1.0/etc vs non-framework Contents/Resources/etc).
    _setCaBundleIfUnset("SSL_CERT_FILE", {
                                             _cleanJoin(rootDir, "etc/ssl/certs/ca-certificates.crt"),
                                             _cleanJoin(appDir, "../Resources/etc/ssl/certs/ca-certificates.crt"),
                                         });

#elif defined(Q_OS_WIN)
    const QString libDir = _cleanJoin(appDir, "../lib");
    const QString helperDir = _cleanJoin(appDir, "../libexec/gstreamer-1.0");
    const BundleLayout layout{
        .pluginDirs = {_cleanJoin(libDir, "gstreamer-1.0")},
        .gioModuleDirs = {_cleanJoin(libDir, "gio/modules")},
        .scannerCandidates = {_cleanJoin(helperDir, "gst-plugin-scanner.exe")},
        .ptpCandidates = {_cleanJoin(helperDir, "gst-ptp-helper.exe")},
    };

    if (_applyBundleLayout(QStringLiteral("Windows"), layout)) {
        // Put the app's bin dir on PATH so child processes (gst-plugin-scanner.exe) can locate
        // GStreamer DLLs installed alongside the main executable.
        const QString curPath = qEnvironmentVariable("PATH");
        const QString binDir = QDir::toNativeSeparators(appDir);
        // Windows paths are case-insensitive; compare accordingly to avoid duplicate prepends.
        if (!curPath.split(QDir::listSeparator()).contains(binDir, Qt::CaseInsensitive)) {
            _setGstEnv("PATH", binDir + QDir::listSeparator() + curPath);
        }

        // gioopenssl.dll's compiled-in CA path is relative to the Cerbero SDK root and breaks once
        // detached; point OpenSSL at the bundled CA file (installed by gstreamer_install_windows_sdk).
        _setCaBundleIfUnset("SSL_CERT_FILE", {_cleanJoin(appDir, "../etc/ssl/certs/ca-certificates.crt")},
                            /*nativePath=*/true);
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
            // Writable-cache registry, no fork rescan: sandbox rejects fork, static plugins need no scanner.
            _setGstEnv("GST_REGISTRY", _cleanJoin(cache, "registry.bin"));
            _setGstEnv("GST_REGISTRY_FORK", QStringLiteral("no"));
        }
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
            _setCaBundleIfUnset("CA_CERTIFICATES", {_cleanJoin(resources, "ssl/certs/ca-certificates.crt")});
        }
    }

#elif defined(Q_OS_ANDROID)
    // Static plugins — no GST_PLUGIN_PATH; APK assets are read-only, so the CA bundle is extracted
    // to filesDir on first launch (we run gst_init from C++, not Java's GStreamer.init).
    {
        const QString filesDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

        if (!filesDir.isEmpty()) {
            _setGstEnv("HOME", filesDir);
            _setGstEnv("FONTCONFIG_PATH", _cleanJoin(filesDir, "fontconfig"));
            const QString caBundle = _cleanJoin(filesDir, "ssl/certs/ca-certificates.crt");
            if (_extractApkAsset("ssl/certs/ca-certificates.crt", caBundle) &&
                qEnvironmentVariableIsEmpty("CA_CERTIFICATES")) {
                _setGstEnv("CA_CERTIFICATES", caBundle);
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

        _setGstEnv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", QStringLiteral("no"));
    }

#elif defined(Q_OS_LINUX) && !defined(QGC_GST_SYSTEM_PLUGINS)
    // An AppImage or other bundle ships its runtime under <prefix>/lib; a plugin path set by the user wins.
    if (!qEnvironmentVariableIsSet("GST_PLUGIN_PATH_1_0") && !qEnvironmentVariableIsSet("GST_PLUGIN_PATH") &&
        !qEnvironmentVariableIsSet("GST_PLUGIN_SYSTEM_PATH_1_0") &&
        !qEnvironmentVariableIsSet("GST_PLUGIN_SYSTEM_PATH")) {
        const QString libDir = _cleanJoin(appDir, "../lib");
        const QString gioMod = _cleanJoin(libDir, "gio/modules");
        const QStringList helperDirs = {_cleanJoin(libDir, "gstreamer1.0/gstreamer-1.0"),
                                        _cleanJoin(appDir, "../libexec/gstreamer-1.0")};
        const BundleLayout layout{
            .pluginDirs = {_cleanJoin(libDir, "gstreamer-1.0")},
            .gioModuleDirs = {gioMod},
            .scannerCandidates = _cleanJoinAll(helperDirs, "gst-plugin-scanner"),
            .ptpCandidates = _cleanJoinAll(helperDirs, "gst-ptp-helper"),
        };

        if (_applyBundleLayout(QStringLiteral("Linux"), layout)) {
            _applyGioCompatOverride(gioMod);
        }
    }
#endif

    return result;
}

}  // anonymous namespace

namespace GStreamer::Environment {

ValidationResult prepareEnvironment()
{
    // qputenv is not thread-safe against concurrent getenv; this must run on the GUI thread before
    // the QtConcurrent init worker spawns. Warn loudly rather than corrupt the environment silently.
    if (QCoreApplication::instance() && (QThread::currentThread() != QCoreApplication::instance()->thread())) {
        qCWarning(GStreamerEnvLog) << "prepareEnvironment() called off the GUI thread; qputenv is not thread-safe";
    }
    const QMutexLocker setEnvLocker(&s_setEnvVarsMutex);
    return _setGstEnvVars();
}

QString usePrivateRegistry(const QString& cacheDir)
{
    const QMutexLocker setEnvLocker(&s_setEnvVarsMutex);
    return _usePrivateRegistry(cacheDir);
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
    dumpVar("GST_REGISTRY_REUSE_PLUGIN_SCANNER");
    dumpVar("GST_REGISTRY");
}

}  // namespace GStreamer::Environment
