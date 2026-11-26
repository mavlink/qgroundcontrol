#include "VideoRecordingService.h"

#include <algorithm>
#include <utility>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFutureWatcher>
#include <QtCore/QStorageInfo>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QtVideoRecorder.h"
#include "SettingsManager.h"
#include "SubtitleWriter.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"

QGC_LOGGING_CATEGORY(VideoRecordingServiceLog, "Video.VideoRecordingService")

namespace {

constexpr std::chrono::minutes kStorageLimitInterval{1};
// A file written this recently may be a recording's that has yet to report its first frame.
constexpr std::chrono::minutes kBeingWritten{1};
// VideoRecordingService::kFreeSpaceFloor counts on it.
constexpr std::chrono::seconds kFreeSpaceInterval{10};
constexpr qint64 kFreeSpaceFloorMB = VideoRecordingService::kFreeSpaceFloor / (1024 * 1024);

/// The bytes free on the volume of @p directory; -1 when that isn't known: no volume is found for it, or the volume
/// tells no size of its own, as some network and virtual file systems do, which report nothing free either.
qint64 volumeFreeSpace(const QString& directory)
{
    const qint64 available = QGCFileHelper::availableDiskSpace(directory);
    return (QStorageInfo(directory).bytesTotal() > 0) ? available : -1;
}

}  // namespace

VideoRecordingService::VideoRecordingService(const QList<VideoStreamController*>& sources, QObject* parent)
    : QObject(parent)
    , _subtitleWriter(new SubtitleWriter(this))
    , _freeSpace(&volumeFreeSpace)
{
    for (VideoStreamController* source : sources) {
        addSource(source);
    }

    _storageLimitTimer.setInterval(kStorageLimitInterval);
    (void) connect(&_storageLimitTimer, &QTimer::timeout, this, &VideoRecordingService::_applyStorageLimit);
    _freeSpaceTimer.setInterval(kFreeSpaceInterval);
    (void) connect(&_freeSpaceTimer, &QTimer::timeout, this, &VideoRecordingService::_checkFreeSpace);
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
            _freeSpaceTimer.start();
        } else {
            _storageLimitTimer.stop();
            _freeSpaceTimer.stop();
        }
        emit recordingChanged(_recording);
    }
}

std::optional<QFuture<QStringList>> VideoRecordingService::_applyStorageLimit()
{
    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    const QString directory = SettingsManager::instance()->appSettings()->videoSavePath();
    if (directory.isEmpty() || !videoSettings->enableStorageLimit()->rawValue().toBool()) {
        return std::nullopt;
    }
    const quint64 maxBytes = videoSettings->maxVideoSize()->rawValue().toULongLong() * 1024 * 1024;
    return QtConcurrent::run(&VideoRecordingService::pruneStorage, directory, maxBytes, _openFiles());
}

bool VideoRecordingService::_freeSpaceShort(const QString& directory) const
{
    const qint64 freeSpace = _freeSpace(directory);
    return (freeSpace >= 0) && (freeSpace < kFreeSpaceFloor);
}

void VideoRecordingService::_checkFreeSpace()
{
    const QString directory = SettingsManager::instance()->appSettings()->videoSavePath();
    if (!_freeSpaceShort(directory)) {
        return;
    }
    // Told once: the sources take a moment to finish their files, and say so then.
    _freeSpaceTimer.stop();
    qCWarning(VideoRecordingServiceLog) << "Stopping the recording: too little free space in" << directory;
    stopRecording();
    QGC::showAppMessage(
        tr("Video recording stopped: less than %1 MB free in %2").arg(kFreeSpaceFloorMB).arg(directory));
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
    if (_pendingStart) {
        qCWarning(VideoRecordingServiceLog) << "Not starting a recording: one waits for the storage limit";
        return;
    }
    _startRecording(baseName, /*pruned=*/false);
}

void VideoRecordingService::_startRecording(const QString& baseName, bool pruned)
{
    if (_recording) {
        // Left to the receivers, one would go on in the new file and another keep to its own.
        qCWarning(VideoRecordingServiceLog) << "Not starting a recording: one is running";
        return;
    }

    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    const int formatValue = videoSettings->recordingFormat()->rawValue().toInt();
    if (!VideoReceiver::isValidFileFormat(formatValue)) {
        QGC::showAppMessage(tr("Invalid video format defined."));
        return;
    }
    const auto format = static_cast<VideoReceiver::FileFormat>(formatValue);
    // For the sources that encode their recordings anew: the USB camera's and Qt Multimedia's.
    const VideoReceiver::RecordingEncoding encoding{
        .codec = QtVideoRecorder::recordingCodec(videoSettings->uvcRecordingCodec()->rawValue().toString()),
        .quality = QtVideoRecorder::recordingQuality(videoSettings->uvcRecordingQuality()->rawValue().toInt())};

    const QString directory = SettingsManager::instance()->appSettings()->videoSavePath();
    if (directory.isEmpty()) {
        QGC::showAppMessage(tr("Unable to record video. Video save path must be specified in Settings."));
        return;
    }

    QList<VideoStreamController*> sources;
    for (VideoStreamController* source : std::as_const(_sources)) {
        if (source->canRecord() && !source->tile()) {
            sources.append(source);
        }
    }
    if (sources.isEmpty()) {
        QGC::showAppMessage(tr("Unable to record video: no video stream is active."));
        return;
    }

    // The storage limit makes room first. Pruning only adds to the free space, so a start with enough of it goes on
    // while the pruning runs; one short of it waits for the pruning, off this thread, and looks again.
    const std::optional<QFuture<QStringList>> pruning = pruned ? std::nullopt : _applyStorageLimit();
    if (_freeSpaceShort(directory)) {
        if (pruning) {
            _pendingStart = baseName;
            // A watcher tells from the event loop, of a pruning that is done by now as well.
            auto* const watcher = new QFutureWatcher<QStringList>(this);
            (void) connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
                watcher->deleteLater();
                if (const std::optional<QString> pending = std::exchange(_pendingStart, std::nullopt)) {
                    _startRecording(*pending, /*pruned=*/true);
                }
            });
            watcher->setFuture(*pruning);
            return;
        }
        qCWarning(VideoRecordingServiceLog) << "Not starting a recording: too little free space in" << directory;
        QGC::showAppMessage(
            tr("Unable to record video: less than %1 MB free in %2").arg(kFreeSpaceFloorMB).arg(directory));
        return;
    }

    const QString name = baseName.isEmpty() ? uniqueBaseName(directory, QDateTime::currentDateTime()) : baseName;
    for (VideoStreamController* source : std::as_const(sources)) {
        source->startRecording(recordingFile(directory, name, source, format), format, encoding);
    }
}

void VideoRecordingService::stopRecording()
{
    _pendingStart.reset();
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
