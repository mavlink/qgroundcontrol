#pragma once

#include <QtCore/QObject>
#include <functional>
#include <memory>

#include "VideoSourceResolver.h"

class VideoFrameDelivery;
class VideoRecorder;
class VideoReceiver;
class VideoStream;

/// Owns recorder creation, signal wiring, test injection, and release/rebuild.
class VideoRecordingService : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoRecordingService)

public:
    using RecorderFactory = std::function<VideoRecorder*(VideoStream*)>;

    explicit VideoRecordingService(VideoStream* owner, QObject* parent = nullptr);

    [[nodiscard]] VideoRecorder* recorder() const { return _recorder.get(); }

    void create(const VideoSourceResolver::VideoSource& source,
                VideoReceiver* receiver,
                VideoFrameDelivery* delivery,
                std::unique_ptr<VideoRecorder> preferredRecorder = {});
    void destroy();
    [[nodiscard]] std::unique_ptr<VideoRecorder> release();
    void setForTest(VideoRecorder* recorder);
    void setFactoryForTest(RecorderFactory factory) { _recorderFactoryForTest = std::move(factory); }

signals:
    void recordingStarted(const QString& path);
    void recordingChanged(bool active);
    void recordingError(const QString& message);
    void recorderChanged();

private:
    void _wireRecorderSignals();

    VideoStream* _owner = nullptr;
    std::unique_ptr<VideoRecorder> _recorder;
    RecorderFactory _recorderFactoryForTest;
};
