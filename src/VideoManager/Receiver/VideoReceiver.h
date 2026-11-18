#pragma once

#include <chrono>
#include <cstdint>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaRecorder>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoScreenshot.h"
#include "VideoStreamStats.h"

class QQuickItem;
class QVideoFrame;
class QVideoSink;

/// One video source: a network stream, which a backend's receiver connects to at the config's URI, or a local camera
/// (UVCCameraController), which the URI names. Shows it in a QML VideoOutput, records it and takes its screenshots.
///
/// Intent-based: callers state what they want (setConfig(), setActive(), setVideoOutput(), startRecording()) and the
/// receiver gets there on its own, restarting after a config change and reconnecting after a failure. Call every
/// method on the GUI thread; every signal is emitted there.
///
/// Backends implement the protected on*() hooks, which run after the base stored the new value, and publish their
/// state through the protected setters.
class VideoReceiver : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged FINAL)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged FINAL)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged FINAL)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged FINAL)
    Q_PROPERTY(VideoStreamStats stats READ stats NOTIFY statsChanged FINAL)

public:
    enum class State : uint8_t
    {
        Stopped,       ///< Inactive, no URI, or given up after a failure (autoReconnect off).
        Connecting,    ///< Pipeline running, no data yet.
        Streaming,     ///< Data is arriving.
        Reconnecting,  ///< Waiting out the backoff after a failure.
    };
    Q_ENUM(State)

    /// Values are persisted by VideoSettings::recordingFormat.
    enum class FileFormat : uint8_t
    {
        Mkv = 0,
        Mov = 1,
        Mp4 = 2,
    };
    Q_ENUM(FileFormat)

    struct Config
    {
        QString uri;
        /// No data, or no decoded frame while displaying, for this long is a stream failure. Not before the first data
        /// of a stream the receiver listens for on a UDP port, which may start at any time.
        std::chrono::seconds timeout{3};
        bool lowLatency = false;
        int rtpJitterLatencyMs = 80;
        /// Reconnect with backoff after a failure. Applies at once; changing any other field restarts the stream.
        bool autoReconnect = true;

        friend bool operator==(const Config&, const Config&) = default;
    };

    /// How the display sink presents frames. Construct-only: a change applies when restart() rebuilds the sink.
    struct DisplayConfig
    {
        QByteArray conversionElement;  ///< Converter element factory; empty for the backend's default.
        bool disablePixelAspectRatio = false;
        bool gpuZeroCopy = true;       ///< Off when the CPU video path or a software decoder is forced.

        friend bool operator==(const DisplayConfig&, const DisplayConfig&) = default;
    };

    /// How a receiver that encodes its recordings anew (encodesRecordings()) encodes them, as QtVideoRecorder::start()
    /// takes it: from VideoSettings' uvcRecordingCodec and uvcRecordingQuality.
    struct RecordingEncoding
    {
        /// Tried first; Unspecified for the first codec that encodes.
        QMediaFormat::VideoCodec codec = QMediaFormat::VideoCodec::Unspecified;
        QMediaRecorder::Quality quality = QMediaRecorder::HighQuality;

        friend bool operator==(const RecordingEncoding&, const RecordingEncoding&) = default;
    };

    explicit VideoReceiver(QObject* parent = nullptr);
    ~VideoReceiver() override;

    /// "mkv", "mov" or "mp4".
    static QString fileExtension(FileFormat format);
    /// The comment recordings carry, of GStreamer and of a USB camera alike: the application and its version.
    static QString recordingComment();
    static bool isValidFileFormat(int format);
    /// The size @p frame is presented at: its format's viewport (the picture without decoder padding or crop), else the
    /// whole frame, turned by the rotation Qt presents it with. Invalid for an invalid frame. A valid @p pictureSize
    /// stands in for the viewport's, which is smaller where it leaves out the padding of a squeezed texture.
    static QSize presentedSize(const QVideoFrame& frame, QSize pictureSize = {});

    QString name() const { return _name; }

    /// Names the stream in logs and pipeline dumps. Call once, before the first setActive(true).
    void setName(const QString& name);

    const Config& config() const { return _config; }

    const DisplayConfig& displayConfig() const { return _displayConfig; }

    bool active() const { return _active; }

    QQuickItem* videoOutput() const;

    State state() const { return _state; }

    bool streaming() const { return _state == State::Streaming; }

    bool decoding() const { return _decoding; }

    bool recording() const { return _recording; }

    /// The size of the frames as they are presented, before the orientation the VideoOutput adds.
    QSize videoSize() const { return _videoSize; }

    /// The stream's health, measured about once a second while it streams; invalid while it doesn't.
    VideoStreamStats stats() const { return _stats; }

    /// Stores @p config. While active, a change to anything but autoReconnect restarts the stream; an empty URI stops
    /// it until a URI is set again.
    void setConfig(const Config& config);

    /// Stores @p config for the display sink restart() builds next.
    void setDisplayConfig(const DisplayConfig& config) { _displayConfig = config; }

    /// True for a receiver that encodes its recordings anew, as recordingEncoding() says: a USB camera, or Qt
    /// Multimedia's player. False for one that records the stream as it arrives.
    virtual bool encodesRecordings() const { return false; }

    const RecordingEncoding& recordingEncoding() const { return _recordingEncoding; }

    /// Stores @p encoding for the recordings that start from now on; one in progress keeps its own.
    void setRecordingEncoding(const RecordingEncoding& encoding) { _recordingEncoding = encoding; }

    /// Streams while true and the URI is set. False stops the stream, any pending reconnect and any recording.
    void setActive(bool active);
    /// setActive(false) for a receiver that outlives its use, before the application goes away: once it has finished
    /// what the exit would cut short.
    virtual void shutDown();
    /// Tears the stream down and connects again now, skipping any reconnect backoff, and rebuilds the display sink so
    /// construct-only sink settings apply. No-op while inactive.
    void restart();
    /// Connects again now when the stream is waiting out a reconnect backoff, and starts the backoff over: for when the
    /// stream's source is likely back, as when its vehicle's link returned. Nothing otherwise.
    void reconnectNow();
    /// Displays the stream in @p videoOutput, a QML VideoOutput; nullptr stops displaying. Decoding runs only while an
    /// output is set. Setting the current output again binds it anew, for one that moved to another window. The
    /// output's destruction is tracked: it detaches as if nullptr had been set.
    void setVideoOutput(QQuickItem* videoOutput);

    /// Records to @p file until stopRecording() or setActive(false). Accepted only while active (otherwise
    /// errorOccurred()); the file opens once streaming, at the next keyframe for a receiver that records the stream as
    /// it arrives. Such a receiver continues the recording after a reconnect, and after each hour, into
    /// "<base>_part<N>.<ext>", N counting from 2; one that encodes the frames anew, as recordingEncoding() says, keeps
    /// to the same file.
    void startRecording(const QString& file, FileFormat format);
    void stopRecording();

    /// Saves the frame on display to @p file as shown: cropped to its format's viewport, rotated and mirrored as the
    /// frame and then the VideoOutput (its orientation and mirrored properties) say, with @p exif for a JPEG. The frame
    /// is converted here; it is encoded and written on the thread pool. Returns false, with a warning logged, when
    /// there is no output or no frame. A file that can't be written is reported later, through errorOccurred().
    /// A receiver that takes photos saves one instead: see onTakeScreenshot().
    bool takeScreenshot(const QString& file, const VideoScreenshot::Exif& exif = {});

signals:
    void stateChanged(VideoReceiver::State state);
    void streamingChanged(bool streaming);
    void decodingChanged(bool decoding);
    void recordingChanged(bool recording);
    /// A recording file opened: the first one after startRecording(), or the recording's next part.
    void recordingStarted(const QString& file);
    /// A recording file closed. @p error is empty when the file was finalized cleanly.
    void recordingFinished(const QString& file, const QString& error);
    void videoSizeChanged(QSize size);
    void statsChanged(const VideoStreamStats& stats);
    /// A failure worth telling the user about (stream error with autoReconnect off, a stream that is received but
    /// can't be decoded, recording error, a screenshot that couldn't be saved). Not emitted for failures the receiver
    /// recovers from by reconnecting.
    void errorOccurred(const QString& message);

protected:
    virtual void onConfigChanged() = 0;
    virtual void onActiveChanged() = 0;
    virtual void onRestartRequested() = 0;
    virtual void onReconnectRequested() = 0;
    /// Also called when the current output is set again.
    virtual void onVideoOutputChanged() = 0;
    virtual void onStartRecording(const QString& file, FileFormat format) = 0;
    virtual void onStopRecording() = 0;
    /// takeScreenshot() with an output set: saves the frame on display, unless overridden.
    virtual bool onTakeScreenshot(const QString& file, const VideoScreenshot::Exif& exif);

    /// The VideoOutput's QVideoSink, or nullptr.
    QVideoSink* videoSink() const;

    // Each emits its NOTIFY signal (and streamingChanged for setState()) when the value changes.
    void setState(State state);
    void setDecoding(bool decoding);
    void setRecording(bool recording);
    void setVideoSize(QSize size);
    /// Backends publish a new measurement, and an invalid one once the stream stops or reconnects.
    void setStats(const VideoStreamStats& stats);

private:
    void _onVideoOutputDestroyed();

    QString _name;
    Config _config;
    DisplayConfig _displayConfig;
    RecordingEncoding _recordingEncoding;
    bool _active = false;
    QPointer<QQuickItem> _videoOutput;
    QMetaObject::Connection _videoOutputDestroyed;
    State _state = State::Stopped;
    bool _decoding = false;
    bool _recording = false;
    QSize _videoSize;
    VideoStreamStats _stats;
};
