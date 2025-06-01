/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VideoManager.h"
#include "AppSettings.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "SubtitleWriter.h"
#include "Vehicle.h"
#include "VideoReceiver.h"
#include "VideoSettings.h"
#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#else
#include "VideoItemStub.h"
#endif
#include "QtMultimediaReceiver.h"
#include "UVCReceiver.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QDir>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

QGC_LOGGING_CATEGORY(VideoManagerLog, "qgc.videomanager.videomanager")

static constexpr const char *kFileExtension[VideoReceiver::FILE_FORMAT_MAX + 1] = {
    "mkv",
    "mov",
    "mp4"
};

Q_APPLICATION_STATIC(VideoManager, _videoManagerInstance);

VideoManager::VideoManager(QObject *parent)
    : QObject(parent)
    , _subtitleWriter(new SubtitleWriter(this))
    , _videoSettings(SettingsManager::instance()->videoSettings())
{
    // qCDebug(VideoManagerLog) << this;

    (void) qRegisterMetaType<VideoReceiver::STATUS>("STATUS");

#ifdef QGC_GST_STREAMING
    if (!GStreamer::initialize()) {
        qCCritical(VideoManagerLog) << "Failed To Initialize GStreamer";
    }
#endif
}

VideoManager::~VideoManager()
{
    // qCDebug(VideoManagerLog) << this;
}

VideoManager *VideoManager::instance()
{
    return _videoManagerInstance();
}

void VideoManager::registerQmlTypes()
{
    (void) qmlRegisterUncreatableType<VideoManager>("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
    (void) qmlRegisterUncreatableType<VideoReceiver>("QGroundControl", 1, 0, "VideoReceiver","Reference only");
#ifndef QGC_GST_STREAMING
    (void) qmlRegisterType<VideoItemStub>("org.freedesktop.gstreamer.Qt6GLVideoItem", 1, 0, "GstGLQt6VideoItem");
#endif
}

void VideoManager::init(QQuickWindow *window)
{
    if (_initialized) {
        return;
    }

    if (!window) {
        qCCritical(VideoManagerLog) << "Failed To Init Video Manager - window is NULL";
        return;
    }

    // TODO: VideoSettings _configChanged/streamConfiguredChanged
    (void) connect(_videoSettings->videoSource(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
    (void) connect(_videoSettings->udpUrl(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
    (void) connect(_videoSettings->rtspUrl(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
    (void) connect(_videoSettings->tcpUrl(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
    (void) connect(_videoSettings->aspectRatio(), &Fact::rawValueChanged, this, &VideoManager::aspectRatioChanged);
    (void) connect(_videoSettings->lowLatencyMode(), &Fact::rawValueChanged, this, [this](const QVariant &value) { Q_UNUSED(value); _restartAllVideos(); });
    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);

    (void) connect(this, &VideoManager::autoStreamConfiguredChanged, this, &VideoManager::_videoSourceChanged);

    static const QStringList videoStreamList = {
        "videoContent",
        "thermalVideo"
    };
    for (const QString &streamName : videoStreamList) {
        VideoReceiver *receiver = QGCCorePlugin::instance()->createVideoReceiver(this);
        if (!receiver) {
            continue;
        }
        receiver->setName(streamName);

        _initVideoReceiver(receiver, window);
    }

    window->scheduleRenderJob(new FinishVideoInitialization(), QQuickWindow::BeforeSynchronizingStage);

    _initialized = true;
}

void VideoManager::cleanup()
{
    for (VideoReceiver *receiver : std::as_const(_videoReceivers)) {
        QGCCorePlugin::instance()->releaseVideoSink(receiver->sink());
    }
}

void VideoManager::_cleanupOldVideos()
{
    if (!SettingsManager::instance()->videoSettings()->enableStorageLimit()->rawValue().toBool()) {
        return;
    }

    const QString savePath = SettingsManager::instance()->appSettings()->videoSavePath();
    QDir videoDir = QDir(savePath);
    videoDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    videoDir.setSorting(QDir::Time);

    QStringList nameFilters;
    for (size_t i = 0; i < std::size(kFileExtension); i++) {
        nameFilters << QStringLiteral("*.") + kFileExtension[i];
    }

    videoDir.setNameFilters(nameFilters);
    QFileInfoList vidList = videoDir.entryInfoList();
    if (vidList.isEmpty()) {
        return;
    }

    uint64_t total = 0;
    for (const QFileInfo &video : std::as_const(vidList)) {
        total += video.size();
    }

    const uint64_t maxSize = SettingsManager::instance()->videoSettings()->maxVideoSize()->rawValue().toUInt() * qPow(1024, 2);
    while ((total >= maxSize) && !vidList.isEmpty()) {
        const QFileInfo info = vidList.takeLast();
        total -= info.size();
        const QString path = info.filePath();
        qCDebug(VideoManagerLog) << "Removing old video file:" << path;
        (void) QFile::remove(path);
    }
}

void VideoManager::startRecording(const QString &videoFile)
{
    const VideoReceiver::FILE_FORMAT fileFormat = static_cast<VideoReceiver::FILE_FORMAT>(_videoSettings->recordingFormat()->rawValue().toInt());
    if (!VideoReceiver::isValidFileFormat(fileFormat)) {
        qgcApp()->showAppMessage(tr("Invalid video format defined."));
        return;
    }

    _cleanupOldVideos();

    const QString savePath = SettingsManager::instance()->appSettings()->videoSavePath();
    if (savePath.isEmpty()) {
        qgcApp()->showAppMessage(tr("Unabled to record video. Video save path must be specified in Settings."));
        return;
    }

    const QString videoFileUrl = videoFile.isEmpty() ? QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss") : videoFile;
    const QString ext = kFileExtension[fileFormat];

    const QString videoFileName = savePath + "/" + videoFileUrl + ".%1" + ext;

    for (VideoReceiver *receiver : std::as_const(_videoReceivers)) {
        if (!receiver->started()) {
            qCDebug(VideoManagerLog) << "Video receiver is not ready.";
            continue;
        }
        receiver->startRecording(videoFileName.arg(receiver->name()), fileFormat);
    }
}

void VideoManager::stopRecording()
{
    for (VideoReceiver *receiver : std::as_const(_videoReceivers)) {
        receiver->stopRecording();
    }
}

void VideoManager::grabImage(const QString &imageFile)
{
    if (imageFile.isEmpty()) {
        _imageFile = SettingsManager::instance()->appSettings()->photoSavePath();
        _imageFile += QStringLiteral("/") + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss.zzz") + QStringLiteral(".jpg");
    } else {
        _imageFile = imageFile;
    }

    emit imageFileChanged(_imageFile);

    for (VideoReceiver *receiver : std::as_const(_videoReceivers)) {
        receiver->takeScreenshot(_imageFile);
        // QSharedPointer<QQuickItemGrabResult> result = receiver->widget()->grabToImage(const QSize &targetSize = QSize())
    }
}

double VideoManager::aspectRatio() const
{
    for (VideoReceiver *receiver : _videoReceivers) {
        QGCVideoStreamInfo *pInfo = receiver->videoStreamInfo();
        if (!receiver->isThermal() && pInfo && !pInfo->isThermal()) {
            return pInfo->aspectRatio();
        }
    }

    // FIXME: use _videoReceiver->videoSize() to calculate AR (if AR is not specified in the settings?)
    return _videoSettings->aspectRatio()->rawValue().toDouble();
}

double VideoManager::thermalAspectRatio() const
{
    for (VideoReceiver *receiver : _videoReceivers) {
        QGCVideoStreamInfo *pInfo = receiver->videoStreamInfo();
        if (receiver->isThermal() && pInfo && pInfo->isThermal()) {
            return pInfo->aspectRatio();
        }
    }

    return 1.0;
}

double VideoManager::hfov() const
{
    for (VideoReceiver *receiver : _videoReceivers) {
        QGCVideoStreamInfo *pInfo = receiver->videoStreamInfo();
        if (!receiver->isThermal() && pInfo && !pInfo->isThermal()) {
            return pInfo->hfov();
        }
    }

    return 1.0;
}

double VideoManager::thermalHfov() const
{
    for (VideoReceiver *receiver : _videoReceivers) {
        QGCVideoStreamInfo *pInfo = receiver->videoStreamInfo();
        if (receiver->isThermal() && pInfo && pInfo->isThermal()) {
            return pInfo->hfov();
        }
    }

    return _videoSettings->aspectRatio()->rawValue().toDouble();
}

bool VideoManager::hasThermal() const
{
    for (VideoReceiver *receiver : _videoReceivers) {
        QGCVideoStreamInfo *pInfo = receiver->videoStreamInfo();
        if (receiver->isThermal() && pInfo && pInfo->isThermal()) {
            return true;
        }
    }

    return false;
}

bool VideoManager::hasVideo() const
{
    return (_videoSettings->streamEnabled()->rawValue().toBool() && _videoSettings->streamConfigured());
}

bool VideoManager::isUvc() const
{
    return (!_uvcVideoSourceID.isEmpty() && uvcEnabled() && hasVideo());
}

bool VideoManager::gstreamerEnabled()
{
#ifdef QGC_GST_STREAMING
    return true;
#else
    return false;
#endif
}

bool VideoManager::uvcEnabled()
{
    return UVCReceiver::enabled();
}

bool VideoManager::qtmultimediaEnabled()
{
    return QtMultimediaReceiver::enabled();
}

void VideoManager::setfullScreen(bool on)
{
    if (on) {
        if (!_activeVehicle || _activeVehicle->vehicleLinkManager()->communicationLost()) {
            on = false;
        }
    }

    if (on != _fullScreen) {
        _fullScreen = on;
        emit fullScreenChanged();
    }
}

bool VideoManager::isStreamSource() const
{
    static const QStringList videoSourceList = {
        VideoSettings::videoSourceUDPH264,
        VideoSettings::videoSourceUDPH265,
        VideoSettings::videoSourceRTSP,
        VideoSettings::videoSourceTCP,
        VideoSettings::videoSourceMPEGTS,
        VideoSettings::videoSource3DRSolo,
        VideoSettings::videoSourceParrotDiscovery,
        VideoSettings::videoSourceYuneecMantisG,
        VideoSettings::videoSourceHerelinkAirUnit,
        VideoSettings::videoSourceHerelinkHotspot,
    };
    const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return (videoSourceList.contains(videoSource) || autoStreamConfigured());
}

void VideoManager::_videoSourceChanged()
{
    bool changed = false;

    for (VideoReceiver *receiver : std::as_const(_videoReceivers)) {
        changed |= _updateSettings(receiver);
    }

    if (changed) {
        emit hasVideoChanged();
        emit isStreamSourceChanged();
        emit isAutoStreamChanged();

        if (hasVideo()) {
            _restartAllVideos();
        } else {
            stopVideo();
        }

        qCDebug(VideoManagerLog) << "New Video Source:" << _videoSettings->videoSource()->rawValue().toString();
    }
}

bool VideoManager::_updateUVC(VideoReceiver *receiver)
{
    bool result = false;

    const QString oldUvcVideoSrcID = _uvcVideoSourceID;

    if (!uvcEnabled() || !hasVideo() || isStreamSource()) {
        _uvcVideoSourceID = QString();
    } else {
        _uvcVideoSourceID = UVCReceiver::getSourceId();
    }

    if (oldUvcVideoSrcID != _uvcVideoSourceID) {
        qCDebug(VideoManagerLog) << "UVC changed from [" << oldUvcVideoSrcID << "] to [" << _uvcVideoSourceID << "]";
        if (!_uvcVideoSourceID.isEmpty()) {
            UVCReceiver::checkPermission();
        }
        result = true;
        emit uvcVideoSourceIDChanged();
        emit isUvcChanged();
    }

    return result;
}

bool VideoManager::autoStreamConfigured() const
{
    for (VideoReceiver *receiver : _videoReceivers) {
        QGCVideoStreamInfo *pInfo = receiver->videoStreamInfo();
        if (!receiver->isThermal() && pInfo && !pInfo->isThermal()) {
            return !pInfo->uri().isEmpty();
        }
    }

    return false;
}

bool VideoManager::_updateAutoStream(VideoReceiver *receiver)
{
    const QGCVideoStreamInfo *pInfo = receiver->videoStreamInfo();
    if (!pInfo) {
        return false;
    }

    qCDebug(VideoManagerLog) << QString("Configure stream (%1):").arg(receiver->name()) << pInfo->uri();

    QString source, url;
    switch (pInfo->type()) {
    case VIDEO_STREAM_TYPE_RTSP:
        source = VideoSettings::videoSourceRTSP;
        url = pInfo->uri();
        break;
    case VIDEO_STREAM_TYPE_TCP_MPEG:
        source = VideoSettings::videoSourceTCP;
        url = pInfo->uri();
        break;
    case VIDEO_STREAM_TYPE_RTPUDP:
        if (pInfo->encoding() == VIDEO_STREAM_ENCODING_H265) {
            source = VideoSettings::videoSourceUDPH265;
            url = pInfo->uri().contains("udp265://") ? pInfo->uri() : QStringLiteral("udp265://0.0.0.0:%1").arg(pInfo->uri());
        } else {
            source = VideoSettings::videoSourceUDPH264;
            url = pInfo->uri().contains("udp://") ? pInfo->uri() : QStringLiteral("udp://0.0.0.0:%1").arg(pInfo->uri());
        }
        break;
    case VIDEO_STREAM_TYPE_MPEG_TS:
        source = VideoSettings::videoSourceMPEGTS;
        url = pInfo->uri().contains("mpegts://") ? pInfo->uri() : QStringLiteral("mpegts://0.0.0.0:%1").arg(pInfo->uri());
        break;
    default:
        qCWarning(VideoManagerLog) << "Unknown VIDEO_STREAM_TYPE";
        source = VideoSettings::videoSourceNoVideo;
        url = pInfo->uri();
        break;
    }

    const bool settingsChanged = _updateVideoUri(receiver, url);
    if (settingsChanged) {
        if (!receiver->isThermal()) {
            _videoSettings->videoSource()->setRawValue(source);
        }

        emit autoStreamConfiguredChanged();
    }

    return settingsChanged;
}

bool VideoManager::_updateVideoUri(VideoReceiver *receiver, const QString &uri)
{
    if (!receiver) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL";
        return false;
    }

    if ((uri == receiver->uri()) && !receiver->uri().isNull()) {
        return false;
    }

    qCDebug(VideoManagerLog) << "New Video URI" << uri;

    receiver->setUri(uri);

    return true;
}

bool VideoManager::_updateSettings(VideoReceiver *receiver)
{
    if (!receiver) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL";
        return false;
    }

    bool settingsChanged = false;

    const bool lowLatency = _videoSettings->lowLatencyMode()->rawValue().toBool();
    if (lowLatency != receiver->lowLatency()) {
        receiver->setLowLatency(lowLatency);
        settingsChanged = true;
    }

    if (receiver->isThermal()) {
        return settingsChanged;
    }

    settingsChanged |= _updateUVC(receiver);
    settingsChanged |= _updateAutoStream(receiver);

    const QString source = _videoSettings->videoSource()->rawValue().toString();
    if (source == VideoSettings::videoSourceUDPH264) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("udp://%1").arg(_videoSettings->udpUrl()->rawValue().toString()));
    } else if (source == VideoSettings::videoSourceUDPH265) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("udp265://%1").arg(_videoSettings->udpUrl()->rawValue().toString()));
    } else if (source == VideoSettings::videoSourceMPEGTS) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("mpegts://%1").arg(_videoSettings->udpUrl()->rawValue().toString()));
    } else if (source == VideoSettings::videoSourceRTSP) {
        settingsChanged |= _updateVideoUri(receiver, _videoSettings->rtspUrl()->rawValue().toString());
    } else if (source == VideoSettings::videoSourceTCP) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("tcp://%1").arg(_videoSettings->tcpUrl()->rawValue().toString()));
    } else if (source == VideoSettings::videoSource3DRSolo) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("udp://0.0.0.0:5600"));
    } else if (source == VideoSettings::videoSourceParrotDiscovery) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("udp://0.0.0.0:8888"));
    } else if (source == VideoSettings::videoSourceYuneecMantisG) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("rtsp://192.168.42.1:554/live"));
    } else if (source == VideoSettings::videoSourceHerelinkAirUnit) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("rtsp://192.168.0.10:8554/H264Video"));
    } else if (source == VideoSettings::videoSourceHerelinkHotspot) {
        settingsChanged |= _updateVideoUri(receiver, QStringLiteral("rtsp://192.168.43.1:8554/fpv_stream"));
    } else if ((source == VideoSettings::videoDisabled) || (source == VideoSettings::videoSourceNoVideo)) {
        settingsChanged |= _updateVideoUri(receiver, QString());
    } else {
        settingsChanged |= _updateVideoUri(receiver, QString());
        if (!isUvc()) {
            qCCritical(VideoManagerLog) << "Video source URI \"" << source << "\" is not supported. Please add support!";
        }
    }

    return settingsChanged;
}

void VideoManager::_setActiveVehicle(Vehicle *vehicle)
{
    if (_activeVehicle) {
        (void) disconnect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, &VideoManager::_communicationLostChanged);
        MavlinkCameraControl *pCamera = _activeVehicle->cameraManager()->currentCameraInstance();
        if (pCamera) {
            pCamera->stopStream();
        }
        (void) disconnect(_activeVehicle->cameraManager(), &QGCCameraManager::streamChanged, this, &VideoManager::_videoSourceChanged);

        for (VideoReceiver *receiver : std::as_const(_videoReceivers)) {
            // disconnect(receiver->videoStreamInfo(), &QGCVideoStreamInfo::infoChanged, ))
            receiver->setVideoStreamInfo(nullptr);
        }
    }

    _activeVehicle = vehicle;
    if (_activeVehicle) {
        (void) connect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, &VideoManager::_communicationLostChanged);
        (void) connect(_activeVehicle->cameraManager(), &QGCCameraManager::streamChanged, this, &VideoManager::_videoSourceChanged);
        MavlinkCameraControl *pCamera = _activeVehicle->cameraManager()->currentCameraInstance();
        if (pCamera) {
            pCamera->resumeStream();
        }

        for (VideoReceiver *receiver : std::as_const(_videoReceivers)) {
            if (receiver->isThermal()) {
                receiver->setVideoStreamInfo(_activeVehicle->cameraManager()->thermalStreamInstance());
            } else {
                receiver->setVideoStreamInfo(_activeVehicle->cameraManager()->currentStreamInstance());
            }
            // connect(receiver->videoStreamInfo(), &QGCVideoStreamInfo::infoChanged, ))
        }
    } else {
        setfullScreen(false);
    }
}

void VideoManager::_communicationLostChanged(bool connectionLost)
{
    if (connectionLost) {
        setfullScreen(false);
    }
}

void VideoManager::_restartAllVideos()
{
    for (VideoReceiver *videoReceiver : std::as_const(_videoReceivers)) {
        _restartVideo(videoReceiver);
    }
}

void VideoManager::_restartVideo(VideoReceiver *receiver)
{
    if (!receiver) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL";
        return;
    }

    qCDebug(VideoManagerLog) << "Restart video receiver" << receiver->name();

    if (receiver->started()) {
        _stopReceiver(receiver);
        // onStopComplete Signal Will Restart It
    } else {
        _startReceiver(receiver);
    }
}

void VideoManager::_stopReceiver(VideoReceiver *receiver)
{
    if (!receiver) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL";
        return;
    }

    if (receiver->started()) {
        receiver->stop();
    }
}

void VideoManager::stopVideo()
{
    for (VideoReceiver *receiver : std::as_const(_videoReceivers)) {
        _stopReceiver(receiver);
    }
}

void VideoManager::_startReceiver(VideoReceiver *receiver)
{
    if (!receiver) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL";
        return;
    }

    if (receiver->started()) {
        qCDebug(VideoManagerLog) << "VideoReceiver is already started" << receiver->name();
        return;
    }

    if (receiver->uri().isEmpty()) {
        qCDebug(VideoManagerLog) << "VideoUri is NULL" << receiver->name();
        return;
    }

    const QString source = _videoSettings->videoSource()->rawValue().toString();
    /* The gstreamer rtsp source will switch to tcp if udp is not available after 5 seconds.
       So we should allow for some negotiation time for rtsp */

    const uint32_t timeout = ((source == VideoSettings::videoSourceRTSP) ? _videoSettings->rtspTimeout()->rawValue().toUInt() : 3);

    receiver->start(timeout);
}

void VideoManager::_initVideoReceiver(VideoReceiver *receiver, QQuickWindow *window)
{
    if (_videoReceivers.contains(receiver)) {
        qCWarning(VideoManagerLog) << "Receiver already initialized";
    }

    QQuickItem *widget = window->findChild<QQuickItem*>(receiver->name());
    if (!widget) {
        qCCritical(VideoManagerLog) << "stream widget not found" << receiver->name();
    }
    receiver->setWidget(widget);

    void *sink = QGCCorePlugin::instance()->createVideoSink(receiver->widget(), receiver);
    if (!sink) {
        qCCritical(VideoManagerLog) << "createVideoSink() failed" << receiver->name();
    }
    receiver->setSink(sink);

    (void) connect(receiver, &VideoReceiver::onStartComplete, this, [this, receiver](VideoReceiver::STATUS status) {
        if (!receiver) {
            return;
        }

        qCDebug(VideoManagerLog) << "Video" << receiver->name() << "Start complete, status:" << status;
        switch (status) {
        case VideoReceiver::STATUS_OK:
            receiver->setStarted(true);
            if (receiver->sink()) {
                receiver->startDecoding(receiver->sink());
            }
            break;
        case VideoReceiver::STATUS_INVALID_URL:
        case VideoReceiver::STATUS_INVALID_STATE:
            break;
        default:
            _restartVideo(receiver);
            break;
        }
    });

    (void) connect(receiver, &VideoReceiver::onStopComplete, this, [this, receiver](VideoReceiver::STATUS status) {
        qCDebug(VideoManagerLog) << "Video" << receiver->name() << "Stop complete, status:" << status;
        receiver->setStarted(false);
        if (status == VideoReceiver::STATUS_INVALID_URL) {
            qCDebug(VideoManagerLog) << "Invalid video URL. Not restarting";
        } else {
            _startReceiver(receiver);
        }
    });

    (void) connect(receiver, &VideoReceiver::streamingChanged, this, [this, receiver](bool active) {
        qCDebug(VideoManagerLog) << "Video" << receiver->name() << "streaming changed, active:" << (active ? "yes" : "no");
        if (!receiver->isThermal()) {
            _streaming = active;
            emit streamingChanged();
        }
    });

    (void) connect(receiver, &VideoReceiver::decodingChanged, this, [this, receiver](bool active) {
        qCDebug(VideoManagerLog) << "Video" << receiver->name() << "decoding changed, active:" << (active ? "yes" : "no");
        if (!receiver->isThermal()) {
            _decoding = active;
            emit decodingChanged();
        }
    });

    (void) connect(receiver, &VideoReceiver::recordingChanged, this, [this, receiver](bool active) {
        qCDebug(VideoManagerLog) << "Video" << receiver->name() << "recording changed, active:" << (active ? "yes" : "no");
        if (!receiver->isThermal()) {
            _recording = active;
            if (!active) {
                _subtitleWriter->stopCapturingTelemetry();
            }
            emit recordingChanged();
        }
    });

    (void) connect(receiver, &VideoReceiver::recordingStarted, this, [this, receiver](const QString &filename) {
        qCDebug(VideoManagerLog) << "Video" << receiver->name() << "recording started";
        if (!receiver->isThermal()) {
            _subtitleWriter->startCapturingTelemetry(filename, videoSize());
        }
    });

    (void) connect(receiver, &VideoReceiver::videoSizeChanged, this, [this, receiver](QSize size) {
        qCDebug(VideoManagerLog) << "Video" << receiver->name() << "resized. New resolution:" << size.width() << "x" << size.height();
        if (!receiver->isThermal()) {
            _videoSize = size;
            emit videoSizeChanged();
        }
    });

    (void) connect(receiver, &VideoReceiver::onTakeScreenshotComplete, this, [receiver](VideoReceiver::STATUS status) {
        if (status == VideoReceiver::STATUS_OK) {
            qCDebug(VideoManagerLog) << "Video" << receiver->name() << "screenshot taken";
        } else {
            qCWarning(VideoManagerLog) << "Video" << receiver->name() << "screenshot failed";
        }
    });

    (void) connect(receiver, &VideoReceiver::videoStreamInfoChanged, this, [this, receiver]() {
        const QGCVideoStreamInfo *videoStreamInfo = receiver->videoStreamInfo();
        qCDebug(VideoManagerLog) << "Video" << receiver->name() << "stream info:" << (videoStreamInfo ? "received" : "lost");

        (void) _updateAutoStream(receiver);
    });

    (void) _updateSettings(receiver);

    _videoReceivers.append(receiver);

    if (hasVideo()) {
        _startReceiver(receiver);
    }
}

void VideoManager::startVideo()
{
    if (!hasVideo()) {
        qCDebug(VideoManagerLog) << "Stream not enabled/configured";
        return;
    }

    _restartAllVideos();
}

/*===========================================================================*/

FinishVideoInitialization::FinishVideoInitialization()
    : QRunnable()
{
    // qCDebug(VideoManagerLog) << this;
}

FinishVideoInitialization::~FinishVideoInitialization()
{
    // qCDebug(VideoManagerLog) << this;
}

void FinishVideoInitialization::run()
{
    VideoManager::instance()->startVideo();
}
