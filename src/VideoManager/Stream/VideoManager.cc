#include "VideoManager.h"

#include "AppSettings.h"
#include "CameraControlBridge.h"
#include "MultiVehicleManager.h"
#include "QGCCameraManager.h"
#include "QGCLoggingCategory.h"
#include "QtFutureHelpers.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "VideoBackendBootstrap.h"
#include "VideoCameraBinder.h"
#include "VideoMediaServices.h"
#include "VideoSettings.h"
#include "VideoStream.h"
#include "VideoStreamModel.h"
#include "VideoStreamOrchestrator.h"
#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#endif
#include <QtCore/QApplicationStatic>
#include <QtCore/QFuture>
#include <QtCore/QPointer>
#include <QtCore/QRunnable>
#include <QtCore/QStandardPaths>
#include <QtQuick/QQuickWindow>

QGC_LOGGING_CATEGORY(VideoManagerLog, "Video.VideoManager")

Q_APPLICATION_STATIC(VideoManager, _videoManagerInstance);


// ═══════════════════════════════════════════════════════════════════════════
// Construction / singleton
// ═══════════════════════════════════════════════════════════════════════════

VideoManager::VideoManager(QObject* parent)
    : QObject(parent),
      _videoSettings(SettingsManager::instance()->videoSettings()),
      _streamOrchestrator(new VideoStreamOrchestrator(_videoSettings, this)),
      _mediaServices(new VideoMediaServices(this)),
      _cameraBridge(new CameraControlBridge(
          [this]() {
              return CameraControlBridge::State{hasVideo(), decoding(), recording()};
          },
          this)),
      _cameraBinder(new VideoCameraBinder(_streamOrchestrator, _cameraBridge, this))
{
    qCDebug(VideoManagerLog) << this;

    _mediaServices->bindStreamOrchestrator(_streamOrchestrator);

    // Camera → facade verbs. Bridge filters the MAVLink camera-control protocol.
    (void)connect(_cameraBridge, &CameraControlBridge::localRecordingRequested, this,
                  [this]() { startRecording(); });
    (void)connect(_cameraBridge, &CameraControlBridge::localRecordingStopRequested, this,
                  [this]() { stopRecording(); });
    (void)connect(_cameraBridge, &CameraControlBridge::localImageCaptureRequested, this,
                  [this](const QString& path) { grabImage(path); });

    (void)connect(this, &VideoManager::_cameraStateDirty, _cameraBridge,
                  &CameraControlBridge::pushState);

    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::decodingChanged, this,
                  &VideoManager::_cameraStateDirty);
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::recordingChanged,
                  _cameraBinder, &VideoCameraBinder::setRecording);
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::recordingChanged, this,
                  [this](bool recording) {
                      Q_UNUSED(recording)
                      emit _cameraStateDirty();
                  });
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::hasVideoChanged, this, [this]() {
        emit hasVideoChanged();
        emit _cameraStateDirty();
    });
    (void)connect(_streamOrchestrator, &VideoStreamOrchestrator::isStreamSourceChanged,
                  this, &VideoManager::isStreamSourceChanged);

    (void)connect(_mediaServices, &VideoMediaServices::recordingStarted, this,
                  &VideoManager::recordingStarted);
    (void)connect(_mediaServices, &VideoMediaServices::imageFileChanged, this,
                  &VideoManager::imageFileChanged);

    _backendDisabledForUnitTests = VideoBackendBootstrap::shouldSkipForUnitTests();
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

    if (_backendDisabledForUnitTests) {
        qCInfo(VideoManagerLog) << "Backend initialization disabled for unit tests";
    }

    _backendInitFuture = VideoBackendBootstrap::start(_backendDisabledForUnitTests);
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
                                     if (GStreamer::isAvailable())
                                         GStreamer::setDebugLevel(value.toInt());
#else
        Q_UNUSED(value);
#endif
                                 });
    (void)connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                  &VideoManager::_setActiveVehicle);
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

    // Recover any sessions left behind by a prior crash. Deferred to keep
    // init() off the 3-second per-orphan probe path.
    _mediaServices->scheduleOrphanScan(
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
}

// ═══════════════════════════════════════════════════════════════════════════
// Recording — unified entry points; all work delegated to VideoStream
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::startRecording(const QString& videoFile)
{
    (void)_mediaServices->startRecording(
        videoFile,
        _streamOrchestrator,
        _activeVehicle,
        videoSize(),
        _videoSettings,
        SettingsManager::instance()->appSettings()->videoSavePath());
}

void VideoManager::stopRecording()
{
    _mediaServices->stopRecording();
}

// ═══════════════════════════════════════════════════════════════════════════
// Image grab
// ═══════════════════════════════════════════════════════════════════════════

void VideoManager::grabImage(const QString& imageFile)
{
    _mediaServices->grabImage(
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
    return _mediaServices->imageFile();
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
    return _streamOrchestrator->autoStreamConfigured();
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
    _cameraBinder->setCameraManager(newCameraManager);
}
