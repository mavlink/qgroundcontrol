#pragma once

#include <chrono>
#include <cstdint>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class QQuickItem;
class QVideoFrame;
class QVideoSink;

/// One network video stream: connects to a URI, shows it in a QML VideoOutput and records it.
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
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged)

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
        /// No data, or no decoded frame while displaying, for this long is a stream failure.
        std::chrono::seconds timeout{3};
        bool lowLatency = false;
        int rtpJitterLatencyMs = 80;
        /// Reconnect with backoff after a failure. Applies at once; changing any other field restarts the stream.
        bool autoReconnect = true;

        friend bool operator==(const Config&, const Config&) = default;
    };

    explicit VideoReceiver(QObject* parent = nullptr);
    ~VideoReceiver() override;

    /// "mkv", "mov" or "mp4".
    static QString fileExtension(FileFormat format);
    static bool isValidFileFormat(int format);
    /// The size @p frame is presented at: its format's viewport (the picture without decoder padding or crop), else the
    /// whole frame, turned by the rotation Qt presents it with. Invalid for an invalid frame.
    static QSize presentedSize(const QVideoFrame& frame);

    QString name() const { return _name; }

    /// Names the stream in logs and pipeline dumps. Call once, before the first setActive(true).
    void setName(const QString& name);

    const Config& config() const { return _config; }

    bool active() const { return _active; }

    QQuickItem* videoOutput() const;

    State state() const { return _state; }

    bool streaming() const { return _state == State::Streaming; }

    bool decoding() const { return _decoding; }

    bool recording() const { return _recording; }

    QSize videoSize() const { return _videoSize; }

    /// Stores @p config. While active, a change to anything but autoReconnect restarts the stream; an empty URI stops
    /// it until a URI is set again.
    void setConfig(const Config& config);
    /// Streams while true and the URI is set. False stops the stream, any pending reconnect and any recording.
    void setActive(bool active);
    /// Tears the stream down and connects again now, skipping any reconnect backoff, and rebuilds the display sink so
    /// construct-only sink settings apply. No-op while inactive.
    void restart();
    /// Displays the stream in @p videoOutput, a QML VideoOutput; nullptr stops displaying. Decoding runs only while an
    /// output is set. Setting the current output again binds it anew, for one that moved to another window. The
    /// output's destruction is tracked: it detaches as if nullptr had been set.
    void setVideoOutput(QQuickItem* videoOutput);

    /// Records to @p file until stopRecording() or setActive(false). Accepted only while active (otherwise
    /// errorOccurred()); the file opens at the next keyframe once streaming. After a reconnect the recording continues
    /// into "<base>_part<N>.<ext>", N counting from 2.
    void startRecording(const QString& file, FileFormat format);
    void stopRecording();

    /// Saves the frame on display to @p file as shown: cropped to its format's viewport, rotated and mirrored. Returns
    /// false, with a warning logged, when there is no output, no frame, or the file can't be written.
    bool takeScreenshot(const QString& file) const;

signals:
    void stateChanged(VideoReceiver::State state);
    void streamingChanged(bool streaming);
    void decodingChanged(bool decoding);
    void recordingChanged(bool recording);
    /// A recording file opened: the first one after startRecording(), or the next part after a reconnect.
    void recordingStarted(const QString& file);
    /// A recording file closed. @p error is empty when the file was finalized cleanly.
    void recordingFinished(const QString& file, const QString& error);
    void videoSizeChanged(QSize size);
    /// A failure worth telling the user about (stream error with autoReconnect off, recording error). Not emitted for
    /// failures the receiver recovers from by reconnecting.
    void errorOccurred(const QString& message);

protected:
    virtual void onConfigChanged() = 0;
    virtual void onActiveChanged() = 0;
    virtual void onRestartRequested() = 0;
    /// Also called when the current output is set again.
    virtual void onVideoOutputChanged() = 0;
    virtual void onStartRecording(const QString& file, FileFormat format) = 0;
    virtual void onStopRecording() = 0;

    /// The VideoOutput's QVideoSink, or nullptr.
    QVideoSink* videoSink() const;

    // Each emits its NOTIFY signal (and streamingChanged for setState()) when the value changes.
    void setState(State state);
    void setDecoding(bool decoding);
    void setRecording(bool recording);
    void setVideoSize(QSize size);

private:
    void _onVideoOutputDestroyed();

    QString _name;
    Config _config;
    bool _active = false;
    QPointer<QQuickItem> _videoOutput;
    QMetaObject::Connection _videoOutputDestroyed;
    State _state = State::Stopped;
    bool _decoding = false;
    bool _recording = false;
    QSize _videoSize;
};
