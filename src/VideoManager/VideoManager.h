#pragma once

#include <cstdint>
#include <functional>

#include <QtCore/QFuture>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQml/QQmlListProperty>
#include <QtQmlIntegration/QtQmlIntegration>

class MavlinkCameraControlInterface;
class QGCVideoStreamInfo;
class QQuickWindow;
class UVCCameraController;
class Vehicle;
class VideoReceiver;
class VideoRecordingService;
class VideoSettings;
class VideoStreamController;
class VideoTelemetryOverlay;

/// QML facade of the video sources, each a VideoStreamController: the primary stream, the thermal stream, the USB
/// camera, whose receiver is uvcCamera, and the streams shown in tiles. One of the primary stream and the camera is
/// the main picture: the camera while the video source setting names one and no vehicle camera advertises a stream.
/// decoding and aspectRatio forward to the primary stream, thermalAspectRatio to the thermal stream, and recording
/// covers the main picture and the thermal stream. The other properties, such as hasVideo and isUvc, are the manager's
/// own state.
///
/// The tiles show what else there is to see, up to VideoSettings::maxVideoTiles of them: the USB camera
/// VideoSettings::uvcTileCamera names, while a stream is the video source, then the other streams of the active
/// vehicle's current camera, then its other cameras' streams. Each stream in a tile has a source of its own, which
/// plays that stream for as long as it is shown. When the vehicle's current stream becomes one that plays in a tile,
/// that source becomes the primary stream and the former one takes a tile: neither stream restarts. The camera
/// selected in its tile (selectSource()) is the main picture as well, with the primary stream in a tile, until a
/// stream is selected.
class VideoManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("UVCCameraController.h")
    Q_MOC_INCLUDE("VideoStreamController.h")

    /// Every source, for what treats them alike: the primary stream, the thermal stream, the USB camera, then the
    /// streams in tiles, in the tiles' order.
    Q_PROPERTY(QQmlListProperty<VideoStreamController> sources READ sources NOTIFY sourcesChanged FINAL)
    /// The source of the vehicle's current stream, or of the stream the settings configure. Never null.
    Q_PROPERTY(VideoStreamController* primaryStream READ primaryStream NOTIFY primaryStreamChanged FINAL)
    Q_PROPERTY(VideoStreamController* thermalStream READ thermalStream CONSTANT FINAL)
    Q_PROPERTY(UVCCameraController* uvcCamera READ uvcCamera CONSTANT FINAL)
    Q_PROPERTY(bool autoStreamConfigured READ autoStreamConfigured NOTIFY autoStreamConfiguredChanged FINAL)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged FINAL)
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY fullScreenChanged FINAL)
    Q_PROPERTY(bool hasThermal READ hasThermal NOTIFY hasThermalChanged FINAL)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged FINAL)
    Q_PROPERTY(bool isStreamSource READ isStreamSource NOTIFY isStreamSourceChanged FINAL)
    Q_PROPERTY(bool isUvc READ isUvc NOTIFY isUvcChanged FINAL)
    Q_PROPERTY(NoVideoReason noVideoReason READ noVideoReason NOTIFY noVideoReasonChanged FINAL)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged FINAL)
    Q_PROPERTY(double aspectRatio READ aspectRatio NOTIFY aspectRatioChanged FINAL)
    Q_PROPERTY(double thermalAspectRatio READ thermalAspectRatio NOTIFY aspectRatioChanged FINAL)
    Q_PROPERTY(QString uvcVideoSourceID READ uvcVideoSourceID NOTIFY uvcVideoSourceIDChanged FINAL)
    /// The telemetry VideoView shows over the primary stream or the USB camera: empty while
    /// VideoSettings::showTelemetryOverlay is off.
    Q_PROPERTY(QString telemetryOverlayText READ telemetryOverlayText NOTIFY telemetryOverlayTextChanged FINAL)

    friend class VideoManagerInitTest;
    friend class VideoManagerTest;
    friend class VideoManagerVehicleTest;

public:
    explicit VideoManager(QObject* parent = nullptr);
    ~VideoManager() override;

    static VideoManager* instance();

    /// Why a video view without a picture has none.
    enum class NoVideoReason : uint8_t
    {
        Waiting,                ///< Nothing keeps the video from playing: it is awaited.
        Stopped,                ///< stopVideo(), until startVideo().
        DisabledWhileDisarmed,  ///< VideoSettings::disableWhenDisarmed, until the vehicle arms.
    };
    Q_ENUM(NoVideoReason)

    /// Saves the frame on display, or the USB camera's photo, to @p imageFile (default: a timestamped file in the
    /// photo directory), and the thermal stream's frame, when it is streaming, to the same name tagged
    /// ".thermalVideo". The files are written off the GUI thread; the user is told when there is no video to save, or
    /// a file can't be written.
    Q_INVOKABLE void grabImage(const QString& imageFile = QString());
    /// See VideoRecordingService::startRecording().
    Q_INVOKABLE void startRecording(const QString& baseName = QString());
    /// Undoes stopVideo() and "disable when disarmed", and reconnects the streams.
    Q_INVOKABLE void startVideo();
    Q_INVOKABLE void stopRecording();
    /// Stops the sources, and any recording, until startVideo().
    Q_INVOKABLE void stopVideo();
    /// Makes @p source, one shown in a tile, the main picture. A stream of a vehicle's camera becomes that camera's
    /// current stream, and its camera the current camera, as the camera panel selects them. The USB camera takes
    /// the main picture from the stream, which takes its tile, until a stream is selected again: nothing is stored.
    Q_INVOKABLE void selectSource(VideoStreamController* source);

    void init(QQuickWindow* mainWindow);
    /// Starts the video backend's asynchronous global init. GUI thread only, like everything else here.
    void startVideoBackendInit();
    /// Blocks until the backend init finishes, starting it if needed. True when the backend is ready.
    bool waitForVideoBackendReady();
    /// Shuts the sources down, for good: deletes the streams' receivers and stops the USB camera, and nothing starts
    /// again afterwards. Call before the QML engine and the application go away.
    void cleanup();

    QQmlListProperty<VideoStreamController> sources();

    VideoStreamController* primaryStream() const { return _primaryStream; }

    VideoStreamController* thermalStream() const { return _thermalStream; }

    UVCCameraController* uvcCamera() const { return _uvcCamera; }

    bool autoStreamConfigured() const { return _autoStreamConfigured; }

    bool decoding() const;

    bool fullScreen() const { return _fullScreen; }

    bool hasThermal() const { return _hasThermal; }

    bool hasVideo() const { return _hasVideo; }

    bool isStreamSource() const { return _isStreamSource; }

    bool isUvc() const { return !_uvcVideoSourceID.isEmpty(); }

    NoVideoReason noVideoReason() const { return _noVideoReason; }

    bool recording() const;
    double aspectRatio() const;
    double thermalAspectRatio() const;

    QString uvcVideoSourceID() const { return _uvcVideoSourceID; }

    QString telemetryOverlayText() const;

    void setFullScreen(bool on);

signals:
    void sourcesChanged();
    void primaryStreamChanged();
    void aspectRatioChanged();
    void autoStreamConfiguredChanged();
    void decodingChanged();
    void fullScreenChanged();
    void hasThermalChanged();
    void hasVideoChanged();
    void isStreamSourceChanged();
    void isUvcChanged();
    void noVideoReasonChanged();
    void recordingChanged(bool recording);
    void telemetryOverlayTextChanged();
    void uvcVideoSourceIDChanged();

private:
    enum class InitState : uint8_t
    {
        NotStarted,
        Pending,
        Ready,
        Failed,
    };

    /// A stream a camera of the active vehicle advertises.
    struct AdvertisedStream
    {
        QPointer<QGCVideoStreamInfo> streamInfo;
        QPointer<MavlinkCameraControlInterface> camera;
        /// For the user: the camera's name, when the vehicle has several, and the stream's.
        QString name;
    };

    void _connectSettings();
    void _onMainWindowReady();
    void _watchBackendInit(const QFuture<bool>& future);
    void _onBackendInitComplete(bool success);

    /// Ready or failed: either way the backend makes receivers, after a failure the ones that need no initialization.
    bool _backendInitFinished() const { return (_initState == InitState::Ready) || (_initState == InitState::Failed); }

    void _createVideoReceivers();
    /// Rebuilds the streams' display sinks and reconnects them; tries a camera that failed again.
    void _restartSources();
    /// Recomputes the derived state and which sources are enabled, after any input changed. A source that lets go of a
    /// receiver that is recording runs the event loop as it does: an input that changes there is taken up by another
    /// pass once this one is over, not within it.
    void _updateState();
    void _updateStateNow();
    void _setActiveVehicle(Vehicle* vehicle);
    /// Reads the streams the active vehicle's cameras advertise.
    void _updateStreamInfo();
    /// Shows @p current as the primary stream and @p thermal as the thermal stream; @p others, in the order they are
    /// to fill the tiles, are the streams to show in tiles. The source that plays @p current in a tile, if one does,
    /// becomes the primary stream; otherwise the primary stream plays it.
    void _setAdvertisedStreams(const AdvertisedStream& current, QGCVideoStreamInfo* thermal,
                               const QList<AdvertisedStream>& others);
    /// Gives the first @p count of the streams to show in tiles a source each, and removes the other tiles' sources.
    /// A stream whose address, its URI or the port it is listened for at, a stream on display already has gets none:
    /// its camera sends one of them at a time.
    void _updateTileStreams(int count);
    /// The tile's source that plays @p streamInfo, if any.
    VideoStreamController* _tileStream(const QGCVideoStreamInfo* streamInfo) const;
    /// Tells each camera of the active vehicle which of its streams play in tiles.
    void _nameExtraStreams();
    void _connectSource(VideoStreamController* source);
    void _onArmedChanged(bool armed);
    void _communicationLostChanged(bool communicationLost);
    /// The streams' sources are likely back, with a vehicle that connected or whose link returned: a stream waiting
    /// to reconnect does so now.
    void _reconnectStreams();

    VideoSettings* const _videoSettings;
    /// The camera source's receiver.
    UVCCameraController* const _uvcCamera;
    VideoStreamController* _primaryStream;
    VideoStreamController* const _thermalStream;
    VideoStreamController* const _cameraSource;
    /// The sources of the streams in tiles, in the tiles' order.
    QList<VideoStreamController*> _tileStreams;
    QList<VideoStreamController*> _sources;
    VideoRecordingService* const _recordingService;
    VideoTelemetryOverlay* const _telemetryOverlay;
    std::function<VideoReceiver*(QObject* parent)> _receiverFactory;
    /// Whether a local camera of that description is attached.
    std::function<bool(const QString& camera)> _cameraAttached;
    QPointer<Vehicle> _activeVehicle;
    /// The streams to show in tiles, in the order they are to fill them.
    QList<AdvertisedStream> _otherStreams;

    InitState _initState = InitState::NotStarted;
    QFuture<bool> _backendInitFuture;

    bool _hasMainWindow = false;
    bool _cleanedUp = false;
    /// The sources are being gone through, which _updateState() waits out (_updateStatePending).
    bool _updatingState = false;
    bool _updateStatePending = false;
    bool _fullScreen = false;
    bool _videoStopped = false;
    /// The camera in a tile was selected: it is the main picture, and the primary stream is in a tile.
    bool _cameraTileOnMain = false;
    /// How many tile sources there have been, which names the next one.
    int _tileStreamSerial = 0;
    bool _disabledWhileDisarmed = false;
    bool _autoStreamConfigured = false;
    bool _hasThermal = false;
    bool _hasVideo = false;
    bool _isStreamSource = false;
    NoVideoReason _noVideoReason = NoVideoReason::Waiting;
    QString _uvcVideoSourceID;
};
