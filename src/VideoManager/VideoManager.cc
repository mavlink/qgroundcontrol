#include "VideoManager.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <utility>

#include <QtCore/QApplicationStatic>
#include <QtCore/QDateTime>
#include <QtCore/QScopedValueRollback>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkAvailabilityMonitor.h"
#include "QGCVideoStreamInfo.h"
#include "SettingsManager.h"
#include "UVCCameraController.h"
#include "UVCDevices.h"
#include "VideoBackend.h"
#include "VideoBackendInit.h"
#include "VideoReceiver.h"
#include "VideoRecordingService.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoStreamResolver.h"
#include "VideoStreamUri.h"
#include "VideoTelemetryOverlay.h"

#if defined(Q_OS_ANDROID)
#include "AndroidInterface.h"
#endif

QGC_LOGGING_CATEGORY(VideoManagerLog, "Video.VideoManager")

namespace {

// What VideoSettings::maxVideoTiles allows: as many tiles as fit beside the main picture.
constexpr uint kMaxVideoTiles = 4;

/// What only one stream at a time can be played at: the local port of a stream that is listened for, else its URI.
QString streamAddress(const QString& uri)
{
    const QUrl url(uri);
    const std::optional<VideoStreamUri::Transport> transport = VideoStreamUri::transportFor(url);
    return (transport && VideoStreamUri::listens(*transport)) ? QStringLiteral("udp:%1").arg(url.port()) : uri;
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
    , _receiverFactory([](QObject* owner) { return QGCCorePlugin::instance()->createVideoReceiver(owner); })
    , _cameraAttached(&UVCDevices::deviceExists)
    , _vehicleStreams(new VehicleVideoStreams(this))
    , _backendInit(new VideoBackendInit(this))
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
    (void) connect(_telemetryOverlay, &VideoTelemetryOverlay::textChanged, this,
                   &VideoManager::telemetryOverlayTextChanged);
    (void) connect(_vehicleStreams, &VehicleVideoStreams::vehicleChanged, this, &VideoManager::_onVehicleChanged);
    (void) connect(_vehicleStreams, &VehicleVideoStreams::streamsChanged, this, &VideoManager::_setAdvertisedStreams);
    (void) connect(_vehicleStreams, &VehicleVideoStreams::armedChanged, this, &VideoManager::_onArmedChanged);
    (void) connect(_vehicleStreams, &VehicleVideoStreams::communicationLostChanged, this,
                   &VideoManager::_communicationLostChanged);
    (void) connect(_backendInit, &VideoBackendInit::finished, this, &VideoManager::_onBackendInitFinished);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // A mobile app in the background shows nothing, and decoding there costs the battery. A desktop app's windows
    // stay in view without the focus.
    (void) connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &VideoManager::_onApplicationStateChanged);
#endif
#if defined(Q_OS_ANDROID)
    // Android cuts the network of an app in the background, which would end a recording there.
    (void) connect(this, &VideoManager::recordingChanged, this, [](bool recording) {
        AndroidInterface::setBackgroundWork(AndroidInterface::BackgroundWork::VideoRecording, recording);
    });
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
    (void) connect(source, &VideoStreamController::streamInfoChanged, this, &VideoManager::_updateState);
    // The thermal stream's camera is asked for it only while it is in use.
    (void) connect(source, &VideoStreamController::idleChanged, this, &VideoManager::_updateState);
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
        degraded = degraded || stats.degraded(streamInfo ? streamInfo->framerate() : 0.0);
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

    // How a change to a video setting reaches the sources. Not listed: forcedAspectRatio, which each
    // VideoStreamController follows itself; videoFit and gridLines, read by QML; showTelemetryOverlay, read by
    // VideoTelemetryOverlay; recordingFormat, uvcRecordingCodec and uvcRecordingQuality, read when a recording starts;
    // enableStorageLimit and maxVideoSize, read when one starts and each minute while it runs; uvcCameraFormat,
    // uvcCameraRotation, uvcCameraMirror, uvcWhiteBalance, uvcZoom, uvcExposureCompensation and uvcFocusMode, which go
    // to the USB camera (below).

    // Live: the sources resolve their config again. VideoReceiver restarts for a changed config, except for a change
    // of autoReconnect alone, which applies at once.
    const auto reconfigure = [this] {
        _updateState();
        for (VideoStreamController* source : std::as_const(_sources)) {
            source->scheduleApply();
        }
    };
    for (Fact* const fact : {_videoSettings->videoSource(), _videoSettings->udpUrl(), _videoSettings->tcpUrl(),
                             _videoSettings->rtspUrl(), _videoSettings->rtspTimeout(), _videoSettings->lowLatencyMode(),
                             _videoSettings->rtpJitterLatencyMs(), _videoSettings->softwareDecoderThreads(),
                             _videoSettings->rtspAutoReconnect(), _videoSettings->disableWhenDisarmed(),
                             // Which sources there are, and what the camera source plays.
                             _videoSettings->maxVideoTiles(), _videoSettings->uvcTileCamera(),
                             _videoSettings->uvcTileCameraAsMainPicture()}) {
        (void) connect(fact, &Fact::rawValueChanged, this, reconfigure);
    }

    // Construct-only properties of the display sink: the sources restart, which rebuilds a stream's sink.
    for (Fact* const fact : {_videoSettings->videoConversionElement(), _videoSettings->forceCpuVideoPath()}) {
        (void) connect(fact, &Fact::rawValueChanged, this, &VideoManager::_restartSources);
    }

    // The decoder ranks, which are global: once the backend is ready they are applied again, and the sources restart
    // so the streams' new pipelines pick decoders by them.
    (void) connect(_videoSettings->forceVideoDecoder(), &Fact::rawValueChanged, this, [this](const QVariant& value) {
        // Until then, _onBackendInitFinished() applies the setting.
        if (_backendInit->state() == VideoBackendInit::State::Ready) {
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
        if (_backendInit->isFinished() && _hasMainWindow) {
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
    const auto offerUvcCameraFormats = [this] { _videoSettings->offerCameraFormats(_uvcCamera->formats()); };
    offerUvcCameraFormats();
    (void) connect(_uvcCamera, &UVCCameraController::formatsChanged, this, offerUvcCameraFormats);

    const auto applyUvcTransform = [this] {
        const int rotation = _videoSettings->uvcCameraRotation()->rawValue().toInt();
        const bool mirrored = _videoSettings->uvcCameraMirror()->rawValue().toBool();
        // On display, and in the camera's photos. uvcCameraRotation's values are degrees, as QtVideo::Rotation's are.
        _cameraSource->setTransform(rotation, mirrored);
        _uvcCamera->setTransform(static_cast<QtVideo::Rotation>(rotation), mirrored);
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
}

void VideoManager::startVideoBackendInit()
{
    _backendInit->start();
}

bool VideoManager::waitForVideoBackendReady()
{
    return _backendInit->waitUntilFinished();
}

void VideoManager::_onBackendInitFinished(bool ready)
{
    if (ready) {
        _videoSettings->pruneUnavailableDecoders();
        // Also for a setting that was just pruned, which its own handler applied already: the ranks are the same.
        VideoBackend::instance().applyDecoderPriorities(
            videoDecoderOptionFromSetting(_videoSettings->forceVideoDecoder()->rawValue().toInt()));
        qCDebug(VideoManagerLog) << "video backend ready";
    } else {
        qCCritical(VideoManagerLog) << "video initialization failed";
        QGC::showAppMessage(VideoBackend::instance().initFailureMessage());
    }
    // After a failure the backend makes the receivers that need no initialization.
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

    _vehicleStreams->followActiveVehicle();
    _followNetwork(new QGCNetworkAvailabilityMonitor(this));

    _onMainWindowReady();
}

void VideoManager::_onMainWindowReady()
{
    _hasMainWindow = true;
    // The receivers need the backend and nothing from QML: the VideoOutputs register themselves whenever they exist.
    startVideoBackendInit();
    if (_backendInit->isFinished()) {
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
    _restartedWhileDisarmed = true;
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
    exif.position = _vehicleStreams->position();

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

void VideoManager::setFullScreen(bool on)
{
    if (on && (!_hasVideo || !_vehicleStreams->connected())) {
        on = false;
    }

    if (on != _fullScreen) {
        _fullScreen = on;
        emit fullScreenChanged();
    }
}

int VideoManager::idleSourceGraceMs() const
{
    return static_cast<int>(std::chrono::milliseconds(VideoStreamController::kIdleGrace).count());
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
    if ((source != _primaryStream) && (stream != _otherStreams.cend())) {
        _vehicleStreams->select(*stream);
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
    if (!_vehicleDisarmed || !_videoSettings->disableWhenDisarmed()->rawValue().toBool()) {
        // Nothing for a restart to undo: the video is off again the next time the vehicle is disarmed with the
        // setting on.
        _restartedWhileDisarmed = false;
    }
    const bool disabledWhileDisarmed = _disabledWhileDisarmed();

    const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    const bool streamWanted = !disabledWhileDisarmed;
    // The reason that takes the most to lift comes first: arming doesn't bring back video that was stopped.
    const NoVideoReason noVideoReason = _videoStopped           ? NoVideoReason::Stopped
                                        : disabledWhileDisarmed ? NoVideoReason::DisabledWhileDisarmed
                                                                : NoVideoReason::Waiting;
    const bool autoStreamConfigured = VideoStreamResolver::isAutoStream(_primaryStream->streamInfo());
    const bool isStreamSource = VideoStreamResolver::isNetworkSource(videoSource) || autoStreamConfigured;
    const int tilesAllowed =
        static_cast<int>(std::min(_videoSettings->maxVideoTiles()->rawValue().toUInt(), kMaxVideoTiles));
    // No room is known of until a row of tiles that is shown tells its own: nothing plays for a tile nobody sees.
    const int maxTiles = std::min(tilesAllowed, std::max(_tileRoom, 0));
    const bool cameraAsMainPicture = _videoSettings->uvcTileCameraAsMainPicture()->rawValue().toBool();
    // A stream to play takes the place of the camera the video source names: the camera source then plays the camera
    // to show in a tile.
    const QString cameraName =
        !isStreamSource ? videoSource
                        : ((tilesAllowed > 0) ? _videoSettings->uvcTileCamera()->rawValue().toString() : QString());
    const bool cameraWanted = streamWanted && !cameraName.isEmpty();
    const bool hasUvcCamera = cameraWanted && _cameraAttached(cameraName);
    // The tile's camera plays while there is room for its tile. Made the main picture, it needs none to stay it.
    const bool cameraPlays = hasUvcCamera && (!isStreamSource || (maxTiles > 0) || cameraAsMainPicture);
    const QString camera = cameraPlays ? cameraName : QString();
    // A camera selected in its tile is the main picture whenever it plays beside the stream: the setting outlasts a
    // camera that is unplugged, and a restart.
    const bool cameraOnMain = isStreamSource && !camera.isEmpty() && cameraAsMainPicture;
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
    // Once the sources are enabled or not, which tells whether the one that plays a thermal stream wants it.
    _nameExtraStreams();
    // After the extra streams: they leave the display first.
    _vehicleStreams->setPlaying(streamWanted && !_videoStopped);

    // Every member changes before the first signal: a slot or a QML binding that runs on one of them finds the others
    // as they are to be, not as they were.
    const bool wasAutoStreamConfigured = std::exchange(_autoStreamConfigured, autoStreamConfigured);
    const bool wasStreamSource = std::exchange(_isStreamSource, isStreamSource);
    const QString formerUvcVideoSourceID = std::exchange(_uvcVideoSourceID, uvcVideoSourceID);
    const bool hadVideo = std::exchange(_hasVideo, hasVideo);
    const bool hadThermal = std::exchange(_hasThermal, hasThermal);
    const bool hadUvcCamera = std::exchange(_hasUvcCamera, hasUvcCamera);
    const NoVideoReason formerNoVideoReason = std::exchange(_noVideoReason, noVideoReason);

    if (autoStreamConfigured != wasAutoStreamConfigured) {
        emit autoStreamConfiguredChanged();
    }
    if (isStreamSource != wasStreamSource) {
        emit isStreamSourceChanged();
    }
    if (uvcVideoSourceID != formerUvcVideoSourceID) {
        qCDebug(VideoManagerLog) << "UVC changed from [" << formerUvcVideoSourceID << "] to [" << uvcVideoSourceID
                                 << "]";
        emit isUvcChanged();
        const bool wasUvc = !formerUvcVideoSourceID.isEmpty();
        if (isUvc() != wasUvc) {
            emit mainSourceChanged();
        }
    }
    if (hasVideo != hadVideo) {
        emit hasVideoChanged();
        if (!hasVideo) {
            // The Fly View swaps the video out of its PiP when hasVideo drops; nothing in QML could leave full screen.
            setFullScreen(false);
        }
    }
    if (hasThermal != hadThermal) {
        emit hasThermalChanged();
    }
    if (hasUvcCamera != hadUvcCamera) {
        emit hasUvcCameraChanged();
    }
    if (noVideoReason != formerNoVideoReason) {
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
}

void VideoManager::_nameExtraStreams()
{
    QList<AdvertisedStream> inTiles;
    for (const AdvertisedStream& stream : std::as_const(_otherStreams)) {
        if (!_videoStopped && _tileStream(stream.streamInfo)) {
            inTiles.append(stream);
        }
    }
    QList<const QGCVideoStreamInfo*> mainStreams;
    for (const VideoStreamController* source : {_primaryStream, _thermalStream}) {
        if (source->streamInfo() && source->wantsStream()) {
            mainStreams.append(source->streamInfo());
        }
    }
    _vehicleStreams->setExtraStreams(inTiles, mainStreams);
}

bool VideoManager::_disabledWhileDisarmed() const
{
    return _vehicleDisarmed && !_restartedWhileDisarmed && _videoSettings->disableWhenDisarmed()->rawValue().toBool();
}

void VideoManager::_onVehicleChanged()
{
    _vehicleDisarmed = _vehicleStreams->disarmed();
    _restartedWhileDisarmed = false;
    if (!_vehicleStreams->hasVehicle()) {
        setFullScreen(false);
        return;
    }
    // With the video off, _updateState() has the camera stop transmitting instead.
    if (!_videoStopped && !_disabledWhileDisarmed()) {
        _vehicleStreams->resumeCurrentStream();
    }
    _reconnectStreams();
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
        if (!isUvc()) {
            emit mainSourceChanged();
        }
        emit decodingChanged();
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
    qCDebug(VideoManagerLog) << "vehicle armed:" << armed;
    _vehicleDisarmed = !armed;
    _restartedWhileDisarmed = false;
    _updateState();
}

void VideoManager::_communicationLostChanged(bool communicationLost)
{
    if (communicationLost) {
        setFullScreen(false);
    } else {
        _reconnectStreams();
    }
}

void VideoManager::_followNetwork(QGCNetworkAvailabilityMonitor* network)
{
    (void) connect(network, &QGCNetworkAvailabilityMonitor::availableChanged, this, [this](bool available) {
        if (available) {
            _reconnectStreams();
        }
    });
}

void VideoManager::_reconnectStreams()
{
    for (VideoStreamController* source : std::as_const(_sources)) {
        source->reconnectNow();
    }
}
