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

class QMediaCaptureSession;
class QtVideoRecorder;
class QVideoFrameInput;
class QVideoSink;

/// A network video stream played by Qt Multimedia's QMediaPlayer (FFmpeg): the receiver of builds without GStreamer,
/// and of VideoSettings::videoBackend's Qt Multimedia choice. Plays RTSP, MPEG-TS over UDP or TCP, and H.264 or H.265
/// RTP over UDP with payload type 96, which an SDP description tells FFmpeg about. Records by encoding the frames it
/// displays anew (QtVideoRecorder): only while an output displays the stream, and after a reconnect in the same file.
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
        /// RTSP interleaved over its TCP connection, rather than over UDP with FFmpeg's fallback to TCP.
        bool rtspOverTcp = false;
    };

    explicit QtMultimediaVideoReceiver(QObject* parent = nullptr);
    ~QtMultimediaVideoReceiver() override;

    /// The source for @p uri, one of VideoStreamResolver's: rtsp(s):// and tcp:// as they are, rtspt:// as rtsp:// over
    /// TCP, as GStreamer plays it, mpegts:// as udp://, and udp:// (H.264) or udp265:// (H.265) RTP as an SDP
    /// description. Other URLs play as they are; nullopt for an invalid one.
    static std::optional<Source> playerSource(const QString& uri);

protected:
    void onConfigChanged() override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onVideoOutputChanged() override;
    void onStartRecording(const QString& file, FileFormat format) override;
    void onStopRecording() override;

private:
    /// A user start, stop or restart, a new config, or the output going: the backoff and the reconnect count start
    /// over.
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
    /// Hands @p frame, on display, to the recording.
    void _recordFrame(const QVideoFrame& frame);
    /// The time in the recording, in µs, of a frame of stream time @p streamUs that arrived @p arrivalUs after the
    /// recording's first, after one at @p lastRecordedUs of stream time @p lastStreamUs (both -1 for the first): the
    /// stream's spacing while it runs on, the arrival time across a jump, and always at least kMinFrameStepUs after
    /// the last, since the encoder rejects a frame no later than the one before it.
    static qint64 recordingTimeUs(qint64 lastRecordedUs, qint64 lastStreamUs, qint64 streamUs, qint64 arrivalUs);

    QMediaPlayer* _player = nullptr;
    bool _openPending = false;
    /// Set by the destructor: no session opens while it waits for a recording's file to be finished.
    bool _closing = false;
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
    QtVideoRecorder* const _recorder;
    /// Created for the first recording: the frames on display reach the recorder through its frame input.
    QMediaCaptureSession* _recordingSession = nullptr;
    QVideoFrameInput* _frameInput = nullptr;
    /// Since the recorder started a file: the times of the frames in it.
    QElapsedTimer _sinceRecordingStarted;
    /// The recording and stream times of the last frame the recording took; -1 before its first.
    qint64 _lastRecordedUs = -1;
    qint64 _lastStreamUs = -1;
};
