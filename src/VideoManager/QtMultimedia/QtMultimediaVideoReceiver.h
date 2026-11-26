#pragma once

#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtMultimedia/QMediaPlayer>

#include "StreamSupervisor.h"
#include "VideoReceiver.h"
#include "VideoStreamStatsSampler.h"

class QMediaCaptureSession;
class QtVideoRecorder;
class QVideoFrameInput;

/// A network video stream played by Qt Multimedia's QMediaPlayer (FFmpeg): the receiver of builds without GStreamer,
/// and of VideoSettings::videoBackend's Qt Multimedia choice. Plays RTSP, MPEG-TS over UDP or TCP, and H.264 or H.265
/// RTP over UDP with payload type 96, which an SDP description tells FFmpeg about. Records by encoding the frames it
/// displays anew (QtVideoRecorder), as startRecording() is told: only while an output displays the stream, and after a
/// reconnect in the same file.
///
/// Streaming once the player has buffered the stream or its first frame arrives; decoding while frames reach the
/// output. A stream that errors, ends, or shows no frame for the config's timeout while displaying (5 s more for the
/// first, while FFmpeg probes the stream) has failed: its StreamSupervisor reconnects it after 1 s, doubling up to
/// 30 s, while autoReconnect is on, and it stops with errorOccurred() otherwise. A UDP port nobody sends to yet hasn't
/// failed: once FFmpeg gives up on it, the receiver listens again at once, for as long as it takes.
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
        /// An SDP names the streams, this one's or the RTSP server's: FFmpeg needn't read the stream to find them.
        bool streamsDescribed = false;
    };

    explicit QtMultimediaVideoReceiver(QObject* parent = nullptr);
    ~QtMultimediaVideoReceiver() override;

    /// The source for @p uri, one of VideoStreamResolver's: rtsp(s):// and tcp:// as they are, rtspt:// as rtsp:// over
    /// TCP, as GStreamer plays it, mpegts:// as udp://, and udp:// (H.264) or udp265:// (H.265) RTP as an SDP
    /// description. Other URLs play as they are; nullopt for an invalid one, and for one of those whose address no
    /// receiver opens (VideoStreamUri::addressError()) or names port 0 to listen on.
    static std::optional<Source> playerSource(const QString& uri);

    bool encodesRecordings() const override { return true; }

protected:
    void onConfigChanged() override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onReconnectRequested() override;
    void onVideoOutputChanged() override;
    /// A start while a recording is in progress, or awaits its first frame, is refused: reported as
    /// recordingFinished() for @p file with the reason, while that recording goes on.
    void onStartRecording(const QString& file, FileFormat format, const RecordingEncoding& encoding) override;
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
    /// The URI without its credentials, for logs and messages.
    QString _redactedUri() const;
    /// The session listens on a UDP port, for at least its timeout, and no stream has arrived.
    bool _awaitingSender() const;
    /// The session failed for @p reason: reconnects with backoff, or stops and reports it. One still awaiting its
    /// sender starts anew instead.
    void _fail(const QString& reason);
    /// The stream has stopped for good, until it is started over: ends its recording and reports @p message.
    void _onHalted(const QString& message);
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
    QMetaObject::Connection _frameConnection;
    StreamSupervisor* const _supervisor;
    QTimer _sampleTimer;
    /// Since the session's player was created.
    QElapsedTimer _sinceOpened;
    /// Since the session started, or its last frame.
    QElapsedTimer _sinceFrame;
    VideoStreamStatsSampler _statsSampler;
    quint64 _frames = 0;
    bool _gpuFrames = false;
    QtVideoRecorder* const _recorder;
    /// Created for the first recording: the frames on display reach the recorder through its frame input.
    QMediaCaptureSession* _recordingSession = nullptr;
    QVideoFrameInput* _frameInput = nullptr;
    /// Since the first frame of the file being recorded: the times of the frames in it.
    QElapsedTimer _sinceRecordingStarted;
    /// The recording and stream times of the last frame the recording took; -1 before its first.
    qint64 _lastRecordedUs = -1;
    qint64 _lastStreamUs = -1;
};
