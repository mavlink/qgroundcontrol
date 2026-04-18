#pragma once

#include <QtCore/QLoggingCategory>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaRecorder>
#include <QtMultimedia/QVideoFrameInput>

#include "VideoRecorder.h"

Q_DECLARE_LOGGING_CATEGORY(BridgeRecorderLog)

class VideoFrameDelivery;
class QVideoFrame;

/// VideoRecorder implementation backed by Qt's QMediaRecorder.
///
/// Feeds QVideoFrames from VideoFrameDelivery into QVideoFrameInput →
/// QMediaCaptureSession → QMediaRecorder. Works with any receiver type
/// (GStreamer, QtMultimedia, UVC) since all deliver frames through delivery.
///
/// Subscribes to VideoFrameDelivery::frameArrived and pulls lastRawFrame()
/// rather than polling QVideoSink::videoFrame().
class BridgeRecorder : public VideoRecorder
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(BridgeRecorder)

public:
    explicit BridgeRecorder(VideoFrameDelivery* delivery, QObject* parent = nullptr);
    ~BridgeRecorder() override;

    bool start(const QString& path, QMediaFormat::FileFormat format) override;
    void stop() override;
    [[nodiscard]] Capabilities capabilities() const override;

private:
    void _onRecorderStateChanged(QMediaRecorder::RecorderState state);
    void _tryPushFrame();
    void _selectBestVideoCodec();

    VideoFrameDelivery* _delivery = nullptr;
    QVideoFrameInput* _frameInput = nullptr;
    QMediaCaptureSession _captureSession;
    QMediaRecorder _recorder;
    QMetaObject::Connection _frameConn;
    QMetaObject::Connection _readyConn;
};
