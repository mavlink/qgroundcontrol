#include "VideoRecordingService.h"

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

VideoRecordingService::VideoRecordingService(VideoStreamController* primaryStream, VideoStreamController* thermalStream,
                                             QObject* parent)
    : QObject(parent)
    , _primaryStream(primaryStream)
    , _thermalStream(thermalStream)
    , _subtitleWriter(new SubtitleWriter(this))
{
    _connectStream(_primaryStream);
    _connectStream(_thermalStream);

    // Each file of the primary stream, including the parts after a reconnect, gets its own subtitle file.
    (void) connect(_primaryStream, &VideoStreamController::recordingStarted, this, [this](const QString& file) {
        _subtitleWriter->stopCapturingTelemetry();
        _subtitleWriter->startCapturingTelemetry(file, _primaryStream->videoSize());
    });
    (void) connect(_primaryStream, &VideoStreamController::recordingChanged, this, [this](bool recording) {
        if (!recording) {
            _subtitleWriter->stopCapturingTelemetry();
        }
    });
}

VideoRecordingService::~VideoRecordingService() = default;

void VideoRecordingService::_connectStream(VideoStreamController* stream)
{
    (void) connect(stream, &VideoStreamController::recordingChanged, this, &VideoRecordingService::_updateRecording);
    (void) connect(stream, &VideoStreamController::recordingStarted, this, [this](const QString& file) {
        qCDebug(VideoRecordingServiceLog) << "Recording to" << file;
        emit recordingStarted(file);
    });
    (void) connect(stream, &VideoStreamController::recordingFinished, this,
                   [](const QString& file, const QString& error) {
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
    const bool recording = _primaryStream->recording() || _thermalStream->recording();
    if (recording != _recording) {
        _recording = recording;
        emit recordingChanged(_recording);
    }
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
    if (streams.isEmpty()) {
        QGC::showAppMessage(tr("Unable to record video: no video stream is active."));
        return;
    }

    if (videoSettings->enableStorageLimit()->rawValue().toBool()) {
        const quint64 maxBytes = videoSettings->maxVideoSize()->rawValue().toULongLong() * 1024 * 1024;
        (void) QtConcurrent::run(&VideoRecordingService::pruneStorage, directory, maxBytes);
    }

    const QString name = baseName.isEmpty() ? uniqueBaseName(directory, QDateTime::currentDateTime()) : baseName;
    for (VideoStreamController* stream : std::as_const(streams)) {
        stream->startRecording(recordingFile(directory, name, stream, format), format);
    }
}

void VideoRecordingService::stopRecording()
{
    _primaryStream->stopRecording();
    _thermalStream->stopRecording();
}

QString VideoRecordingService::recordingFile(const QString& directory, const QString& baseName,
                                             const VideoStreamController* stream, VideoReceiver::FileFormat format)
{
    const QString tag =
        (stream->role() == VideoStreamController::Role::Primary) ? QString() : (QLatin1Char('.') + stream->name());
    return QStringLiteral("%1/%2%3.%4").arg(directory, baseName, tag, VideoReceiver::fileExtension(format));
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

QStringList VideoRecordingService::pruneStorage(const QString& directory, quint64 maxBytes)
{
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
