#pragma once

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimedia/qtvideo.h>

/// Hands the USB camera's frames on to the VideoOutput's sink, turned and mirrored as the rotation and mirror settings
/// say. Turning the frames instead of the VideoOutput (VideoOutput.orientation) keeps upright what Qt draws over them:
/// the telemetry overlay, the output's subtitle.
///
/// Frames arrive on the camera's thread; the latest is handed on on this object's thread, older ones it replaced
/// dropped, as the display would.
class UVCFrameRelay : public QObject
{
    Q_OBJECT

    friend class UVCFrameRelayTest;

public:
    explicit UVCFrameRelay(QObject* parent = nullptr);
    ~UVCFrameRelay() override;

    /// The sink the capture session hands the camera's frames to.
    QVideoSink* input() { return &_input; }

    /// The sink the frames go on to; nullptr drops them.
    void setOutput(QVideoSink* output);

    /// Turns and mirrors each frame on top of its own rotation and mirroring.
    void setTransform(QtVideo::Rotation rotation, bool mirrored);

    QtVideo::Rotation rotation() const { return _rotation; }

    bool mirrored() const { return _mirrored; }

    /// A frame showing @p frame's picture, turned by @p rotation and mirrored by @p mirrored on top of @p frame's own
    /// rotation and mirroring. It shares @p frame's buffer but not its metadata: copies of a QVideoFrame share their
    /// rotation, and the recorder and image capture hold copies of the camera's frames.
    static QVideoFrame transformed(const QVideoFrame& frame, QtVideo::Rotation rotation, bool mirrored);

private:
    /// Any thread.
    void _onFrame(const QVideoFrame& frame);
    void _deliver();

    QVideoSink _input;
    QPointer<QVideoSink> _output;
    QtVideo::Rotation _rotation = QtVideo::Rotation::None;
    bool _mirrored = false;

    QMutex _pendingMutex;
    /// Guarded by _pendingMutex.
    QVideoFrame _pending;
    bool _deliveryQueued = false;
};
