#pragma once

#include <gst/gst.h>

namespace GStreamer
{
    void redirectGLibLogging();
    void resetExternalPluginLoaderFailure();
    bool didExternalPluginLoaderFail();

    void qtGstLog(GstDebugCategory *category,
                  GstDebugLevel level,
                  const gchar *file,
                  const gchar *function,
                  gint line,
                  GObject *object,
                  GstDebugMessage *message,
                  gpointer data);

    /// Install the Qt-bridged log function and apply the persisted debug-level
    /// setting. Skips threshold override when GST_DEBUG is set externally.
    void configureDebugLogging();

    /// Enumerate registered video-decoder factories and log them (sorted by
    /// rank) to `GStreamerDecoderRanksLog`, annotated HW/SW.
    void logDecoderRanks();
}
