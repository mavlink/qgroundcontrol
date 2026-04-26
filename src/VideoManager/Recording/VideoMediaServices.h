#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QString>

class RecordingCoordinator;
class Vehicle;
class VideoSettings;
class VideoStream;
class VideoStreamOrchestrator;

Q_DECLARE_LOGGING_CATEGORY(VideoMediaServicesLog)

/// Owns VideoManager's media side effects: recording/capture coordination,
/// subtitle sink routing, recording error reporting, and orphan-session scans.
class VideoMediaServices : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoMediaServices)

public:
    explicit VideoMediaServices(QObject* parent = nullptr);
    ~VideoMediaServices() override;

    [[nodiscard]] QString imageFile() const;

    void bindStreamOrchestrator(VideoStreamOrchestrator* orchestrator);

    bool startRecording(const QString& videoFile,
                        VideoStreamOrchestrator* orchestrator,
                        const Vehicle* activeVehicle,
                        QSize videoSize,
                        VideoSettings* videoSettings,
                        const QString& savePath);
    void stopRecording();

    void grabImage(const QString& imageFile,
                   VideoStream* primaryStream,
                   const QString& photoSavePath);

    void scheduleOrphanScan(const QString& moviesDir);

signals:
    void recordingStarted(const QString& filename);
    void imageFileChanged(const QString& filename);

private:
    void _disconnectOrchestrator();
    void _refreshLiveSubtitleSink();

    RecordingCoordinator* _recordingCoordinator = nullptr;
    QPointer<VideoStreamOrchestrator> _streamOrchestrator;
    QList<QMetaObject::Connection> _orchestratorConnections;
};
