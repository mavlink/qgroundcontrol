#pragma once

#include <QtCore/QByteArrayView>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "VideoBackend.h"

/// Process-wide GStreamer environment setup: plugin-path discovery, GIO compat override,
/// env var management, APK asset extraction, mobile CA file.
namespace GStreamer::Environment {

/// The desktop GStreamer runtime bundled with the app that prepareEnvironment() pointed GStreamer at.
struct BundledRuntime
{
    QStringList pluginDirs;  ///< Empty when GStreamer uses the system's plugins, or Android's and iOS's static ones.
    QString gioModuleDir;    ///< Empty when the bundle has no GIO modules.
};

/// Where a desktop install keeps its GStreamer runtime, as candidate paths in priority order.
struct BundleLayout
{
    QStringList pluginDirs;         ///< Every existing entry is searched.
    QStringList gioModuleDirs;      ///< The first existing entry wins.
    QStringList scannerCandidates;  ///< The first existing entry wins.
    QStringList ptpCandidates;      ///< The first existing entry wins.
};

/// The desktop installs prepareEnvironment() knows how to point GStreamer into.
enum class DesktopInstall
{
    MacOSFramework,  ///< An app bundle with GStreamer.framework, whose paths win over the app's own lib.
    MacOSFlat,       ///< An app bundle with a flat SDK copied next to the app, whose paths win over the framework's.
    Windows,         ///< bin/ beside lib/gstreamer-1.0 and libexec/gstreamer-1.0.
    Linux,           ///< An AppImage or other prefix: bin/ beside lib/ and libexec/.
};

/// The layout of @p install for the app in @p appDir. Only reads the filesystem to find the macOS framework's version
/// directory, so every platform's layout can be checked on any host.
BundleLayout bundleLayout(DesktopInstall install, const QString& appDir);

/// Apply all GStreamer env vars for this process from bundled-paths discovery + platform validation, for the app in
/// @p appDir (default: QCoreApplication::applicationDirPath()). A desktop runtime bundled with the app replaces the
/// default plugin path; GStreamer keeps scanning its plugins out of process, so a plugin that crashes while loading is
/// blacklisted instead of taking QGC down. Idempotent: a later call applies the same environment, as it doesn't take
/// the values an earlier one set for the user's. Returns the validation outcome by value (threaded explicitly to
/// initialize(), not a cross-thread global).
/// Not thread-safe: call it on the GUI thread, before gst_init() and any thread that reads the environment.
VideoBackend::EnvPrepResult prepareEnvironment(const QString& appDir = QString());

/// Point GST_REGISTRY at gstreamer-<runtime version>-<arch>-<install hash>.bin in @p cacheDir, creating the directory,
/// unless the user set GST_REGISTRY or GST_REGISTRY_1_0 or @p cacheDir is empty. The hash names the install at
/// @p installPath by what stays put across launches: the AppImage file ($APPIMAGE) for a path in its mount ($APPDIR),
/// and the app's name for a macOS app translocated to a random directory. Touches the registry, and removes the
/// gstreamer-*.bin registries in @p cacheDir that no install has touched for 30 days. prepareEnvironment() does this
/// with the app's cache location and plugin path for a bundled desktop runtime. Returns the registry it set, else an
/// empty string.
QString usePrivateRegistry(const QString& cacheDir, const QString& installPath);

/// Makes the file at @p path hold @p content, creating its directory: written only when it differs, and through a
/// temporary file, so it is never left half written. False when it can't. prepareEnvironment() extracts the Android
/// APK's CA bundle with it.
bool writeIfChanged(const QString& path, QByteArrayView content);

/// The bundled runtime the last prepareEnvironment() applied; empty before it ran.
BundledRuntime bundledRuntime();

/// Dump the GST_PLUGIN_*/GST_REGISTRY* vars managed by this layer to the log at critical level.
/// Called from plugin-verification failure paths so a stripped registry shows how the loader was
/// configured.
void logDiagnostics();

}  // namespace GStreamer::Environment
