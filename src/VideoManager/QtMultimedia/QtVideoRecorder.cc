#include "QtVideoRecorder.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>
#include <QtCore/QMimeType>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtMultimedia/QMediaMetaData>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QtVideoRecorderLog, "Video.QtVideoRecorder")

namespace {

QString fileSuffix(const QMediaFormat& format)
{
    switch (format.fileFormat()) {
        case QMediaFormat::Matroska:
            return QStringLiteral("mkv");
        case QMediaFormat::QuickTime:
            return QStringLiteral("mov");
        case QMediaFormat::MPEG4:
            return QStringLiteral("mp4");
        default:
            return format.mimeType().preferredSuffix();
    }
}

/// The containers a recording can go into, as VideoSettings::recordingFormat offers them.
constexpr QMediaFormat::FileFormat kRecordingContainers[] = {QMediaFormat::MPEG4, QMediaFormat::Matroska,
                                                             QMediaFormat::QuickTime};

/// The codecs a recording falls back to, best first.
constexpr QMediaFormat::VideoCodec kFallbackCodecs[] = {
    QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::AV1,
    QMediaFormat::VideoCodec::VP9,  QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::MPEG2,
};

std::optional<QMediaFormat> encodableFormat(QMediaFormat::FileFormat container, QMediaFormat::VideoCodec codec)
{
    QMediaFormat format(container);
    format.setVideoCodec(codec);
    return format.isSupported(QMediaFormat::Encode) ? std::optional(format) : std::nullopt;
}

void removeIfEmpty(const QString& file)
{
    const QFileInfo info(file);
    if (info.exists() && (info.size() == 0)) {
        (void) QFile::remove(file);
    }
}

}  // namespace

QtVideoRecorder::QtVideoRecorder(QObject* parent)
    : QObject(parent)
    , _recorder(new QMediaRecorder(this))
{
    (void) connect(_recorder, &QMediaRecorder::recorderStateChanged, this, &QtVideoRecorder::_onRecorderStateChanged);
    (void) connect(_recorder, &QMediaRecorder::errorOccurred, this,
                   [this](QMediaRecorder::Error, const QString& errorString) { _onRecorderError(errorString); });
    // The first encoded frame starts the file: Qt reports RecordingState before its encoder is set up.
    (void) connect(_recorder, &QMediaRecorder::durationChanged, this, [this](qint64 duration) {
        if (!_fileStarted && (duration > 0) && (_recorder->recorderState() == QMediaRecorder::RecordingState)) {
            _fileStarted = true;
            emit recordingStarted(_recordingFile);
        }
    });
}

QtVideoRecorder::~QtVideoRecorder() = default;

qint64 QtVideoRecorder::recordedMs() const
{
    return _fileStarted ? _recorder->duration() : -1;
}

qint64 QtVideoRecorder::recordedBytes() const
{
    return _fileStarted ? QFileInfo(_recordingFile).size() : -1;
}

QList<QMediaFormat> QtVideoRecorder::recordingFormats(QMediaFormat::FileFormat container,
                                                      QMediaFormat::VideoCodec codec)
{
    QList<QMediaFormat::FileFormat> containers{container};
    for (const QMediaFormat::FileFormat other : kRecordingContainers) {
        if (!containers.contains(other)) {
            containers.append(other);
        }
    }
    QList<QMediaFormat> formats;
    if (codec != QMediaFormat::VideoCodec::Unspecified) {
        for (const QMediaFormat::FileFormat candidate : std::as_const(containers)) {
            if (const std::optional<QMediaFormat> format = encodableFormat(candidate, codec)) {
                formats.append(*format);
                break;
            }
        }
    }
    for (const QMediaFormat::FileFormat candidate : std::as_const(containers)) {
        QList<QMediaFormat> fallbacks;
        for (const QMediaFormat::VideoCodec fallback : kFallbackCodecs) {
            if (const std::optional<QMediaFormat> format = encodableFormat(candidate, fallback)) {
                fallbacks.append(*format);
            }
        }
        if (!fallbacks.isEmpty()) {
            for (const QMediaFormat& format : std::as_const(fallbacks)) {
                if (!formats.contains(format)) {
                    formats.append(format);
                }
            }
            break;
        }
    }
    return formats;
}

QStringList QtVideoRecorder::recordingCodecs()
{
    QList<QMediaFormat::VideoCodec> codecs(std::begin(kFallbackCodecs), std::end(kFallbackCodecs));
    const QMetaEnum codecEnum = QMetaEnum::fromType<QMediaFormat::VideoCodec>();
    for (int i = 0; i < codecEnum.keyCount(); i++) {
        const auto codec = static_cast<QMediaFormat::VideoCodec>(codecEnum.value(i));
        if ((codec != QMediaFormat::VideoCodec::Unspecified) && !codecs.contains(codec)) {
            codecs.append(codec);
        }
    }

    QStringList keys;
    for (const QMediaFormat::VideoCodec codec : std::as_const(codecs)) {
        const bool encodable = std::ranges::any_of(kRecordingContainers, [codec](QMediaFormat::FileFormat container) {
            return encodableFormat(container, codec).has_value();
        });
        if (encodable) {
            keys.append(QString::fromLatin1(codecEnum.valueToKey(static_cast<int>(codec))));
        }
    }
    return keys;
}

QMediaFormat::VideoCodec QtVideoRecorder::recordingCodec(const QString& key)
{
    bool ok = false;
    const int value = QMetaEnum::fromType<QMediaFormat::VideoCodec>().keyToValue(key.toLatin1().constData(), &ok);
    return ok ? static_cast<QMediaFormat::VideoCodec>(value) : QMediaFormat::VideoCodec::Unspecified;
}

QString QtVideoRecorder::recordingCodecName(const QString& key)
{
    return QMediaFormat::videoCodecDescription(recordingCodec(key));
}

QMediaFormat::FileFormat QtVideoRecorder::container(VideoReceiver::FileFormat format)
{
    switch (format) {
        case VideoReceiver::FileFormat::Mkv:
            return QMediaFormat::Matroska;
        case VideoReceiver::FileFormat::Mov:
            return QMediaFormat::QuickTime;
        case VideoReceiver::FileFormat::Mp4:
            break;
    }
    return QMediaFormat::MPEG4;
}

QMediaRecorder::Quality QtVideoRecorder::recordingQuality(int value)
{
    const bool valid = (value >= QMediaRecorder::VeryLowQuality) && (value <= QMediaRecorder::VeryHighQuality);
    return valid ? static_cast<QMediaRecorder::Quality>(value) : QMediaRecorder::HighQuality;
}

void QtVideoRecorder::start(const QString& file, QMediaFormat::FileFormat container, QMediaFormat::VideoCodec codec,
                            QMediaRecorder::Quality quality)
{
    _pendingFormats = recordingFormats(container, codec);
    if (_pendingFormats.isEmpty()) {
        qCWarning(QtVideoRecorderLog) << "No video encoder:" << file;
        emit recordingFinished(file, tr("No video encoder is available"));
        return;
    }
    const QFileInfo info(file);
    _recordingBase = QStringLiteral("%1/%2").arg(info.path(), info.completeBaseName());
    _quality = quality;
    _stopRequested = false;
    _recordNextFormat();
}

void QtVideoRecorder::_recordNextFormat()
{
    const QMediaFormat format = _pendingFormats.takeFirst();
    _recordingFile = QStringLiteral("%1.%2").arg(_recordingBase, fileSuffix(format));
    _recordingError.clear();
    qCDebug(QtVideoRecorderLog) << "record" << _recordingFile << format.fileFormat() << format.videoCodec();
    _recorder->setMediaFormat(format);
    // Qt defect, seen in Qt 6.12.0 on Android: the recorder opens the encoder with a time base of one frame, then
    // stamps the frames in the muxer's (1/15360 s in MP4). MediaCodec takes them for seconds apart and spends several
    // times the bit rate Qt worked out from the quality: 45 Mbit/s rather than 6 for 1080p at High on a Snapdragon
    // 778G. No setting of the recorder corrects the times; a lower quality still makes a smaller file.
    _recorder->setQuality(_quality);
    // Tagged with the time it starts and the application, as GStreamer's recordings are (RecordingBranch).
    QMediaMetaData metaData;
    metaData.insert(QMediaMetaData::Date, QDateTime::currentDateTimeUtc());
    metaData.insert(QMediaMetaData::Comment, VideoReceiver::recordingComment());
    _recorder->setMetaData(metaData);
    _recorder->setOutputLocation(QUrl::fromLocalFile(_recordingFile));
    _recorder->record();
}

void QtVideoRecorder::_retryNextFormat()
{
    _retryQueued = false;
    if (!_stopRequested && !active() && !_pendingFormats.isEmpty()) {
        _recordNextFormat();
    }
    _endRecordingIfStopped();
}

void QtVideoRecorder::_setRecording(bool recording)
{
    if (recording != _recording) {
        _recording = recording;
        emit recordingChanged(_recording);
    }
}

void QtVideoRecorder::_endRecordingIfStopped()
{
    if (!active() && !_retryQueued) {
        _setRecording(false);
    }
}

void QtVideoRecorder::stop()
{
    _stopRequested = true;
    _recorder->stop();
}

void QtVideoRecorder::stopAndWait(std::chrono::milliseconds timeout)
{
    QEventLoop loop;
    QTimer::singleShot(timeout, &loop, &QEventLoop::quit);
    // After _onRecorderStateChanged(), connected first: the recording is reported, or its empty file removed, by then.
    (void) connect(_recorder, &QMediaRecorder::recorderStateChanged, &loop,
                   [&loop](QMediaRecorder::RecorderState state) {
                       if (state == QMediaRecorder::StoppedState) {
                           loop.quit();
                       }
                   });
    stop();
    if (active()) {
        (void) loop.exec();
    }
}

void QtVideoRecorder::_onRecorderStateChanged(QMediaRecorder::RecorderState state)
{
    if (state == QMediaRecorder::RecordingState) {
        _setRecording(true);
        return;
    }
    if (state != QMediaRecorder::StoppedState) {
        return;
    }
    if (_fileStarted) {
        _fileStarted = false;
        emit recordingFinished(_recordingFile, _recordingError);
    } else {
        _onRecordingNotStarted();
    }
    _endRecordingIfStopped();
}

void QtVideoRecorder::_onRecorderError(const QString& errorString)
{
    qCWarning(QtVideoRecorderLog) << "Recording error:" << _recordingFile << errorString;
    _recordingError = errorString;
    // A recorder that is still running reports the end when it stops.
    if (!_fileStarted && (_recorder->recorderState() == QMediaRecorder::StoppedState)) {
        _onRecordingNotStarted();
    }
}

void QtVideoRecorder::_onRecordingNotStarted()
{
    removeIfEmpty(_recordingFile);
    if (_recordingError.isEmpty() || _stopRequested) {
        return;
    }
    if (!_pendingFormats.isEmpty() && _recorder->captureSession()) {
        qCDebug(QtVideoRecorderLog) << "Encoder failed, trying the next format:" << _recordingError;
        // Not from the recorder's own signal.
        _retryQueued = true;
        QMetaObject::invokeMethod(this, &QtVideoRecorder::_retryNextFormat, Qt::QueuedConnection);
        return;
    }
    const QString error = std::exchange(_recordingError, QString());
    emit recordingFinished(_recordingFile, error);
}
