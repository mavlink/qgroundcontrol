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
#include "SettingsManager.h"
#include "UVCCameraController.h"
#include "UVCDevices.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "VideoBackend.h"
#include "VideoReceiver.h"
#include "VideoRecordingService.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoStreamResolver.h"

QGC_LOGGING_CATEGORY(VideoManagerLog, "Video.VideoManager")

namespace {

/// How a change to a video setting reaches the streams.
enum class VideoSettingReaction : uint8_t
{
    /// Live: the streams resolve their config again. VideoReceiver restarts for a changed config, except for a change
    /// of autoReconnect alone, which applies at once.
    Reconfigure,
    /// Construct-only property of the display sink: the streams restart, which rebuilds the sink.
    RebuildSink,
    /// The decoder ranks, which are global: once the backend is ready they are applied again, and the streams restart
    /// so their new pipelines pick decoders by them.
    RerankDecoders,
};

struct VideoSettingReactionEntry
{
    Fact* (VideoSettings::*fact)();
    VideoSettingReaction reaction;
};

// Not listed: aspectRatio, read by VideoStreamController::aspectRatio(); videoFit, gridLines and showRecControl, read
// by QML; recordingFormat, enableStorageLimit and maxVideoSize, read when a recording starts; uvcCameraFormat, which
// goes to the USB camera (_connectSettings()).
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
};

}  // namespace

Q_APPLICATION_STATIC(VideoManager, _videoManagerInstance);

VideoManager::VideoManager(QObject* parent)
    : QObject(parent)
    , _videoSettings(SettingsManager::instance()->videoSettings())
    , _primaryStream(new VideoStreamController(VideoStreamController::Role::Primary, _videoSettings, this))
    , _thermalStream(new VideoStreamController(VideoStreamController::Role::Thermal, _videoSettings, this))
    , _uvcCamera(new UVCCameraController(this))
    , _recordingService(new VideoRecordingService(_primaryStream, _thermalStream, _uvcCamera, this))
    , _receiverFactory([](QObject* owner) { return QGCCorePlugin::instance()->createVideoReceiver(owner); })
{
    qCDebug(VideoManagerLog) << this;

    (void) connect(_primaryStream, &VideoStreamController::decodingChanged, this, &VideoManager::decodingChanged);
    for (VideoStreamController* stream : {_primaryStream, _thermalStream}) {
        (void) connect(stream, &VideoStreamController::aspectRatioChanged, this, &VideoManager::aspectRatioChanged);
        (void) connect(stream, &VideoStreamController::streamInfoChanged, this, &VideoManager::_updateState);
        (void) connect(stream, &VideoStreamController::errorOccurred, this, [stream](const QString& message) {
            qCWarning(VideoManagerLog) << stream->name() << message;
            QGC::showAppMessage(message);
        });
    }
    (void) connect(_recordingService, &VideoRecordingService::recordingChanged, this, &VideoManager::recordingChanged);
    (void) connect(_uvcCamera, &UVCCameraController::camerasChanged, this, &VideoManager::_updateState);

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
                    _primaryStream->scheduleApply();
                    _thermalStream->scheduleApply();
                });
                break;
            case VideoSettingReaction::RebuildSink:
                (void) connect(fact, &Fact::rawValueChanged, this, [this] {
                    _primaryStream->restart();
                    _thermalStream->restart();
                });
                break;
            case VideoSettingReaction::RerankDecoders:
                (void) connect(fact, &Fact::rawValueChanged, this, [this](const QVariant& value) {
                    // Until then, _onBackendInitComplete() applies the setting.
                    if (_initState == InitState::Ready) {
                        VideoBackend::instance().applyDecoderPriorities(value.toInt());
                    }
                    _primaryStream->restart();
                    _thermalStream->restart();
                });
                break;
        }
    }

    Fact* const uvcCameraFormat = _videoSettings->uvcCameraFormat();
    _uvcCamera->setFormat(uvcCameraFormat->rawValue().toString());
    (void) connect(uvcCameraFormat, &Fact::rawValueChanged, this,
                   [this](const QVariant& value) { _uvcCamera->setFormat(value.toString()); });
}

void VideoManager::startVideoBackendInit()
{
    if (_initState != InitState::NotStarted) {
        return;
    }

    VideoBackend& backend = VideoBackend::instance();
    if (!backend.needsAsyncInit() || VideoBackend::disabledForUnitTests()) {
        _initState = InitState::Ready;
        return;
    }

    _initState = InitState::Pending;
    const VideoBackend::EnvPrepResult envResult = backend.prepareEnvironment();
    _watchBackendInit(QtConcurrent::run([&backend, arguments = QCoreApplication::arguments(), envResult,
                                         debugLevel = VideoBackend::persistedDebugLevel()] {
        return backend.initialize(arguments, envResult, debugLevel);
    }));
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
        return;
    }

    _videoSettings->pruneUnavailableDecoders();
    VideoBackend::instance().applyDecoderPriorities(_videoSettings->forceVideoDecoder()->rawValue().toInt());

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
    if (_initState == InitState::Ready) {
        _createVideoReceivers();
    }
}

void VideoManager::_createVideoReceivers()
{
    for (VideoStreamController* stream : {_primaryStream, _thermalStream}) {
        if (stream->receiver()) {
            continue;
        }
        VideoReceiver* const receiver = _receiverFactory(stream);
        if (!receiver) {
            qCDebug(VideoManagerLog) << "No video receiver for" << stream->name();
            continue;
        }
        stream->setReceiver(receiver);
    }
}

void VideoManager::cleanup()
{
    // Receivers must die here, not in ~VideoManager: that runs inside ~QCoreApplication, where queued events are no
    // longer delivered, so a receiver's worker thread never gets its quit task and the destructor waits forever.
    _primaryStream->setReceiver(nullptr);
    _thermalStream->setReceiver(nullptr);
    // Qt Multimedia's backend goes away with the application, before ~VideoManager; it writes a recording's file on a
    // thread of its own, which the exit would cut short.
    _uvcCamera->stopRecordingAndWait(5000);
    _uvcCamera->setDevice(QString());
}

void VideoManager::startVideo()
{
    qCDebug(VideoManagerLog) << "startVideo";

    _videoStopped = false;
    _disabledWhileDisarmed = false;
    _updateState();
    _primaryStream->restart();
    _thermalStream->restart();
}

void VideoManager::stopVideo()
{
    qCDebug(VideoManagerLog) << "stopVideo";

    _videoStopped = true;
    _primaryStream->stop();
    _thermalStream->stop();
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
    QString file = imageFile;
    if (file.isEmpty()) {
        file = SettingsManager::instance()->appSettings()->photoSavePath() + QStringLiteral("/") +
               QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_hh.mm.ss.zzz")) +
               QStringLiteral(".jpg");
    }

    if (_primaryStream->canTakeScreenshot()) {
        (void) _primaryStream->takeScreenshot(file);
    } else if (_uvcCamera->canTakeScreenshot()) {
        (void) _uvcCamera->takeScreenshot(file);
    } else {
        qCDebug(VideoManagerLog) << "No video on display to take a screenshot from";
    }

    if (_thermalStream->canTakeScreenshot()) {
        (void) _thermalStream->takeScreenshot(_thermalStream->taggedFile(file));
    }
}

bool VideoManager::decoding() const
{
    return _primaryStream->decoding();
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

void VideoManager::setfullScreen(bool on)
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
    if (!_videoSettings->disableWhenDisarmed()->rawValue().toBool()) {
        _disabledWhileDisarmed = false;
    }

    const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    const bool streamWanted = _videoSettings->streamEnabled()->rawValue().toBool() && !_disabledWhileDisarmed;
    const bool autoStreamConfigured = VideoStreamResolver::isAutoStream(_primaryStream->streamInfo());
    const bool isStreamSource = VideoStreamResolver::isNetworkSource(videoSource) || autoStreamConfigured;
    const bool cameraSource = streamWanted && !isStreamSource;
    if (cameraSource) {
        // A camera plugged in or unplugged changes uvcVideoSourceID.
        _uvcCamera->watchCameras();
    }
    const QString uvcVideoSourceID = (cameraSource && UVCDevices::deviceExists(videoSource)) ? videoSource : QString();
    const bool hasVideo = streamWanted && (_primaryStream->configured() || !uvcVideoSourceID.isEmpty());
    const QGCVideoStreamInfo* const thermalStreamInfo = _thermalStream->streamInfo();
    const bool hasThermal = thermalStreamInfo && thermalStreamInfo->isThermal();

    _primaryStream->setEnabled(streamWanted && !_videoStopped);
    _thermalStream->setEnabled(streamWanted && !_videoStopped);

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
        _uvcCamera->setDevice(_uvcVideoSourceID);
        emit uvcVideoSourceIDChanged();
        emit isUvcChanged();
    }
    if (hasVideo != _hasVideo) {
        _hasVideo = hasVideo;
        emit hasVideoChanged();
        if (!_hasVideo) {
            // The Fly View swaps the video out of its PiP when hasVideo drops; nothing in QML could leave full screen.
            setfullScreen(false);
        }
    }
    if (hasThermal != _hasThermal) {
        _hasThermal = hasThermal;
        emit hasThermalChanged();
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
    } else {
        setfullScreen(false);
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
        setfullScreen(false);
    }
}
