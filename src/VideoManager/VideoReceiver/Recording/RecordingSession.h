#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtMultimedia/QMediaFormat>
#include <memory>
#include <vector>

Q_DECLARE_LOGGING_CATEGORY(RecordingSessionLog)

class VideoRecorder;

/// Orchestrates a multi-stream recording session with crash-recovery support.
///
/// On start(), writes a manifest.json atomically via QSaveFile. Each recorder
/// is started in sequence; on any failure, already-started recorders are stopped
/// and false is returned. On clean stop(), the manifest is deleted.
///
/// Crashed recordings (manifest present, stopTimestamp absent) are detected by
/// scanForOrphans(), which probes file playability and either renames or moves
/// files to a corrupted/ subdir.
class RecordingSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(QString manifestPath READ manifestPath NOTIFY manifestPathChanged)

public:
    explicit RecordingSession(QObject* parent = nullptr);
    ~RecordingSession() override;

    struct StreamEntry {
        QString role;                             ///< VideoStream::Role name (e.g. "videoContent")
        QString path;                             ///< Final video file path on disk
        std::unique_ptr<VideoRecorder> recorder;  ///< Owning; session destroys on stop/finalize.
        QMediaFormat::FileFormat format;

        StreamEntry() = default;
        StreamEntry(StreamEntry&&) noexcept = default;
        StreamEntry& operator=(StreamEntry&&) noexcept = default;
        // Non-copyable because unique_ptr is non-copyable.
        StreamEntry(const StreamEntry&) = delete;
        StreamEntry& operator=(const StreamEntry&) = delete;
    };

    /// Start a new recording session. Writes manifest, then starts each recorder.
    /// Takes ownership of all recorders in `streams`. Returns false if any
    /// recorder fails — started recorders are stopped and ownership is retained
    /// by the session (destroyed on next stop/teardown).
    /// Uses std::vector because StreamEntry is move-only (QList copies in some
    /// template paths, which would fail to compile).
    bool start(const QString& recordingDir, std::vector<StreamEntry>&& streams,
               const QString& subtitleFile = {}, const QString& vehicleUid = {});

    /// Stop all recorders, finalize manifest, then delete it on clean completion.
    void stop();

    [[nodiscard]] bool isActive() const { return !_manifestPath.isEmpty(); }
    [[nodiscard]] QString manifestPath() const { return _manifestPath; }

    /// Scan directory for orphaned session manifests (from crashed recordings).
    /// Returns count of orphaned manifests found.
    static int scanForOrphans(const QString& recordingDir, QObject* notifyParent = nullptr);

signals:
    void activeChanged();
    void manifestPathChanged();
    void started();
    void stopped();
    void error(const QString& message);
    void corruptionDetected(const QStringList& movedFiles);

private slots:
    void _onRecorderStopped(const QString& path);
    void _onRecorderError(const QString& message);

private:
    bool _writeManifest(bool includeStopTimestamp = false);
    void _finalizeAndCleanup();

    QString _manifestPath;
    QString _recordingDir;
    QString _subtitleFile;
    QString _vehicleUid;
    QString _startTimestampIso;
    QString _stopTimestampIso;
    std::vector<StreamEntry> _streams;
    int _activeRecorders = 0;
    QList<QMetaObject::Connection> _recorderConns;
};
