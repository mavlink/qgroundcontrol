#include "VideoManager.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
#include "MavlinkCameraControlInterface.h"
#include "MultiVehicleManager.h"
#include "QGCCameraManager.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "QtMultimediaVideoReceiver.h"
#include "QtVideoRecorder.h"
#include "SettingsManager.h"
#include "UVCCameraController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "VideoBackend.h"
#include "VideoReceiver.h"
#include "VideoRecordingService.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoStreamResolver.h"
#include "VideoTelemetryOverlay.h"

QGC_LOGGING_CATEGORY(VideoManagerLog, "Video.VideoManager")

namespace {

/// How a change to a video setting reaches the sources.
enum class VideoSettingReaction : uint8_t
{
    /// Live: the sources resolve their config again. VideoReceiver restarts for a changed config, except for a change
    /// of autoReconnect alone, which applies at once.
    Reconfigure,
    /// Construct-only property of the display sink: the sources restart, which rebuilds a stream's sink.
    RebuildSink,
    /// The decoder ranks, which are global: once the backend is ready they are applied again, and the sources restart
    /// so the streams' new pipelines pick decoders by them.
    RerankDecoders,
    /// Which receiver plays the streams: each stream gets a new one.
    RecreateReceivers,
};

struct VideoSettingReactionEntry
{
    Fact* (VideoSettings::*fact)();
    VideoSettingReaction reaction;
};

// Not listed: aspectRatio, read by VideoStreamController::aspectRatio(); videoFit, gridLines and showRecControl, read
// by QML; showTelemetryOverlay, read by VideoTelemetryOverlay; recordingFormat, enableStorageLimit and maxVideoSize,
// read when a recording starts; uvcCameraFormat, uvcCameraRotation, uvcCameraMirror, uvcWhiteBalance, uvcZoom,
// uvcExposureCompensation, uvcFocusMode, uvcRecordingCodec and uvcRecordingQuality, which go to the USB camera
// (_connectSettings()).
constexpr VideoSettingReactionEntry kVideoSettingReactions[] = {
    {&VideoSettings::videoSource, VideoSettingReaction::Reconfigure},
    {&VideoSettings::udpUrl, VideoSettingReaction::Reconfigure},
    {&VideoSettings::tcpUrl, VideoSettingReaction::Reconfigure},
    {&VideoSettings::rtspUrl, VideoSettingReaction::Reconfigure},
    {&VideoSettings::rtspTimeout, VideoSettingReaction::Reconfigure},
    {&VideoSettings::lowLatencyMode, VideoSettingReaction::Reconfigure},
    {&VideoSettings::rtpJitterLatencyMs, VideoSettingReaction::Reconfigure},
    {&VideoSettings::rtspAutoReconnect, VideoSettingReaction::Reconfigure},
    {&VideoSettings::streamEnabled, VideoSettingReaction::Reconfigure},
    {&VideoSettings::disableWhenDisarmed, VideoSettingReaction::Reconfigure},
    {&VideoSettings::videoConversionElement, VideoSettingReaction::RebuildSink},
    {&VideoSettings::disablePixelAspectRatio, VideoSettingReaction::RebuildSink},
    {&VideoSettings::forceCpuVideoPath, VideoSettingReaction::RebuildSink},
    {&VideoSettings::forceVideoDecoder, VideoSettingReaction::RerankDecoders},
    {&VideoSettings::videoBackend, VideoSettingReaction::RecreateReceivers},
};

}  // namespace

Q_APPLICATION_STATIC(VideoManager, _videoManagerInstance);

VideoManager::VideoManager(QObject* parent)
    : QObject(parent)
    , _videoSettings(SettingsManager::instance()->videoSettings())
    , _uvcCamera(new UVCCameraController(this))
    , _primaryStream(new VideoStreamController(VideoStreamController::Role::Primary, _videoSettings, this))
    , _thermalStream(new VideoStreamController(VideoStreamController::Role::Thermal, _videoSettings, this))
    , _cameraSource(new VideoStreamController(VideoStreamController::Role::Camera, _videoSettings, this, _uvcCamera))
    , _sources{_primaryStream, _thermalStream, _cameraSource}
    , _recordingService(new VideoRecordingService(_sources, this))
    , _telemetryOverlay(new VideoTelemetryOverlay(_videoSettings->showTelemetryOverlay(), this))
    , _receiverFactory([this](QObject* owner) -> VideoReceiver* {
        if (_videoSettings->videoBackend()->rawValue().toInt() == VideoSettings::VideoBackendQtMultimedia) {
            return new QtMultimediaVideoReceiver(owner);
        }
        return QGCCorePlugin::instance()->createVideoReceiver(owner);
    })
{
    qCDebug(VideoManagerLog) << this;

    (void) connect(_primaryStream, &VideoStreamController::decodingChanged, this, &VideoManager::decodingChanged);
    for (VideoStreamController* source : _sources) {
        (void) connect(source, &VideoStreamController::aspectRatioChanged, this, &VideoManager::aspectRatioChanged);
        (void) connect(source, &VideoStreamController::streamInfoChanged, this, &VideoManager::_updateState);
        (void) connect(source, &VideoStreamController::errorOccurred, this, [source](const QString& message) {
            qCWarning(VideoManagerLog) << source->name() << message;
            QGC::showAppMessage(message);
        });
    }
    (void) connect(_recordingService, &VideoRecordingService::recordingChanged, this, &VideoManager::recordingChanged);
    // A camera plugged in or unplugged changes whether the camera source has one to run.
    (void) connect(_uvcCamera, &UVCCameraController::camerasChanged, this, [this] {
        _updateState();
        _cameraSource->scheduleApply();
    });
    (void) connect(_telemetryOverlay, &VideoTelemetryOverlay::textChanged, this,
                   &VideoManager::telemetryOverlayTextChanged);

    _connectSettings();
    _updateState();
}

VideoManager::~VideoManager()
{
    qCDebug(VideoManagerLog) << this;
}

VideoManager* VideoManager::instance()
{
    return _videoManagerInstance();
}

void VideoManager::_connectSettings()
{
    for (const VideoSettingReactionEntry& entry : kVideoSettingReactions) {
        Fact* const fact = (_videoSettings->*entry.fact)();
        switch (entry.reaction) {
            case VideoSettingReaction::Reconfigure:
                (void) connect(fact, &Fact::rawValueChanged, this, [this] {
                    _updateState();
                    for (VideoStreamController* source : _sources) {
                        source->scheduleApply();
                    }
                });
                break;
            case VideoSettingReaction::RebuildSink:
                (void) connect(fact, &Fact::rawValueChanged, this, &VideoManager::_restartSources);
                break;
            case VideoSettingReaction::RecreateReceivers:
                (void) connect(fact, &Fact::rawValueChanged, this, [this] {
                    for (VideoStreamController* source : _sources) {
                        source->setReceiver(nullptr);
                    }
                    if (_backendInitFinished() && _hasMainWindow) {
                        _createVideoReceivers();
                    }
                });
                break;
            case VideoSettingReaction::RerankDecoders:
                (void) connect(fact, &Fact::rawValueChanged, this, [this](const QVariant& value) {
                    // Until then, _onBackendInitComplete() applies the setting.
                    if (_initState == InitState::Ready) {
                        VideoBackend::instance().applyDecoderPriorities(videoDecoderOptionFromSetting(value.toInt()));
                    }
                    _restartSources();
                });
                break;
        }
    }

    Fact* const uvcCameraFormat = _videoSettings->uvcCameraFormat();
    _uvcCamera->setFormat(uvcCameraFormat->rawValue().toString());
    (void) connect(uvcCameraFormat, &Fact::rawValueChanged, this,
                   [this](const QVariant& value) { _uvcCamera->setFormat(value.toString()); });

    const auto applyUvcTransform = [this] {
        // uvcCameraRotation's values are degrees, as QtVideo::Rotation's are.
        _uvcCamera->setTransform(
            static_cast<QtVideo::Rotation>(_videoSettings->uvcCameraRotation()->rawValue().toInt()),
            _videoSettings->uvcCameraMirror()->rawValue().toBool());
    };
    applyUvcTransform();
    (void) connect(_videoSettings->uvcCameraRotation(), &Fact::rawValueChanged, this, applyUvcTransform);
    (void) connect(_videoSettings->uvcCameraMirror(), &Fact::rawValueChanged, this, applyUvcTransform);

    const auto applyUvcControls = [this] {
        UVCCameraController::Controls controls;
        // uvcWhiteBalance's and uvcFocusMode's values are QCamera's.
        controls.whiteBalance =
            static_cast<QCamera::WhiteBalanceMode>(_videoSettings->uvcWhiteBalance()->rawValue().toInt());
        controls.zoom = _videoSettings->uvcZoom()->rawValue().toFloat();
        controls.exposureCompensation = _videoSettings->uvcExposureCompensation()->rawValue().toFloat();
        controls.focusMode = static_cast<QCamera::FocusMode>(_videoSettings->uvcFocusMode()->rawValue().toInt());
        _uvcCamera->setControls(controls);
    };
    applyUvcControls();
    for (Fact* const fact : {_videoSettings->uvcWhiteBalance(), _videoSettings->uvcZoom(),
                             _videoSettings->uvcExposureCompensation(), _videoSettings->uvcFocusMode()}) {
        (void) connect(fact, &Fact::rawValueChanged, this, applyUvcControls);
    }

    const auto applyUvcRecordingEncoding = [this] {
        _uvcCamera->setRecordingEncoding(
            QtVideoRecorder::recordingCodec(_videoSettings->uvcRecordingCodec()->rawValue().toString()),
            QtVideoRecorder::recordingQuality(_videoSettings->uvcRecordingQuality()->rawValue().toInt()));
    };
    applyUvcRecordingEncoding();
    (void) connect(_videoSettings->uvcRecordingCodec(), &Fact::rawValueChanged, this, applyUvcRecordingEncoding);
    (void) connect(_videoSettings->uvcRecordingQuality(), &Fact::rawValueChanged, this, applyUvcRecordingEncoding);
}

void VideoManager::startVideoBackendInit()
{
    if (_initState != InitState::NotStarted) {
        return;
    }

    VideoBackend& backend = VideoBackend::instance();
    if (!backend.capabilities().asyncInit || VideoBackend::disabledForUnitTests()) {
        _initState = InitState::Ready;
        return;
    }

    _initState = InitState::Pending;
    const VideoBackend::InitParams params{.arguments = QCoreApplication::arguments(),
                                          .environment = backend.prepareEnvironment(),
                                          .debugLevel = VideoBackend::persistedDebugLevel()};
    _watchBackendInit(QtConcurrent::run([&backend, params] { return backend.initialize(params); }));
}

void VideoManager::_watchBackendInit(const QFuture<bool>& future)
{
    _backendInitFuture = future;
    // Ignored when waitForVideoBackendReady() already took the result.
    (void) _backendInitFuture.then(this, [this](bool success) { _onBackendInitComplete(success); })
        .onCanceled(this, [this] { _onBackendInitComplete(false); });
}

bool VideoManager::waitForVideoBackendReady()
{
    if (_initState == InitState::NotStarted) {
        startVideoBackendInit();
    }

    if (_initState == InitState::Pending) {
        _backendInitFuture.waitForFinished();
        const bool success =
            !_backendInitFuture.isCanceled() && (_backendInitFuture.resultCount() > 0) && _backendInitFuture.result();
        _onBackendInitComplete(success);
    }

    return _initState == InitState::Ready;
}

void VideoManager::_onBackendInitComplete(bool success)
{
    if (_initState != InitState::Pending) {
        return;
    }

    if (!success) {
        _initState = InitState::Failed;
        qCCritical(VideoManagerLog) << "video initialization failed";
        QGC::showAppMessage(VideoBackend::instance().initFailureMessage());
        // The backend then makes the receivers that need no initialization.
        if (_hasMainWindow) {
            _createVideoReceivers();
        }
        return;
    }

    _videoSettings->pruneUnavailableDecoders();
    VideoBackend::instance().applyDecoderPriorities(
        videoDecoderOptionFromSetting(_videoSettings->forceVideoDecoder()->rawValue().toInt()));

    _initState = InitState::Ready;
    qCDebug(VideoManagerLog) << "video backend ready";
    if (_hasMainWindow) {
        _createVideoReceivers();
    }
}

void VideoManager::init(QQuickWindow* mainWindow)
{
    if (_hasMainWindow) {
        qCDebug(VideoManagerLog) << "Video Manager already initialized";
        return;
    }

    if (!mainWindow) {
        qCCritical(VideoManagerLog) << "Failed To Init Video Manager - mainWindow is NULL";
        return;
    }

    VideoBackend::instance().onMainWindowReady(mainWindow);
    VideoBackend::bindDebugLevelFact(SettingsManager::instance()->appSettings()->gstDebugLevel(), this);

    MultiVehicleManager* const multiVehicleManager = MultiVehicleManager::instance();
    (void) connect(multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this,
                   &VideoManager::_setActiveVehicle);
    _setActiveVehicle(multiVehicleManager->activeVehicle());

    _onMainWindowReady();
}

void VideoManager::_onMainWindowReady()
{
    _hasMainWindow = true;
    // The receivers need the backend and nothing from QML: the VideoOutputs register themselves whenever they exist.
    startVideoBackendInit();
    if (_backendInitFinished()) {
        _createVideoReceivers();
    }
}

void VideoManager::_createVideoReceivers()
{
    if (_cleanedUp) {
        return;
    }
    for (VideoStreamController* source : _sources) {
        // The camera source has its own.
        if (source->receiver()) {
            continue;
        }
        VideoReceiver* const receiver = _receiverFactory(source);
        if (!receiver) {
            qCDebug(VideoManagerLog) << "No video receiver for" << source->name();
            continue;
        }
        source->setReceiver(receiver);
    }
}

void VideoManager::_restartSources()
{
    for (VideoStreamController* source : _sources) {
        source->restart();
    }
}

void VideoManager::cleanup()
{
    // Latched first: the waits below run the event loop, and what it delivers (the backend's init finishing, a setting
    // or a camera changing) must not bring back what this takes down.
    _cleanedUp = true;
    // Receivers must die here, not in ~VideoManager: that runs inside ~QCoreApplication, where queued events are no
    // longer delivered, so a receiver's worker thread never gets its quit task and the destructor waits forever. The
    // camera stops here too: Qt Multimedia's backend goes away with the application, before ~VideoManager.
    for (VideoStreamController* source : _sources) {
        source->shutDown();
    }
}

void VideoManager::startVideo()
{
    qCDebug(VideoManagerLog) << "startVideo";

    _videoStopped = false;
    _disabledWhileDisarmed = false;
    _updateState();
    _restartSources();
}

void VideoManager::stopVideo()
{
    qCDebug(VideoManagerLog) << "stopVideo";

    _videoStopped = true;
    _updateState();
    for (VideoStreamController* source : _sources) {
        source->stop();
    }
}

void VideoManager::startRecording(const QString& baseName)
{
    _recordingService->startRecording(baseName);
}

void VideoManager::stopRecording()
{
    _recordingService->stopRecording();
}

void VideoManager::grabImage(const QString& imageFile)
{
    // One reading of the clock names the file and dates its EXIF data.
    const QDateTime now = QDateTime::currentDateTime();
    QString file = imageFile;
    if (file.isEmpty()) {
        const QString directory = SettingsManager::instance()->appSettings()->photoSavePath();
        if (directory.isEmpty()) {
            QGC::showAppMessage(tr("Unable to save the photo. The save path must be specified in Settings."));
            return;
        }
        file = QStringLiteral("%1/%2.jpg").arg(directory, now.toString(QStringLiteral("yyyy-MM-dd_hh.mm.ss.zzz")));
    }

    VideoScreenshot::Exif exif;
    exif.taken = now;
    if (_activeVehicle) {
        exif.position = _activeVehicle->coordinate();
    }

    bool taken = false;
    for (VideoStreamController* source : _sources) {
        if (source->canTakeScreenshot()) {
            taken = source->takeScreenshot(source->taggedFile(file), exif) || taken;
        }
    }
    // What goes wrong from here on, the source reports.
    if (!taken) {
        qCWarning(VideoManagerLog) << "No video on display to take a screenshot from";
        QGC::showAppMessage(tr("Unable to take a photo: no video is on display."));
    }
}

QQmlListProperty<VideoStreamController> VideoManager::sources()
{
    using Sources = QQmlListProperty<VideoStreamController>;
    // Read-only: QML can't add or remove a source.
    return Sources(
        this, nullptr, [](Sources* sources) { return static_cast<VideoManager*>(sources->object)->_sources.size(); },
        [](Sources* sources, qsizetype index) {
            return static_cast<VideoManager*>(sources->object)->_sources.at(index);
        });
}

bool VideoManager::decoding() const
{
    return _primaryStream->decoding();
}

QString VideoManager::telemetryOverlayText() const
{
    return _telemetryOverlay->text();
}

bool VideoManager::recording() const
{
    return _recordingService->recording();
}

double VideoManager::aspectRatio() const
{
    return _primaryStream->aspectRatio();
}

double VideoManager::thermalAspectRatio() const
{
    return _thermalStream->aspectRatio();
}

void VideoManager::setFullScreen(bool on)
{
    if (on && (!_hasVideo || !_activeVehicle || _activeVehicle->vehicleLinkManager()->communicationLost())) {
        on = false;
    }

    if (on != _fullScreen) {
        _fullScreen = on;
        emit fullScreenChanged();
    }
}

void VideoManager::_updateState()
{
    if (_cleanedUp) {
        return;
    }
    if (!_videoSettings->disableWhenDisarmed()->rawValue().toBool()) {
        _disabledWhileDisarmed = false;
    }

    const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    const bool streamEnabled = _videoSettings->streamEnabled()->rawValue().toBool();
    const bool streamWanted = streamEnabled && !_disabledWhileDisarmed;
    // The reason that takes the most to lift comes first: arming doesn't bring back video that was stopped.
    const NoVideoReason noVideoReason = !streamEnabled           ? NoVideoReason::Disabled
                                        : _videoStopped          ? NoVideoReason::Stopped
                                        : _disabledWhileDisarmed ? NoVideoReason::DisabledWhileDisarmed
                                                                 : NoVideoReason::Waiting;
    const bool autoStreamConfigured = VideoStreamResolver::isAutoStream(_primaryStream->streamInfo());
    const bool isStreamSource = VideoStreamResolver::isNetworkSource(videoSource) || autoStreamConfigured;
    // A stream to play takes the place of the camera.
    const bool cameraWanted = streamWanted && !isStreamSource;
    if (cameraWanted) {
        // A camera plugged in or unplugged changes uvcVideoSourceID.
        _uvcCamera->watchCameras();
    }
    const QString uvcVideoSourceID = (cameraWanted && _cameraSource->configured()) ? videoSource : QString();
    const bool hasVideo = streamWanted && (_primaryStream->configured() || !uvcVideoSourceID.isEmpty());
    const QGCVideoStreamInfo* const thermalStreamInfo = _thermalStream->streamInfo();
    const bool hasThermal = thermalStreamInfo && thermalStreamInfo->isThermal();

    for (VideoStreamController* source : _sources) {
        source->setEnabled(streamWanted && !_videoStopped && ((source != _cameraSource) || cameraWanted));
    }

    if (autoStreamConfigured != _autoStreamConfigured) {
        _autoStreamConfigured = autoStreamConfigured;
        emit autoStreamConfiguredChanged();
    }
    if (isStreamSource != _isStreamSource) {
        _isStreamSource = isStreamSource;
        emit isStreamSourceChanged();
    }
    if (uvcVideoSourceID != _uvcVideoSourceID) {
        qCDebug(VideoManagerLog) << "UVC changed from [" << _uvcVideoSourceID << "] to [" << uvcVideoSourceID << "]";
        _uvcVideoSourceID = uvcVideoSourceID;
        emit uvcVideoSourceIDChanged();
        emit isUvcChanged();
    }
    if (hasVideo != _hasVideo) {
        _hasVideo = hasVideo;
        emit hasVideoChanged();
        if (!_hasVideo) {
            // The Fly View swaps the video out of its PiP when hasVideo drops; nothing in QML could leave full screen.
            setFullScreen(false);
        }
    }
    if (hasThermal != _hasThermal) {
        _hasThermal = hasThermal;
        emit hasThermalChanged();
    }
    if (noVideoReason != _noVideoReason) {
        _noVideoReason = noVideoReason;
        emit noVideoReasonChanged();
    }
}

void VideoManager::_setActiveVehicle(Vehicle* vehicle)
{
    qCDebug(VideoManagerLog) << "new vehicle" << vehicle << "old active vehicle" << _activeVehicle;

    if (_activeVehicle) {
        (void) disconnect(_activeVehicle, nullptr, this, nullptr);
        (void) disconnect(_activeVehicle->vehicleLinkManager(), nullptr, this, nullptr);
        QGCCameraManager* const cameraManager = _activeVehicle->cameraManager();
        if (cameraManager) {
            (void) disconnect(cameraManager, nullptr, this, nullptr);
            MavlinkCameraControlInterface* const camera = cameraManager->currentCameraInstance();
            if (camera) {
                camera->stopStream();
            }
        }
    }

    _activeVehicle = vehicle;
    _disabledWhileDisarmed = false;

    if (_activeVehicle) {
        (void) connect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this,
                       &VideoManager::_communicationLostChanged);
        (void) connect(_activeVehicle, &Vehicle::armedChanged, this, &VideoManager::_onArmedChanged);
        QGCCameraManager* const cameraManager = _activeVehicle->cameraManager();
        if (cameraManager) {
            (void) connect(cameraManager, &QGCCameraManager::streamChanged, this, &VideoManager::_updateStreamInfo);
            MavlinkCameraControlInterface* const camera = cameraManager->currentCameraInstance();
            if (camera) {
                camera->resumeStream();
            }
        }
        _reconnectStreams();
    } else {
        setFullScreen(false);
    }

    _updateStreamInfo();
}

void VideoManager::_updateStreamInfo()
{
    QGCCameraManager* const cameraManager = _activeVehicle ? _activeVehicle->cameraManager() : nullptr;
    _primaryStream->setStreamInfo(cameraManager ? cameraManager->currentStreamInstance() : nullptr);
    _thermalStream->setStreamInfo(cameraManager ? cameraManager->thermalStreamInstance() : nullptr);
    _updateState();
}

void VideoManager::_onArmedChanged(bool armed)
{
    const bool disabled = !armed && _videoSettings->disableWhenDisarmed()->rawValue().toBool();
    if (disabled != _disabledWhileDisarmed) {
        qCDebug(VideoManagerLog) << "disabled while disarmed:" << disabled;
        _disabledWhileDisarmed = disabled;
        _updateState();
    }
}

void VideoManager::_communicationLostChanged(bool communicationLost)
{
    if (communicationLost) {
        setFullScreen(false);
    } else {
        _reconnectStreams();
    }
}

void VideoManager::_reconnectStreams()
{
    for (VideoStreamController* source : _sources) {
        source->reconnectNow();
    }
}
