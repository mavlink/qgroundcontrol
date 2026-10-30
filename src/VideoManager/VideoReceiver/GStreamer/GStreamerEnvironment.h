#pragma once

#include <QtCore/QString>

/// Process-wide GStreamer environment setup: plugin-path discovery, GIO compat override,
/// env var management, APK asset extraction, mobile CA file.
namespace GStreamer::Environment {

/// Outcome of environment preparation: whether the discovered plugin/scanner
/// paths validated, plus a human-readable error when they did not.
struct ValidationResult
{
    bool ok = true;
    QString error;
};

/// Apply all GStreamer env vars for this process from bundled-paths discovery + platform validation, for the app in
/// @p appDir (default: QCoreApplication::applicationDirPath()). A desktop runtime bundled with the app replaces the
/// default plugin path; GStreamer keeps scanning its plugins out of process, so a plugin that crashes while loading is
/// blacklisted instead of taking QGC down. Idempotent: clears prior managed vars first so a retry starts clean. Returns
/// the validation outcome by value (threaded explicitly to initialize(), not a cross-thread global).
/// Not thread-safe: call it on the GUI thread, before gst_init() and any thread that reads the environment.
ValidationResult prepareEnvironment(const QString& appDir = QString());

/// Point GST_REGISTRY at gstreamer-<runtime version>-<arch>-<install hash>.bin in @p cacheDir, creating the directory,
/// unless GST_REGISTRY or GST_REGISTRY_1_0 is already set or @p cacheDir is empty. The hash names the install at
/// @p installPath by what stays put across launches: the AppImage file ($APPIMAGE) for a path in its mount ($APPDIR),
/// and the app's name for a macOS app translocated to a random directory. Touches the registry, and removes the
/// gstreamer-*.bin registries in @p cacheDir that no install has touched for 30 days. prepareEnvironment() does this
/// with the app's cache location and plugin path for a bundled desktop runtime. Returns the registry it set, else an
/// empty string.
QString usePrivateRegistry(const QString& cacheDir, const QString& installPath);

/// Dump the GST_PLUGIN_*/GST_REGISTRY* vars managed by this layer to the log at critical level.
/// Called from plugin-verification failure paths so a stripped registry shows how the loader was
/// configured.
void logDiagnostics();

}  // namespace GStreamer::Environment
