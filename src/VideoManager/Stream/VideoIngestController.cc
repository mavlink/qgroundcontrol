#include "VideoIngestController.h"

#include "VideoRecorder.h"

#include <utility>

VideoIngestController::VideoIngestController(QString streamName, QObject* parent)
    : QObject(parent)
    , _ingestSession(new VideoIngestSessionController(std::move(streamName), this))
{
    connect(_ingestSession, &VideoIngestSessionController::errorOccurred, this,
            [this](VideoReceiver::ErrorCategory category, const QString& message) {
                if (category == VideoReceiver::ErrorCategory::Fatal)
                    markRefreshNeeded();
                emit errorOccurred(category, message);
            });
    connect(_ingestSession, &VideoIngestSessionController::endOfStream, this, [this]() {
        markRefreshNeeded();
        emit endOfStream();
    });
}

void VideoIngestController::setResolverForTest(Resolver resolver)
{
    _resolverForTest = std::move(resolver);
}

void VideoIngestController::setSource(const VideoSourceResolver::SourceDescriptor& source)
{
    stopIngestSession();
    _source = source;
    _setInput(_resolve(_source));
    _refreshNeeded = false;
}

bool VideoIngestController::refreshForStart()
{
    if (!_source.isValid())
        return false;

    bool refreshNeeded = !hasInput() || _refreshNeeded;
    if (_resolverForTest && _source.needsIngestSession())
        refreshNeeded = true;
    if (_source.needsIngestSession() && !ingestSessionRunning())
        refreshNeeded = true;

    if (!refreshNeeded)
        return hasInput();

    VideoPlaybackInput input = _resolve(_source);
    if (input.uri.isEmpty())
        return false;

    _setInput(std::move(input));
    _refreshNeeded = false;
    return hasInput();
}

void VideoIngestController::markRefreshNeeded()
{
    _refreshNeeded = true;
}

void VideoIngestController::stopIngestSession()
{
    if (_ingestSession)
        _ingestSession->stop();

    if (_source.needsIngestSession()) {
        _input.kind = VideoPlaybackInput::Kind::None;
        _input.device = nullptr;
        _input.deviceUrl = {};
        _input.playbackPolicy = {};
    }
}

bool VideoIngestController::ingestSessionRunning() const
{
    return _ingestSession && _ingestSession->running();
}

std::unique_ptr<VideoRecorder> VideoIngestController::createIngestRecorder(QObject* parent)
{
    return _ingestSession ? _ingestSession->createIngestRecorder(parent) : std::unique_ptr<VideoRecorder>();
}

VideoPlaybackInput VideoIngestController::_resolve(const VideoSourceResolver::SourceDescriptor& source)
{
    if (!source.isValid())
        return {};

    if (_resolverForTest) {
        VideoPlaybackInput input = _resolverForTest(source);
        input.playbackPolicy = source.playbackPolicy;
        return input;
    }

    return _ingestSession->resolvePlaybackInput(source);
}

void VideoIngestController::_setInput(VideoPlaybackInput input)
{
    if (input.kind == VideoPlaybackInput::Kind::None)
        input.kind = _kindForInput(input);
    _input = std::move(input);
}

VideoPlaybackInput::Kind VideoIngestController::_kindForInput(const VideoPlaybackInput& input) const
{
    if (input.uri.isEmpty())
        return VideoPlaybackInput::Kind::None;
    if (input.device)
        return VideoPlaybackInput::Kind::StreamDevice;
    if (_source.isLocalCamera)
        return VideoPlaybackInput::Kind::LocalCamera;
    return VideoPlaybackInput::Kind::DirectUrl;
}
