#pragma once

#include <QtCore/QElapsedTimer>
#include <QtMultimedia/QMediaPlayer>

#include "SessionVideoReceiver.h"

class QMediaCaptureSession;
class QtVideoRecorder;
class QVideoFrameInput;

/// A network video stream played by Qt Multimedia's QMediaPlayer (FFmpeg): the receiver of builds without GStreamer,
/// and of VideoSettings::videoBackend's Qt Multimedia choice. Plays RTSP, MPEG-TS over UDP or TCP, and H.264 or H.265
/// RTP over UDP with payload type 96, which an SDP description tells FFmpeg about (VideoStreamUri::ffmpegSource()).
/// Records by encoding the frames it displays anew (QtVideoRecorder), as startRecording() is told: only while an output
/// displays the stream, and after a reconnect in the same file.
///
/// Streaming once the player has buffered the stream or its first frame arrives; decoding while frames reach the
/// output. A stream that errors, ends, or shows no frame for the config's timeout while displaying (5 s more for the
/// first, while FFmpeg probes the stream) has failed, and SessionVideoReceiver reconnects or stops it. The player
/// doesn't tell when its stream arrives, so the frames at the output are all its watchdog is told.
class QtMultimediaVideoReceiver : public SessionVideoReceiver
{
    Q_OBJECT

    friend class QtMultimediaVideoReceiverTest;

public:
    explicit QtMultimediaVideoReceiver(QObject* parent = nullptr);
    ~QtMultimediaVideoReceiver() override;

    bool encodesRecordings() const override { return true; }

protected:
    void onVideoOutputChanged() override;

    bool hasSession() const override { return _player != nullptr; }

    /// Queued, after the event loop has unloaded the replaced player.
    void openWhenReady() override;
    void openSession(const VideoStreamUri::FFmpegSource& source) override;
    void closeSession() override;
    bool pictureDue() const override;
    void sampleSession() override;
    /// A start while a recording is in progress, or awaits its first frame, is refused: reported as
    /// recordingFinished() for @p file with the reason, while that recording goes on.
    void beginRecording(const QString& file, FileFormat format, const RecordingEncoding& encoding) override;
    void endRecording() override;

private:
    void _bindVideoSink();
    void _onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void _onVideoFrame(const QVideoFrame& frame);
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
    QMetaObject::Connection _frameConnection;
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
