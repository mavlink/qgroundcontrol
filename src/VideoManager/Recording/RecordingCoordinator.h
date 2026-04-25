#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtMultimedia/QMediaFormat>

#include <memory>

class QVideoSink;
class RecordingSession;
class SubtitleWriter;
class Vehicle;
class VideoSettings;
class VideoStream;
template <typename T> class QList;

Q_DECLARE_LOGGING_CATEGORY(RecordingCoordinatorLog)

/// Owns recording + image-capture concerns extracted from VideoManager:
/// `RecordingSession` lifecycle, `SubtitleWriter` telemetry capture, image
/// grab, and orphan-session recovery. VideoManager retains aggregate
/// stream-level state and forwards the user-facing verbs here.
///
/// Lifetime: owned by VideoManager; all internal state is torn down when the
/// coordinator is destroyed. `RecordingSession` is `std::unique_ptr`-owned;
/// `SubtitleWriter` is a QObject child.
class RecordingCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit RecordingCoordinator(QObject* parent = nullptr);
    ~RecordingCoordinator() override;

    [[nodiscard]] QString imageFile() const { return _imageFile; }

    /// Builds per-stream recording paths, transfers recorders off each
    /// `recordable` stream into a fresh `RecordingSession`, starts the session,
    /// and begins subtitle telemetry capture against `videoSize`.
    ///
    /// `videoFile` may be empty — `VideoFileNaming::buildRecordingTemplate`
    /// will synthesize a timestamped base name. Returns false when validation
    /// fails or no stream yielded a recorder.
    bool startRecording(const QString& videoFile,
                        const QList<VideoStream*>& recordable,
                        const Vehicle* activeVehicle,
                        QSize videoSize,
                        QMediaFormat::FileFormat fileFormat,
                        const QString& savePath);

    /// User-facing recording entry point. Owns the settings-derived policy:
    /// file-format validation, save-path validation, and storage pruning before
    /// constructing a RecordingSession.
    bool startRecordingFromSettings(const QString& videoFile,
                                    const QList<VideoStream*>& recordable,
                                    const Vehicle* activeVehicle,
                                    QSize videoSize,
                                    VideoSettings* videoSettings,
                                    const QString& savePath);

    void stopRecording();

    /// Async grab via the primary stream frame delivery. Falls back to emitting
    /// `imageFileChanged` with the target path so QML's `grabToImage` can
    /// take over when no frame delivery is available.
    void grabImage(const QString& imageFile,
                   VideoStream* primaryStream,
                   const QString& photoSavePath);

    /// Stops subtitle telemetry. Called by VideoManager when aggregate
    /// stream-level recording flips to false.
    void stopSubtitleTelemetry();

    /// Updates the live-sink binding on the subtitle writer. Called when the
    /// primary stream frame delivery is recreated (receiver rebuild / stream swap).
    void setLiveSubtitleSink(QVideoSink* sink);

    /// Schedules orphan-session recovery for the next event-loop iteration.
    /// Deferred to avoid blocking startup on the 3-second QMediaPlayer probe
    /// per orphan.
    void scheduleOrphanScan(const QString& moviesDir);

signals:
    /// Re-emit of `RecordingSession::started`.
    void sessionStarted();

    /// Re-emit of `RecordingSession::stopped`.
    void sessionStopped();

    void recordingStarted(const QString& filename);
    void imageFileChanged(const QString& filename);

private:
    SubtitleWriter* _subtitleWriter = nullptr;
    std::unique_ptr<RecordingSession> _recordingSession;
    QString _imageFile;
};
