#pragma once

#include <QtCore/QLoggingCategory>

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
}
