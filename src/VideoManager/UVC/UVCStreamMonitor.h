#pragma once

#include <optional>

#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoStreamStats.h"
#include "VideoStreamStatsMeter.h"

class QCamera;
class QVideoFrame;
class QVideoSink;

/// Measures the USB camera on display, from the frames its capture session hands the video sink, about once a second
/// while frames arrive. UVCCameraController attaches its camera and the sink of the registered VideoOutput.
///
/// stats fills valid, fps, codec (the camera's capture format, e.g. "MJPEG" or "YUYV"), gpuPath ("CPU" for frames in
/// memory, "GPU" for textures) and uptimeMs; the network stream fields keep their defaults.
class UVCStreamMonitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(VideoStreamStats stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY statsChanged)
    Q_PROPERTY(double cameraFrameRate READ cameraFrameRate NOTIFY statsChanged)
    Q_PROPERTY(QString cameraName READ cameraName NOTIFY cameraNameChanged)

    friend class UVCStreamMonitorTest;

public:
    /// @p clock defaults to the steady clock.
    explicit UVCStreamMonitor(QObject* parent = nullptr, VideoStreamStatsMeter::Clock clock = {});

    /// Measures the frames @p videoSink receives from @p camera (optional), in place of any other sink attached.
    void attach(QCamera* camera, QVideoSink* videoSink);
    /// Stops measuring.
    void detach();

    VideoStreamStats stats() const { return _stats; }

    QSize videoSize() const { return _videoSize; }

    /// The frame rate the camera was set up for; 0 while unknown.
    double cameraFrameRate() const { return _cameraFrameRate; }

    /// The camera's description; empty while no camera is attached.
    QString cameraName() const { return _cameraName; }

signals:
    void statsChanged();
    void cameraNameChanged();

private:
    /// The frames received since the last restart, as the latest one described them.
    struct FrameCounters
    {
        quint64 frames = 0;
        QSize size;
        QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
        double streamFrameRate = 0.0;
        bool gpu = false;
    };

    void _onVideoFrame(const QVideoFrame& frame);
    void _sample();
    /// A new stream starts: the measurements so far are cleared.
    void _restart();
    void _disconnect();
    void _updateCameraName();
    void _publish(const VideoStreamStats& stats, QSize videoSize, double cameraFrameRate);

    VideoStreamStatsMeter::Clock _clock;
    VideoStreamStatsMeter _meter;
    QTimer _sampleTimer;
    QPointer<QCamera> _camera;
    QPointer<QVideoSink> _videoSink;
    QList<QMetaObject::Connection> _connections;

    FrameCounters _frameCounters;

    std::optional<qint64> _streamingSinceMs;
    VideoStreamStats _stats;
    QSize _videoSize;
    double _cameraFrameRate = 0.0;
    QString _cameraName;
};
