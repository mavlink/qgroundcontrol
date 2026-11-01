#pragma once

#include <functional>

#include <QtCore/QFuture>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class QQuickWindow;
class UVCCameraController;
class Vehicle;
class VideoReceiver;
class VideoRecordingService;
class VideoSettings;
class VideoStreamController;
class VideoTelemetryOverlay;

/// QML facade of the video streams, each a VideoStreamController (primary, thermal). decoding and aspectRatio forward
/// to the primary stream, thermalAspectRatio to the thermal stream, and recording covers both streams. uvcCamera
/// runs the USB camera on display. The other properties, such as hasVideo and isUvc, are the manager's own state.
class VideoManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("UVCCameraController.h")
    Q_MOC_INCLUDE("VideoStreamController.h")

    Q_PROPERTY(VideoStreamController* primaryStream READ primaryStream CONSTANT)
    Q_PROPERTY(VideoStreamController* thermalStream READ thermalStream CONSTANT)
    Q_PROPERTY(UVCCameraController* uvcCamera READ uvcCamera CONSTANT)
    Q_PROPERTY(bool autoStreamConfigured READ autoStreamConfigured NOTIFY autoStreamConfiguredChanged)
    Q_PROPERTY(bool decoding READ decoding NOTIFY decodingChanged)
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setfullScreen NOTIFY fullScreenChanged)
    Q_PROPERTY(bool hasThermal READ hasThermal NOTIFY hasThermalChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(bool isStreamSource READ isStreamSource NOTIFY isStreamSourceChanged)
    Q_PROPERTY(bool isUvc READ isUvc NOTIFY isUvcChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(double aspectRatio READ aspectRatio NOTIFY aspectRatioChanged)
    Q_PROPERTY(double thermalAspectRatio READ thermalAspectRatio NOTIFY aspectRatioChanged)
    Q_PROPERTY(QString uvcVideoSourceID READ uvcVideoSourceID NOTIFY uvcVideoSourceIDChanged)
    /// The telemetry the primary stream's and the USB camera's outputs show as their subtitle: empty while
    /// VideoSettings::showTelemetryOverlay is off.
    Q_PROPERTY(QString telemetryOverlayText READ telemetryOverlayText NOTIFY telemetryOverlayTextChanged)

    friend class VideoManagerInitTest;
    friend class VideoManagerTest;

public:
    explicit VideoManager(QObject* parent = nullptr);
    ~VideoManager() override;

    static VideoManager* instance();

    /// Saves the frame on display to @p imageFile (default: a timestamped file in the photo directory), and the
    /// thermal stream's frame, when it is streaming, to the same name tagged ".thermalVideo".
    Q_INVOKABLE void grabImage(const QString& imageFile = QString());
    /// See VideoRecordingService::startRecording().
    Q_INVOKABLE void startRecording(const QString& baseName = QString());
    /// Undoes stopVideo() and "disable when disarmed", and reconnects the streams.
    Q_INVOKABLE void startVideo();
    Q_INVOKABLE void stopRecording();
    /// Stops the streams, and any recording, until startVideo().
    Q_INVOKABLE void stopVideo();

    void init(QQuickWindow* mainWindow);
    /// Starts the video backend's asynchronous global init. GUI thread only, like everything else here.
    void startVideoBackendInit();
    /// Blocks until the backend init finishes, starting it if needed. True when the backend is ready.
    bool waitForVideoBackendReady();
    /// Deletes the receivers. Call before the QML engine and the application go away.
    void cleanup();

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

    bool recording() const;
    double aspectRatio() const;
    double thermalAspectRatio() const;

    QString uvcVideoSourceID() const { return _uvcVideoSourceID; }

    QString telemetryOverlayText() const;

    void setfullScreen(bool on);

signals:
    void aspectRatioChanged();
    void autoStreamConfiguredChanged();
    void decodingChanged();
    void fullScreenChanged();
    void hasThermalChanged();
    void hasVideoChanged();
    void isStreamSourceChanged();
    void isUvcChanged();
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
    void _createVideoReceivers();
    /// Recomputes the derived state and what each stream should do, after any input changed.
    void _updateState();
    void _setActiveVehicle(Vehicle* vehicle);
    void _updateStreamInfo();
    void _onArmedChanged(bool armed);
    void _communicationLostChanged(bool communicationLost);

    VideoSettings* const _videoSettings;
    VideoStreamController* const _primaryStream;
    VideoStreamController* const _thermalStream;
    UVCCameraController* const _uvcCamera;
    VideoRecordingService* const _recordingService;
    VideoTelemetryOverlay* const _telemetryOverlay;
    std::function<VideoReceiver*(QObject* parent)> _receiverFactory;
    QPointer<Vehicle> _activeVehicle;

    InitState _initState = InitState::NotStarted;
    QFuture<bool> _backendInitFuture;

    bool _hasMainWindow = false;
    bool _fullScreen = false;
    bool _videoStopped = false;
    bool _disabledWhileDisarmed = false;
    bool _autoStreamConfigured = false;
    bool _hasThermal = false;
    bool _hasVideo = false;
    bool _isStreamSource = false;
    QString _uvcVideoSourceID;
};
