#pragma once

#include <QtCore/QString>

/// Process-wide GStreamer environment setup: plugin-path discovery, GIO compat override,
/// env var management, APK asset extraction, scanner sanitization.
namespace GStreamer::Environment {

/// Outcome of environment preparation: whether the discovered plugin/scanner
/// paths validated, plus a human-readable error when they did not.
struct ValidationResult
{
    bool ok = true;
    QString error;
};

/// Apply all GStreamer env vars for this process from bundled-paths discovery + platform
/// validation. Idempotent: clears prior managed vars first so a retry starts clean. Returns the
/// validation outcome by value (threaded explicitly to initialize(), not a cross-thread global).
ValidationResult prepareEnvironment();

/// Dump the GST_PLUGIN_*/GST_REGISTRY_* vars managed by this layer to the log at critical level.
/// Called from plugin-verification failure paths so a stripped registry shows how the loader was
/// configured.
void logDiagnostics();

}  // namespace GStreamer::Environment
