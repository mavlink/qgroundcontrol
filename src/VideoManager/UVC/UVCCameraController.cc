#include "UVCCameraController.h"

#include <algorithm>
#include <optional>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>
#include <QtCore/QMimeType>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QMediaRecorder>
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

}  // namespace

UVCCameraController::UVCCameraController(QObject* parent)
    : QObject(parent)
    , _monitor(new UVCStreamMonitor(this))
    , _frameRelay(new UVCFrameRelay(this))
    , _mediaDevices(new QMediaDevices(this))
    , _retryTimer(new QTimer(this))
    , _imageCapture(new QImageCapture(this))
{
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

namespace {

QString uvcFileSuffix(const QMediaFormat& format)
{
    switch (format.fileFormat()) {
        case QMediaFormat::Matroska:
            return QStringLiteral("mkv");
        case QMediaFormat::QuickTime:
            return QStringLiteral("mov");
        case QMediaFormat::MPEG4:
            return QStringLiteral("mp4");
        default:
            return format.mimeType().preferredSuffix();
    }
}

/// The containers a recording can go into, as VideoSettings::recordingFormat offers them.
constexpr QMediaFormat::FileFormat kRecordingContainers[] = {QMediaFormat::MPEG4, QMediaFormat::Matroska,
                                                             QMediaFormat::QuickTime};

/// The codecs a recording falls back to, best first.
constexpr QMediaFormat::VideoCodec kFallbackCodecs[] = {
    QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::AV1,
    QMediaFormat::VideoCodec::VP9,  QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::MPEG2,
};

std::optional<QMediaFormat> encodableFormat(QMediaFormat::FileFormat container, QMediaFormat::VideoCodec codec)
{
    QMediaFormat format(container);
    format.setVideoCodec(codec);
    return format.isSupported(QMediaFormat::Encode) ? std::optional(format) : std::nullopt;
}

void removeIfEmpty(const QString& file)
{
    const QFileInfo info(file);
    if (info.exists() && (info.size() == 0)) {
        (void) QFile::remove(file);
    }
}

}  // namespace

QList<QMediaFormat> UVCCameraController::recordingFormats(QMediaFormat::FileFormat container,
                                                          QMediaFormat::VideoCodec codec)
{
    QList<QMediaFormat::FileFormat> containers{container};
    for (const QMediaFormat::FileFormat other : kRecordingContainers) {
        if (!containers.contains(other)) {
            containers.append(other);
        }
    }
    QList<QMediaFormat> formats;
    if (codec != QMediaFormat::VideoCodec::Unspecified) {
        for (const QMediaFormat::FileFormat candidate : std::as_const(containers)) {
            if (const std::optional<QMediaFormat> format = encodableFormat(candidate, codec)) {
                formats.append(*format);
                break;
            }
        }
    }
    for (const QMediaFormat::FileFormat candidate : std::as_const(containers)) {
        QList<QMediaFormat> fallbacks;
        for (const QMediaFormat::VideoCodec fallback : kFallbackCodecs) {
            if (const std::optional<QMediaFormat> format = encodableFormat(candidate, fallback)) {
                fallbacks.append(*format);
            }
        }
        if (!fallbacks.isEmpty()) {
            for (const QMediaFormat& format : std::as_const(fallbacks)) {
                if (!formats.contains(format)) {
                    formats.append(format);
                }
            }
            break;
        }
    }
    return formats;
}

QStringList UVCCameraController::recordingCodecs()
{
    QList<QMediaFormat::VideoCodec> codecs(std::begin(kFallbackCodecs), std::end(kFallbackCodecs));
    const QMetaEnum codecEnum = QMetaEnum::fromType<QMediaFormat::VideoCodec>();
    for (int i = 0; i < codecEnum.keyCount(); i++) {
        const auto codec = static_cast<QMediaFormat::VideoCodec>(codecEnum.value(i));
        if ((codec != QMediaFormat::VideoCodec::Unspecified) && !codecs.contains(codec)) {
            codecs.append(codec);
        }
    }

    QStringList keys;
    for (const QMediaFormat::VideoCodec codec : std::as_const(codecs)) {
        const bool encodable = std::ranges::any_of(kRecordingContainers, [codec](QMediaFormat::FileFormat container) {
            return encodableFormat(container, codec).has_value();
        });
        if (encodable) {
            keys.append(QString::fromLatin1(codecEnum.valueToKey(static_cast<int>(codec))));
        }
    }
    return keys;
}

QMediaFormat::VideoCodec UVCCameraController::recordingCodec(const QString& key)
{
    bool ok = false;
    const int value = QMetaEnum::fromType<QMediaFormat::VideoCodec>().keyToValue(key.toLatin1().constData(), &ok);
    return ok ? static_cast<QMediaFormat::VideoCodec>(value) : QMediaFormat::VideoCodec::Unspecified;
}

QString UVCCameraController::recordingCodecName(const QString& key)
{
    return QMediaFormat::videoCodecDescription(recordingCodec(key));
}

void UVCCameraController::startRecording(const QString& file, QMediaFormat::FileFormat container,
                                         QMediaFormat::VideoCodec codec)
{
    const bool recorderBusy = _recorder && (_recorder->recorderState() != QMediaRecorder::StoppedState);
    if (recorderBusy || !_captureSession || !canRecord()) {
        qCWarning(UVCCameraControllerLog) << "Can't record the camera now:" << file;
        emit recordingFinished(file, tr("The USB camera isn't running"));
        return;
    }
    _pendingFormats = recordingFormats(container, codec);
    if (_pendingFormats.isEmpty()) {
        qCWarning(UVCCameraControllerLog) << "No video encoder:" << file;
        emit recordingFinished(file, tr("No video encoder is available"));
        return;
    }
    if (!_recorder) {
        _recorder = new QMediaRecorder(this);
        (void) connect(_recorder, &QMediaRecorder::recorderStateChanged, this,
                       &UVCCameraController::_onRecorderStateChanged);
        (void) connect(_recorder, &QMediaRecorder::errorOccurred, this,
                       [this](QMediaRecorder::Error, const QString& errorString) { _onRecorderError(errorString); });
        // The first encoded frame starts the recording: Qt reports RecordingState before its encoder is set up.
        (void) connect(_recorder, &QMediaRecorder::durationChanged, this, [this](qint64 duration) {
            if (!_recording && (duration > 0) && (_recorder->recorderState() == QMediaRecorder::RecordingState)) {
                _recording = true;
                emit recordingStarted(_recordingFile);
                emit recordingChanged(true);
            }
        });
    }
    _captureSession->setRecorder(_recorder);

    const QFileInfo info(file);
    _recordingBase = QStringLiteral("%1/%2").arg(info.path(), info.completeBaseName());
    _stopRequested = false;
    _recordNextFormat();
}

void UVCCameraController::_recordNextFormat()
{
    const QMediaFormat format = _pendingFormats.takeFirst();
    _recordingFile = QStringLiteral("%1.%2").arg(_recordingBase, uvcFileSuffix(format));
    _recordingError.clear();
    qCDebug(UVCCameraControllerLog) << "record" << _recordingFile << format.fileFormat() << format.videoCodec();
    _recorder->setMediaFormat(format);
    _recorder->setQuality(QMediaRecorder::HighQuality);
    _recorder->setOutputLocation(QUrl::fromLocalFile(_recordingFile));
    _recorder->record();
}

void UVCCameraController::stopRecording()
{
    _stopRequested = true;
    if (_recorder) {
        _recorder->stop();
    }
}

void UVCCameraController::stopRecordingAndWait(int timeoutMs)
{
    if (!_recording) {
        return;
    }
    QEventLoop loop;
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    (void) connect(this, &UVCCameraController::recordingChanged, &loop, [&loop](bool recording) {
        if (!recording) {
            loop.quit();
        }
    });
    stopRecording();
    if (_recording) {
        (void) loop.exec();
    }
}

void UVCCameraController::_onRecorderStateChanged(QMediaRecorder::RecorderState state)
{
    if (state != QMediaRecorder::StoppedState) {
        return;
    }
    if (_recording) {
        _recording = false;
        emit recordingFinished(_recordingFile, _recordingError);
        emit recordingChanged(false);
    } else {
        _onRecordingNotStarted();
    }
}

void UVCCameraController::_onRecorderError(const QString& errorString)
{
    qCWarning(UVCCameraControllerLog) << "Recording error:" << _recordingFile << errorString;
    _recordingError = errorString;
    // A recorder that is still running reports the end when it stops.
    if (!_recording && (_recorder->recorderState() == QMediaRecorder::StoppedState)) {
        _onRecordingNotStarted();
    }
}

void UVCCameraController::_onRecordingNotStarted()
{
    removeIfEmpty(_recordingFile);
    if (_recordingError.isEmpty() || _stopRequested) {
        return;
    }
    if (!_pendingFormats.isEmpty() && _captureSession) {
        qCDebug(UVCCameraControllerLog) << "Encoder failed, trying the next format:" << _recordingError;
        // Not from the recorder's own signal.
        QMetaObject::invokeMethod(this, &UVCCameraController::_recordNextFormat, Qt::QueuedConnection);
        return;
    }
    const QString error = std::exchange(_recordingError, QString());
    emit recordingFinished(_recordingFile, error);
}

bool UVCCameraController::canTakeScreenshot() const
{
    return _camera && _camera->isActive() && _videoSink();
}

bool UVCCameraController::takeScreenshot(const QString& file) const
{
    QVideoSink* const videoSink = _videoSink();
    if (!videoSink) {
        qCWarning(UVCCameraControllerLog) << "No video output to take a screenshot from";
        return false;
    }
    // The frame on display carries the transform already (UVCFrameRelay).
    return VideoScreenshot::save(videoSink->videoFrame(), file);
}

void UVCCameraController::setTransform(QtVideo::Rotation rotation, bool mirrored)
{
    _frameRelay->setTransform(rotation, mirrored);
}

void UVCCameraController::capturePhoto(const QString& file)
{
    const int id =
        (_captureSession && canTakeScreenshot() && _imageCapture->isReadyForCapture()) ? _imageCapture->capture() : -1;
    if (id < 0) {
        _savePhotoOnDisplay(file);
        return;
    }
    qCDebug(UVCCameraControllerLog) << "photo" << id << file;
    _photoFiles.insert(id, file);
}

void UVCCameraController::_onImageAvailable(int id, const QVideoFrame& frame)
{
    const QString file = _photoFiles.take(id);
    if (file.isEmpty()) {
        return;
    }
    const QtVideo::Rotation rotation = _frameRelay->rotation();
    const bool mirrored = _frameRelay->mirrored();
    // Converting and encoding a full-resolution photo would stall the UI.
    (void) QtConcurrent::run(
        [frame, file, rotation, mirrored] { (void) VideoScreenshot::save(frame, file, rotation, mirrored); });
}

void UVCCameraController::_onImageCaptureError(int id, const QString& errorString)
{
    const QString file = _photoFiles.take(id);
    qCWarning(UVCCameraControllerLog) << "Photo capture failed:" << id << file << errorString;
    if (!file.isEmpty()) {
        _savePhotoOnDisplay(file);
    }
}

void UVCCameraController::_savePhotoOnDisplay(const QString& file) const
{
    QVideoSink* const videoSink = _videoSink();
    if (!videoSink) {
        qCWarning(UVCCameraControllerLog) << "No video output to take a photo from";
        return;
    }
    // Converting and encoding a full-resolution photo would stall the UI. The frame on display carries the transform
    // already (UVCFrameRelay).
    (void) QtConcurrent::run([frame = videoSink->videoFrame(), file] { (void) VideoScreenshot::save(frame, file); });
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
    _captureSession = new QMediaCaptureSession(this);
    _captureSession->setCamera(_camera);
    if (_recorder) {
        _captureSession->setRecorder(_recorder);
    }
    if (kStillPhotos) {
        _captureSession->setImageCapture(_imageCapture);
    }
}

void UVCCameraController::_releaseCamera()
{
    // The capture session's photos still pending won't come: save the frame on display instead.
    for (const QString& file : std::as_const(_photoFiles)) {
        _savePhotoOnDisplay(file);
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
