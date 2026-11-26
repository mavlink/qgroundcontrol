#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtMultimedia/QVideoFrame>

#include "FFmpegStreamRecorder.h"
#include "FFmpegStreamSession.h"
#include "RecordingPlan.h"
#include "StreamSupervisor.h"
#include "VideoReceiver.h"
#include "VideoStreamStatsSampler.h"

/// Also logs the receiver's FFmpegStreamSession.
Q_DECLARE_LOGGING_CATEGORY(FFmpegVideoReceiverLog)

/// Experimental, and built only with QGC_ENABLE_FFMPEG_VIDEO: a network video stream received directly with the
/// FFmpeg libraries Qt ships, for VideoSettings::videoBackend's FFmpeg choice. Plays the stream URIs the other
/// receivers play (VideoStreamUri::ffmpegSource()): RTSP, also over TCP (rtspt://), H.264 or H.265 RTP over UDP with
/// payload type 96, and MPEG-TS over UDP or TCP.
///
/// Each connection is an FFmpegStreamSession, which opens, reads and decodes the stream on a thread of its own and
/// hands each decoded frame to this receiver; a frame not yet shown when the next one arrives is dropped. Streaming
/// once the stream's first video packet arrives; decoding while frames reach the output. A session that fails, or gets
/// no packet, or no frame while displaying, for the config's timeout (5 s more for a server's first packet and for the
/// first frame) has failed: its StreamSupervisor reconnects it after 1 s, doubling up to 30 s, while autoReconnect is
/// on, and it stops with errorOccurred() otherwise. A UDP port nobody sends to yet hasn't failed: the session listens
/// for as long as it takes. A new session opens once the one it replaces has ended, as a UDP port can't be bound twice.
///
/// Records the stream as it arrives, without decoding it (FFmpegStreamRecorder), also with no output: from the next
/// keyframe, and after a reconnect, and after each hour, in the next part its RecordingPlan names.
///
/// Threading: everything here runs on the GUI thread, which also sets each frame on the output's QVideoSink. The
/// sessions' and the recorder's threads reach it only through queued calls, and the frame they hand over under
/// _frameMutex. The destructor waits for the sessions' threads, which end within ~0.1 s of being asked to, and then
/// for the recorder's, which finishes the open file.
class FFmpegVideoReceiver : public VideoReceiver,
                            private FFmpegStreamSession::Listener,
                            private FFmpegStreamRecorder::Listener
{
    Q_OBJECT

    friend class FFmpegVideoReceiverStreamTest;

public:
    explicit FFmpegVideoReceiver(QObject* parent = nullptr);
    ~FFmpegVideoReceiver() override;

protected:
    void onConfigChanged() override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onReconnectRequested() override;
    void onVideoOutputChanged() override;
    void onStartRecording(const QString& file, FileFormat format, const RecordingEncoding& encoding) override;
    void onStopRecording() override;

private:
    /// A user start, stop or restart, or a new config: the backoff and the reconnect count start over.
    void _startOver();
    /// Starts a new session when the stream should run; stops it otherwise.
    void _start();
    /// Opens the session _start() started, once the sessions it replaces have ended.
    void _open();
    /// Ends the current session, which finishes on its thread: no frame or stats from it any more.
    void _stopSession();
    QString _redactedUri() const;
    /// The session listens on a UDP port, for at least its timeout, and no stream has arrived.
    bool _awaitingSender() const;
    /// The session failed for @p reason: reconnects with backoff, or stops and reports it. One still awaiting its
    /// sender starts anew instead.
    void _fail(const QString& reason, bool permanent = false);
    /// The stream has stopped for good, until it is started over: ends its recording and reports @p message.
    void _onHalted(const QString& message);
    /// Tells the recorder what the recording records now: the current part's file, from the current session.
    void _updateRecorder();
    void _endRecording();
    /// Sets the newest frame handed over on the output, on the GUI thread.
    void _showFrame();
    /// Once a second while a session runs: the stats, and the watchdog.
    void _sample();
    /// The session with @p id ended: frees it, and opens the next one where it waited for that.
    void _onSessionEnded(quint64 id, const std::optional<FFmpegStreamSession::Failure>& failure);

    bool _isCurrent(quint64 id) const { return _session && (_session->id() == id); }

    // FFmpegStreamSession::Listener, on the sessions' threads.
    void onSessionStreaming(quint64 session, const QString& codec) override;
    void onSessionDecoder(quint64 session, const FFmpegStreamSession::DecoderInfo& decoder) override;
    void onSessionFrame(quint64 session, QVideoFrame frame) override;
    void onSessionDecoderFailed(quint64 session, const QString& message) override;
    void onSessionEnded(quint64 session, const std::optional<FFmpegStreamSession::Failure>& failure) override;

    // FFmpegStreamRecorder::Listener, on the recorder's thread.
    void onRecordingFileOpened(const QString& file) override;
    void onRecordingFileClosed(const QString& file, const QString& error) override;
    void onRecordingNeedsFormat(const QString& file, FileFormat format) override;
    void onRecordingFailed(const QString& file, const QString& error) override;

    StreamSupervisor* const _supervisor;
    /// Destroyed after the sessions, which hand it their packets.
    std::unique_ptr<FFmpegStreamRecorder> _recorder;
    /// The recording asked for, until it stops or fails.
    std::optional<RecordingPlan> _recording;
    QTimer _sampleTimer;
    VideoStreamStatsSampler _statsSampler;
    std::unique_ptr<FFmpegStreamSession> _session;
    /// Asked to stop, and not ended yet.
    std::vector<std::unique_ptr<FFmpegStreamSession>> _endingSessions;
    quint64 _lastSessionId = 0;
    /// A session waits for those ending.
    bool _openPending = false;
    /// The config the session started with.
    Config _sessionConfig;
    /// Since the session was created.
    QElapsedTimer _sinceOpened;
    /// Since the session started, the output was set, or the last frame was shown.
    QElapsedTimer _sinceFrame;
    quint64 _framesShown = 0;
    FFmpegStreamSession::DecoderInfo _decoderInfo;
    QString _codec;
    /// The session's stream can't be decoded: it goes on without a picture.
    bool _sessionUndecodable = false;
    /// errorOccurred() told that the stream can't be decoded: not again until it is started anew.
    bool _decoderFailureReported = false;

    QMutex _frameMutex;
    /// The newest frame handed over and not shown yet, of session _pendingFrameSession.
    QVideoFrame _pendingFrame;
    quint64 _pendingFrameSession = 0;
    /// A _showFrame() is queued.
    bool _frameShowPosted = false;
    /// Frames replaced by a newer one before they were shown, of the current session.
    quint64 _framesDropped = 0;
};
