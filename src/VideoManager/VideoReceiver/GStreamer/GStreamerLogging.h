#pragma once

#include <gst/gst.h>

namespace GStreamer {
void redirectGLibLogging();

/// Whether GStreamer warned, through the GLib log redirectGLibLogging() installs, that its gst-plugin-scanner helper
/// failed ("External plugin loader failed") since resetExternalPluginLoaderFailed(). GStreamer then loads the plugins
/// in-process, so video still works; only the self-test fails on it.
bool externalPluginLoaderFailed();
void resetExternalPluginLoaderFailed();

/// True when GST_DEBUG, GST_DEBUG_FILE or GST_TRACERS is set: the user asked for GStreamer's own debug output, so
/// QGC keeps gst_debug_log_default (which alone honours GST_DEBUG_FILE and writes the format gst-stats and tracer
/// tools parse) and does not bridge GStreamer's log into Qt logging.
bool nativeDebugLogRequested();

// Unless nativeDebugLogRequested(), replace gst's default log function with qtGstLog. Then apply the persisted
// gstDebugLevel setting, unless GST_DEBUG is set.
void configureDebugLogging();

// Diagnostic dump of installed video-decoder factories, sorted by rank, with
// HW/SW classification. No-op once the registry is empty.
void logDecoderRanks();

void qtGstLog(GstDebugCategory* category, GstDebugLevel level, const gchar* file, const gchar* function, gint line,
              GObject* object, GstDebugMessage* message, gpointer data);
}  // namespace GStreamer
