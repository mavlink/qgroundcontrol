#include "VideoManager.h"

#include "AppSettings.h"
#include "CameraControlBridge.h"
#include "VideoBackendRegistry.h"
#include "MultiVehicleManager.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "QtFutureHelpers.h"
#include "QtMultimediaReceiver.h"
#include "SettingsManager.h"
#include "RecordingCoordinator.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "VideoFrameDelivery.h"
#include "VideoReceiver.h"
#include "VideoRecorder.h"
#include "VideoSettings.h"
#include "VideoFileNaming.h"
#include "VideoStorageCleaner.h"
#include "VideoStream.h"
#include "VideoStreamModel.h"
#include "VideoStreamOrchestrator.h"
#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#endif
#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QFuture>
#include <QtCore/QPointer>
#include <QtCore/QRunnable>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtMultimedia/QMediaFormat>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickWindow>

QGC_LOGGING_CATEGORY(VideoManagerLog, "Video.VideoManager")

Q_APPLICATION_STATIC(VideoManager, _videoManagerInstance);


// ═══════════════════════════════════════════════════════════════════════════
// Construction / singleton
// ═══════════════════════════════════════════════════════════════════════════

bool VideoManager::_shouldSkipBackendForUnitTests()
{
    return VideoBackendRegistry::instance().isAvailable(VideoReceiver::BackendKind::GStreamer) && qgcApp()
           && QGC::runningUnitTests() && !qEnvironmentVariableIsSet("QGC_TEST_ENABLE_GSTREAMER");
}

VideoManager::VideoManager(QObject* parent)
    : QObject(parent),
      _videoSettings(SettingsManager::instance()->videoSettings()),
      _streamOrchestrator(new VideoStreamOrchestrator(_videoSettings, this)),
      _recordingCoordinator(new RecordingCoordinator(this)),
      _cameraBridge(new CameraControlBridge(
          [this]() {
              return CameraControlBridge::State{hasVideo(), decoding(), recording()};
          },
          this))
{
    qCDebug(VideoManagerLog) << this;

    // Camera → facade verbs. Bridge filters the MAVLink camera-control protocol.
    (void)connect(_cameraBridge, &CameraControlBridge::localRecordingRequested, this,
                  [this]() { startRecording(); });
    (void)connect(_cameraBridge, &CameraControlBridge::localRecordingStopRequested, this,
                  [this]() { stopRecording(); });
    (void)connect(_cameraBridge, &CameraControlBridge::localImageCaptureRequested, this,
                  [this](const QString& path) { grabImage(path); });

    // Aggregate state transitions coalesce through _cameraStateDirty; one
    // connection is enough for the bridge to keep the camera in sync.
    (void)connect(this, &VideoManager::_cameraStateDirty, _cameraBridge,
                  &CameraControlBridge::pushState);

    // Orchestrator aggregate transitions → coalesced camera-state push.
    // QML no longer binds the per-stream aggregates here; per-stream Q_PROPERTYs
    // on VideoStream serve QML directly via streamModel.
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::decodingChanged, this,
                  &VideoManager::_cameraStateDirty);
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::recordingChanged, this,
                  [this](bool recording) {
                      if (!recording) {
                          _recordingCoordinator->stopSubtitleTelemetry();
                          // Drain any vehicle-swap deferred during recording. Doing this
                          // mid-recording would tear down the receiver, restart against
                          // the new vehicle's URI, and corrupt the recording file with
                          // mixed sources.
                          if (_hasPendingCameraSwap) {
                              _hasPendingCameraSwap = false;
                              QGCCameraManager* cm = _pendingCameraManager.data();
                              _pendingCameraManager.clear();
                              _applyCameraManager(cm);
                          }
                      }
                      emit _cameraStateDirty();
                  });
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::hasVideoChanged, this, [this]() {
        emit hasVideoChanged();
        emit _cameraStateDirty();
    });
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::isStreamSourceChanged,
                  this, &VideoManager::isStreamSourceChanged);

    // Primary bridge swap → tell coordinator about the new sink for subtitle overlay.
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::primaryBridgeChanged, this, [this]() {
        auto* primary = _streamOrchestrator->primaryStream();
        auto* bridge = primary ? primary->bridge() : nullptr;
        _recordingCoordinator->setLiveSubtitleSink(bridge ? bridge->videoSink() : nullptr);
    });

    // Per-stream recording error → toast.
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::recordingError, this,
                  [](const QString& msg) {
                      qCWarning(VideoManagerLog) << "Recording error:" << msg;
                      qgcApp()->showAppMessage(tr("Video recording error: %1").arg(msg));
                  });

    // Coordinator session transitions. Aggregate `recording` state is driven by
    // the orchestrator's per-stream signal — no synthetic recordingChanged here.
    (void)connect(_recordingCoordinator, &RecordingCoordinator::recordingStarted, this,
                  &VideoManager::recordingStarted);
    (void)connect(_recordingCoordinator, &RecordingCoordinator::imageFileChanged, this,
                  &VideoManager::imageFileChanged);

    _backendDisabledForUnitTests = _shouldSkipBackendForUnitTests();
    if (_backendDisabledForUnitTests) {
        qCInfo(VideoManagerLog) << "Skipping backend initialization for unit tests";
    }
}

VideoManager::~VideoManager()
{
    qCDebug(VideoManagerLog) << this;
}

VideoManager* VideoManager::instance()
{
    return _videoManagerInstance();
}

QSize VideoManager::videoSize() const
{
    return _streamOrchestrator->videoSize();
}

VideoStreamModel* VideoManager::streamModel() const
{
    return _streamOrchestrator->streamModel();
}

// ═══════════════════════════════════════════════════════════════════════════
// Initialization — asynchronous backend init via QFuture/QPromise pipeline
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::startBackendInit()
{
    if (_backendInitFuture.isValid()) {
        qCWarning(VideoManagerLog) << "Backend init already started";
        return;
    }

    // Unit-test skip path and no-GStreamer build: synthesize a ready-true
    // future so the rest of the pipeline doesn't need to branch on validity.
    if (_backendDisabledForUnitTests) {
        qCInfo(VideoManagerLog) << "Backend initialization disabled for unit tests";
        _backendInitFuture = QtFuture::makeReadyValueFuture(true);
        return;
    }

    // The offscreen platform (CI boot tests) needs an explicit graphics API
    if (qApp->platformName() == QLatin1String("offscreen")) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    }

#ifdef QGC_GST_STREAMING
    const int decoderOption = _videoSettings->forceVideoDecoder()->rawValue().toInt();
    _backendInitFuture = GStreamer::initAsync(decoderOption);
#else
    _backendInitFuture = QtFuture::makeReadyValueFuture(true);
#endif
}

bool VideoManager::waitForBackendInit(int timeoutMs)
{
    if (!_backendInitFuture.isValid())
        startBackendInit();

    const auto result = waitFor(_backendInitFuture, timeoutMs);
    if (!result) {
        qCCritical(VideoManagerLog) << "Timed out waiting for backend init";
        return false;
    }
    return *result;
}

void VideoManager::init(QQuickWindow* mainWindow)
{
    if (_initialized) {
        qCDebug(VideoManagerLog) << "Video Manager already initialized";
        return;
    }

    if (!mainWindow) {
        qCCritical(VideoManagerLog) << "Failed To Init Video Manager - mainWindow is NULL";
        return;
    }
    _mainWindow = mainWindow;

    _streamOrchestrator->bindToSettings();

    // Stored so cleanup() can disconnect — SettingsManager may outlive us.
    QPointer<VideoManager> selfPtr = this;
    _gstDebugLevelConn = connect(SettingsManager::instance()->appSettings()->gstDebugLevel(), &Fact::rawValueChanged,
                                 this, [selfPtr](const QVariant& value) {
                                     if (!selfPtr)
                                         return;
#ifdef QGC_GST_STREAMING
                                     if (VideoBackendRegistry::instance().isAvailable(VideoReceiver::BackendKind::GStreamer))
                                         GStreamer::setDebugLevel(value.toInt());
#else
        Q_UNUSED(value);
#endif
                                 });
    (void)connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                  &VideoManager::_setActiveVehicle);
    (void)connect(this, &VideoManager::autoStreamConfiguredChanged, _streamOrchestrator,
                  &VideoStreamOrchestrator::refreshFromSettings);

    if (!_backendInitFuture.isValid())
        startBackendInit();

    // Stream creation waits on two gates chained together:
    //   1. `_backendInitFuture` (always valid after startBackendInit())
    //   2. `_qmlSyncPromise`, resolved from the first scheduleRenderJob tick
    // Linear future chain — no convergence bookkeeping; Qt context-object
    // guards on each .then() handle destruction cleanly.
    _qmlSyncPromise.start();

    QPointer<VideoManager> self = this;
    _mainWindow->scheduleRenderJob(QRunnable::create([self] {
                                       if (!self)
                                           return;
                                       QMetaObject::invokeMethod(
                                           self.data(),
                                           [self] {
                                               if (!self)
                                                   return;
                                               if (!self->_qmlSyncPromise.future().isFinished())
                                                   self->_qmlSyncPromise.finish();
                                           },
                                           Qt::QueuedConnection);
                                   }),
                                   QQuickWindow::AfterSynchronizingStage);

    _backendInitFuture.then(this, [this](bool backendOk) {
        if (!backendOk) {
            qCCritical(VideoManagerLog) << "Backend initialization failed";
            return;
        }
        _qmlSyncPromise.future().then(this, [this] { _streamOrchestrator->createStreams(); });
    });

    _initialized = true;

    // Recover any sessions left behind by a prior crash. Deferred inside the
    // coordinator to keep init() off the 3-second per-orphan probe path.
    _recordingCoordinator->scheduleOrphanScan(
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
}

// ═══════════════════════════════════════════════════════════════════════════
// Storage cleanup
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::_cleanupOldVideos()
{
    auto* videoSettings = SettingsManager::instance()->videoSettings();
    if (!videoSettings->enableStorageLimit()->rawValue().toBool())
        return;

    const QString savePath = SettingsManager::instance()->appSettings()->videoSavePath();
    // Must match the containers listed in VideoRecorder::Capabilities.
    static const QStringList nameFilters{QStringLiteral("*.mkv"), QStringLiteral("*.mov"), QStringLiteral("*.mp4")};

    const uint64_t maxSize = videoSettings->maxVideoSize()->rawValue().toUInt() * qPow(1024, 2);
    VideoStorageCleaner::pruneToLimit(savePath, nameFilters, maxSize);
}

// ═══════════════════════════════════════════════════════════════════════════
// Recording — unified entry points; all work delegated to VideoStream
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::startRecording(const QString& videoFile)
{
    const int rawFormat = _videoSettings->recordingFormat()->rawValue().toInt();
    const auto fileFormat = static_cast<QMediaFormat::FileFormat>(rawFormat);

    if (fileFormat != QMediaFormat::Matroska && fileFormat != QMediaFormat::QuickTime
        && fileFormat != QMediaFormat::MPEG4) {
        qgcApp()->showAppMessage(tr("Invalid video format defined."));
        return;
    }

    const QString savePath = SettingsManager::instance()->appSettings()->videoSavePath();
    if (savePath.isEmpty()) {
        qgcApp()->showAppMessage(
            tr("Unabled to record video. Video save path must be specified in Settings."));
        return;
    }

    _cleanupOldVideos();

    QList<VideoStream*> recordable;
    _streamOrchestrator->forEachRecordableStream([&](VideoStream* s) { recordable.append(s); });

    // Aggregate `recording` flips when the per-stream recorder reports
    // recording=true; no synthetic emit needed here.
    (void)_recordingCoordinator->startRecording(videoFile, recordable, _activeVehicle,
                                                videoSize(), fileFormat, savePath);
}

void VideoManager::stopRecording()
{
    _recordingCoordinator->stopRecording();
}

// ═══════════════════════════════════════════════════════════════════════════
// Image grab
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::grabImage(const QString& imageFile)
{
    _recordingCoordinator->grabImage(
        imageFile,
        _streamOrchestrator->primaryStream(),
        SettingsManager::instance()->appSettings()->photoSavePath());
}

// ═══════════════════════════════════════════════════════════════════════════
// Property accessors — delegate to orchestrator
// ═══════════════════════════════════════════════════════════════════════════

// Internal aggregate accessors — not Q_PROPERTY. Sole consumer is the
// CameraControlBridge state provider (push to MAVLink camera). QML reads
// per-stream state via streamModel.
bool VideoManager::decoding() const   { return _streamOrchestrator->decoding(); }
bool VideoManager::recording() const  { return _streamOrchestrator->recording(); }
bool VideoManager::hasVideo() const   { return _streamOrchestrator->hasVideo(); }

bool VideoManager::isBackendAvailable(VideoReceiver::BackendKind kind)
{
    return VideoBackendRegistry::instance().isAvailable(kind);
}

void VideoManager::setfullScreen(bool on)
{
    if (on) {
        if (!_activeVehicle || _activeVehicle->vehicleLinkManager()->communicationLost())
            on = false;
    }
    if (on != _fullScreen) {
        _fullScreen = on;
        emit fullScreenChanged();
    }
}

QString VideoManager::imageFile() const
{
    return _recordingCoordinator->imageFile();
}

bool VideoManager::isStreamSource() const
{
    switch (_videoSettings->currentSourceType()) {
        case VideoSettings::SourceType::NoVideo:
        case VideoSettings::SourceType::Disabled:
        case VideoSettings::SourceType::UVC:
        case VideoSettings::SourceType::Unknown:
            break;
        default:
            return true;
    }
    return autoStreamConfigured();
}

bool VideoManager::autoStreamConfigured() const
{
    auto* s = _streamOrchestrator->primaryStream();
    auto* info = s ? s->videoStreamInfo() : nullptr;
    return info && !info->isThermal() && !info->uri().isEmpty();
}

// ═══════════════════════════════════════════════════════════════════════════
// Cleanup
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::cleanup()
{
    // SettingsManager may outlive us — drop the gstDebugLevel lambda now.
    disconnect(_gstDebugLevelConn);
    _streamOrchestrator->cleanup();
}

// ═══════════════════════════════════════════════════════════════════════════
// Video start/stop — delegate to orchestrator
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::startVideo() { _streamOrchestrator->startVideo(); }
void VideoManager::stopVideo()  { _streamOrchestrator->stopVideo(); }

// ═══════════════════════════════════════════════════════════════════════════
// Vehicle management
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::_setActiveVehicle(Vehicle* vehicle)
{
    qCDebug(VideoManagerLog) << Q_FUNC_INFO << "new:" << vehicle << "old:" << _activeVehicle;

    if (_activeVehicle)
        disconnect(_commLostConn);

    _activeVehicle = vehicle;

    // Comm-lost wiring follows the active vehicle regardless of recording —
    // it only drives fullscreen exit, no stream lifecycle impact.
    if (_activeVehicle) {
        _commLostConn = connect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged,
                                this, [this](bool lost) {
                                    if (lost)
                                        setfullScreen(false);
                                });
    } else {
        setfullScreen(false);
    }

    QGCCameraManager* newCameraManager = vehicle ? vehicle->cameraManager() : nullptr;

    // Defer the camera/stream rebind while recording. Switching `videoStreamInfo`
    // mid-recording triggers `updateFromSettings` → URI change → restart, which
    // tears down the recorder and restarts it against the new vehicle's stream
    // — the recording file ends up with mixed A+gap+B data. The deferred swap
    // fires from the orchestrator's `recordingChanged(false)` handler above.
    if (recording()) {
        qCInfo(VideoManagerLog)
            << "Recording in progress — deferring camera/stream swap until stop";
        _pendingCameraManager = newCameraManager;
        _hasPendingCameraSwap = true;
        return;
    }

    _applyCameraManager(newCameraManager);
}

void VideoManager::_applyCameraManager(QGCCameraManager* cm)
{
    _streamOrchestrator->setCameraManager(cm);
    // Bridge handles stream stop/resume on the old/new camera and all
    // camera-control signal (dis)connection.
    _cameraBridge->setCamera(cm ? cm->currentCameraInstance() : nullptr);
}

