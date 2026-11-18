#include "UVCCameraController.h"

#include <algorithm>
#include <utility>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQuick/QQuickItem>

#include "AppMessages.h"
#include "QGCLoggingCategory.h"
#include "QtVideoRecorder.h"
#include "UVCDevices.h"
#include "UVCStreamMonitor.h"
#include "VideoScreenshot.h"

QGC_LOGGING_CATEGORY(UVCCameraControllerLog, "Video.UVCCameraController")

namespace {

// Qt takes still photos with Android's and Apple's cameras. With the others QImageCapture converts the camera's next
// frame on the camera's thread, the GUI thread for a V4L2 camera, so the photo is the frame on display instead.
#if defined(Q_OS_ANDROID) || defined(Q_OS_DARWIN)
constexpr bool kStillPhotos = true;
#else
constexpr bool kStillPhotos = false;
#endif

// Qt defect, seen in Qt 6.12.0: QV4L2Camera::initV4L2Controls() reads the focus range into the exposure compensation
// range and never sets its focus range (qtmultimedia/src/plugins/multimedia/ffmpeg/qv4l2camera.cpp), so neither
// control does what it is asked. Fixed after 6.12.0 by qtmultimedia b7f6f39957 (6.12 branch): remove this constant and
// its two uses once the minimum Qt has that fix.
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
constexpr bool kExposureAndFocusControls = false;
#else
constexpr bool kExposureAndFocusControls = true;
#endif

constexpr QCamera::WhiteBalanceMode kWhiteBalancePresets[] = {
    QCamera::WhiteBalanceSunlight, QCamera::WhiteBalanceCloudy,      QCamera::WhiteBalanceShade,
    QCamera::WhiteBalanceTungsten, QCamera::WhiteBalanceFluorescent, QCamera::WhiteBalanceFlash,
    QCamera::WhiteBalanceSunset,
};

}  // namespace

UVCCameraController::UVCCameraController(QObject* parent)
    : VideoReceiver(parent)
    , _monitor(new UVCStreamMonitor(this))
    , _mediaDevices(new QMediaDevices(this))
    , _retryTimer(new QTimer(this))
    , _imageCapture(new QImageCapture(this))
    , _recorder(new QtVideoRecorder(this))
{
    (void) connect(_recorder, &QtVideoRecorder::recordingChanged, this, &UVCCameraController::setRecording);
    (void) connect(_recorder, &QtVideoRecorder::recordingStarted, this, &UVCCameraController::recordingStarted);
    (void) connect(_recorder, &QtVideoRecorder::recordingFinished, this, &UVCCameraController::recordingFinished);
    (void) connect(_imageCapture, &QImageCapture::imageAvailable, this, &UVCCameraController::_onImageAvailable);
    (void) connect(
        _imageCapture, &QImageCapture::errorOccurred, this,
        [this](int id, QImageCapture::Error, const QString& errorString) { _onImageCaptureError(id, errorString); });
    _retryTimer->setSingleShot(true);
    _retryTimer->setInterval(kRetryInterval);
    (void) connect(_retryTimer, &QTimer::timeout, this, &UVCCameraController::_recreateCamera);
    (void) connect(_monitor, &UVCStreamMonitor::statsChanged, this, [this] {
        setStats(_monitor->stats());
        // Frames flowing again end a run of retries.
        if (_monitor->stats().valid) {
            _retries = 0;
        }
    });
}

UVCCameraController::~UVCCameraController()
{
    _releaseCamera();
}

void UVCCameraController::onConfigChanged()
{
    qCDebug(UVCCameraControllerLog) << "device" << device();
    _startOver();
}

void UVCCameraController::onActiveChanged()
{
    qCDebug(UVCCameraControllerLog) << "active" << active();
    if (!active()) {
        _recorder->stop();
    }
    _startOver();
}

void UVCCameraController::onRestartRequested()
{
    // A camera that runs has no display sink to rebuild, and a new one would end its recording.
    if (_gaveUp || _retryTimer->isActive()) {
        _startOver();
    }
}

void UVCCameraController::onReconnectRequested()
{
    // The retry keeps to its timer: a USB camera doesn't come back with a vehicle's link.
}

void UVCCameraController::shutDown()
{
    _recorder->stopAndWait();
    VideoReceiver::shutDown();
}

void UVCCameraController::_startOver()
{
    _retryTimer->stop();
    _retries = 0;
    _recreateCamera();
}

void UVCCameraController::setFormat(const QString& name)
{
    if (name == _formatName) {
        return;
    }
    qCDebug(UVCCameraControllerLog) << "format" << name;
    _formatName = name;
    if (_camera) {
        _recreateCamera();
    }
}

void UVCCameraController::watchCameras()
{
    (void) connect(_mediaDevices, &QMediaDevices::videoInputsChanged, this, &UVCCameraController::_onVideoInputsChanged,
                   Qt::UniqueConnection);
}

void UVCCameraController::registerVideoOutput(QQuickItem* videoOutput)
{
    // Not for the output already set: the camera has nothing to bind anew, and its measurements would start over.
    if (videoOutput && (videoOutput != this->videoOutput())) {
        setVideoOutput(videoOutput);
    }
}

void UVCCameraController::unregisterVideoOutput(QQuickItem* videoOutput)
{
    if (!videoOutput || (videoOutput != this->videoOutput())) {
        return;
    }
    // Queued: the output that takes this one's place, in the main picture or in a tile, registers in the same pass.
    (void) QMetaObject::invokeMethod(
        this,
        [this, output = QPointer<QQuickItem>(videoOutput)] {
            if (output == this->videoOutput()) {
                setVideoOutput(nullptr);
            }
        },
        Qt::QueuedConnection);
}

void UVCCameraController::onVideoOutputChanged()
{
    qCDebug(UVCCameraControllerLog) << "video output" << videoOutput();
    _connectVideoOutput();
}

void UVCCameraController::onStartRecording(const QString& file, FileFormat format)
{
    if (_recorder->active()) {
        qCWarning(UVCCameraControllerLog) << "Not recording the camera to" << file << "- recording already";
        emit recordingFinished(file, tr("A recording is in progress already"));
        return;
    }
    if (!_running()) {
        qCWarning(UVCCameraControllerLog) << "Can't record the camera now:" << file;
        emit recordingFinished(file, tr("The USB camera isn't running"));
        return;
    }
    _recorder->start(file, QtVideoRecorder::container(format), recordingEncoding().codec, recordingEncoding().quality);
}

void UVCCameraController::onStopRecording()
{
    _recorder->stop();
}

bool UVCCameraController::_running() const
{
    return _camera && _camera->isActive() && videoSink();
}

void UVCCameraController::setTransform(QtVideo::Rotation rotation, bool mirrored)
{
    if ((rotation == _rotation) && (mirrored == _mirrored)) {
        return;
    }
    _rotation = rotation;
    _mirrored = mirrored;
    emit transformChanged();
}

void UVCCameraController::setControls(const Controls& controls)
{
    if (controls == _controls) {
        return;
    }
    _controls = controls;
    _applyControls();
}

bool UVCCameraController::whiteBalanceAdjustable() const
{
    return _camera && std::ranges::any_of(kWhiteBalancePresets, [this](QCamera::WhiteBalanceMode mode) {
               return _camera->isWhiteBalanceModeSupported(mode);
           });
}

qreal UVCCameraController::minimumZoom() const
{
    return _camera ? _camera->minimumZoomFactor() : 1.;
}

qreal UVCCameraController::maximumZoom() const
{
    return _camera ? std::max(_camera->maximumZoomFactor(), _camera->minimumZoomFactor()) : 1.;
}

bool UVCCameraController::exposureAdjustable() const
{
    return kExposureAndFocusControls && _camera &&
           _camera->supportedFeatures().testFlag(QCamera::Feature::ExposureCompensation);
}

bool UVCCameraController::focusAdjustable() const
{
    return kExposureAndFocusControls && _camera && _camera->isFocusModeSupported(QCamera::FocusModeInfinity);
}

void UVCCameraController::_applyControls()
{
    if (!_camera) {
        return;
    }
    // QCamera leaves a white balance or focus mode the camera lacks as it is.
    _camera->setWhiteBalanceMode(_controls.whiteBalance);
    if (maximumZoom() > minimumZoom()) {
        _camera->setZoomFactor(std::clamp(static_cast<qreal>(_controls.zoom), minimumZoom(), maximumZoom()));
    }
    if (exposureAdjustable()) {
        _camera->setExposureCompensation(_controls.exposureCompensation);
    }
    if (focusAdjustable() && _camera->isFocusModeSupported(_controls.focusMode)) {
        _camera->setFocusMode(_controls.focusMode);
    }
}

bool UVCCameraController::onTakeScreenshot(const QString& file, const VideoScreenshot::Exif& exif)
{
    const int id = (_running() && _imageCapture->isReadyForCapture()) ? _imageCapture->capture() : -1;
    if (id < 0) {
        return VideoReceiver::onTakeScreenshot(file, exif);
    }
    qCDebug(UVCCameraControllerLog) << "photo" << id << file;
    _photoFiles.insert(id, {file, exif});
    return true;
}

void UVCCameraController::_onImageAvailable(int id, const QVideoFrame& frame)
{
    const PendingPhoto photo = _photoFiles.take(id);
    if (photo.file.isEmpty()) {
        return;
    }
    _savePhoto(frame, photo);
}

void UVCCameraController::_onImageCaptureError(int id, const QString& errorString)
{
    const PendingPhoto photo = _photoFiles.take(id);
    qCWarning(UVCCameraControllerLog) << "Photo capture failed:" << id << photo.file << errorString;
    if (!photo.file.isEmpty()) {
        _savePhotoOnDisplay(photo);
    }
}

void UVCCameraController::_savePhotoOnDisplay(const PendingPhoto& photo)
{
    // Not takeScreenshot(), which would ask for another capture.
    if (!videoSink() || !VideoReceiver::onTakeScreenshot(photo.file, photo.exif)) {
        qCWarning(UVCCameraControllerLog) << "No video on display to take the photo from:" << photo.file;
        emit errorOccurred(tr("Unable to take a photo: no video is on display."));
    }
}

void UVCCameraController::_savePhoto(const QVideoFrame& frame, const PendingPhoto& photo)
{
    // Converting and encoding a full-resolution photo would stall the UI.
    (void) QtConcurrent::run([frame, photo, rotation = _rotation, mirrored = _mirrored] {
        return VideoScreenshot::save(frame, photo.file, photo.exif, rotation, mirrored);
    }).then(this, [this, file = photo.file](bool saved) {
        if (!saved) {
            emit errorOccurred(tr("Unable to save the photo %1.").arg(QFileInfo(file).fileName()));
        }
    });
}

void UVCCameraController::_createCamera()
{
    const QString deviceName = active() ? device() : QString();
    const QCameraDevice device = UVCDevices::device(deviceName);
    const QStringList formats = UVCDevices::formatNames(device);
    if (formats != _formats) {
        _formats = formats;
        emit formatsChanged();
    }
    if (deviceName.isEmpty()) {
        return;
    }
    if (device.isNull()) {
        qCWarning(UVCCameraControllerLog) << "No camera named" << deviceName;
        return;
    }

    // Until the user grants the permission, Qt's Android and Apple cameras refuse to start, without an error: the
    // camera starts once it is granted.
    UVCDevices::requestPermission(this, [this] { _connectVideoOutput(); });
    _cameraId = device.id();
    _camera = new QCamera(device, this);
    if (const QCameraFormat format = UVCDevices::format(device, _formatName); !format.isNull()) {
        qCDebug(UVCCameraControllerLog) << "camera format" << _formatName;
        _camera->setCameraFormat(format);
    }
    (void) connect(_camera, &QCamera::errorOccurred, this,
                   [this](QCamera::Error, const QString& errorString) { _onCameraError(errorString); });
    // Some platforms know what a camera supports, or take its settings, only once it runs.
    (void) connect(_camera, &QCamera::supportedFeaturesChanged, this, &UVCCameraController::controlsChanged);
    (void) connect(_camera, &QCamera::minimumZoomFactorChanged, this, &UVCCameraController::controlsChanged);
    (void) connect(_camera, &QCamera::maximumZoomFactorChanged, this, &UVCCameraController::controlsChanged);
    (void) connect(_camera, &QCamera::activeChanged, this, [this](bool active) {
        if (active) {
            _applyControls();
            emit controlsChanged();
        }
    });
    _applyControls();
    _captureSession = new QMediaCaptureSession(this);
    _captureSession->setCamera(_camera);
    _captureSession->setRecorder(_recorder->mediaRecorder());
    if (kStillPhotos) {
        _captureSession->setImageCapture(_imageCapture);
    }
}

void UVCCameraController::_releaseCamera()
{
    // The capture session's photos still pending won't come: save the frame on display instead.
    for (const PendingPhoto& photo : std::exchange(_photoFiles, {})) {
        _savePhotoOnDisplay(photo);
    }
    _monitor->detach();
    delete _captureSession;
    _captureSession = nullptr;
    delete _camera;
    _camera = nullptr;
    _cameraId.clear();
    _framesArriving = false;
    _gaveUp = false;
}

void UVCCameraController::_recreateCamera()
{
    _releaseCamera();
    _createCamera();
    emit controlsChanged();
    _connectVideoOutput();
}

void UVCCameraController::_connectVideoOutput()
{
    QVideoSink* const sink = videoSink();
    (void) disconnect(_frameConnection);
    if (sink) {
        _frameConnection = connect(sink, &QVideoSink::videoFrameChanged, this, &UVCCameraController::_onVideoFrame);
    } else {
        _framesArriving = false;
    }
    if (_captureSession) {
        _captureSession->setVideoSink(sink);
    }
    if (_camera && sink) {
        _monitor->attach(_camera, sink);
    } else {
        _monitor->detach();
    }
    if (_camera) {
        _camera->setActive(sink != nullptr);
    }
    _updateState();
}

void UVCCameraController::_onVideoFrame(const QVideoFrame& frame)
{
    // The sink clears the display with an invalid frame when the camera stops.
    if (!_camera || !frame.isValid()) {
        return;
    }
    setVideoSize(presentedSize(frame));
    if (!_framesArriving) {
        _framesArriving = true;
        _updateState();
    }
}

void UVCCameraController::_updateState()
{
    State state = State::Stopped;
    if (_retryTimer->isActive()) {
        state = State::Reconnecting;
    } else if (_camera && !_gaveUp && videoSink()) {
        state = _framesArriving ? State::Streaming : State::Connecting;
    }
    setState(state);
    setDecoding(state == State::Streaming);
}

void UVCCameraController::_onCameraError(const QString& errorString)
{
    qCWarning(UVCCameraControllerLog) << "Camera error:" << device() << errorString << "retries" << _retries;
    _framesArriving = false;
    if (_retries >= kMaxRetries) {
        _gaveUp = true;
        QGC::showAppMessage(tr("USB camera %1: %2").arg(device(), errorString));
    } else {
        if (_retries == 0) {
            QGC::showAppMessage(tr("USB camera %1: %2. Retrying.").arg(device(), errorString));
        }
        ++_retries;
        // Deferred: the camera emitting this error can't be deleted from its own signal.
        _retryTimer->start();
    }
    _updateState();
}

void UVCCameraController::_onVideoInputsChanged()
{
    if (!_cameraId.isEmpty() && !_cameraAttached()) {
        // Qt stops an unplugged camera without an error.
        qCWarning(UVCCameraControllerLog) << "Camera unplugged:" << device() << _cameraId;
        QGC::showAppMessage(tr("USB camera %1 was disconnected").arg(device()));
        // Runs another camera of the same name, if one is attached.
        _recreateCamera();
    } else if (active() && !_camera && _cameraAttached()) {
        // Plugged back in.
        _recreateCamera();
    }
    emit camerasChanged();
}

bool UVCCameraController::_cameraAttached() const
{
    if (_cameraId.isEmpty()) {
        return UVCDevices::deviceExists(device());
    }
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    return std::any_of(cameras.cbegin(), cameras.cend(),
                       [this](const QCameraDevice& camera) { return camera.id() == _cameraId; });
}
