#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtMultimedia/QMediaFormat>

Q_DECLARE_LOGGING_CATEGORY(VideoRecorderLog)

/// Abstract interface for video recording implementations.
///
/// The default implementation feeds QVideoFrames from VideoFrameDelivery into
/// QMediaRecorder. It is transcoded and works for any display receiver type,
/// including streams that used GStreamer only as transport ingest.
///
/// VideoStream owns one VideoRecorder instance created by VideoRecordingPolicy.
/// RecordingCoordinator takes ownership via VideoStream::releaseRecorder() and
/// drives recorders through RecordingSession.
class VideoRecorder : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoRecorder)

public:
    /// Runtime-queried recording capabilities — determined by the stream and platform,
    /// not hard-coded per receiver type (e.g. mux availability, encoder support).
    struct Capabilities
    {
        bool lossless = false;  ///< true = tee (no re-encode); false = transcoded
        QList<QMediaFormat::FileFormat> formats;  ///< supported container formats
        QString description;  ///< human-readable capability description
    };

    /// Lifecycle state — used internally and exposed for observability.
    enum class State : uint8_t
    {
        Idle,      ///< Not recording
        Starting,  ///< Recording requested, waiting for first keyframe / encoder init
        Recording, ///< Actively recording
        Stopping,  ///< Stop requested, waiting for muxer finalization
    };
    Q_ENUM(State)

    explicit VideoRecorder(QObject* parent = nullptr);
    ~VideoRecorder() override = default;

    /// Start recording to the given absolute path with the specified container format.
    /// The format must be in capabilities().formats.
    /// Returns false if already recording or if preconditions fail (no stream, no sink).
    virtual bool start(const QString& path, QMediaFormat::FileFormat format) = 0;

    /// Stop recording. Idempotent — safe to call when not recording.
    /// Emits stopped() when the file is finalized.
    virtual void stop() = 0;

    /// Runtime-determined capabilities. Implementations may inspect pipeline caps
    /// or Qt's supported-format list rather than returning compile-time constants.
    [[nodiscard]] virtual Capabilities capabilities() const = 0;

    [[nodiscard]] bool isRecording() const { return _state == State::Recording || _state == State::Starting; }

    [[nodiscard]] State state() const { return _state; }

    [[nodiscard]] QString currentPath() const { return _currentPath; }

signals:
    /// Emitted once recording is active.
    void started(const QString& path);

    /// Emitted when the file is fully finalized (mux trailer written). path is the
    /// final on-disk path (may differ from requested if a fallback was applied).
    void stopped(const QString& path);

    /// Emitted on any recording error (non-fatal: recording is stopped as a result).
    void error(const QString& message);

    /// Emitted on every state transition.
    void stateChanged(State newState);

protected:
    void setState(State s);

    QString _currentPath;
    State _state = State::Idle;
};
