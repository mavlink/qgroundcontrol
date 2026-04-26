#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "VideoPlaybackInput.h"
#include "VideoReceiver.h"
#include "VideoSourceResolver.h"

#include <memory>

#ifdef QGC_GST_STREAMING
class GstIngestSession;
#endif
class VideoRecorder;

class VideoIngestSessionController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoIngestSessionController)

public:
    explicit VideoIngestSessionController(QString streamName, QObject* parent = nullptr);
    ~VideoIngestSessionController() override;

    [[nodiscard]] VideoPlaybackInput resolvePlaybackInput(const VideoSourceResolver::SourceDescriptor& source);
    [[nodiscard]] bool running() const;
    [[nodiscard]] std::unique_ptr<VideoRecorder> createIngestRecorder(QObject* parent);

    void stop();

signals:
    void errorOccurred(VideoReceiver::ErrorCategory category, const QString& message);
    void endOfStream();

private:
    QString _streamName;

#ifdef QGC_GST_STREAMING
    std::unique_ptr<GstIngestSession> _gstIngestSession;
#endif
};
