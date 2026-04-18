#pragma once

namespace GStreamer {

/// Register statically-linked GStreamer plugins. No-op when QGC is built
/// against a shared GStreamer (plugins are discovered via GST_PLUGIN_PATH).
void registerPlugins();

/// Verify all plugins listed in QGC_GST_REQUIRED_PLUGINS are present in the
/// registry, and emit diagnostic info (blacklist, env vars) when any are
/// missing. Returns false if a required plugin is absent.
bool verifyPlugins();

}  // namespace GStreamer
