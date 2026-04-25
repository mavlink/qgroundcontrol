#pragma once

#include <QtCore/QObject>
#include <functional>
#include <memory>

#include "VideoIngestSessionController.h"
#include "VideoPlaybackInput.h"
#include "VideoSourceResolver.h"

class VideoRecorder;

class VideoIngestController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoIngestController)

public:
    using Resolver = std::function<VideoPlaybackInput(const VideoSourceResolver::SourceDescriptor& source)>;

    explicit VideoIngestController(QString streamName, QObject* parent = nullptr);
    ~VideoIngestController() override = default;

    void setResolverForTest(Resolver resolver);
    void setSource(const VideoSourceResolver::SourceDescriptor& source);
    [[nodiscard]] bool refreshForStart();
    void markRefreshNeeded();
    void stopIngestSession();

    [[nodiscard]] bool hasInput() const { return _input.isValid(); }
    [[nodiscard]] const VideoPlaybackInput& input() const { return _input; }
    [[nodiscard]] bool ingestSessionRunning() const;
    [[nodiscard]] std::unique_ptr<VideoRecorder> createIngestRecorder(QObject* parent);

signals:
    void errorOccurred(VideoReceiver::ErrorCategory category, const QString& message);
    void endOfStream();

private:
    [[nodiscard]] VideoPlaybackInput _resolve(const VideoSourceResolver::SourceDescriptor& source);
    void _setInput(VideoPlaybackInput input);
    [[nodiscard]] VideoPlaybackInput::Kind _kindForInput(const VideoPlaybackInput& input) const;

    VideoSourceResolver::SourceDescriptor _source;
    VideoPlaybackInput _input;
    Resolver _resolverForTest;
    VideoIngestSessionController* _ingestSession = nullptr;
    bool _refreshNeeded = false;
};
