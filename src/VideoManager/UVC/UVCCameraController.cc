#include "UVCCameraController.h"

#include <algorithm>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QTimer>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QVideoSink>
#include <QtQuick/QQuickItem>

#include "AppMessages.h"
#include "QGCLoggingCategory.h"
#include "UVCDevices.h"
#include "UVCFrameRelay.h"
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

// Qt's V4L2 camera (Qt 6.12) reads the focus range into the exposure compensation range and never sets its focus
// range, so neither control does what it is asked.
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
    : QObject(parent)
    , _monitor(new UVCStreamMonitor(this))
    , _frameRelay(new UVCFrameRelay(this))
    , _mediaDevices(new QMediaDevices(this))
    , _retryTimer(new QTimer(this))
    , _imageCapture(new QImageCapture(this))
    , _recorder(new QtVideoRecorder(this))
{
    (void) connect(_recorder, &QtVideoRecorder::recordingChanged, this, &UVCCameraController::recordingChanged);
    (void) connect(_recorder, &QtVideoRecorder::recordingStarted, this, &UVCCameraController::recordingStarted);
    (void) connect(_recorder, &QtVideoRecorder::recordingFinished, this, &UVCCameraController::recordingFinished);
    (void) connect(_imageCapture, &QImageCapture::imageAvailable, this, &UVCCameraController::_onImageAvailable);
    (void) connect(
        _imageCapture, &QImageCapture::errorOccurred, this,
        [this](int id, QImageCapture::Error, const QString& errorString) { _onImageCaptureError(id, errorString); });
    _retryTimer->setSingleShot(true);
    _retryTimer->setInterval(kRetryIntervalMs);
    (void) connect(_retryTimer, &QTimer::timeout, this, &UVCCameraController::_recreateCamera);
    // Frames flowing again end a run of retries.
    (void) connect(_monitor, &UVCStreamMonitor::statsChanged, this, [this] {
        if (_monitor->stats().valid) {
            _retries = 0;
        }
    });
}

UVCCameraController::~UVCCameraController()
{
    _releaseCamera();
}

void UVCCameraController::setDevice(const QString& description)
{
    if (description == _deviceName) {
        return;
    }
    qCDebug(UVCCameraControllerLog) << "device" << description;
    _retryTimer->stop();
    _retries = 0;
    _deviceName = description;
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
    if (!videoOutput || (videoOutput == _videoOutput)) {
        return;
    }
    qCDebug(UVCCameraControllerLog) << "video output" << videoOutput;
    _videoOutput = videoOutput;
    _connectVideoOutput();
}

void UVCCameraController::unregisterVideoOutput(QQuickItem* videoOutput)
{
    if (!videoOutput || (videoOutput != _videoOutput)) {
        return;
    }
    qCDebug(UVCCameraControllerLog) << "video output removed" << videoOutput;
    _videoOutput = nullptr;
    _connectVideoOutput();
}

void UVCCameraController::startRecording(const QString& file, QMediaFormat::FileFormat container,
                                         QMediaFormat::VideoCodec codec)
{
    if (_recorder->active() || !_captureSession || !canRecord()) {
        qCWarning(UVCCameraControllerLog) << "Can't record the camera now:" << file;
        emit recordingFinished(file, tr("The USB camera isn't running"));
        return;
    }
    _recorder->start(file, container, codec);
}

void UVCCameraController::stopRecording()
{
    _recorder->stop();
}

void UVCCameraController::stopRecordingAndWait(int timeoutMs)
{
    _recorder->stopAndWait(timeoutMs);
}

bool UVCCameraController::canTakeScreenshot() const
{
    return _camera && _camera->isActive() && _videoSink();
}

bool UVCCameraController::takeScreenshot(const QString& file, const VideoScreenshot::Exif& exif) const
{
    QVideoSink* const videoSink = _videoSink();
    if (!videoSink) {
        qCWarning(UVCCameraControllerLog) << "No video output to take a screenshot from";
        return false;
    }
    // The frame on display carries the transform already (UVCFrameRelay).
    return VideoScreenshot::save(videoSink->videoFrame(), file, exif);
}

void UVCCameraController::setTransform(QtVideo::Rotation rotation, bool mirrored)
{
    _frameRelay->setTransform(rotation, mirrored);
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

void UVCCameraController::capturePhoto(const QString& file, const VideoScreenshot::Exif& exif)
{
    const int id =
        (_captureSession && canTakeScreenshot() && _imageCapture->isReadyForCapture()) ? _imageCapture->capture() : -1;
    if (id < 0) {
        _savePhotoOnDisplay(file, exif);
        return;
    }
    qCDebug(UVCCameraControllerLog) << "photo" << id << file;
    _photoFiles.insert(id, {file, exif});
}

void UVCCameraController::_onImageAvailable(int id, const QVideoFrame& frame)
{
    const PendingPhoto photo = _photoFiles.take(id);
    if (photo.file.isEmpty()) {
        return;
    }
    const QtVideo::Rotation rotation = _frameRelay->rotation();
    const bool mirrored = _frameRelay->mirrored();
    // Converting and encoding a full-resolution photo would stall the UI.
    (void) QtConcurrent::run([frame, photo, rotation, mirrored] {
        (void) VideoScreenshot::save(frame, photo.file, photo.exif, rotation, mirrored);
    });
}

void UVCCameraController::_onImageCaptureError(int id, const QString& errorString)
{
    const PendingPhoto photo = _photoFiles.take(id);
    qCWarning(UVCCameraControllerLog) << "Photo capture failed:" << id << photo.file << errorString;
    if (!photo.file.isEmpty()) {
        _savePhotoOnDisplay(photo.file, photo.exif);
    }
}

void UVCCameraController::_savePhotoOnDisplay(const QString& file, const VideoScreenshot::Exif& exif) const
{
    QVideoSink* const videoSink = _videoSink();
    if (!videoSink) {
        qCWarning(UVCCameraControllerLog) << "No video output to take a photo from";
        return;
    }
    // Converting and encoding a full-resolution photo would stall the UI. The frame on display carries the transform
    // already (UVCFrameRelay).
    (void) QtConcurrent::run(
        [frame = videoSink->videoFrame(), file, exif] { (void) VideoScreenshot::save(frame, file, exif); });
}

void UVCCameraController::_createCamera()
{
    const QCameraDevice device = UVCDevices::device(_deviceName);
    const QStringList formats = UVCDevices::formatNames(device);
    if (formats != _formats) {
        _formats = formats;
        emit formatsChanged();
    }
    if (_deviceName.isEmpty()) {
        return;
    }
    if (device.isNull()) {
        qCWarning(UVCCameraControllerLog) << "No camera named" << _deviceName;
        return;
    }

    UVCDevices::requestPermission();
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
    for (const PendingPhoto& photo : std::as_const(_photoFiles)) {
        _savePhotoOnDisplay(photo.file, photo.exif);
    }
    _photoFiles.clear();
    _monitor->detach();
    delete _captureSession;
    _captureSession = nullptr;
    delete _camera;
    _camera = nullptr;
    _cameraId.clear();
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
    QVideoSink* const videoSink = _videoSink();
    _frameRelay->setOutput(videoSink);
    if (_captureSession) {
        _captureSession->setVideoSink(videoSink ? _frameRelay->input() : nullptr);
    }
    if (_camera && videoSink) {
        _monitor->attach(_camera, videoSink);
    } else {
        _monitor->detach();
    }
    if (_camera) {
        _camera->setActive(videoSink != nullptr);
    }
}

void UVCCameraController::_onCameraError(const QString& errorString)
{
    qCWarning(UVCCameraControllerLog) << "Camera error:" << _deviceName << errorString << "retries" << _retries;
    if (_retries >= kMaxRetries) {
        QGC::showAppMessage(tr("USB camera %1: %2").arg(_deviceName, errorString));
        return;
    }
    if (_retries == 0) {
        QGC::showAppMessage(tr("USB camera %1: %2. Retrying.").arg(_deviceName, errorString));
    }
    ++_retries;
    // Deferred: the camera emitting this error can't be deleted from its own signal.
    _retryTimer->start();
}

void UVCCameraController::_onVideoInputsChanged()
{
    if (!_cameraId.isEmpty() && !_cameraAttached()) {
        // Qt stops an unplugged camera without an error.
        qCWarning(UVCCameraControllerLog) << "Camera unplugged:" << _deviceName << _cameraId;
        QGC::showAppMessage(tr("USB camera %1 was disconnected").arg(_deviceName));
        // Runs another camera of the same name, if one is attached.
        _recreateCamera();
    } else if (!_deviceName.isEmpty() && !_camera && _cameraAttached()) {
        // Plugged back in.
        _recreateCamera();
    }
    emit camerasChanged();
}

bool UVCCameraController::_cameraAttached() const
{
    if (_cameraId.isEmpty()) {
        return UVCDevices::deviceExists(_deviceName);
    }
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    return std::any_of(cameras.cbegin(), cameras.cend(),
                       [this](const QCameraDevice& camera) { return camera.id() == _cameraId; });
}

QVideoSink* UVCCameraController::_videoSink() const
{
    return _videoOutput ? _videoOutput->property("videoSink").value<QVideoSink*>() : nullptr;
}
