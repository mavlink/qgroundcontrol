#pragma once

#include <functional>
#include <memory>

#include <QtCore/QObject>

#include "VideoIngestController.h"
#include "VideoPlaybackInput.h"
#include "VideoReceiver.h"
#include "VideoSourceResolver.h"

class VideoRecorder;

/// Owns playback-input runtime state for one stream: GStreamer ingest refresh,
/// direct/local playback input, receiver input application, and ingest recorder
/// creation. VideoStreamSession owns lifecycle decisions around this helper.
class VideoPlaybackRuntime : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoPlaybackRuntime)

public:
    using PlaybackInput = VideoPlaybackInput;
    using Resolver = VideoIngestController::Resolver;

    explicit VideoPlaybackRuntime(QString streamName, QObject* parent = nullptr);
    ~VideoPlaybackRuntime() override = default;

    void setResolverForTest(Resolver resolver);
    void setSource(const VideoSourceResolver::SourceDescriptor& source);
    [[nodiscard]] bool refreshForStart();
    void markRefreshNeeded();
    void stop();

    [[nodiscard]] bool hasInput() const;
    [[nodiscard]] const PlaybackInput& input() const;
    [[nodiscard]] bool ingestSessionRunning() const;

    void applyToReceiver(VideoReceiver* receiver) const;
    [[nodiscard]] std::unique_ptr<VideoRecorder> createIngestRecorder(QObject* parent);

signals:
    void errorOccurred(VideoReceiver::ErrorCategory category, const QString& message);
    void endOfStream();

private:
    VideoIngestController* _ingestController = nullptr;
};
