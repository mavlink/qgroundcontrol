#include "GstIngestRecorder.h"

#include "GstRemuxPipeline.h"
#include "GstIngestSession.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstIngestRecorderLog, "VideoManager.GStreamer.IngestRecorder")

namespace {

QList<QMediaFormat::FileFormat> availableFormats()
{
    QList<QMediaFormat::FileFormat> formats;
    for (const QMediaFormat::FileFormat format :
         {QMediaFormat::Matroska, QMediaFormat::QuickTime, QMediaFormat::MPEG4}) {
        const char* muxer = nullptr;
        switch (format) {
            case QMediaFormat::Matroska:
                muxer = "matroskamux";
                break;
            case QMediaFormat::QuickTime:
                muxer = "qtmux";
                break;
            case QMediaFormat::MPEG4:
                muxer = "mp4mux";
                break;
            default:
                break;
        }
        if (GstRemuxPipeline::factoryAvailable(muxer))
            formats.append(format);
    }
    return formats;
}

}  // namespace

GstIngestRecorder::GstIngestRecorder(GstIngestSession* session, QObject* parent)
    : VideoRecorder(parent)
    , _session(session)
{
}

GstIngestRecorder::~GstIngestRecorder()
{
    if (isRecording())
        stop();
}

bool GstIngestRecorder::start(const QString& path, QMediaFormat::FileFormat format)
{
    if (!_session || !_session->running()) {
        emit error(QStringLiteral("GStreamer ingest session is not running"));
        return false;
    }

    if (!availableFormats().contains(format)) {
        emit error(QStringLiteral("GStreamer ingest recorder does not support the requested container"));
        return false;
    }

    if (!_session->startRecording(path, format)) {
        emit error(QStringLiteral("Failed to attach GStreamer ingest recording branch"));
        return false;
    }

    _currentPath = path;
    setState(State::Recording);
    qCDebug(GstIngestRecorderLog) << "Started ingest recording to" << path;
    emit started(path);
    return true;
}

void GstIngestRecorder::stop()
{
    if (!isRecording())
        return;

    if (_session)
        _session->stopRecording();

    const QString path = _currentPath;
    setState(State::Idle);
    qCDebug(GstIngestRecorderLog) << "Stopped ingest recording" << path;
    emit stopped(path);
}

VideoRecorder::Capabilities GstIngestRecorder::capabilities() const
{
    return Capabilities{
        true,
        availableFormats(),
        QStringLiteral("GStreamer ingest recorder (shared ingest session)"),
    };
}
