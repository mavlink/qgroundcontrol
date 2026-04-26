#include "VideoIngestSessionController.h"

#include "QGCLoggingCategory.h"

#include <utility>

#ifdef QGC_GST_STREAMING
#include "GstIngestRecorder.h"
#include "GStreamer.h"
#include "GstIngestSession.h"
#endif

QGC_LOGGING_CATEGORY(VideoIngestSessionControllerLog, "Video.VideoIngestSessionController")

VideoIngestSessionController::VideoIngestSessionController(QString streamName, QObject* parent)
    : QObject(parent)
    , _streamName(std::move(streamName))
{
}

VideoIngestSessionController::~VideoIngestSessionController()
{
    stop();
}

VideoPlaybackInput VideoIngestSessionController::resolvePlaybackInput(const VideoSourceResolver::SourceDescriptor& source)
{
    if (!source.isValid())
        return {};

#ifdef QGC_GST_STREAMING
    if (source.needsIngestSession() && GStreamer::isAvailable()) {
        if (!_gstIngestSession) {
            _gstIngestSession = std::make_unique<GstIngestSession>(this);
            connect(_gstIngestSession.get(), &GstIngestSession::errorOccurred,
                    this, &VideoIngestSessionController::errorOccurred);
            connect(_gstIngestSession.get(), &GstIngestSession::endOfStream,
                    this, &VideoIngestSessionController::endOfStream);
        }

        if (_gstIngestSession->start(source, source.lowLatencyRecommended)) {
            return {VideoPlaybackInput::Kind::StreamDevice,
                    _gstIngestSession->playbackUri(),
                    _gstIngestSession->playbackDevice(),
                    _gstIngestSession->playbackDeviceUrl(),
                    source.playbackPolicy};
        }

        qCWarning(VideoIngestSessionControllerLog) << _streamName
                                             << "GStreamer ingest session failed for" << source.uri
                                             << "- falling back to receiver URI";
        stop();
    }
#endif

    return {source.isLocalCamera ? VideoPlaybackInput::Kind::LocalCamera : VideoPlaybackInput::Kind::DirectUrl,
            source.uri,
            nullptr,
            {},
            source.playbackPolicy};
}

bool VideoIngestSessionController::running() const
{
#ifdef QGC_GST_STREAMING
    return _gstIngestSession && _gstIngestSession->running();
#else
    return false;
#endif
}

std::unique_ptr<VideoRecorder> VideoIngestSessionController::createIngestRecorder(QObject* parent)
{
#ifdef QGC_GST_STREAMING
    if (_gstIngestSession && _gstIngestSession->running())
        return std::make_unique<GstIngestRecorder>(_gstIngestSession.get(), parent);
#endif
    Q_UNUSED(parent);
    return {};
}

void VideoIngestSessionController::stop()
{
#ifdef QGC_GST_STREAMING
    if (_gstIngestSession) {
        _gstIngestSession->stop();
        _gstIngestSession.reset();
    }
#endif
}
