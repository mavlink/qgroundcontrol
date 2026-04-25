#include "VideoPlaybackRuntime.h"

#include "VideoRecorder.h"

#include <utility>

VideoPlaybackRuntime::VideoPlaybackRuntime(QString streamName, QObject* parent)
    : QObject(parent)
    , _ingestController(new VideoIngestController(std::move(streamName), this))
{
    connect(_ingestController, &VideoIngestController::errorOccurred,
            this, &VideoPlaybackRuntime::errorOccurred);
    connect(_ingestController, &VideoIngestController::endOfStream,
            this, &VideoPlaybackRuntime::endOfStream);
}

void VideoPlaybackRuntime::setResolverForTest(Resolver resolver)
{
    _ingestController->setResolverForTest(std::move(resolver));
}

void VideoPlaybackRuntime::setSource(const VideoSourceResolver::SourceDescriptor& source)
{
    _ingestController->setSource(source);
}

bool VideoPlaybackRuntime::refreshForStart()
{
    return _ingestController->refreshForStart();
}

void VideoPlaybackRuntime::markRefreshNeeded()
{
    _ingestController->markRefreshNeeded();
}

void VideoPlaybackRuntime::stop()
{
    _ingestController->stopIngestSession();
}

bool VideoPlaybackRuntime::hasInput() const
{
    return _ingestController->hasInput();
}

const VideoPlaybackRuntime::PlaybackInput& VideoPlaybackRuntime::input() const
{
    return _ingestController->input();
}

bool VideoPlaybackRuntime::ingestSessionRunning() const
{
    return _ingestController->ingestSessionRunning();
}

void VideoPlaybackRuntime::applyToReceiver(VideoReceiver* receiver) const
{
    if (!receiver)
        return;

    const PlaybackInput& playbackInput = input();
    receiver->setUri(playbackInput.uri);
    if (playbackInput.kind == PlaybackInput::Kind::StreamDevice)
        receiver->setSourceDevice(playbackInput.device, playbackInput.deviceUrl);
    else
        receiver->setSourceDevice(nullptr, {});
    receiver->setPlaybackPolicy(playbackInput.playbackPolicy);
}

std::unique_ptr<VideoRecorder> VideoPlaybackRuntime::createIngestRecorder(QObject* parent)
{
    return _ingestController->createIngestRecorder(parent);
}
