#include "VideoRecordingService.h"

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"
#include "VideoReceiver.h"
#include "VideoRecorder.h"
#include "VideoRecordingPolicy.h"
#include "VideoStream.h"

QGC_LOGGING_CATEGORY(VideoRecordingServiceLog, "Video.RecordingService")

VideoRecordingService::VideoRecordingService(VideoStream* owner, QObject* parent)
    : QObject(parent)
    , _owner(owner)
{
}

void VideoRecordingService::create(const VideoSourceResolver::VideoSource& source,
                                   VideoReceiver* receiver,
                                   VideoFrameDelivery* delivery,
                                   std::unique_ptr<VideoRecorder> preferredRecorder)
{
    destroy();

    if (!receiver)
        return;

    _recorder = preferredRecorder
                    ? std::move(preferredRecorder)
                    : VideoRecordingPolicy::createRecorder(source, receiver, delivery, this, _owner, _recorderFactoryForTest);
    if (!_recorder)
        return;
    _recorder->setParent(this);

    _wireRecorderSignals();
    qCDebug(VideoRecordingServiceLog) << "Created recorder for" << source.uri;
    emit recorderChanged();
}

void VideoRecordingService::destroy()
{
    if (!_recorder)
        return;

    if (_recorder->isRecording())
        _recorder->stop();

    _recorder.reset();
    emit recorderChanged();
}

std::unique_ptr<VideoRecorder> VideoRecordingService::release()
{
    if (_recorder) {
        _recorder->disconnect(this);
        _recorder->setParent(nullptr);
    }

    return std::move(_recorder);
}

void VideoRecordingService::setForTest(VideoRecorder* recorder)
{
    destroy();
    if (!recorder)
        return;

    recorder->setParent(this);
    _recorder.reset(recorder);
    _wireRecorderSignals();
    emit recorderChanged();
}

void VideoRecordingService::_wireRecorderSignals()
{
    connect(_recorder.get(), &VideoRecorder::started, this, [this](const QString& path) {
        emit recordingStarted(path);
        emit recordingChanged(true);
    });
    connect(_recorder.get(), &VideoRecorder::stopped, this, [this](const QString&) {
        emit recordingChanged(false);
    });
    connect(_recorder.get(), &VideoRecorder::error, this, [this](const QString& message) {
        emit recordingError(message);
        emit recordingChanged(false);
    });
}
