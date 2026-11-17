#include "VideoManager.h"

#include <algorithm>
#include <optional>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QScopedValueRollback>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
#include "MavlinkCameraControlInterface.h"
#include "MultiVehicleManager.h"
#include "QGCCameraManager.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "QmlObjectListModel.h"
#include "QtMultimediaVideoReceiver.h"
#include "QtVideoRecorder.h"
#include "SettingsManager.h"
#include "UVCCameraController.h"
#include "UVCDevices.h"
#include "UVCStreamMonitor.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "VideoBackend.h"
#include "VideoReceiver.h"
#include "VideoRecordingService.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoStreamResolver.h"
#include "VideoStreamUri.h"
#include "VideoTelemetryOverlay.h"

QGC_LOGGING_CATEGORY(VideoManagerLog, "Video.VideoManager")

namespace {

// What VideoSettings::maxVideoTiles allows: as many tiles as fit beside the main picture.
constexpr uint kMaxVideoTiles = 4;

/// What @p videoSettings has the receivers that encode their recordings anew record with: the USB camera's and Qt
/// Multimedia's.
VideoReceiver::RecordingEncoding recordingEncodingSetting(VideoSettings* videoSettings)
{
    return {.codec = QtVideoRecorder::recordingCodec(videoSettings->uvcRecordingCodec()->rawValue().toString()),
            .quality = QtVideoRecorder::recordingQuality(videoSettings->uvcRecordingQuality()->rawValue().toInt())};
}

/// What only one stream at a time can be played at: the local port of a stream that is listened for, else its URI.
QString streamAddress(const QString& uri)
{
    const QUrl url(uri);
    const std::optional<VideoStreamUri::Transport> transport = VideoStreamUri::transportFor(url);
    return (transport && VideoStreamUri::listens(*transport)) ? QStringLiteral("udp:%1").arg(url.port()) : uri;
}

/// The cameras of @p cameraManager.
QList<MavlinkCameraControlInterface*> vehicleCameras(QGCCameraManager* cameraManager)
{
    QList<MavlinkCameraControlInterface*> cameras;
    QmlObjectListModel* const model = cameraManager->cameras();
    for (int i = 0; i < model->count(); ++i) {
        if (auto* const camera = qobject_cast<MavlinkCameraControlInterface*>(model->get(i))) {
            cameras.append(camera);
        }
    }
    return cameras;
}

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
    , _measuredSource(_primaryStream)
    , _recordingService(new VideoRecordingService(_sources, this))
    , _telemetryOverlay(new VideoTelemetryOverlay(_videoSettings->showTelemetryOverlay(), this))
    , _receiverFactory([this](QObject* owner) -> VideoReceiver* {
        if (_videoSettings->videoBackend()->rawValue().toInt() == VideoSettings::VideoBackendQtMultimedia) {
            return new QtMultimediaVideoReceiver(owner);
        }
        return QGCCorePlugin::instance()->createVideoReceiver(owner);
    })
    , _cameraAttached(&UVCDevices::deviceExists)
{
    qCDebug(VideoManagerLog) << this;

    for (VideoStreamController* source : std::as_const(_sources)) {
        _connectSource(source);
    }
    (void) connect(_recordingService, &VideoRecordingService::recordingChanged, this, &VideoManager::recordingChanged);
    // A camera plugged in or unplugged changes whether the camera source has one to run.
    (void) connect(_uvcCamera, &UVCCameraController::camerasChanged, this, [this] {
        _updateState();
        _cameraSource->scheduleApply();
    });
    // The frame rate the camera is set up for may change without its figures.
    (void) connect(_uvcCamera->monitor(), &UVCStreamMonitor::statsChanged, this, &VideoManager::_updateSourceSummary);
    (void) connect(_telemetryOverlay, &VideoTelemetryOverlay::textChanged, this,
                   &VideoManager::telemetryOverlayTextChanged);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // A mobile app in the background shows nothing, and decoding there costs the battery. A desktop app's windows
    // stay in view without the focus.
    (void) connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &VideoManager::_onApplicationStateChanged);
#endif

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

void VideoManager::_onApplicationStateChanged(Qt::ApplicationState state)
{
    // Inactive is in view without the focus, as beside another app or under a system dialog.
    const bool suspended = (state == Qt::ApplicationHidden) || (state == Qt::ApplicationSuspended);
    if (suspended == _displaySuspended) {
        return;
    }
    _displaySuspended = suspended;
    qCDebug(VideoManagerLog) << (suspended ? "In the background: display suspended" : "Display resumed");
    for (VideoStreamController* source : std::as_const(_sources)) {
        source->setDisplaySuspended(suspended);
    }
}

void VideoManager::_connectSource(VideoStreamController* source)
{
    (void) connect(source, &VideoStreamController::decodingChanged, this, [this, source] {
        if (source == _primaryStream) {
            emit decodingChanged();
        }
    });
    (void) connect(source, &VideoStreamController::aspectRatioChanged, this, &VideoManager::aspectRatioChanged);
    (void) connect(source, &VideoStreamController::streamInfoChanged, this, &VideoManager::_updateState);
    for (const auto changed : {&VideoStreamController::statsChanged, &VideoStreamController::decodingChanged,
                               &VideoStreamController::streamInfoChanged, &VideoStreamController::tileChanged}) {
        (void) connect(source, changed, this, &VideoManager::_updateSourceSummary);
    }
    (void) connect(source, &VideoStreamController::errorOccurred, this, [source](const QString& message) {
        qCWarning(VideoManagerLog) << source->name() << message;
        QGC::showAppMessage(message);
    });
}

void VideoManager::_updateSourceSummary()
{
    int tileCount = 0;
    VideoStreamController* measured = nullptr;
    VideoStreamController* measuredInTile = nullptr;
    bool degraded = false;
    for (VideoStreamController* source : std::as_const(_sources)) {
        const VideoStreamStats stats = source->stats();
        if (source->tile()) {
            ++tileCount;
        }
        if (stats.valid) {
            VideoStreamController*& first = source->tile() ? measuredInTile : measured;
            if (!first) {
                first = source;
            }
        }
        // A stream that isn't decoded, as the thermal stream while its view is off, shows no frames to compare with
        // the frame rate its camera advertises.
        const QGCVideoStreamInfo* const streamInfo = source->decoding() ? source->streamInfo() : nullptr;
        const double cameraFps = (source == _cameraSource) ? _uvcCamera->monitor()->cameraFrameRate()
                                                           : (streamInfo ? streamInfo->framerate() : 0.0);
        degraded = degraded || stats.degraded(cameraFps);
    }
    if (!measured) {
        measured = measuredInTile ? measuredInTile : _primaryStream;
    }

    if (tileCount != _tileCount) {
        _tileCount = tileCount;
        emit tileCountChanged();
    }
    if (measured != _measuredSource) {
        _measuredSource = measured;
        emit measuredSourceChanged();
    }
    if (degraded != _degraded) {
        _degraded = degraded;
        emit degradedChanged();
    }
}

void VideoManager::_connectSettings()
{
    // The choice of the camera as the main picture was of the camera the tile had: another one starts in the tile.
    // Connected before the reactions, which then find both settings as they are to be.
    (void) connect(_videoSettings->uvcTileCamera(), &Fact::rawValueChanged, this,
                   [this] { _videoSettings->uvcTileCameraAsMainPicture()->setRawValue(false); });

    // How a change to a video setting reaches the sources. Not listed: aspectRatio, read by
    // VideoStreamController::aspectRatio(); videoFit, gridLines and showRecControl, read by QML; showTelemetryOverlay,
    // read by VideoTelemetryOverlay; recordingFormat, enableStorageLimit and maxVideoSize, read when a recording
    // starts; uvcCameraFormat, uvcCameraRotation, uvcCameraMirror, uvcWhiteBalance, uvcZoom, uvcExposureCompensation
    // and uvcFocusMode, which go to the USB camera, and uvcRecordingCodec and uvcRecordingQuality, which go to every
    // source (below).

    // Live: the sources resolve their config again. VideoReceiver restarts for a changed config, except for a change
    // of autoReconnect alone, which applies at once.
    const auto reconfigure = [this] {
        _updateState();
        for (VideoStreamController* source : std::as_const(_sources)) {
            source->scheduleApply();
        }
    };
    for (Fact* const fact :
         {_videoSettings->videoSource(), _videoSettings->udpUrl(), _videoSettings->tcpUrl(), _videoSettings->rtspUrl(),
          _videoSettings->rtspTimeout(), _videoSettings->lowLatencyMode(), _videoSettings->rtpJitterLatencyMs(),
          _videoSettings->rtspAutoReconnect(), _videoSettings->disableWhenDisarmed(),
          // Which sources there are, and what the camera source plays.
          _videoSettings->maxVideoTiles(), _videoSettings->uvcTileCamera(),
          _videoSettings->uvcTileCameraAsMainPicture()}) {
        (void) connect(fact, &Fact::rawValueChanged, this, reconfigure);
    }

    // Construct-only properties of the display sink: the sources restart, which rebuilds a stream's sink.
    for (Fact* const fact : {_videoSettings->videoConversionElement(), _videoSettings->disablePixelAspectRatio(),
                             _videoSettings->forceCpuVideoPath()}) {
        (void) connect(fact, &Fact::rawValueChanged, this, &VideoManager::_restartSources);
    }

    // The decoder ranks, which are global: once the backend is ready they are applied again, and the sources restart
    // so the streams' new pipelines pick decoders by them.
    (void) connect(_videoSettings->forceVideoDecoder(), &Fact::rawValueChanged, this, [this](const QVariant& value) {
        // Until then, _onBackendInitComplete() applies the setting.
        if (_initState == InitState::Ready) {
            VideoBackend::instance().applyDecoderPriorities(videoDecoderOptionFromSetting(value.toInt()));
        }
        _restartSources();
    });

    // Which receiver plays the streams: each stream gets a new one.
    (void) connect(_videoSettings->videoBackend(), &Fact::rawValueChanged, this, [this] {
        {
            // A receiver that is recording runs the event loop as it goes: the sources stay as they are until each has
            // let go of its own.
            const QScopedValueRollback<bool> sourcesHeld(_updatingState, true);
            for (VideoStreamController* source : std::as_const(_sources)) {
                source->setReceiver(nullptr);
            }
        }
        if (_backendInitFinished() && _hasMainWindow) {
            _createVideoReceivers();
        }
        if (!_updatingState && _updateStatePending) {
            _updateState();
        }
    });

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

    const auto applyRecordingEncoding = [this] {
        const VideoReceiver::RecordingEncoding encoding = recordingEncodingSetting(_videoSettings);
        for (VideoStreamController* source : std::as_const(_sources)) {
            source->setRecordingEncoding(encoding);
        }
    };
    applyRecordingEncoding();
    (void) connect(_videoSettings->uvcRecordingCodec(), &Fact::rawValueChanged, this, applyRecordingEncoding);
    (void) connect(_videoSettings->uvcRecordingQuality(), &Fact::rawValueChanged, this, applyRecordingEncoding);
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
    (void) connect(SettingsManager::instance()->appSettings()->gstDebugLevel(), &Fact::rawValueChanged, this,
                   [](const QVariant& value) { VideoBackend::instance().setDebugLevel(value.toInt()); });

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
    for (VideoStreamController* source : std::as_const(_sources)) {
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
    for (VideoStreamController* source : std::as_const(_sources)) {
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
    for (VideoStreamController* source : std::as_const(_sources)) {
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
    for (VideoStreamController* source : std::as_const(_sources)) {
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
    for (VideoStreamController* source : std::as_const(_sources)) {
        // The main picture and the thermal stream with it: not the tiles.
        if (!source->tile() && source->canTakeScreenshot()) {
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

void VideoManager::setTileRoom(int room)
{
    if (room == _tileRoom) {
        return;
    }
    _tileRoom = room;
    emit tileRoomChanged();
    (void) QMetaObject::invokeMethod(this, &VideoManager::_updateState, Qt::QueuedConnection);
}

void VideoManager::selectSource(VideoStreamController* source)
{
    if (!source || !source->tile()) {
        return;
    }
    qCDebug(VideoManagerLog) << "select" << source->name();

    // A stream takes the main picture back from the camera. The sources follow the setting as it changes.
    _videoSettings->uvcTileCameraAsMainPicture()->setRawValue(source == _cameraSource);
    if (source == _cameraSource) {
        return;
    }

    const auto stream =
        std::find_if(_otherStreams.cbegin(), _otherStreams.cend(), [source](const AdvertisedStream& other) {
            return other.streamInfo && (other.streamInfo == source->streamInfo());
        });
    QGCCameraManager* const cameraManager = _activeVehicle ? _activeVehicle->cameraManager() : nullptr;
    if ((source != _primaryStream) && (stream != _otherStreams.cend()) && stream->camera && cameraManager) {
        // Copied: the camera manager tells of each change at once, which replaces the streams.
        MavlinkCameraControlInterface* const camera = stream->camera;
        const int streamIndex = camera->streamLabels().indexOf(stream->streamInfo->name());
        // The stream first: the camera then becomes the current one with the stream to show.
        if (streamIndex >= 0) {
            camera->setCurrentStream(streamIndex);
        }
        cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(camera));
    }
    _updateState();
}

void VideoManager::_updateState()
{
    if (_updatingState) {
        _updateStatePending = true;
        return;
    }
    const QScopedValueRollback<bool> updating(_updatingState, true);
    do {
        _updateStatePending = false;
        _updateStateNow();
    } while (_updateStatePending);
}

void VideoManager::_updateStateNow()
{
    if (_cleanedUp) {
        return;
    }
    if (!_videoSettings->disableWhenDisarmed()->rawValue().toBool()) {
        _disabledWhileDisarmed = false;
    }

    const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    const bool streamWanted = !_disabledWhileDisarmed;
    // The reason that takes the most to lift comes first: arming doesn't bring back video that was stopped.
    const NoVideoReason noVideoReason = _videoStopped            ? NoVideoReason::Stopped
                                        : _disabledWhileDisarmed ? NoVideoReason::DisabledWhileDisarmed
                                                                 : NoVideoReason::Waiting;
    const bool autoStreamConfigured = VideoStreamResolver::isAutoStream(_primaryStream->streamInfo());
    const bool isStreamSource = VideoStreamResolver::isNetworkSource(videoSource) || autoStreamConfigured;
    const int tilesAllowed =
        static_cast<int>(std::min(_videoSettings->maxVideoTiles()->rawValue().toUInt(), kMaxVideoTiles));
    const int maxTiles = (_tileRoom < 0) ? tilesAllowed : std::min(tilesAllowed, _tileRoom);
    // A stream to play takes the place of the camera the video source names: the camera source then plays the camera
    // to show in a tile.
    const QString cameraName =
        !isStreamSource ? videoSource
                        : ((maxTiles > 0) ? _videoSettings->uvcTileCamera()->rawValue().toString() : QString());
    const bool cameraWanted = streamWanted && !cameraName.isEmpty();
    if (cameraWanted) {
        // A camera plugged in or unplugged changes whether the camera source has one to play.
        _uvcCamera->watchCameras();
    }
    const QString camera = (cameraWanted && _cameraAttached(cameraName)) ? cameraName : QString();
    const bool hasUvcCamera = !camera.isEmpty();
    // A camera selected in its tile is the main picture whenever it plays beside the stream: the setting outlasts a
    // camera that is unplugged, and a restart.
    const bool cameraOnMain =
        isStreamSource && !camera.isEmpty() && _videoSettings->uvcTileCameraAsMainPicture()->rawValue().toBool();
    const bool cameraTile = isStreamSource && !camera.isEmpty() && !cameraOnMain;
    const QString uvcVideoSourceID = cameraTile ? QString() : camera;
    const bool hasVideo = streamWanted && (_primaryStream->configured() || !uvcVideoSourceID.isEmpty());
    const QGCVideoStreamInfo* const thermalStreamInfo = _thermalStream->streamInfo();
    // The thermal stream goes with the primary stream's picture.
    const bool hasThermal = thermalStreamInfo && thermalStreamInfo->isThermal() && !cameraOnMain;

    _cameraSource->setCamera(camera);
    _cameraSource->setDisplayName(camera);
    _cameraSource->setTile(cameraTile);
    _primaryStream->setTile(cameraOnMain);
    if (!autoStreamConfigured) {
        _primaryStream->setDisplayName(_videoSettings->videoSource()->enumStringValue());
    }
    // Without video there is no view to show tiles in. The camera, or the stream it took the main picture from, has
    // one of them.
    _updateTileStreams(hasVideo ? (maxTiles - ((cameraTile || cameraOnMain) ? 1 : 0)) : 0);

    for (VideoStreamController* source : std::as_const(_sources)) {
        source->setEnabled(streamWanted && !_videoStopped && ((source != _thermalStream) || !cameraOnMain));
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
    if (hasUvcCamera != _hasUvcCamera) {
        _hasUvcCamera = hasUvcCamera;
        emit hasUvcCameraChanged();
    }
    if (noVideoReason != _noVideoReason) {
        _noVideoReason = noVideoReason;
        emit noVideoReasonChanged();
    }
}

VideoStreamController* VideoManager::_tileStream(const QGCVideoStreamInfo* streamInfo) const
{
    const auto tile =
        std::find_if(_tileStreams.cbegin(), _tileStreams.cend(),
                     [streamInfo](const VideoStreamController* source) { return source->streamInfo() == streamInfo; });
    return (streamInfo && (tile != _tileStreams.cend())) ? *tile : nullptr;
}

void VideoManager::_updateTileStreams(int count)
{
    const VideoStreamResolver::Settings settings = VideoStreamResolver::snapshot(_videoSettings);
    QStringList shownAddresses;
    for (const VideoStreamController* source : {_primaryStream, _thermalStream}) {
        if (const auto config = VideoStreamResolver::resolve(settings, source->streamInfo(), source->role())) {
            shownAddresses.append(streamAddress(config->uri));
        }
    }

    QList<VideoStreamController*> tiles;
    for (const AdvertisedStream& stream : std::as_const(_otherStreams)) {
        if (tiles.size() >= count) {
            break;
        }
        const auto config =
            VideoStreamResolver::resolve(settings, stream.streamInfo, VideoStreamController::Role::Tile);
        if (!config || shownAddresses.contains(streamAddress(config->uri))) {
            continue;
        }
        shownAddresses.append(streamAddress(config->uri));

        VideoStreamController* tile = _tileStream(stream.streamInfo);
        if (!tile) {
            tile = new VideoStreamController(VideoStreamController::Role::Tile, _videoSettings, this);
            tile->setName(QStringLiteral("videoTile%1").arg(++_tileStreamSerial));
            tile->setStreamInfo(stream.streamInfo);
            // Its source records once it is the primary stream.
            tile->setRecordingEncoding(recordingEncodingSetting(_videoSettings));
            tile->setDisplaySuspended(_displaySuspended);
            _connectSource(tile);
            _recordingService->addSource(tile);
        }
        tile->setDisplayName(stream.name);
        tile->setTile(true);
        tiles.append(tile);
    }

    QList<VideoStreamController*> removed;
    for (VideoStreamController* const tile : std::as_const(_tileStreams)) {
        if (!tiles.contains(tile)) {
            removed.append(tile);
        }
    }
    _tileStreams = tiles;

    const QList<VideoStreamController*> sources =
        QList<VideoStreamController*>{_primaryStream, _thermalStream, _cameraSource} + _tileStreams;
    if (sources != _sources) {
        _sources = sources;
        // Before the sources that left are deleted: none of them is the measured one from here on.
        _updateSourceSummary();
        emit sourcesChanged();
    }
    // Once QML has let go of them.
    for (VideoStreamController* const tile : std::as_const(removed)) {
        qCDebug(VideoManagerLog) << "tile removed" << tile->name();
        tile->shutDown();
        tile->deleteLater();
    }
    // The streams have their receivers: so do the new ones.
    if (_thermalStream->receiver()) {
        _createVideoReceivers();
    }
    _nameExtraStreams();
}

void VideoManager::_nameExtraStreams()
{
    QGCCameraManager* const cameraManager = _activeVehicle ? _activeVehicle->cameraManager() : nullptr;
    if (!cameraManager) {
        return;
    }
    const QList<MavlinkCameraControlInterface*> cameras = vehicleCameras(cameraManager);
    for (MavlinkCameraControlInterface* const camera : cameras) {
        QList<quint8> streamIds;
        for (const AdvertisedStream& stream : std::as_const(_otherStreams)) {
            if ((stream.camera == camera) && _tileStream(stream.streamInfo)) {
                streamIds.append(stream.streamInfo->streamID());
            }
        }
        camera->setExtraStreams(streamIds);
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
            // The streams in tiles leave the display first, the current stream after them.
            const QList<MavlinkCameraControlInterface*> cameras = vehicleCameras(cameraManager);
            for (MavlinkCameraControlInterface* const camera : cameras) {
                camera->setExtraStreams({});
            }
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
    if (!cameraManager) {
        _setAdvertisedStreams({}, nullptr, {});
        return;
    }

    // The current camera's streams come first, then the other cameras', in the cameras' order.
    QList<MavlinkCameraControlInterface*> cameras = vehicleCameras(cameraManager);
    MavlinkCameraControlInterface* const currentCamera = cameraManager->currentCameraInstance();
    if (cameras.removeOne(currentCamera)) {
        cameras.prepend(currentCamera);
    }
    QGCVideoStreamInfo* const currentStreamInfo = cameraManager->currentStreamInstance();
    AdvertisedStream current;
    QList<AdvertisedStream> others;
    for (MavlinkCameraControlInterface* const camera : std::as_const(cameras)) {
        QmlObjectListModel* const streams = camera->streams();
        for (int i = 0; streams && (i < streams->count()); ++i) {
            auto* const streamInfo = qobject_cast<QGCVideoStreamInfo*>(streams->get(i));
            if (!streamInfo || streamInfo->isThermal()) {
                continue;
            }
            const QString name = (cameras.size() > 1)
                                     ? QStringLiteral("%1: %2").arg(camera->modelName(), streamInfo->name())
                                     : streamInfo->name();
            if (streamInfo == currentStreamInfo) {
                current = {streamInfo, camera, name};
            } else {
                others.append({streamInfo, camera, name});
            }
        }
    }
    _setAdvertisedStreams(current, cameraManager->thermalStreamInstance(), others);
}

void VideoManager::_setAdvertisedStreams(const AdvertisedStream& current, QGCVideoStreamInfo* thermal,
                                         const QList<AdvertisedStream>& others)
{
    if (_cleanedUp) {
        return;
    }
    _otherStreams = others;

    if (VideoStreamController* const playing = _tileStream(current.streamInfo)) {
        // The stream to display plays in a tile: its source becomes the primary stream as it is, and the former
        // primary stream, which plays on, gets a tile if its stream is still one to show.
        VideoStreamController* const former = _primaryStream;
        _tileStreams.replace(_tileStreams.indexOf(playing), former);
        _primaryStream = playing;
        playing->setTile(former->tile());
        former->setRole(VideoStreamController::Role::Tile);
        playing->setRole(VideoStreamController::Role::Primary);
        emit primaryStreamChanged();
        emit decodingChanged();
        emit aspectRatioChanged();
    } else {
        _primaryStream->setStreamInfo(current.streamInfo);
    }
    if (current.streamInfo) {
        _primaryStream->setDisplayName(current.name);
    }
    _thermalStream->setStreamInfo(thermal);
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
    for (VideoStreamController* source : std::as_const(_sources)) {
        source->reconnectNow();
    }
}
