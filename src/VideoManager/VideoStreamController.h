#pragma once

#include <optional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoReceiver.h"
#include "VideoStreamResolver.h"

class QGCVideoStreamInfo;
class QQuickItem;
class VideoSettings;

/// One video stream, primary or thermal. Resolves the stream's config from the video settings and the camera's
/// advertised stream, drives the stream's VideoReceiver, and connects the receiver to the QML VideoOutput that
/// displays it.
///
/// The VideoOutput registers itself: registerVideoOutput() once it is created and again after it moved to another
/// window, unregisterVideoOutput() when it is destroyed (see FlightDisplayViewVideoOutput.qml). The registration
/// outlives receivers, so an output registered before the video backend is ready is attached once a receiver exists.
class VideoStreamController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged)
    Q_PROPERTY(double aspectRatio READ aspectRatio NOTIFY aspectRatioChanged)

public:
    using Role = VideoStreamResolver::Role;

    VideoStreamController(Role role, VideoSettings* videoSettings, QObject* parent = nullptr);
    ~VideoStreamController() override;

    /// "videoContent" or "thermalVideo": names the receiver, and tags the thermal stream's files.
    QString name() const;
    /// @p file for this stream: the thermal stream's name goes before the extension, "<base>.thermalVideo.<ext>".
    QString taggedFile(const QString& file) const;

    VideoReceiver* receiver() const { return _receiver; }

    /// Takes ownership of @p receiver, replacing (and deleting) the current one; nullptr just deletes it.
    void setReceiver(VideoReceiver* receiver);

    QGCVideoStreamInfo* streamInfo() const;
    /// The stream the camera advertises for this role, which takes precedence over the manual settings.
    void setStreamInfo(QGCVideoStreamInfo* streamInfo);

    /// The stream plays while it is enabled and configured().
    void setEnabled(bool enabled);

    bool configured() const { return _resolvedConfig().has_value(); }

    /// Pushes the current config and enabled state to the receiver on the next event loop pass. Calls before then
    /// coalesce into one update.
    void scheduleApply();
    /// Disables the stream and stops the receiver now.
    void stop();
    /// Reconnects now and rebuilds the receiver's display sink, so construct-only sink settings apply.
    void restart();

    QQuickItem* videoOutput() const;

    /// Displays the stream in @p videoOutput, a QML VideoOutput, replacing any other. Registering the current output
    /// again re-attaches it: a VideoOutput that moved to another window must do this.
    Q_INVOKABLE void registerVideoOutput(QQuickItem* videoOutput);
    /// Stops displaying in @p videoOutput, if it is the registered output.
    Q_INVOKABLE void unregisterVideoOutput(QQuickItem* videoOutput);

    /// True when the receiver is active, accepting startRecording().
    bool active() const;
    void startRecording(const QString& file, VideoReceiver::FileFormat format);
    void stopRecording();

    /// True while the stream is streaming into a registered output.
    bool canTakeScreenshot() const;
    bool takeScreenshot(const QString& file) const;

    bool streaming() const;
    bool decoding() const;
    bool recording() const;
    QSize videoSize() const;
    /// From the decoded frames, else the camera's advertised resolution, else the aspect ratio setting (primary) or 1.
    double aspectRatio() const;

signals:
    void streamingChanged();
    void decodingChanged();
    void recordingChanged(bool recording);
    void videoSizeChanged();
    void aspectRatioChanged();
    void streamInfoChanged();
    void recordingStarted(const QString& file);
    void recordingFinished(const QString& file, const QString& error);
    void errorOccurred(const QString& message);

private:
    /// What the receiver is (or will be) configured with; nullopt when there is no stream to play.
    std::optional<VideoReceiver::Config> _resolvedConfig() const;
    void _apply();
    void _onStreamInfoChanged();

    const Role _role;
    VideoSettings* const _videoSettings;
    VideoReceiver* _receiver = nullptr;
    QPointer<QGCVideoStreamInfo> _streamInfo;
    QPointer<QQuickItem> _videoOutput;
    bool _enabled = false;
    bool _applyPending = false;
};
