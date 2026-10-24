#include "VideoManager.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtQuick/QQuickWindow>

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
};

struct VideoSettingReactionEntry
{
    Fact* (VideoSettings::*fact)();
    VideoSettingReaction reaction;
};

// Not listed: aspectRatio, read by VideoStreamController::aspectRatio(); videoFit, gridLines and showRecControl, read
// by QML; recordingFormat, enableStorageLimit and maxVideoSize, read when a recording starts; forceVideoDecoder and
// forceCpuVideoPath, which take effect after an application restart.
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
    {&VideoSettings::videoConversionElement, VideoSettingReaction::RebuildSink},
    {&VideoSettings::disablePixelAspectRatio, VideoSettingReaction::RebuildSink},
};

}  // namespace

Q_APPLICATION_STATIC(VideoManager, _videoManagerInstance);

VideoManager::VideoManager(QObject* parent)
    : QObject(parent)
    , _videoSettings(SettingsManager::instance()->videoSettings())
    , _primaryStream(new VideoStreamController(VideoStreamController::Role::Primary, _videoSettings, this))
    , _thermalStream(new VideoStreamController(VideoStreamController::Role::Thermal, _videoSettings, this))
    , _recordingService(new VideoRecordingService(_primaryStream, _thermalStream, this))
    , _receiverFactory([](QObject* owner) { return QGCCorePlugin::instance()->createVideoReceiver(owner); })
{
    qCDebug(VideoManagerLog) << this;

    if (VideoBackend::needsAsyncInit()) {
        _backendDisabledForTests = VideoBackend::disabledForUnitTests();
        if (_backendDisabledForTests) {
            qCDebug(VideoManagerLog) << "Skipping video backend initialization for unit tests";
        }
    }

    (void) connect(_primaryStream, &VideoStreamController::streamingChanged, this, &VideoManager::streamingChanged);
    (void) connect(_primaryStream, &VideoStreamController::decodingChanged, this, &VideoManager::decodingChanged);
    (void) connect(_primaryStream, &VideoStreamController::videoSizeChanged, this, &VideoManager::videoSizeChanged);
    for (VideoStreamController* stream : {_primaryStream, _thermalStream}) {
        (void) connect(stream, &VideoStreamController::aspectRatioChanged, this, &VideoManager::aspectRatioChanged);
        (void) connect(stream, &VideoStreamController::hfovChanged, this, &VideoManager::aspectRatioChanged);
        (void) connect(stream, &VideoStreamController::streamInfoChanged, this, &VideoManager::_updateState);
        (void) connect(stream, &VideoStreamController::errorOccurred, this, [stream](const QString& message) {
            qCWarning(VideoManagerLog) << stream->name() << message;
            QGC::showAppMessage(message);
        });
    }
    (void) connect(_recordingService, &VideoRecordingService::recordingChanged, this, &VideoManager::recordingChanged);
    (void) connect(_recordingService, &VideoRecordingService::recordingStarted, this, &VideoManager::recordingStarted);

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
        }
    }
}

void VideoManager::startVideoBackendInit()
{
    if (_initState != InitState::NotStarted) {
        return;
    }

    if (!VideoBackend::needsAsyncInit() || _backendDisabledForTests) {
        _initState = InitState::Ready;
        return;
    }

    _initState = InitState::Pending;
    const VideoBackend::EnvPrepResult envResult = VideoBackend::prepareEnvironment();
    _watchBackendInit(QtConcurrent::run(&VideoBackend::initialize, QCoreApplication::arguments(), envResult));
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
    VideoBackend::applyDecoderPriorities(_videoSettings->forceVideoDecoder()->rawValue().toInt());

    _initState = InitState::Ready;
    qCDebug(VideoManagerLog) << "video backend ready";
    if (_mainWindow) {
        _createVideoReceivers();
    }
}

void VideoManager::init(QQuickWindow* mainWindow)
{
    if (_mainWindow) {
        qCDebug(VideoManagerLog) << "Video Manager already initialized";
        return;
    }

    if (!mainWindow) {
        qCCritical(VideoManagerLog) << "Failed To Init Video Manager - mainWindow is NULL";
        return;
    }

    VideoBackend::onMainWindowReady(mainWindow);
    VideoBackend::bindDebugLevelFact(SettingsManager::instance()->appSettings()->gstDebugLevel(), this);

    MultiVehicleManager* const multiVehicleManager = MultiVehicleManager::instance();
    (void) connect(multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this,
                   &VideoManager::_setActiveVehicle);
    _setActiveVehicle(multiVehicleManager->activeVehicle());

    _setMainWindow(mainWindow);
}

void VideoManager::_setMainWindow(QQuickWindow* mainWindow)
{
    _mainWindow = mainWindow;
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
}

void VideoManager::startVideo()
{
    qCDebug(VideoManagerLog) << "startVideo";

    _videoStopped = false;
    _updateState();
    for (VideoStreamController* stream : {_primaryStream, _thermalStream}) {
        stream->scheduleApply();
        stream->restart();
    }
}

void VideoManager::stopVideo()
{
    qCDebug(VideoManagerLog) << "stopVideo";

    _videoStopped = true;
    _primaryStream->stop();
    _thermalStream->stop();
}

void VideoManager::startRecording(const QString& videoFile)
{
    _recordingService->startRecording(videoFile);
}

void VideoManager::stopRecording()
{
    _recordingService->stopRecording();
}

void VideoManager::grabImage(const QString& imageFile)
{
    if (imageFile.isEmpty()) {
        _imageFile = SettingsManager::instance()->appSettings()->photoSavePath() + QStringLiteral("/") +
                     QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_hh.mm.ss.zzz")) +
                     QStringLiteral(".jpg");
    } else {
        _imageFile = imageFile;
    }
    emit imageFileChanged(_imageFile);

    if (_primaryStream->canTakeScreenshot()) {
        (void) _primaryStream->takeScreenshot(_imageFile);
    } else {
        qCDebug(VideoManagerLog) << "No video on display to take a screenshot from";
    }

    if (_thermalStream->canTakeScreenshot()) {
        // As with recordings, the thermal stream's name goes before the extension.
        QString thermalImageFile = _imageFile;
        const qsizetype dot = thermalImageFile.lastIndexOf(QLatin1Char('.'));
        const QString tag = QLatin1Char('.') + _thermalStream->name();
        if (dot > thermalImageFile.lastIndexOf(QLatin1Char('/'))) {
            (void) thermalImageFile.insert(dot, tag);
        } else {
            (void) thermalImageFile.append(tag);
        }
        (void) _thermalStream->takeScreenshot(thermalImageFile);
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

bool VideoManager::streaming() const
{
    return _primaryStream->streaming();
}

QSize VideoManager::videoSize() const
{
    return _primaryStream->videoSize();
}

double VideoManager::aspectRatio() const
{
    return _primaryStream->aspectRatio();
}

double VideoManager::hfov() const
{
    return _primaryStream->hfov();
}

double VideoManager::thermalAspectRatio() const
{
    return _thermalStream->aspectRatio();
}

double VideoManager::thermalHfov() const
{
    return _thermalStream->hfov();
}

void VideoManager::setfullScreen(bool on)
{
    if (on && (!_activeVehicle || _activeVehicle->vehicleLinkManager()->communicationLost())) {
        on = false;
    }

    if (on != _fullScreen) {
        _fullScreen = on;
        emit fullScreenChanged();
    }
}

void VideoManager::_updateState()
{
    const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    const bool streamWanted = _videoSettings->streamEnabled()->rawValue().toBool();
    const bool autoStreamConfigured = VideoStreamResolver::isAutoStream(_primaryStream->streamInfo());
    const bool isStreamSource = VideoStreamResolver::isNetworkSource(videoSource) || autoStreamConfigured;
    const QString uvcVideoSourceID =
        (streamWanted && !isStreamSource && UVCDevices::deviceExists(videoSource)) ? videoSource : QString();
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
        if (!_uvcVideoSourceID.isEmpty()) {
            UVCDevices::requestPermission();
        }
        emit uvcVideoSourceIDChanged();
        emit isUvcChanged();
    }
    if (hasVideo != _hasVideo) {
        _hasVideo = hasVideo;
        emit hasVideoChanged();
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

    if (_activeVehicle) {
        (void) connect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this,
                       &VideoManager::_communicationLostChanged);
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

void VideoManager::_communicationLostChanged(bool communicationLost)
{
    if (communicationLost) {
        setfullScreen(false);
    }
}
