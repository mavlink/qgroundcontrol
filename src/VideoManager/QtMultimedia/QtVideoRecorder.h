#pragma once

#include <chrono>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaRecorder>

#include "VideoReceiver.h"

/// Records the video of a QMediaCaptureSession with Qt Multimedia's encoders: a USB camera's (UVCCameraController), or
/// a stream's decoded frames fed in through a QVideoFrameInput (QtMultimediaVideoReceiver). The capture session records
/// with mediaRecorder(), which outlives it: a session going away mid-recording makes it finish the file and report it.
///
/// start() records to "<file without suffix>.<suffix of the format used>", at the quality asked for, trying
/// recordingFormats() in turn until one encodes. recordingStarted() follows once the first frame is encoded, or
/// recordingFinished() with an error; a failed attempt leaves no file behind. Recordings carry their start time and
/// VideoReceiver::recordingComment().
class QtVideoRecorder : public QObject
{
    Q_OBJECT

    friend class QtVideoRecorderTest;

public:
    /// How long an owner going away waits for a recording's file to be finished.
    static constexpr std::chrono::seconds kFinishTimeout{5};

    explicit QtVideoRecorder(QObject* parent = nullptr);
    ~QtVideoRecorder() override;

    QMediaRecorder* mediaRecorder() const { return _recorder; }

    /// From the first encoded frame until the recorder stops.
    bool recording() const { return _recording; }

    /// Recording, or waiting for the first frame to encode.
    bool active() const { return _recorder->recorderState() != QMediaRecorder::StoppedState; }

    /// Records the capture session mediaRecorder() is set on to @p file, in @p container and @p codec where Qt can
    /// encode them: see recordingFormats(). @p quality sets how large the file gets: Qt encodes for a constant
    /// quality, or for a bit rate it works out from one where the encoder takes only that (Android, iOS, Intel Macs).
    /// Reports recordingFinished() with an error when no format encodes.
    void start(const QString& file, QMediaFormat::FileFormat container,
               QMediaFormat::VideoCodec codec = QMediaFormat::VideoCodec::Unspecified,
               QMediaRecorder::Quality quality = QMediaRecorder::HighQuality);
    void stop();
    /// Stops recording and waits up to @p timeout for the file to be finished: Qt writes it on its own thread, which
    /// the application's exit would cut short. Also for a recorder still waiting for its first frame, which holds the
    /// file open until it has stopped.
    void stopAndWait(std::chrono::milliseconds timeout = kFinishTimeout);

    /// The formats start() tries for @p container, best first: @p codec where Qt can encode it, then H.264 and the
    /// other codecs Qt can encode here, down to software MPEG-2. In @p container where Qt can write it, else in another
    /// container. Qt counts a hardware encoder as encodable even where the hardware turns it down, hence the fallbacks.
    static QList<QMediaFormat> recordingFormats(QMediaFormat::FileFormat container,
                                                QMediaFormat::VideoCodec codec = QMediaFormat::VideoCodec::Unspecified);
    /// The codecs Qt can encode a recording in here, in any of the recording containers, as the keys of
    /// QMediaFormat::VideoCodec (such as "H264"): the values of VideoSettings::uvcRecordingCodec. recordingFormats()'s
    /// fallbacks come first, in its order.
    static QStringList recordingCodecs();
    /// The codec of key @p key, as recordingCodecs() names it; Unspecified for an empty or unknown key.
    static QMediaFormat::VideoCodec recordingCodec(const QString& key);
    /// The name of the codec of key @p key, such as "H.264", for display.
    static QString recordingCodecName(const QString& key);
    /// The container of @p format.
    static QMediaFormat::FileFormat container(VideoReceiver::FileFormat format);
    /// The quality of value @p value, one of VideoSettings::uvcRecordingQuality's; HighQuality for any other.
    static QMediaRecorder::Quality recordingQuality(int value);

signals:
    void recordingChanged(bool recording);
    void recordingStarted(const QString& file);
    /// @p error is empty for a recording that finished normally.
    void recordingFinished(const QString& file, const QString& error);

private:
    /// Records with the next of _pendingFormats.
    void _recordNextFormat();
    /// _recordNextFormat() after a format failed, unless a stop or another start came first.
    void _retryNextFormat();
    void _onRecorderStateChanged(QMediaRecorder::RecorderState state);
    void _onRecorderError(const QString& errorString);
    /// The recorder stopped before it encoded a frame: tries the next format after a failure, else reports it.
    void _onRecordingNotStarted();

    QMediaRecorder* const _recorder;
    QList<QMediaFormat> _pendingFormats;
    QMediaRecorder::Quality _quality = QMediaRecorder::HighQuality;
    /// The file being recorded, without its suffix.
    QString _recordingBase;
    QString _recordingFile;
    QString _recordingError;
    bool _recording = false;
    bool _stopRequested = false;
};
