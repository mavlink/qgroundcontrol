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

class QQuickWindow;
class UVCCameraController;
class Vehicle;
class VideoReceiver;
class VideoRecordingService;
class VideoSettings;
class VideoStreamController;
class VideoTelemetryOverlay;

/// QML facade of the video sources, each a VideoStreamController: the primary stream, the thermal stream and the USB
/// camera, whose receiver is uvcCamera. One of the primary stream and the camera is on display: the camera while the
/// video source setting names one and no vehicle camera advertises a stream. decoding and aspectRatio forward to the
/// primary stream, thermalAspectRatio to the thermal stream, and recording covers every source. The other properties,
/// such as hasVideo and isUvc, are the manager's own state.
class VideoManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("UVCCameraController.h")
    Q_MOC_INCLUDE("VideoStreamController.h")

    /// Every source, for what treats them alike: the primary stream, the thermal stream, then the USB camera.
    Q_PROPERTY(QQmlListProperty<VideoStreamController> sources READ sources CONSTANT)
    Q_PROPERTY(VideoStreamController* primaryStream READ primaryStream CONSTANT)
    Q_PROPERTY(VideoStreamController* thermalStream READ thermalStream CONSTANT)
    Q_PROPERTY(UVCCameraController* uvcCamera READ uvcCamera CONSTANT)
    Q_PROPERTY(bool autoStreamConfigured READ autoStreamConfigured NOTIFY autoStreamConfiguredChanged)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged)
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY fullScreenChanged)
    Q_PROPERTY(bool hasThermal READ hasThermal NOTIFY hasThermalChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(bool isStreamSource READ isStreamSource NOTIFY isStreamSourceChanged)
    Q_PROPERTY(bool isUvc READ isUvc NOTIFY isUvcChanged)
    Q_PROPERTY(NoVideoReason noVideoReason READ noVideoReason NOTIFY noVideoReasonChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(double aspectRatio READ aspectRatio NOTIFY aspectRatioChanged)
    Q_PROPERTY(double thermalAspectRatio READ thermalAspectRatio NOTIFY aspectRatioChanged)
    Q_PROPERTY(QString uvcVideoSourceID READ uvcVideoSourceID NOTIFY uvcVideoSourceIDChanged)
    /// The telemetry VideoView shows over the primary stream or the USB camera: empty while
    /// VideoSettings::showTelemetryOverlay is off.
    Q_PROPERTY(QString telemetryOverlayText READ telemetryOverlayText NOTIFY telemetryOverlayTextChanged)

    friend class VideoManagerInitTest;
    friend class VideoManagerTest;

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

    void _connectSettings();
    void _onMainWindowReady();
    void _watchBackendInit(const QFuture<bool>& future);
    void _onBackendInitComplete(bool success);

    /// Ready or failed: either way the backend makes receivers, after a failure the ones that need no initialization.
    bool _backendInitFinished() const { return (_initState == InitState::Ready) || (_initState == InitState::Failed); }

    void _createVideoReceivers();
    /// Rebuilds the streams' display sinks and reconnects them; tries a camera that failed again.
    void _restartSources();
    /// Recomputes the derived state and which sources are enabled, after any input changed.
    void _updateState();
    void _setActiveVehicle(Vehicle* vehicle);
    void _updateStreamInfo();
    void _onArmedChanged(bool armed);
    void _communicationLostChanged(bool communicationLost);
    /// The streams' sources are likely back, with a vehicle that connected or whose link returned: a stream waiting
    /// to reconnect does so now.
    void _reconnectStreams();

    VideoSettings* const _videoSettings;
    /// The camera source's receiver.
    UVCCameraController* const _uvcCamera;
    VideoStreamController* const _primaryStream;
    VideoStreamController* const _thermalStream;
    VideoStreamController* const _cameraSource;
    const QList<VideoStreamController*> _sources;
    VideoRecordingService* const _recordingService;
    VideoTelemetryOverlay* const _telemetryOverlay;
    std::function<VideoReceiver*(QObject* parent)> _receiverFactory;
    QPointer<Vehicle> _activeVehicle;

    InitState _initState = InitState::NotStarted;
    QFuture<bool> _backendInitFuture;

    bool _hasMainWindow = false;
    bool _cleanedUp = false;
    bool _fullScreen = false;
    bool _videoStopped = false;
    bool _disabledWhileDisarmed = false;
    bool _autoStreamConfigured = false;
    bool _hasThermal = false;
    bool _hasVideo = false;
    bool _isStreamSource = false;
    NoVideoReason _noVideoReason = NoVideoReason::Waiting;
    QString _uvcVideoSourceID;
};
