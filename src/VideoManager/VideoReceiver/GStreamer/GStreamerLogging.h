#pragma once

#include <QtCore/QLoggingCategory>

#include <gst/gst.h>

Q_DECLARE_LOGGING_CATEGORY(GStreamerLog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerAPILog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerDecoderRanksLog)

namespace GStreamer
{
    void redirectGLibLogging();
    void resetExternalPluginLoaderFailure();
    bool didExternalPluginLoaderFail();

    void configureDebugLogging();

    void qtGstLog(GstDebugCategory *category,
                  GstDebugLevel level,
                  const gchar *file,
                  const gchar *function,
                  gint line,
                  GObject *object,
                  GstDebugMessage *message,
                  gpointer data);
}
