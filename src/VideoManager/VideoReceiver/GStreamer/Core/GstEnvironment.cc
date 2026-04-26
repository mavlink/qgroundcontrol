// Environment/path bootstrap for bundled GStreamer runtimes.
//
// Keeps OS-specific env-var and plugin/GIO module path wiring isolated from
// gst_init orchestration (GStreamer.cc) and plugin registration
// (GstPluginRegistry.cc). The single public entry point is
// `GStreamer::prepareEnvironment()`.

#include "GstEnvironment.h"

#include "GStreamer.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QStringLiteral>
#include <QtCore/QtGlobal>

#include <atomic>

#ifdef Q_OS_LINUX
#include <dlfcn.h>
#endif

namespace GStreamer {

namespace {

static std::atomic<bool> s_envPathsValid{true};
static QMutex s_envPathsMutex;
static QString s_envPathsError;

void _resetEnvValidation()
{
    const QMutexLocker locker(&s_envPathsMutex);
    s_envPathsError.clear();
    s_envPathsValid.store(true, std::memory_order_release);
}

QString _cleanJoin(const QString& base, const QString& relative)
{
    return QDir::cleanPath(QDir(base).filePath(relative));
}

void _setGstEnv(const char* name, const QString& value)
{
    qputenv(name, value.toUtf8());
    qCDebug(GStreamerLog) << "  " << name << "=" << value;
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)

void _setEnvValidationError(const QString& error)
{
    const QMutexLocker locker(&s_envPathsMutex);
    s_envPathsError = error;
    s_envPathsValid.store(false, std::memory_order_release);
    qCCritical(GStreamerLog) << error;
}

void _unsetEnv(const char* name)
{
    if (qEnvironmentVariableIsSet(name)) {
        qunsetenv(name);
        qCDebug(GStreamerLog) << "  unset" << name;
    }
}

void _setGstEnvIfExists(const char* name, const QString& path)
{
    if (QFileInfo::exists(path)) {
        _setGstEnv(name, path);
    }
}

#if defined(Q_OS_MACOS)
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
#endif

void _clearManagedGstEnvVars()
{
    static constexpr const char* varsToUnset[] = {
        "GIO_EXTRA_MODULES",      "GIO_MODULE_DIR",      "GIO_USE_VFS",     "GST_PLUGIN_SYSTEM_PATH_1_0",
        "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_PATH_1_0", "GST_PLUGIN_PATH",
    };

    for (const char* name : varsToUnset) {
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

void _applyGstEnvVars(const QString& pluginDir, const QString& gioModDir)
{
    qCDebug(GStreamerLog) << "Applying GStreamer environment:";

    _sanitizePythonEnvForScanner();
    _clearManagedGstEnvVars();
    // GST_REGISTRY_FORK=no forces in-process plugin scanning, eliminating the
    // need to find/bundle/install gst-plugin-scanner and gst-ptp-helper.
    // Safe for bundled builds where we control the plugin set. Dev builds using
    // system GStreamer skip _applyGstEnvVars entirely, keeping the default fork=yes.
    _setGstEnv("GST_REGISTRY_FORK", QStringLiteral("no"));
    _setGstEnvIfExists("GIO_EXTRA_MODULES", gioModDir);
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
    static constexpr const char* kGioSoPaths[] = {
        "libgio-2.0.so.0",
        "/usr/lib/x86_64-linux-gnu/libgio-2.0.so.0",
        "/usr/lib/aarch64-linux-gnu/libgio-2.0.so.0",
        "/usr/lib/arm-linux-gnueabihf/libgio-2.0.so.0",
        "/usr/lib64/libgio-2.0.so.0",
        "/usr/lib/libgio-2.0.so.0",
    };

    for (const char* path : kGioSoPaths) {
        void* handle = dlopen(path, RTLD_LAZY | RTLD_NOLOAD);
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

void _applyGioCompatOverride(const QString& gioModDir)
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

#if defined(Q_OS_MACOS)
bool _validateMacBundlePaths(const QString& bundleFrameworkRoot, const QString& pluginDirs)
{
    if (pluginDirs.isEmpty()) {
        _setEnvValidationError(
            QStringLiteral("GStreamer: bundled macOS framework found but plugin directory is missing under %1")
                .arg(bundleFrameworkRoot));
        return false;
    }
    return true;
}
#endif

bool _validateBundledDesktopPaths(const QString& platformLabel, const QString& pluginDirs)
{
    if (pluginDirs.isEmpty()) {
        _setEnvValidationError(QStringLiteral("GStreamer: %1 bundled plugin directory is missing.").arg(platformLabel));
        return false;
    }
    return true;
}

#endif  // !Q_OS_ANDROID && !Q_OS_IOS

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

    const bool hasBundledFramework = QFileInfo::exists(frameworkDir);

    // Unified runtime path — framework builds prefer framework paths,
    // non-framework (Homebrew) prefers app-relative. No compile-time split.
    const QString pluginDirs = hasBundledFramework ? _joinExistingPaths({
                                                         _cleanJoin(rootDir, "lib/gstreamer-1.0"),
                                                         _cleanJoin(appDir, "../lib/gstreamer-1.0"),
                                                     })
                                                   : _joinExistingPaths({
                                                         _cleanJoin(appDir, "../lib/gstreamer-1.0"),
                                                         _cleanJoin(rootDir, "lib/gstreamer-1.0"),
                                                     });
    const QString gioMod = hasBundledFramework ? _firstExistingPath({
                                                     _cleanJoin(rootDir, "lib/gio/modules"),
                                                     _cleanJoin(appDir, "../lib/gio/modules"),
                                                 })
                                               : _firstExistingPath({
                                                     _cleanJoin(appDir, "../lib/gio/modules"),
                                                     _cleanJoin(rootDir, "lib/gio/modules"),
                                                 });

    bool validBundlePaths = true;
    if (!pluginDirs.isEmpty()) {
        validBundlePaths = _validateBundledDesktopPaths(QStringLiteral("macOS"), pluginDirs);
    }
    if (hasBundledFramework) {
        validBundlePaths = validBundlePaths && _validateMacBundlePaths(rootDir, pluginDirs);
    }

    if (!pluginDirs.isEmpty() && validBundlePaths) {
        _applyGstEnvVars(pluginDirs, gioMod);
    }

    if (hasBundledFramework) {
        _setGstEnv("GTK_PATH", rootDir);
    }

#elif defined(Q_OS_WIN)
    const QString libDir = _cleanJoin(appDir, "../lib");
    const QString pluginDir = _cleanJoin(libDir, "gstreamer-1.0");
    const QString gioMod = _cleanJoin(libDir, "gio/modules");

    if (QFileInfo::exists(pluginDir) && _validateBundledDesktopPaths(QStringLiteral("Windows"), pluginDir)) {
        _applyGstEnvVars(pluginDir, gioMod);

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
    if (!qEnvironmentVariableIsSet("GST_PLUGIN_PATH_1_0") && !qEnvironmentVariableIsSet("GST_PLUGIN_PATH") &&
        !qEnvironmentVariableIsSet("GST_PLUGIN_SYSTEM_PATH_1_0") &&
        !qEnvironmentVariableIsSet("GST_PLUGIN_SYSTEM_PATH")) {
        const QString libDir = _cleanJoin(appDir, "../lib");
        const QString pluginDir = _cleanJoin(libDir, "gstreamer-1.0");
        const QString gioMod = _cleanJoin(libDir, "gio/modules");

        if (QFileInfo::exists(pluginDir) && _validateBundledDesktopPaths(QStringLiteral("Linux"), pluginDir)) {
            _applyGstEnvVars(pluginDir, gioMod);
            _applyGioCompatOverride(gioMod);
        }
    }
#endif
}

}  // anonymous namespace

bool envPathsValid(QString* error)
{
    if (s_envPathsValid.load(std::memory_order_acquire)) {
        return true;
    }
    if (error) {
        const QMutexLocker locker(&s_envPathsMutex);
        *error = s_envPathsError;
    }
    return false;
}

void prepareEnvironment()
{
    _setGstEnvVars();
}

}  // namespace GStreamer
