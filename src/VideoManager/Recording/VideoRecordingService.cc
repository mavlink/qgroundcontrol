#include "VideoRecordingService.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QtVideoRecorder.h"
#include "SettingsManager.h"
#include "SubtitleWriter.h"
#include "UVCCameraController.h"
#include "UVCStreamMonitor.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"

QGC_LOGGING_CATEGORY(VideoRecordingServiceLog, "Video.VideoRecordingService")

namespace {

constexpr std::chrono::minutes kStorageLimitInterval{1};
// A file written this recently may be a recording's that has yet to report its first frame.
constexpr std::chrono::minutes kBeingWritten{1};

QString subtitlePathFor(const QString& videoFile)
{
    const QFileInfo info(videoFile);
    return QStringLiteral("%1/%2.ass").arg(info.path(), info.completeBaseName());
}

}  // namespace

VideoRecordingService::VideoRecordingService(VideoStreamController* primaryStream, VideoStreamController* thermalStream,
                                             UVCCameraController* uvcCamera, QObject* parent)
    : QObject(parent)
    , _primaryStream(primaryStream)
    , _thermalStream(thermalStream)
    , _uvcCamera(uvcCamera)
    , _subtitleWriter(new SubtitleWriter(this))
{
    _connectRecordings(_primaryStream);
    _connectRecordings(_thermalStream);
    _connectRecordings(_uvcCamera);

    _connectSubtitles(_primaryStream, [this] { return _primaryStream->videoSize(); });
    _connectSubtitles(_uvcCamera, [this] { return _uvcCamera->monitor()->videoSize(); });

    _storageLimitTimer.setInterval(kStorageLimitInterval);
    (void) connect(&_storageLimitTimer, &QTimer::timeout, this, &VideoRecordingService::_applyStorageLimit);
}

VideoRecordingService::~VideoRecordingService() = default;

template <class Source, class VideoSize>
void VideoRecordingService::_connectSubtitles(Source* source, VideoSize videoSize)
{
    // Each file, including a stream's parts after a reconnect, gets its own subtitle file.
    (void) connect(source, &Source::recordingStarted, this, [this, videoSize](const QString& file) {
        _subtitleWriter->startCapturingTelemetry(file, videoSize());
    });
    (void) connect(source, &Source::recordingFinished, this, [this](const QString& file) {
        if (file == _subtitleWriter->videoFile()) {
            _subtitleWriter->stopCapturingTelemetry();
        }
    });
    (void) connect(source, &Source::recordingChanged, this, [this](bool recording) {
        if (!recording) {
            _subtitleWriter->stopCapturingTelemetry();
        }
    });
}

template <class Source>
void VideoRecordingService::_connectRecordings(Source* source)
{
    (void) connect(source, &Source::recordingChanged, this, &VideoRecordingService::_updateRecording);
    (void) connect(source, &Source::recordingStarted, this, [this](const QString& file) {
        qCDebug(VideoRecordingServiceLog) << "Recording to" << file;
        _recordingFiles.insert(file);
    });
    (void) connect(source, &Source::recordingFinished, this, [this](const QString& file, const QString& error) {
        _recordingFiles.remove(file);
        if (error.isEmpty()) {
            qCDebug(VideoRecordingServiceLog) << "Recorded" << file;
            return;
        }
        qCWarning(VideoRecordingServiceLog) << "Recording" << file << "failed:" << error;
        QGC::showAppMessage(tr("Video recording %1 failed: %2").arg(QFileInfo(file).fileName(), error));
    });
}

void VideoRecordingService::_updateRecording()
{
    const bool recording = _primaryStream->recording() || _thermalStream->recording() || _uvcCamera->recording();
    if (recording != _recording) {
        _recording = recording;
        if (_recording) {
            _storageLimitTimer.start();
        } else {
            _storageLimitTimer.stop();
        }
        emit recordingChanged(_recording);
    }
}

void VideoRecordingService::_applyStorageLimit()
{
    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    const QString directory = SettingsManager::instance()->appSettings()->videoSavePath();
    if (directory.isEmpty() || !videoSettings->enableStorageLimit()->rawValue().toBool()) {
        return;
    }
    const quint64 maxBytes = videoSettings->maxVideoSize()->rawValue().toULongLong() * 1024 * 1024;
    (void) QtConcurrent::run(&VideoRecordingService::pruneStorage, directory, maxBytes, _openFiles());
}

QStringList VideoRecordingService::_openFiles() const
{
    QStringList files;
    for (const QString& file : _recordingFiles) {
        files << file << subtitlePathFor(file);
    }
    return files;
}

void VideoRecordingService::startRecording(const QString& baseName)
{
    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    const int formatValue = videoSettings->recordingFormat()->rawValue().toInt();
    if (!VideoReceiver::isValidFileFormat(formatValue)) {
        QGC::showAppMessage(tr("Invalid video format defined."));
        return;
    }
    const auto format = static_cast<VideoReceiver::FileFormat>(formatValue);

    const QString directory = SettingsManager::instance()->appSettings()->videoSavePath();
    if (directory.isEmpty()) {
        QGC::showAppMessage(tr("Unable to record video. Video save path must be specified in Settings."));
        return;
    }

    QList<VideoStreamController*> streams;
    for (VideoStreamController* stream : {_primaryStream, _thermalStream}) {
        if (stream->active()) {
            streams.append(stream);
        }
    }
    const bool recordUvc = _uvcCamera->canRecord() && !_uvcCamera->recordingActive();
    if (streams.isEmpty() && !recordUvc) {
        QGC::showAppMessage(tr("Unable to record video: no video stream is active."));
        return;
    }

    _applyStorageLimit();

    const QString name = baseName.isEmpty() ? uniqueBaseName(directory, QDateTime::currentDateTime()) : baseName;
    for (VideoStreamController* stream : std::as_const(streams)) {
        stream->startRecording(recordingFile(directory, name, stream, format), format);
    }
    if (recordUvc) {
        _uvcCamera->startRecording(
            QStringLiteral("%1/%2.%3").arg(directory, name, VideoReceiver::fileExtension(format)),
            QtVideoRecorder::container(format),
            QtVideoRecorder::recordingCodec(videoSettings->uvcRecordingCodec()->rawValue().toString()),
            QtVideoRecorder::recordingQuality(videoSettings->uvcRecordingQuality()->rawValue().toInt()));
    }
}

void VideoRecordingService::stopRecording()
{
    _primaryStream->stopRecording();
    _thermalStream->stopRecording();
    _uvcCamera->stopRecording();
}

QString VideoRecordingService::recordingFile(const QString& directory, const QString& baseName,
                                             const VideoStreamController* stream, VideoReceiver::FileFormat format)
{
    return stream->taggedFile(
        QStringLiteral("%1/%2.%3").arg(directory, baseName, VideoReceiver::fileExtension(format)));
}

QString VideoRecordingService::uniqueBaseName(const QString& directory, const QDateTime& time)
{
    const QString timestamp = time.toString(QStringLiteral("yyyy-MM-dd_hh.mm.ss"));
    const QDir dir(directory);
    QString baseName = timestamp;
    for (int counter = 2; !dir.entryList({baseName + QStringLiteral(".*")}, QDir::Files).isEmpty(); counter++) {
        baseName = QStringLiteral("%1_%2").arg(timestamp).arg(counter);
    }
    return baseName;
}

QStringList VideoRecordingService::pruneStorage(const QString& directory, quint64 maxBytes,
                                                const QStringList& openFiles)
{
    QSet<QString> open;
    for (const QString& file : openFiles) {
        open.insert(QFileInfo(file).absoluteFilePath());
    }
    const QDateTime beingWrittenSince = QDateTime::currentDateTime().addDuration(-kBeingWritten);

    QStringList nameFilters{QStringLiteral("*.ass")};
    for (const VideoReceiver::FileFormat format :
         {VideoReceiver::FileFormat::Mkv, VideoReceiver::FileFormat::Mov, VideoReceiver::FileFormat::Mp4}) {
        nameFilters.append(QStringLiteral("*.") + VideoReceiver::fileExtension(format));
    }

    QDir dir(directory);
    const QFileInfoList files = dir.entryInfoList(
        nameFilters, QDir::Files | QDir::Readable | QDir::Writable | QDir::NoSymLinks, QDir::Time | QDir::Reversed);

    quint64 total = 0;
    for (const QFileInfo& file : files) {
        total += static_cast<quint64>(file.size());
    }

    QStringList removed;
    for (const QFileInfo& file : files) {
        if (total < maxBytes) {
            break;
        }
        if (open.contains(file.absoluteFilePath()) || (file.lastModified() > beingWrittenSince)) {
            continue;
        }
        total -= static_cast<quint64>(file.size());
        const QString path = file.filePath();
        if (QFile::remove(path)) {
            qCDebug(VideoRecordingServiceLog) << "Removed old recording" << path;
            removed.append(path);
        } else {
            qCWarning(VideoRecordingServiceLog) << "Failed to remove old recording" << path;
        }
    }
    return removed;
}
