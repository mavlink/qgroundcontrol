#include "QtMultimediaReceiver.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QPermissions>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QCameraFormat>
#include <QtMultimedia/QImageCapture>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QPlaybackOptions>
#include <QtMultimedia/QVideoFrameFormat>

#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QtFfmpegRuntimePolicy.h"
#include "QtCameraDeviceDiscovery.h"
#include "QtVideoSinkRouter.h"
#include "QtPlaybackTrackPolicy.h"
#include "VideoFrameDelivery.h"
#include "VideoSourceResolver.h"

QGC_LOGGING_CATEGORY(QtMultimediaReceiverLog, "Video.QtMultimediaReceiver")

namespace {

QMediaPlayer* createMediaPlayer(QObject* parent)
{
    QtFfmpegRuntimePolicy::applyDefaults();
    return new QMediaPlayer(parent);
}

QString decodeDescription(const QString& codecName)
{
    const QString hwCandidates = QString::fromLocal8Bit(qgetenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES"));
    if (hwCandidates.isEmpty())
        return QStringLiteral("%1 (Qt FFmpeg)").arg(codecName);
    return QStringLiteral("%1 (Qt FFmpeg, HW candidates: %2)").arg(codecName, hwCandidates);
}

QUrl ingestPlaybackUrl()
{
    return QUrl::fromLocalFile(QDir::temp().absoluteFilePath(QStringLiteral("qgc-gstreamer-session.ts")));
}

}  // namespace

QtMultimediaReceiver::QtMultimediaReceiver(QObject* parent)
    : VideoReceiver(parent),
      _mediaPlayer(createMediaPlayer(this)),
      _sinkRouter(std::make_unique<QtVideoSinkRouter>(this))
{
    _startTimeoutTimer.setSingleShot(true);

    _sinkRouter->setSinkApplier([this](QVideoSink* sink) { _mediaPlayer->setVideoSink(sink); });
    _sinkRouter->routeTo(nullptr);

    (void)connect(_mediaPlayer, &QMediaPlayer::playingChanged, this, [this](bool playing) { _setStreamingActive(playing); });
    (void)connect(_mediaPlayer, &QMediaPlayer::hasVideoChanged, this, [this](bool has) {
        if (has)
            _pullDecoderInfoFromMetadata();
        _setDecoderActive(has);
    });
    (void)connect(_mediaPlayer, &QMediaPlayer::bufferProgressChanged, this, [](float filled) {
        qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << "Buffer Progress:" << filled;
    });
    (void)connect(_mediaPlayer, &QMediaPlayer::metaDataChanged, this,
                  &QtMultimediaReceiver::_pullDecoderInfoFromMetadata);
    (void)connect(_mediaPlayer, &QMediaPlayer::tracksChanged, this,
                  &QtMultimediaReceiver::_applyPlaybackTrackPolicy);
    (void)connect(_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this,
                  &QtMultimediaReceiver::_handleMediaStatusChanged);
    (void)connect(_mediaPlayer, &QMediaPlayer::errorOccurred, this,
                  [this](QMediaPlayer::Error error, const QString& errorString) {
                      qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << errorString;
                      const auto category = (error == QMediaPlayer::FormatError)
                                                ? VideoReceiver::ErrorCategory::MissingPlugin
                                                : VideoReceiver::ErrorCategory::Fatal;
                      emit receiverError(category, errorString);
                  });
}

void QtMultimediaReceiver::_setStreamingActive(bool active)
{
    if (_streamingActive == active)
        return;
    _streamingActive = active;
    emit streamingChanged(active);
}

void QtMultimediaReceiver::_setDecoderActive(bool active)
{
    if (_decoderActive == active)
        return;
    _decoderActive = active;
    emit decodingChanged(active);
}

QtMultimediaReceiver::~QtMultimediaReceiver()
{
    _closeCamera();
    qCDebug(QtMultimediaReceiverLog) << this;
}

void QtMultimediaReceiver::_pullDecoderInfoFromMetadata()
{
    const QMediaMetaData md = _mediaPlayer->metaData();
    const QVariant codec = md.value(QMediaMetaData::VideoCodec);
    if (!codec.isValid())
        return;

    QString codecName;
    if (codec.canConvert<QMediaFormat::VideoCodec>()) {
        codecName = QMediaFormat::videoCodecName(codec.value<QMediaFormat::VideoCodec>());
    } else {
        codecName = codec.toString();
    }
    const QString description = decodeDescription(codecName);
    if (codecName.isEmpty() || description == activeDecoderName())
        return;

    setDecoderInfo(false, description, md);
    qCDebug(QtMultimediaReceiverLog)
        << "Decoder info: codec =" << codecName
        << "backend = Qt FFmpeg"
        << "hardware candidates =" << qgetenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES")
        << "actual hardware decode exposed = false";
}

void QtMultimediaReceiver::_handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    qCDebug(QtMultimediaReceiverLog) << "Media status changed:" << status
                                     << "audio tracks:" << _mediaPlayer->audioTracks().size()
                                     << "subtitle tracks:" << _mediaPlayer->subtitleTracks().size()
                                     << "video tracks:" << _mediaPlayer->videoTracks().size();

    switch (status) {
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferedMedia:
            _pullDecoderInfoFromMetadata();
            _applyPlaybackTrackPolicy();
            break;
        case QMediaPlayer::InvalidMedia:
            qCWarning(QtMultimediaReceiverLog) << "Invalid Qt media:" << _mediaPlayer->errorString();
            break;
        default:
            break;
    }
}

void QtMultimediaReceiver::_applyPlaybackTrackPolicy()
{
    const auto decision = QtPlaybackTrackPolicy::apply(_mediaPlayer, _activeMode == SourceMode::Playback);
    if (decision.disableAudio || decision.disableSubtitles) {
        qCDebug(QtMultimediaReceiverLog) << "Disabled unused playback tracks:"
                                         << "audio" << decision.disableAudio
                                         << "subtitles" << decision.disableSubtitles;
    }
}

void QtMultimediaReceiver::configureSource(const VideoSourceResolver::VideoSource& source)
{
    _sourceMode = source.isLocalCamera ? SourceMode::LocalCamera : SourceMode::Playback;
    _cameraId = source.localCameraId;
}

QCameraDevice QtMultimediaReceiver::findLocalCameraDevice(const QString& cameraId)
{
    return QtCameraDeviceDiscovery::find(cameraId);
}

QString QtMultimediaReceiver::defaultLocalCameraId()
{
    return QtCameraDeviceDiscovery::defaultId();
}

bool QtMultimediaReceiver::localCameraAvailable()
{
    return QtCameraDeviceDiscovery::available();
}

bool QtMultimediaReceiver::localCameraDeviceExists(const QString& device)
{
    return QtCameraDeviceDiscovery::exists(device);
}

QStringList QtMultimediaReceiver::localCameraDeviceNameList()
{
    return QtCameraDeviceDiscovery::nameList();
}

void QtMultimediaReceiver::onSinkAboutToChange()
{
    _sinkRouter->detachObserver();
}

VideoReceiver::SinkChangeAction QtMultimediaReceiver::onSinkChanged(QVideoSink* newSink)
{
    _setSinkTargetForMode(_sourceMode);
    _sinkRouter->routeTo(newSink);
    return SinkChangeAction::NoAction;
}

void QtMultimediaReceiver::_clearStartHandlers()
{
    disconnect(_startPlayingConn);
    disconnect(_startBufferConn);
    disconnect(_startTimeoutConn);
    _startTimeoutTimer.stop();
}

void QtMultimediaReceiver::start(uint32_t timeout)
{
    _activeMode = _sourceMode;
    if (_activeMode == SourceMode::LocalCamera) {
        _startCamera(timeout);
        return;
    }

    _startPlayback(timeout);
}

void QtMultimediaReceiver::_startPlayback(uint32_t timeout)
{
    if (_mediaPlayer->isPlaying()) {
        qCDebug(QtMultimediaReceiverLog) << "Already running!";
        return;
    }

    if (_uri.isEmpty() && !_sourceDevice) {
        qCDebug(QtMultimediaReceiverLog) << "Failed because source is not specified";
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("Empty source"));
        return;
    }

    _clearStartHandlers();

    const std::chrono::milliseconds timeoutMs(static_cast<qint64>(timeout) * 1000);

    if (_lowLatency || _playbackPolicy.lowLatencyStreaming) {
        QPlaybackOptions opts;
        opts.setPlaybackIntent(QPlaybackOptions::PlaybackIntent::LowLatencyStreaming);
        if (_playbackPolicy.probeSizeBytes > 0)
            opts.setProbeSize(_playbackPolicy.probeSizeBytes);
        opts.setNetworkTimeout(timeoutMs);
        _mediaPlayer->setPlaybackOptions(opts);
        qCDebug(QtMultimediaReceiverLog) << "Low-latency streaming enabled";
    } else {
        _mediaPlayer->resetPlaybackOptions();
    }

    if (_sourceDevice) {
        const QUrl sourceUrl = _sourceDeviceUrl.isEmpty()
                                   ? ingestPlaybackUrl()
                                   : _sourceDeviceUrl;
        _mediaPlayer->setSourceDevice(_sourceDevice, sourceUrl);
    } else {
        _mediaPlayer->setSource(QUrl::fromUserInput(_uri));
    }
    _applyPlaybackTrackPolicy();
    // Stash the per-frame watchdog interval for startDecoding() to arm the
    // frame delivery with once decoding actually begins.
    _watchdogInterval = timeoutMs;

    _startPlayingConn = connect(_mediaPlayer, &QMediaPlayer::playingChanged, this, [this](bool playing) {
        if (!playing)
            return;
        qCDebug(QtMultimediaReceiverLog) << "playingChanged(true) — receiver started";
        _clearStartHandlers();
        emit receiverStarted();
    });

    _startTimeoutTimer.setInterval(timeoutMs);
    _startTimeoutConn = connect(&_startTimeoutTimer, &QTimer::timeout, this, [this]() {
        qCWarning(QtMultimediaReceiverLog) << "start() timed out";
        _clearStartHandlers();
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("QMediaPlayer start timed out"));
    });

    _mediaPlayer->play();
    _startTimeoutTimer.start();

    qCDebug(QtMultimediaReceiverLog) << "Starting (async receiverStarted pending)";
}

void QtMultimediaReceiver::stop()
{
    if (_activeMode == SourceMode::LocalCamera || (_camera && _camera->isActive())) {
        _stopCamera();
        return;
    }

    _stopPlayback();
}

void QtMultimediaReceiver::_stopPlayback()
{
    _clearStartHandlers();

    if (_mediaPlayer->playbackState() == QMediaPlayer::StoppedState) {
        qCDebug(QtMultimediaReceiverLog) << "Already stopped — no-op";
        return;
    }

    if (_mediaPlayer->source().isEmpty() && !_sourceDevice) {
        qCWarning(QtMultimediaReceiverLog) << "Stop called on empty source";
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("Stop called on empty source"));
        return;
    }

    _mediaPlayer->stop();
    _mediaPlayer->setSourceDevice(nullptr);
    qCDebug(QtMultimediaReceiverLog) << "Stopped";
    emit receiverStopped();
}

bool QtMultimediaReceiver::_ensureCameraObjects()
{
    if (_camera)
        return true;

    _camera = new QCamera(this);
    _imageCapture = new QImageCapture(this);
    _captureSession = new QMediaCaptureSession(this);
    _mediaDevices = new QMediaDevices(this);

    _captureSession->setCamera(_camera);
    _captureSession->setImageCapture(_imageCapture);

    (void)connect(_mediaDevices, &QMediaDevices::videoInputsChanged, this, [this]() {
        if (!started())
            return;
        if (QMediaDevices::videoInputs().isEmpty()) {
            qCWarning(QtMultimediaReceiverLog) << "All local camera devices gone — closing camera";
            _closeCamera();
        } else {
            qCDebug(QtMultimediaReceiverLog) << "Local camera device list changed — re-opening camera";
            _closeCamera();
            _openCamera();
        }
    });

    (void)connect(_camera, &QCamera::errorOccurred, this, [this](QCamera::Error err, const QString& desc) {
        if (err == QCamera::NoError)
            return;
        qCWarning(QtMultimediaReceiverLog) << "QCamera error:" << err << desc;
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, desc);
    });

    return true;
}

void QtMultimediaReceiver::_setSinkTargetForMode(SourceMode mode)
{
    if (mode == SourceMode::LocalCamera) {
        if (_ensureCameraObjects())
            _sinkRouter->setSinkApplier([this](QVideoSink* sink) { _captureSession->setVideoSink(sink); });
        return;
    }

    _sinkRouter->setSinkApplier([this](QVideoSink* sink) { _mediaPlayer->setVideoSink(sink); });
}

void QtMultimediaReceiver::_checkCameraPermission()
{
    const QCameraPermission cameraPermission;
    _cameraPermissionStatus = qApp->checkPermission(cameraPermission);
    if (_cameraPermissionStatus == Qt::PermissionStatus::Undetermined) {
        qApp->requestPermission(cameraPermission, this, [this](const QPermission& permission) {
            _cameraPermissionStatus = permission.status();
            if (_cameraPermissionStatus != Qt::PermissionStatus::Granted)
                qgcApp()->showAppMessage(QStringLiteral("Failed to get camera permission"));
        });
    }
}

void QtMultimediaReceiver::_startCamera(uint32_t timeout)
{
    qCDebug(QtMultimediaReceiverLog) << "start camera" << timeout;

    if (started())
        return;

    if (!_ensureCameraObjects())
        return;

    _checkCameraPermission();
    if (_cameraPermissionStatus != Qt::PermissionStatus::Granted) {
        const QString msg = (_cameraPermissionStatus == Qt::PermissionStatus::Denied)
                                ? QStringLiteral("Camera permission denied")
                                : QStringLiteral("Camera permission pending — try again once granted");
        qCWarning(QtMultimediaReceiverLog) << msg;
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, msg);
        return;
    }

    _watchdogInterval = std::chrono::milliseconds(static_cast<qint64>(timeout) * 1000);
    _openCamera();

    setStarted(true);
    _setStreamingActive(true);
    emit receiverStarted();
}

void QtMultimediaReceiver::_stopCamera()
{
    qCDebug(QtMultimediaReceiverLog) << "stop camera";

    if (!started() && (!_camera || !_camera->isActive()))
        return;

    _closeCamera();

    setStarted(false);
    _setStreamingActive(false);
    _setDecoderActive(false);
    emit receiverStopped();
}

void QtMultimediaReceiver::_openCamera()
{
    if (!_ensureCameraObjects())
        return;

    const QString sourceId = _cameraId.isEmpty() ? defaultLocalCameraId() : _cameraId;
    if (sourceId.isEmpty()) {
        qCWarning(QtMultimediaReceiverLog) << "No local camera found";
        return;
    }

    const QCameraDevice device = findLocalCameraDevice(sourceId);
    if (device.isNull()) {
        qCWarning(QtMultimediaReceiverLog) << "Configured local camera not found:" << sourceId;
        return;
    }

    _camera->setCameraDevice(device);
    const QCameraFormat format = QtCameraDeviceDiscovery::bestFormat(device);
    if (!format.isNull())
        _camera->setCameraFormat(format);

    _announceCameraFormat();
    _camera->setActive(true);
    qCDebug(QtMultimediaReceiverLog) << "Camera opened:" << _camera->cameraDevice().description()
                                     << "id:" << QtCameraDeviceDiscovery::stableId(_camera->cameraDevice())
                                     << "format:" << _camera->cameraFormat().resolution()
                                     << _camera->cameraFormat().maxFrameRate();
}

void QtMultimediaReceiver::_closeCamera()
{
    if (!_camera)
        return;
    if (_delivery)
        _delivery->disarmWatchdog();
    _camera->setActive(false);
    qCDebug(QtMultimediaReceiverLog) << "Camera closed";
}

void QtMultimediaReceiver::_announceCameraFormat()
{
    if (!_delivery || !_camera)
        return;

    const QCameraFormat format = _camera->cameraFormat();
    if (format.isNull())
        return;

    QVideoFrameFormat videoFormat(format.resolution(), format.pixelFormat());
    _delivery->announceFormat(videoFormat);
}

void QtMultimediaReceiver::startDecoding()
{
    if (!validateFrameDeliveryForDecoding())
        return;

    // Prefer Qt's direct QMediaPlayer -> QVideoSink path. The delivery endpoint
    // observes that sink for stats/recording instead of redisplaying frames.
    _setSinkTargetForMode(_activeMode);
    _sinkRouter->setFrameDelivery(_delivery);
    _sinkRouter->routeTo(_delivery->videoSink());
    qCDebug(QtMultimediaReceiverLog) << "Qt video sink state:"
                                     << "fallback" << _sinkRouter->activeSinkIsFallback()
                                     << "rhi" << _sinkRouter->activeSinkHasRhi();
    if (_activeMode == SourceMode::LocalCamera)
        _announceCameraFormat();

    // Frame watchdog lives on delivery (centralized across receivers). It
    // observes sink frames directly, so no extra forwarding connection is needed here.
    if (_watchdogInterval.count() > 0)
        _delivery->armWatchdog(_watchdogInterval);

    _setDecoderActive(true);
    qCDebug(QtMultimediaReceiverLog) << "Decoding";
}

void QtMultimediaReceiver::stopDecoding()
{
    // Reject only when we're not actually decoding. Proceed even when the sink has been detached so the
    // decode state tears down cleanly — otherwise a sink-detach + reconnect
    // leaves _decoderActive stuck true and _internalFrameConn dangling.
    if (!_decoderActive) {
        qCWarning(QtMultimediaReceiverLog) << "Not decoding — ignoring stopDecoding";
        return;
    }

    if (_delivery) {
        _delivery->disarmWatchdog();
    }
    _sinkRouter->detachObserver();
    _sinkRouter->routeTo(nullptr);

    _setDecoderActive(false);
    qCDebug(QtMultimediaReceiverLog) << "Stopped Decoding";
}

void QtMultimediaReceiver::pause()
{
    if (_mediaPlayer && _mediaPlayer->isPlaying()) {
        qCDebug(QtMultimediaReceiverLog) << "Pausing QMediaPlayer for reconnect";
        _mediaPlayer->pause();
        emit receiverPaused();
    }
}

void QtMultimediaReceiver::resume()
{
    if (_mediaPlayer && _mediaPlayer->playbackState() == QMediaPlayer::PausedState) {
        qCDebug(QtMultimediaReceiverLog) << "Resuming QMediaPlayer";
        _mediaPlayer->play();
        emit receiverResumed();
    }
}
