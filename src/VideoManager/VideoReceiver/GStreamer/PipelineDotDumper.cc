#include "PipelineDotDumper.h"

#include <gst/gst.h>

#include <QtCore/QStandardPaths>

#include "GStreamerHelpers.h"
#include "GstVideoReceiver.h"

void PipelineDotDumper::setReceiverName(const QString& receiverName)
{
    _pipelineName = receiverName.isEmpty()
                        ? QByteArrayLiteral("receiver")
                        : (QStringLiteral("receiver-") + GStreamer::dotFileTag(receiverName)).toUtf8();
}

void PipelineDotDumper::dump(GstElement* pipeline, const char* tag) const
{
    // GST_DEBUG_BIN_TO_DOT_FILE is a no-op unless GST_DEBUG_DUMP_DOT_DIR is set; surface that
    // once per process so field debugging doesn't require re-reading the source.
    [[maybe_unused]] static const bool dotDirHinted =
        []() {
            if (qEnvironmentVariableIsEmpty("GST_DEBUG_DUMP_DOT_DIR")) {
                qCDebug(GstVideoReceiverLog)
                    << "Pipeline dot-graph dumps are disabled. Set GST_DEBUG_DUMP_DOT_DIR=/path/to/dir to enable.";
            }
            return true;
        }();

    if (!pipeline) {
        return;
    }
    const QByteArray fileName = _pipelineName + '-' + tag;
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GStreamer::kDiagnosticDotGraphDetails, fileName.constData());
}

void PipelineDotDumper::snapshot(GstElement* pipeline, const QString& tag) const
{
    if (!pipeline) {
        return;
    }
    const QString safeTag = GStreamer::dotFileTag(tag);
    dump(pipeline, safeTag.toUtf8().constData());

    // The native dump already holds it.
    if (!qEnvironmentVariableIsEmpty("GST_DEBUG_DUMP_DOT_DIR")) {
        return;
    }
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheRoot.isEmpty()) {
        return;
    }
    const QString dotPath = GStreamer::writePipelineDot(pipeline, QString::fromUtf8(_pipelineName) + u'-' + safeTag,
                                                        cacheRoot + QStringLiteral("/qgc-pipeline-dot"));
    if (!dotPath.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Pipeline graph saved to" << dotPath;
    }
}
