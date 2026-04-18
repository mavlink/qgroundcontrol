#pragma once

#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <chrono>

#include "VideoReceiver.h"

class QMediaPlayer;
class QVideoSink;

class QtMultimediaReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    explicit QtMultimediaReceiver(QObject* parent = nullptr);
    virtual ~QtMultimediaReceiver();

    static VideoReceiver* createVideoReceiver(QObject* parent);

    [[nodiscard]] Capabilities capabilities() const override { return CapStreaming | CapRecording; }

    [[nodiscard]] BackendKind kind() const override { return BackendKind::QtMultimedia; }

    [[nodiscard]] bool isStreaming() const override { return _streamingActive; }

    [[nodiscard]] bool isDecoding() const override { return _decoderActive; }

    /// Qt Multimedia (FFmpeg backend) provides no per-frame PTS-vs-clock measurement.
    [[nodiscard]] bool latencySupported() const override { return false; }

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding() override;
    void stopDecoding() override;
    void pause() override;
    void resume() override;

protected:
    void onSinkAboutToChange() override;
    void onSinkChanged(QVideoSink* newSink) override;

    /// Read the current codec/decoder info from the media player's metadata
    /// and update the base-class decoder fields. Called from hasVideoChanged
    /// and metaDataChanged to give QML parity with GstVideoReceiver on the
    /// decoder-name readout (HW decode status is not exposed by Qt's FFmpeg
    /// backend, so hwDecoding stays false).
    void _pullDecoderInfoFromMetadata();

    QMediaPlayer* _mediaPlayer = nullptr;

    /// Internal sink that QMediaPlayer writes to. Its videoFrameChanged signal
    /// is connected to VideoFrameDelivery::deliverFrame so all frames are pushed
    /// through the delivery endpoint symmetrically with the GStreamer path.
    QVideoSink* _internalSink = nullptr;
    QMetaObject::Connection _internalFrameConn;

    /// Per-frame watchdog interval, stashed in start() and applied to the
    /// delivery watchdog in startDecoding(). The delivery endpoint owns the timer.
    std::chrono::milliseconds _watchdogInterval{0};

    /// Connections and timer used for async start-result signalling (receiverStarted / receiverError).
    /// The timer is a value member — it lives as long as this receiver and is
    /// rewired per start() call. Single-shot semantics are set in the ctor body.
    QMetaObject::Connection _startPlayingConn;
    QMetaObject::Connection _startBufferConn;
    QMetaObject::Connection _startTimeoutConn;
    QTimer _startTimeoutTimer;

    void _clearStartHandlers();
    void _rewireInternalSink();

    void _setStreamingActive(bool active);
    void _setDecoderActive(bool active);

    bool _streamingActive = false;
    bool _decoderActive = false;
};
