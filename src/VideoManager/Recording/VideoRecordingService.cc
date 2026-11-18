#include "VideoRecordingService.h"

#include <algorithm>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "SubtitleWriter.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"

QGC_LOGGING_CATEGORY(VideoRecordingServiceLog, "Video.VideoRecordingService")

namespace {

constexpr std::chrono::minutes kStorageLimitInterval{1};
// A file written this recently may be a recording's that has yet to report its first frame.
constexpr std::chrono::minutes kBeingWritten{1};

}  // namespace

VideoRecordingService::VideoRecordingService(const QList<VideoStreamController*>& sources, QObject* parent)
    : QObject(parent)
    , _subtitleWriter(new SubtitleWriter(this))
{
    for (VideoStreamController* source : sources) {
        addSource(source);
    }

    _storageLimitTimer.setInterval(kStorageLimitInterval);
    (void) connect(&_storageLimitTimer, &QTimer::timeout, this, &VideoRecordingService::_applyStorageLimit);
}

VideoRecordingService::~VideoRecordingService() = default;

void VideoRecordingService::addSource(VideoStreamController* source)
{
    _sources.append(source);
    _connectRecordings(source);
    // The telemetry goes with the video on display: the primary stream, or the camera in its place.
    if (source->role() != VideoStreamController::Role::Thermal) {
        _connectSubtitles(source);
    }
    (void) connect(source, &QObject::destroyed, this, [this, source] {
        (void) _sources.removeOne(source);
        _updateRecording();
    });
}

void VideoRecordingService::_connectSubtitles(VideoStreamController* source)
{
    // Each file, including a stream's parts after a reconnect, gets its own subtitle file.
    (void) connect(source, &VideoStreamController::recordingStarted, this, [this, source](const QString& file) {
        // The recording holds the video as it arrives, without the rotation its outputs add.
        const VideoReceiver* const receiver = source->receiver();
        _subtitleWriter->startCapturingTelemetry(file, receiver ? receiver->videoSize() : QSize());
    });
    (void) connect(source, &VideoStreamController::recordingFinished, this, [this](const QString& file) {
        if (file == _subtitleWriter->videoFile()) {
            _subtitleWriter->stopCapturingTelemetry();
        }
    });
    (void) connect(source, &VideoStreamController::recordingChanged, this, [this](bool recording) {
        if (!recording) {
            _subtitleWriter->stopCapturingTelemetry();
        }
    });
}

void VideoRecordingService::_connectRecordings(VideoStreamController* source)
{
    (void) connect(source, &VideoStreamController::recordingChanged, this, &VideoRecordingService::_updateRecording);
    (void) connect(source, &VideoStreamController::recordingStarted, this, [this](const QString& file) {
        qCDebug(VideoRecordingServiceLog) << "Recording to" << file;
        _recordingFiles.insert(file);
    });
    (void) connect(source, &VideoStreamController::recordingFinished, this,
                   [this](const QString& file, const QString& error) {
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
    const bool recording =
        std::ranges::any_of(_sources, [](const VideoStreamController* source) { return source->recording(); });
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
        files << file << SubtitleWriter::subtitleFile(file);
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

    QList<VideoStreamController*> sources;
    for (VideoStreamController* source : std::as_const(_sources)) {
        if (source->active() && !source->tile()) {
            sources.append(source);
        }
    }
    if (sources.isEmpty()) {
        QGC::showAppMessage(tr("Unable to record video: no video stream is active."));
        return;
    }

    _applyStorageLimit();

    const QString name = baseName.isEmpty() ? uniqueBaseName(directory, QDateTime::currentDateTime()) : baseName;
    for (VideoStreamController* source : std::as_const(sources)) {
        source->startRecording(recordingFile(directory, name, source, format), format);
    }
}

void VideoRecordingService::stopRecording()
{
    for (VideoStreamController* source : std::as_const(_sources)) {
        source->stopRecording();
    }
}

QString VideoRecordingService::recordingFile(const QString& directory, const QString& baseName,
                                             const VideoStreamController* source, VideoReceiver::FileFormat format)
{
    return source->taggedFile(
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
        const QString path = file.filePath();
        if (QFile::remove(path)) {
            total -= static_cast<quint64>(file.size());
            qCDebug(VideoRecordingServiceLog) << "Removed old recording" << path;
            removed.append(path);
        } else {
            qCWarning(VideoRecordingServiceLog) << "Failed to remove old recording" << path;
        }
    }
    return removed;
}
