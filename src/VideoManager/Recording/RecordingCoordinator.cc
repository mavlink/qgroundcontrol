#include "RecordingCoordinator.h"

#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "RecordingSession.h"
#include "SubtitleWriter.h"
#include "Vehicle.h"
#include "VideoFileNaming.h"
#include "VideoFrameDelivery.h"
#include "VideoRecorder.h"
#include "VideoStream.h"

#include <QtCore/QList>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(RecordingCoordinatorLog, "Video.RecordingCoordinator")

RecordingCoordinator::RecordingCoordinator(QObject* parent)
    : QObject(parent),
      _subtitleWriter(new SubtitleWriter(this)),
      _recordingSession(std::make_unique<RecordingSession>(this))
{
    (void)connect(_recordingSession.get(), &RecordingSession::started, this,
                  [this]() { emit sessionStarted(); });
    (void)connect(_recordingSession.get(), &RecordingSession::stopped, this, [this]() {
        _subtitleWriter->stopCapturingTelemetry();
        emit sessionStopped();
    });
    (void)connect(_recordingSession.get(), &RecordingSession::error, this, [](const QString& msg) {
        qCWarning(RecordingCoordinatorLog) << "RecordingSession error:" << msg;
        qgcApp()->showAppMessage(QObject::tr("Video recording error: %1").arg(msg));
    });
}

RecordingCoordinator::~RecordingCoordinator() = default;

bool RecordingCoordinator::startRecording(const QString& videoFile,
                                          const QList<VideoStream*>& recordable,
                                          const Vehicle* activeVehicle,
                                          QSize videoSize,
                                          QMediaFormat::FileFormat fileFormat,
                                          const QString& savePath)
{
    const QString videoFileNameTemplate =
        VideoFileNaming::buildRecordingTemplate(savePath, videoFile, fileFormat);

    // releaseRecorder() transfers ownership to the session entry and triggers
    // lazy recreation so the stream's recorder() stays valid afterwards.
    std::vector<RecordingSession::StreamEntry> entries;
    for (VideoStream* stream : recordable) {
        const QString streamName = (stream->role() == VideoStream::Role::Primary)
                                       ? QString()
                                       : (stream->name() + QStringLiteral("."));
        const QString path = videoFileNameTemplate.arg(streamName);

        if (!stream->recorder())
            continue;

        RecordingSession::StreamEntry entry;
        entry.role     = VideoStream::nameForRole(stream->role());
        entry.path     = path;
        entry.recorder = stream->releaseRecorder();
        entry.format   = fileFormat;
        if (!entry.recorder)
            continue;
        entries.push_back(std::move(entry));
    }

    if (entries.empty()) {
        qCWarning(RecordingCoordinatorLog) << "startRecording: no recordable streams";
        return false;
    }

    // Snapshot BEFORE the move — `entries` is left moved-from (empty).
    const QString primaryPath = entries.front().path;

    const QString subtitleFile = VideoFileNaming::subtitleSiblingPath(primaryPath);

    const QString vehicleUid = activeVehicle
        ? QString::number(activeVehicle->id())
        : QString{};

    if (!_recordingSession->start(savePath, std::move(entries), subtitleFile, vehicleUid)) {
        qCWarning(RecordingCoordinatorLog) << "RecordingSession::start() failed";
        return false;
    }

    if (!primaryPath.isEmpty())
        _subtitleWriter->startCapturingTelemetry(primaryPath, videoSize);

    emit recordingStarted(primaryPath);
    return true;
}

void RecordingCoordinator::stopRecording()
{
    _recordingSession->stop();
}

void RecordingCoordinator::grabImage(const QString& imageFile,
                                     VideoStream* primaryStream,
                                     const QString& photoSavePath)
{
    if (imageFile.isEmpty())
        _imageFile = VideoFileNaming::buildImageGrabPath(photoSavePath);
    else
        _imageFile = imageFile;

    // Async grab via the primary bridge; on empty result the signal-driven
    // QML grabToImage fallback below handles the save.
    if (primaryStream && primaryStream->bridge()) {
        const QString target = _imageFile;
        primaryStream->bridge()->grabFrame().then(this, [this, target](const QImage& image) {
            if (!image.isNull() && image.save(target)) {
                qCDebug(RecordingCoordinatorLog) << "Grabbed frame to" << target;
                emit imageFileChanged(target);
                return;
            }
            qCDebug(RecordingCoordinatorLog) << "Bridge grab unavailable, falling back to QML grabToImage";
            emit imageFileChanged(target);
        });
        return;
    }

    qCDebug(RecordingCoordinatorLog) << "Bridge grab unavailable, falling back to QML grabToImage";
    emit imageFileChanged(_imageFile);
}

void RecordingCoordinator::stopSubtitleTelemetry()
{
    _subtitleWriter->stopCapturingTelemetry();
}

void RecordingCoordinator::setLiveSubtitleSink(QVideoSink* sink)
{
    _subtitleWriter->setLiveVideoSink(sink);
}

void RecordingCoordinator::scheduleOrphanScan(const QString& moviesDir)
{
    if (moviesDir.isEmpty())
        return;

    // `this` is captured by-value through Qt's context-object guard on
    // singleShot — the lambda is dropped if the coordinator is destroyed
    // before the event loop runs it.
    QTimer::singleShot(0, this, [this, moviesDir]() {
        if (!_recordingSession)
            return;
        const int orphans = RecordingSession::scanForOrphans(moviesDir, _recordingSession.get());
        if (orphans > 0)
            qCInfo(RecordingCoordinatorLog) << "Found" << orphans
                                            << "orphaned recording session(s) in" << moviesDir;
    });
}
