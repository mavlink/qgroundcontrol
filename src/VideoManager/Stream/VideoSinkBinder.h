#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtMultimedia/QVideoSink>

#include "VideoStream.h"

/// Keeps a QML VideoOutput sink registered with exactly one VideoStream.
class VideoSinkBinder : public QObject
{
    Q_OBJECT

    Q_PROPERTY(VideoStream* stream READ stream WRITE setStream NOTIFY streamChanged)
    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)

public:
    explicit VideoSinkBinder(QObject* parent = nullptr);
    ~VideoSinkBinder() override;

    [[nodiscard]] VideoStream* stream() const { return _stream.data(); }
    void setStream(VideoStream* stream);

    [[nodiscard]] QVideoSink* videoSink() const { return _videoSink.data(); }
    void setVideoSink(QVideoSink* videoSink);

signals:
    void streamChanged();
    void videoSinkChanged();

private:
    void _rebind();
    void _clearRegisteredSink();

    QPointer<VideoStream> _stream;
    QPointer<QVideoSink> _videoSink;
    QPointer<VideoStream> _registeredStream;
    QPointer<QVideoSink> _registeredSink;
};
