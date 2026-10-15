#include "PipelineDotDumper.h"

#include <gst/gst.h>

#include "GStreamerHelpers.h"
#include "GstVideoReceiver.h"

void PipelineDotDumper::dump(GstElement* pipeline, const char* tag) const
{
    // GST_DEBUG_BIN_TO_DOT_FILE is a no-op unless GST_DEBUG_DUMP_DOT_DIR is set; surface that
    // once per process so field debugging doesn't require re-reading the source.
    [[maybe_unused]] static const bool dotDirHinted = []() {
        if (qgetenv("GST_DEBUG_DUMP_DOT_DIR").isEmpty()) {
            qCInfo(GstVideoReceiverLog).noquote()
                << "Pipeline dot-graph dumps are disabled. Set GST_DEBUG_DUMP_DOT_DIR=/path/to/dir to enable.";
        }
        return true;
    }();

    if (!pipeline) {
        return;
    }
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, tag);
}

void PipelineDotDumper::snapshot(GstElement* pipeline, const QString& tag) const
{
    if (!pipeline) {
        return;
    }
    const QByteArray tagUtf8 = tag.toUtf8();
    dump(pipeline, tagUtf8.constData());
    const QString dotPath = GStreamer::writePipelineDot(pipeline, tagUtf8.constData());
    if (!dotPath.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Pipeline graph saved to" << dotPath;
    }
}
