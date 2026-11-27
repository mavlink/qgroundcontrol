#pragma once

#include <cstdint>
#include <functional>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQml/QQmlListProperty>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VehicleVideoStreams.h"

class QGCNetworkAvailabilityMonitor;
class QGCVideoStreamInfo;
class QQuickWindow;
class UVCCameraController;
class VideoBackendInit;
class VideoReceiver;
class VideoRecordingService;
class VideoSettings;
class VideoStreamController;
class VideoTelemetryOverlay;

/// QML facade of the video sources, each a VideoStreamController: the primary stream, the thermal stream, the USB
/// camera, whose receiver is uvcCamera, and the streams shown in tiles. One of the primary stream and the camera is
/// the main picture: the camera while the video source setting names one and no vehicle camera advertises a stream.
/// decoding forwards to the primary stream, and recording covers the main picture and the thermal stream. The other
/// properties, such as hasVideo and isUvc, are the manager's own state.
///
/// The tiles show what else there is to see, up to VideoSettings::maxVideoTiles of them, and no more than the row of
/// tiles has room for (tileRoom): the USB camera VideoSettings::uvcTileCamera names, while a stream is the video
/// source, then the other streams of the active vehicle's current camera, then its other cameras' streams. Each stream
/// in a tile has a source of its own, which plays that stream for as long as it is shown. There are no tiles, and so
/// none of their sources, until a row of tiles that is shown has told its room. When the vehicle's current
/// stream becomes one that plays in a tile, that source becomes the primary stream and the former one takes a tile:
/// neither stream restarts. The camera selected in its tile (selectSource()) is the main picture as well, with the
/// primary stream in a tile, until a stream is selected: it stays the main picture whatever the room for tiles.
///
/// The thermal stream is the active vehicle's: its current camera's or, when that camera has none, that of the first
/// of its other cameras that has one, as with a sender that makes a camera of each stream. A current camera that
/// advertises thermal streams only has its thermal stream played by the primary stream, as the main picture, and
/// there is no thermal stream then.
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
    /// The source of the main picture: the USB camera while isUvc, else the primary stream. Never null.
    Q_PROPERTY(VideoStreamController* mainSource READ mainSource NOTIFY mainSourceChanged FINAL)
    Q_PROPERTY(UVCCameraController* uvcCamera READ uvcCamera CONSTANT FINAL)
    Q_PROPERTY(bool autoStreamConfigured READ autoStreamConfigured NOTIFY autoStreamConfiguredChanged FINAL)
    /// A source falls short (VideoStreamStats::degraded()) of the frame rate its camera is set up for: the one a
    /// stream's camera advertises, while the stream is decoded, or the USB camera's own.
    Q_PROPERTY(bool degraded READ degraded NOTIFY degradedChanged FINAL)
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY fullScreenChanged FINAL)
    Q_PROPERTY(bool hasThermal READ hasThermal NOTIFY hasThermalChanged FINAL)
    /// A USB camera is there to play, as the main picture (isUvc) or in a tile: it plays while its tile has room.
    Q_PROPERTY(bool hasUvcCamera READ hasUvcCamera NOTIFY hasUvcCameraChanged FINAL)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged FINAL)
    Q_PROPERTY(bool isStreamSource READ isStreamSource NOTIFY isStreamSourceChanged FINAL)
    Q_PROPERTY(bool isUvc READ isUvc NOTIFY isUvcChanged FINAL)
    /// The source whose figures stand for the video's: the first one that is measured, those of the main picture before
    /// those in tiles, which puts the primary stream before the thermal stream; the USB camera runs in the primary
    /// stream's place. The primary stream while none is measured. Never null.
    Q_PROPERTY(VideoStreamController* measuredSource READ measuredSource NOTIFY measuredSourceChanged FINAL)
    Q_PROPERTY(NoVideoReason noVideoReason READ noVideoReason NOTIFY noVideoReasonChanged FINAL)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged FINAL)
    /// How many sources are shown in tiles.
    Q_PROPERTY(int tileCount READ tileCount NOTIFY tileCountChanged FINAL)
    /// How many tiles the row of tiles has room for, which it tells while it is shown: no more sources than that play
    /// in tiles, whatever VideoSettings::maxVideoTiles allows. A row that is no longer shown tells 0 once it has been
    /// out of view for idleSourceGraceMs. -1 until a row tells, which leaves no room either: no row shows tiles.
    Q_PROPERTY(int tileRoom READ tileRoom WRITE setTileRoom NOTIFY tileRoomChanged FINAL)
    /// VideoStreamController::kIdleGrace in milliseconds, for the row of tiles.
    Q_PROPERTY(int idleSourceGraceMs READ idleSourceGraceMs CONSTANT FINAL)
    /// The telemetry VideoView shows over the primary stream or the USB camera: empty while
    /// VideoSettings::showTelemetryOverlay is off.
    Q_PROPERTY(QString telemetryOverlayText READ telemetryOverlayText NOTIFY telemetryOverlayTextChanged FINAL)

    friend class TopLevelViewsTest;
    friend class VideoManagerInitTest;
    friend class VideoManagerTest;
    friend class VideoManagerTestBase;
    friend class VideoManagerVehicleTest;
    friend class VideoQmlTest;

public:
    explicit VideoManager(QObject* parent = nullptr);
    ~VideoManager() override;

    static VideoManager* instance();

    /// Why a video view without a picture has none.
    enum class NoVideoReason : uint8_t
    {
        Waiting,                ///< Nothing keeps the video from playing: it is awaited.
        Stopped,                ///< stopVideo(), until startVideo().
        DisabledWhileDisarmed,  ///< VideoSettings::disableWhenDisarmed, until the vehicle arms or startVideo().
    };
    Q_ENUM(NoVideoReason)

    /// Saves the frame on display, or the USB camera's photo, to @p imageFile (default: a timestamped file in the
    /// photo directory), and the thermal stream's frame, when it is streaming, to the same name tagged
    /// ".thermalVideo". The files are written off the GUI thread; the user is told when there is no video to save, or
    /// a file can't be written.
    Q_INVOKABLE void grabImage(const QString& imageFile = QString());
    /// See VideoRecordingService::startRecording().
    Q_INVOKABLE void startRecording(const QString& baseName = QString());
    /// Undoes stopVideo() and reconnects the streams. Video that is off for a disarmed vehicle
    /// (VideoSettings::disableWhenDisarmed) plays again as well, until the vehicle's armed state next changes.
    Q_INVOKABLE void startVideo();
    Q_INVOKABLE void stopRecording();
    /// Stops the sources, and any recording, until startVideo(). The active vehicle's camera is told to stop
    /// transmitting its streams meanwhile, as it is while the video is off for a disarmed vehicle.
    Q_INVOKABLE void stopVideo();
    /// Makes @p source, one shown in a tile, the main picture. A stream of a vehicle's camera becomes that camera's
    /// current stream, and its camera the current camera, as the camera panel selects them. The USB camera takes
    /// the main picture from the stream, which takes its tile, until a stream is selected again or another camera is
    /// chosen for the tile: VideoSettings::uvcTileCameraAsMainPicture stores it.
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

    VideoStreamController* mainSource() const { return isUvc() ? _cameraSource : _primaryStream; }

    UVCCameraController* uvcCamera() const { return _uvcCamera; }

    bool autoStreamConfigured() const { return _autoStreamConfigured; }

    bool decoding() const;

    bool degraded() const { return _degraded; }

    bool fullScreen() const { return _fullScreen; }

    bool hasThermal() const { return _hasThermal; }

    bool hasUvcCamera() const { return _hasUvcCamera; }

    bool hasVideo() const { return _hasVideo; }

    bool isStreamSource() const { return _isStreamSource; }

    bool isUvc() const { return !_uvcVideoSourceID.isEmpty(); }

    VideoStreamController* measuredSource() const { return _measuredSource; }

    NoVideoReason noVideoReason() const { return _noVideoReason; }

    bool recording() const;

    int tileCount() const { return _tileCount; }

    int tileRoom() const { return _tileRoom; }

    int idleSourceGraceMs() const;

    QString telemetryOverlayText() const;

    void setFullScreen(bool on);
    /// The sources follow on the next pass of the event loop, with the room there is then: a view on its way from the
    /// picture-in-picture to its full size tells of the room it has on the way, which mustn't stop the tiles' streams.
    void setTileRoom(int room);

signals:
    void sourcesChanged();
    void primaryStreamChanged();
    void mainSourceChanged();
    void autoStreamConfiguredChanged();
    void decodingChanged();
    void degradedChanged();
    void fullScreenChanged();
    void hasThermalChanged();
    void hasUvcCameraChanged();
    void hasVideoChanged();
    void isStreamSourceChanged();
    void isUvcChanged();
    void measuredSourceChanged();
    void noVideoReasonChanged();
    void recordingChanged(bool recording);
    void telemetryOverlayTextChanged();
    void tileCountChanged();
    void tileRoomChanged();

private:
    /// A stream a camera of the active vehicle advertises.
    using AdvertisedStream = VehicleVideoStreams::Stream;

    void _connectSettings();
    void _onMainWindowReady();
    /// The backend's initialization is over: applies the decoder setting to a backend that is @p ready, tells the
    /// user of one that failed, and gives the streams their receivers either way.
    void _onBackendInitFinished(bool ready);
    void _createVideoReceivers();
    /// Rebuilds the streams' display sinks and reconnects them; tries a camera that failed again.
    void _restartSources();
    /// Recomputes the derived state and which sources are enabled, after any input changed. A source that lets go of a
    /// receiver that is recording runs the event loop as it does: an input that changes there is taken up by another
    /// pass once this one is over, not within it.
    void _updateState();
    void _updateStateNow();
    /// The active vehicle changed, to none perhaps; its streams follow (_setAdvertisedStreams()).
    void _onVehicleChanged();
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
    /// Tells each camera of the active vehicle which of its streams play besides its current one: those in tiles, and
    /// its thermal stream while the source that plays it wants it. None while the video is stopped.
    void _nameExtraStreams();
    /// VideoSettings::disableWhenDisarmed keeps the video off: the active vehicle is disarmed, and the video wasn't
    /// started again since.
    bool _disabledWhileDisarmed() const;
    void _connectSource(VideoStreamController* source);
    /// In the background (hidden or suspended) no stream is displayed, so none is decoded for display.
    void _onApplicationStateChanged(Qt::ApplicationState state);
    /// Recomputes what the sources amount to together: tileCount, measuredSource and degraded.
    void _updateSourceSummary();
    void _onArmedChanged(bool armed);
    void _communicationLostChanged(bool communicationLost);
    /// Reconnects the streams when @p network, the device's own, comes back: no vehicle's link tells of it without
    /// a vehicle, or with video on another network than the telemetry. init() gives the platform's.
    void _followNetwork(QGCNetworkAvailabilityMonitor* network);
    /// The streams' sources are likely back, with a vehicle that connected or whose link returned, or with the
    /// device's network: a stream waiting to reconnect does so now.
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
    bool _displaySuspended = false;
    VideoStreamController* _measuredSource;
    VideoRecordingService* const _recordingService;
    VideoTelemetryOverlay* const _telemetryOverlay;
    std::function<VideoReceiver*(QObject* parent)> _receiverFactory;
    /// Whether a local camera of that description is attached.
    std::function<bool(const QString& camera)> _cameraAttached;
    /// The active vehicle and its cameras.
    VehicleVideoStreams* const _vehicleStreams;
    /// The streams to show in tiles, in the order they are to fill them.
    QList<AdvertisedStream> _otherStreams;

    VideoBackendInit* const _backendInit;

    bool _hasMainWindow = false;
    bool _cleanedUp = false;
    /// The sources are being gone through, which _updateState() waits out (_updateStatePending).
    bool _updatingState = false;
    bool _updateStatePending = false;
    bool _fullScreen = false;
    bool _videoStopped = false;
    /// How many tile sources there have been, which names the next one.
    int _tileStreamSerial = 0;
    int _tileCount = 0;
    int _tileRoom = -1;
    /// The active vehicle is disarmed; false without one.
    bool _vehicleDisarmed = false;
    /// startVideo() was called for video that was off for a disarmed vehicle: it plays until the vehicle's armed
    /// state changes.
    bool _restartedWhileDisarmed = false;
    bool _autoStreamConfigured = false;
    bool _degraded = false;
    bool _hasThermal = false;
    bool _hasUvcCamera = false;
    bool _hasVideo = false;
    bool _isStreamSource = false;
    NoVideoReason _noVideoReason = NoVideoReason::Waiting;
    QString _uvcVideoSourceID;
};
