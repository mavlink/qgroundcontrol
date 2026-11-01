#pragma once

#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtMultimedia/QMediaPlayer>

#include "ExponentialBackoff.h"
#include "VideoReceiver.h"
#include "VideoStreamStatsMeter.h"

class QVideoSink;

/// A network video stream played by Qt Multimedia's QMediaPlayer (FFmpeg): the receiver of builds without GStreamer,
/// and of VideoSettings::videoBackend's Qt Multimedia choice. Plays RTSP, MPEG-TS over UDP or TCP, and H.264 or H.265
/// RTP over UDP with payload type 96, which an SDP description tells FFmpeg about. Doesn't record: startRecording()
/// reports errorOccurred().
///
/// Streaming once the first frame arrives; decoding while frames reach the output. A stream that errors, ends, or
/// shows no frame for the config's timeout while displaying (5 s more for the first, while FFmpeg probes the stream)
/// has failed: it reconnects after 1 s, doubling up to 30 s, while autoReconnect is on, and stops with errorOccurred()
/// otherwise.
class QtMultimediaVideoReceiver : public VideoReceiver
{
    Q_OBJECT

    friend class QtMultimediaVideoReceiverTest;

public:
    /// What QMediaPlayer plays for a stream URI: a URL, or for RTP over UDP an SDP description of it.
    struct Source
    {
        QUrl url;
        QByteArray sdp;
    };

    explicit QtMultimediaVideoReceiver(QObject* parent = nullptr);
    ~QtMultimediaVideoReceiver() override;

    /// The source for @p uri, one of VideoStreamResolver's: rtsp(s):// and tcp:// as they are, mpegts:// as udp://,
    /// and udp:// (H.264) or udp265:// (H.265) RTP as an SDP description. Other URLs play as they are; nullopt for an
    /// invalid one.
    static std::optional<Source> playerSource(const QString& uri);

protected:
    void onConfigChanged() override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onVideoOutputChanged() override;
    void onStartRecording(const QString& file, FileFormat format) override;
    void onStopRecording() override;

private:
    /// A user start, stop or restart, or a new config: the backoff and the reconnect count start over.
    void _startOver();
    /// Starts a new session when the stream should run; stops it otherwise.
    void _start();
    /// Opens the session _start() started, once the event loop has unloaded the replaced player.
    void _open();
    /// Ends the session: no player, frame or stats.
    void _stopSession();
    /// The session failed for @p reason: reconnects with backoff, or stops and reports it.
    void _fail(const QString& reason);
    void _bindVideoSink();
    void _onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void _onVideoFrame(const QVideoFrame& frame);
    /// Once a second while a session runs: the stats, and the frame watchdog.
    void _sample();

    QMediaPlayer* _player = nullptr;
    bool _openPending = false;
    /// The config the session started with.
    Config _sessionConfig;
    QPointer<QVideoSink> _videoSink;
    QMetaObject::Connection _frameConnection;
    QTimer _reconnectTimer;
    QTimer _sampleTimer;
    ExponentialBackoff _backoff{std::chrono::seconds(1), 2, std::chrono::seconds(30)};
    /// Since the session started, or its last frame.
    QElapsedTimer _sinceFrame;
    QElapsedTimer _sinceStreaming;
    VideoStreamStatsMeter _meter;
    quint64 _frames = 0;
    bool _gpuFrames = false;
    /// Sessions that showed a frame since _startOver(), for VideoStreamStats::reconnects.
    int _streamedSessions = 0;
};
