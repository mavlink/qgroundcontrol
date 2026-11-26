#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoReceiver.h"
#include "VideoStreamResolver.h"

class QGCVideoStreamInfo;
class QQuickItem;
class VideoSettings;

/// One video source: the primary stream, the thermal stream, the local camera or a stream shown in a tile. Resolves the
/// source's config from the video settings and the vehicle camera's advertised stream, drives the source's
/// VideoReceiver, and connects the receiver to the QML VideoOutput that displays it.
///
/// A stream's receiver comes from the video backend, and goes when the backend changes: setReceiver(). The local
/// camera's is the source's for good, given to its constructor.
///
/// VideoManager says where the source is shown. The primary stream and a stream in a tile change places, role for
/// role, when the tile's stream becomes the one to display: each goes on playing what it plays.
///
/// The VideoOutput registers itself: registerVideoOutput() once it is created and again after it moved to another
/// window, unregisterVideoOutput() when it is destroyed (see VideoStreamOutput.qml). The registration
/// outlives receivers, so an output registered before the video backend is ready is attached once a receiver exists.
/// An output that registers as another one leaves takes the stream over from it: the receiver, which decodes only
/// into an output, is never without one in between.
///
/// The thermal stream plays only while it is in use: while an output is registered for it, and from startRecording()
/// until its recording has ended. Out of use for kIdleGrace it disconnects, and it connects again with its next use.
class VideoStreamController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(VideoStreamResolver::Role role READ role NOTIFY roleChanged FINAL)
    /// The source is shown in a tile of the video view, rather than as the main picture or with it.
    Q_PROPERTY(bool tile READ tile NOTIFY tileChanged FINAL)
    /// What the user knows the source by: the camera's and the stream's names, the video source, or the local camera.
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged FINAL)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged FINAL)
    Q_PROPERTY(bool reconnecting READ reconnecting NOTIFY reconnectingChanged FINAL)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged FINAL)
    Q_PROPERTY(bool encodesRecordings READ encodesRecordings NOTIFY encodesRecordingsChanged FINAL)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged FINAL)
    Q_PROPERTY(int rotation READ rotation NOTIFY rotationChanged FINAL)
    Q_PROPERTY(bool mirrored READ mirrored NOTIFY mirroredChanged FINAL)
    Q_PROPERTY(double aspectRatio READ aspectRatio NOTIFY aspectRatioChanged FINAL)
    /// Width over height the stream's outputs stretch its picture to (VideoSettings::forcedAspectRatio); 0 for the
    /// video's own, which any stream but the manually configured one keeps.
    Q_PROPERTY(double forcedAspectRatio READ forcedAspectRatio NOTIFY forcedAspectRatioChanged FINAL)
    Q_PROPERTY(VideoStreamStats stats READ stats NOTIFY statsChanged FINAL)
    Q_PROPERTY(QGCVideoStreamInfo* streamInfo READ streamInfo NOTIFY streamInfoChanged FINAL)
    Q_PROPERTY(QString uri READ uri NOTIFY uriChanged FINAL)
    Q_PROPERTY(QString transport READ transport NOTIFY uriChanged FINAL)
    Q_MOC_INCLUDE("QGCVideoStreamInfo.h")

    friend class VideoManagerTest;
    friend class VideoManagerVehicleTest;

public:
    using Role = VideoStreamResolver::Role;

    /// How long a stream that nobody sees any more stays connected: the thermal stream out of use, and the streams
    /// in tiles once their row is no longer shown (VideoManager::tileRoom). A view that is back within that time,
    /// as when the video changes places with the map and back, finds its streams playing.
    static constexpr std::chrono::seconds kIdleGrace{15};

    /// @p ownReceiver, when given, is the source's receiver for good; the source takes ownership of it.
    VideoStreamController(Role role, VideoSettings* videoSettings, QObject* parent = nullptr,
                          VideoReceiver* ownReceiver = nullptr);
    ~VideoStreamController() override;

    Role role() const { return _role; }

    /// Makes a stream in a tile the primary stream, or the reverse. What the source plays stays.
    void setRole(Role role);

    bool tile() const { return _tile; }

    void setTile(bool tile);

    QString displayName() const { return _displayName; }

    void setDisplayName(const QString& displayName);

    /// "videoContent", "thermalVideo", "uvcCamera" or the name given to a tile's stream: names the receiver, and tags
    /// the thermal stream's files. The source keeps the name it started with when its role changes.
    QString name() const { return _name; }

    /// Names a source that has no receiver yet.
    void setName(const QString& name) { _name = name; }

    /// @p file for this source: the thermal stream's name goes before the extension, "<base>.thermalVideo.<ext>".
    QString taggedFile(const QString& file) const;

    VideoReceiver* receiver() const { return _receiver; }

    /// Takes ownership of @p receiver, replacing (and deleting) the current one; nullptr just deletes it. Nothing for
    /// a source with a receiver of its own.
    void setReceiver(VideoReceiver* receiver);
    /// Stops the source for good, before the application goes away: deletes its receiver, or shuts down
    /// (VideoReceiver::shutDown()) one of its own.
    void shutDown();

    QGCVideoStreamInfo* streamInfo() const;
    /// The stream the camera advertises for this role, which takes precedence over the manual settings.
    void setStreamInfo(QGCVideoStreamInfo* streamInfo);

    /// The attached local camera the camera source plays, by its description, which is its config's URI; empty for
    /// none. The stream settings aren't a camera's: its config changes with the camera alone.
    void setCamera(const QString& camera);

    /// The stream plays while it is enabled and configured().
    void setEnabled(bool enabled);

    bool configured() const { return _resolvedConfig().has_value(); }

    /// True while the source is to play its stream, which its receiver then connects to: it is enabled and
    /// configured(), and the thermal stream is in use as well.
    bool wantsStream() const { return _enabled && !_idle && configured(); }

    /// Pushes the current config and enabled state to the receiver on the next event loop pass. Calls before then
    /// coalesce into one update.
    void scheduleApply();
    /// Disables the stream and stops the receiver now.
    void stop();
    /// Reconnects now and rebuilds the receiver's display sink, so construct-only sink settings apply.
    void restart();
    /// VideoReceiver::reconnectNow(): the stream's source is likely back, so a reconnect doesn't wait out its backoff.
    void reconnectNow();

    QQuickItem* videoOutput() const;

    /// Displays the stream in @p videoOutput, a QML VideoOutput, replacing any other. Registering the current output
    /// again re-attaches it: a VideoOutput that moved to another window must do this.
    Q_INVOKABLE void registerVideoOutput(QQuickItem* videoOutput);
    /// Stops displaying in @p videoOutput, if it is the registered output. The receiver is told on the next event
    /// loop pass, unless another output registered by then.
    Q_INVOKABLE void unregisterVideoOutput(QQuickItem* videoOutput);

    /// Keeps the registered output from the receiver while @p suspended, as while the application is in the
    /// background: a receiver without an output decodes nothing, and goes on recording.
    void setDisplaySuspended(bool suspended);

    /// True when the source has video to record: it is enabled and has a stream, or a camera, to play.
    bool canRecord() const;
    /// Records the source (VideoReceiver::startRecording()), which connects for it when it hasn't yet: the thermal
    /// stream out of use does.
    void startRecording(const QString& file, VideoReceiver::FileFormat format,
                        const VideoReceiver::RecordingEncoding& encoding = {});
    void stopRecording();

    /// True when the receiver encodes the source's recordings anew (VideoReceiver::encodesRecordings()), which the
    /// recording codec and quality settings then govern. False without a receiver.
    bool encodesRecordings() const;

    /// True while the stream is streaming into a registered output.
    bool canTakeScreenshot() const;
    bool takeScreenshot(const QString& file, const VideoScreenshot::Exif& exif = {}) const;

    bool decoding() const;

    /// True while the stream, lost after it was decoded, is being recovered: its last frame is still worth showing.
    /// False again once it decodes, when the receiver gives up, and after a stop or a change of URI. Never for the
    /// local camera, whose picture goes with it.
    bool reconnecting() const { return _reconnecting; }

    bool recording() const;

    /// The clockwise rotation, in degrees, the camera reports for the stream (VIDEO_STREAM_INFORMATION.rotation), which
    /// the outputs showing it add to the frames' own: 0, 90, 180 or 270, and 0 for any other angle or without a
    /// camera's stream. The local camera's is the one setTransform() gave.
    int rotation() const { return _rotation; }

    /// Whether the outputs mirror the picture after turning it: the local camera's, as setTransform() said.
    bool mirrored() const { return _mirrored; }

    /// Has the local camera's outputs turn its picture clockwise by @p rotation, a multiple of 90 degrees, and then
    /// mirror it when @p mirrored.
    void setTransform(int rotation, bool mirrored);

    /// The size the stream is shown at: the receiver's video size, turned by rotation().
    QSize videoSize() const;
    /// Of the video's own picture, turned by rotation(): from the decoded frames, else the camera's advertised
    /// resolution, else 1.
    double aspectRatio() const;

    double forcedAspectRatio() const { return _forcedAspectRatio; }
    /// The receiver's VideoReceiver::stats(); invalid without a receiver.
    VideoStreamStats stats() const;
    /// The URI the receiver plays, without user info or query values, for display, or the local camera's description;
    /// empty without a stream or a camera.
    QString uri() const;
    /// How the stream arrives, from its URI: "RTSP", "RTP/UDP (H.264)", "MPEG-TS/TCP", ...; empty without a stream, and
    /// for the local camera.
    QString transport() const;

signals:
    void roleChanged();
    void tileChanged();
    void displayNameChanged();
    void decodingChanged();
    void reconnectingChanged();
    void recordingChanged(bool recording);
    void encodesRecordingsChanged();
    void videoSizeChanged();
    void rotationChanged();
    void mirroredChanged();
    void aspectRatioChanged();
    void forcedAspectRatioChanged();
    void statsChanged();
    void streamInfoChanged();
    void uriChanged();
    /// The thermal stream came into use, or has been out of use for kIdleGrace.
    void idleChanged();
    void recordingStarted(const QString& file);
    void recordingFinished(const QString& file, const QString& error);
    void errorOccurred(const QString& message);

private:
    /// What the receiver is (or will be) configured with; nullopt when there is no stream to play.
    std::optional<VideoReceiver::Config> _resolvedConfig() const;
    VideoReceiver::DisplayConfig _resolvedDisplayConfig() const;
    void _updateForcedAspectRatio();
    void _apply();
    void _onStreamInfoChanged();
    /// Takes the receiver's output away once no output is registered.
    void _detachVideoOutput();
    void _onReceiverStateChanged(VideoReceiver::State state);
    /// Forgets that the stream was decoded: what follows is a new stream, not a reconnect.
    void _forgetDecoded();
    void _updateReconnecting();
    /// Shown in an output, or recorded.
    bool _inUse() const;
    /// The thermal stream, after what _inUse() tells changed: connects for a new use, and starts kIdleGrace once it has
    /// none.
    void _updateInUse();
    void _onIdleTimeout();

    Role _role;
    QString _name;
    QString _displayName;
    bool _tile = false;
    VideoSettings* const _videoSettings;
    VideoReceiver* _receiver = nullptr;
    bool _hasOwnReceiver = false;
    QPointer<QGCVideoStreamInfo> _streamInfo;
    QString _camera;
    QPointer<QQuickItem> _videoOutput;
    bool _displaySuspended = false;
    /// The URI the receiver was last configured with.
    QString _uri;
    bool _enabled = false;
    /// The thermal stream, out of use for kIdleGrace or never used: not connected until it is in use.
    bool _idle;
    QTimer _idleTimer;
    /// From startRecording() until stopRecording() or the end of the recording, which may have yet to begin.
    bool _recordingRequested = false;
    bool _applyPending = false;
    bool _decoded = false;
    bool _reconnecting = false;
    int _rotation = 0;
    bool _mirrored = false;
    double _forcedAspectRatio = 0;
};
