#pragma once

#include <gst/gst.h>

namespace GStreamer {
void redirectGLibLogging();
void resetExternalPluginLoaderFailure();
bool didExternalPluginLoaderFail();

// Install qtGstLog as gst's log function and apply the persisted gstDebugLevel
// setting (unless GST_DEBUG is already set in the environment).
void configureDebugLogging();

// Diagnostic dump of installed video-decoder factories, sorted by rank, with
// HW/SW classification. No-op once the registry is empty.
void logDecoderRanks();

void qtGstLog(GstDebugCategory* category, GstDebugLevel level, const gchar* file, const gchar* function, gint line,
              GObject* object, GstDebugMessage* message, gpointer data);
}  // namespace GStreamer
