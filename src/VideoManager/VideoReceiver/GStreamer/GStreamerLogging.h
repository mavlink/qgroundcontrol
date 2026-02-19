#pragma once

#include <QtCore/QLoggingCategory>

#include <gst/gst.h>

// Logging categories are declared in GStreamer.h (the lightweight public header).
// This header is for internal GStreamer code that needs the full gst/gst.h types.

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
}
