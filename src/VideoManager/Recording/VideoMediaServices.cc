#include "VideoMediaServices.h"

#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "RecordingCoordinator.h"
#include "VideoFrameDelivery.h"
#include "VideoStream.h"
#include "VideoStreamOrchestrator.h"

QGC_LOGGING_CATEGORY(VideoMediaServicesLog, "Video.VideoMediaServices")

VideoMediaServices::VideoMediaServices(QObject* parent)
    : QObject(parent)
    , _recordingCoordinator(new RecordingCoordinator(this))
{
    connect(_recordingCoordinator, &RecordingCoordinator::recordingStarted,
            this, &VideoMediaServices::recordingStarted);
    connect(_recordingCoordinator, &RecordingCoordinator::imageFileChanged,
            this, &VideoMediaServices::imageFileChanged);
}

VideoMediaServices::~VideoMediaServices()
{
    _disconnectOrchestrator();
}

QString VideoMediaServices::imageFile() const
{
    return _recordingCoordinator ? _recordingCoordinator->imageFile() : QString();
}

void VideoMediaServices::bindStreamOrchestrator(VideoStreamOrchestrator* orchestrator)
{
    if (_streamOrchestrator == orchestrator)
        return;

    _disconnectOrchestrator();
    _streamOrchestrator = orchestrator;

    if (!_streamOrchestrator)
        return;

    _orchestratorConnections.append(connect(_streamOrchestrator, &VideoStreamOrchestrator::recordingChanged,
                                            this, [this](bool recording) {
                                                if (!recording && _recordingCoordinator)
                                                    _recordingCoordinator->stopSubtitleTelemetry();
                                            }));
    _orchestratorConnections.append(connect(_streamOrchestrator, &VideoStreamOrchestrator::primaryFrameDeliveryChanged,
                                            this, &VideoMediaServices::_refreshLiveSubtitleSink));
    _orchestratorConnections.append(connect(_streamOrchestrator, &VideoStreamOrchestrator::recordingError,
                                            this, [](const QString& msg) {
                                                qCWarning(VideoMediaServicesLog) << "Recording error:" << msg;
                                                qgcApp()->showAppMessage(
                                                    QObject::tr("Video recording error: %1").arg(msg));
                                            }));

    _refreshLiveSubtitleSink();
}

bool VideoMediaServices::startRecording(const QString& videoFile,
                                        VideoStreamOrchestrator* orchestrator,
                                        const Vehicle* activeVehicle,
                                        QSize videoSize,
                                        VideoSettings* videoSettings,
                                        const QString& savePath)
{
    if (!_recordingCoordinator || !orchestrator)
        return false;

    QList<VideoStream*> recordable;
    orchestrator->forEachRecordableStream([&recordable](VideoStream* stream) {
        recordable.append(stream);
    });

    return _recordingCoordinator->startRecordingFromSettings(
        videoFile,
        recordable,
        activeVehicle,
        videoSize,
        videoSettings,
        savePath);
}

void VideoMediaServices::stopRecording()
{
    if (_recordingCoordinator)
        _recordingCoordinator->stopRecording();
}

void VideoMediaServices::grabImage(const QString& imageFile,
                                   VideoStream* primaryStream,
                                   const QString& photoSavePath)
{
    if (_recordingCoordinator)
        _recordingCoordinator->grabImage(imageFile, primaryStream, photoSavePath);
}

void VideoMediaServices::scheduleOrphanScan(const QString& moviesDir)
{
    if (_recordingCoordinator)
        _recordingCoordinator->scheduleOrphanScan(moviesDir);
}

void VideoMediaServices::_disconnectOrchestrator()
{
    for (const QMetaObject::Connection& connection : _orchestratorConnections)
        disconnect(connection);
    _orchestratorConnections.clear();
    _streamOrchestrator.clear();
}

void VideoMediaServices::_refreshLiveSubtitleSink()
{
    if (!_recordingCoordinator)
        return;

    auto* primary = _streamOrchestrator ? _streamOrchestrator->primaryStream() : nullptr;
    auto* delivery = primary ? primary->frameDelivery() : nullptr;
    _recordingCoordinator->setLiveSubtitleSink(delivery ? delivery->videoSink() : nullptr);
}
