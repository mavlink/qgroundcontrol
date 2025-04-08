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
#include "GLVideoItemStub.h"
#endif
#include "QtMultimediaReceiver.h"
#ifndef QGC_DISABLE_UVC
#include "UVCReceiver.h"
#endif

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QDir>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

QGC_LOGGING_CATEGORY(VideoManagerLog, "qgc.videomanager.videomanager")

static constexpr const char *kFileExtension[VideoReceiver::FILE_FORMAT_MAX - VideoReceiver::FILE_FORMAT_MIN] = {
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
    // qCDebug(VideoManagerLog) << Q_FUNC_INFO << this;

#ifdef QGC_GST_STREAMING
    GStreamer::initialize();
#endif
}

VideoManager::~VideoManager()
{
    // qCDebug(VideoManagerLog) << Q_FUNC_INFO << this;
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
    (void) qmlRegisterType<GLVideoItemStub>("org.freedesktop.gstreamer.Qt6GLVideoItem", 1, 0, "GstGLQt6VideoItem");
#endif
}

void VideoManager::init()
{
    if (_initialized) {
        return;
    }

    // TODO: Those connections should be Per Video, not per VideoManager.
    (void) connect(_videoSettings->videoSource(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
    (void) connect(_videoSettings->udpUrl(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
    (void) connect(_videoSettings->rtspUrl(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
    (void) connect(_videoSettings->tcpUrl(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
    (void) connect(_videoSettings->aspectRatio(), &Fact::rawValueChanged, this, &VideoManager::aspectRatioChanged);
    (void) connect(_videoSettings->lowLatencyMode(), &Fact::rawValueChanged, this, &VideoManager::_lowLatencyModeChanged);
    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);

    int index = 0;
    const QStringList widgetTypes = {"videoContent", "thermalVideo"};
    Q_ASSERT(widgetTypes.length() <= _videoReceiverData.length());
    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        videoReceiver.index = index++;
        videoReceiver.receiver = QGCCorePlugin::instance()->createVideoReceiver(this);
        if (!videoReceiver.receiver) {
            continue;
        }
        videoReceiver.name = widgetTypes[videoReceiver.index];

        (void) connect(videoReceiver.receiver, &VideoReceiver::onStartComplete, this, [this, &videoReceiver](VideoReceiver::STATUS status) {
            qCDebug(VideoManagerLog) << "Video" << videoReceiver.index << "Start complete, status:" << status;
            switch (status) {
            case VideoReceiver::STATUS_OK:
                videoReceiver.started = true;
                if (videoReceiver.sink) {
                    videoReceiver.receiver->startDecoding(videoReceiver.sink);
                }
                break;
            case VideoReceiver::STATUS_INVALID_URL:
            case VideoReceiver::STATUS_INVALID_STATE:
                break;
            default:
                _restartVideo(videoReceiver.index);
                break;
            }
        });

        (void) connect(videoReceiver.receiver, &VideoReceiver::onStopComplete, this, [this, &videoReceiver](VideoReceiver::STATUS status) {
            qCDebug(VideoManagerLog) << "Video" << videoReceiver.index << "Stop complete, status:" << status;
            videoReceiver.started = false;
            if (status == VideoReceiver::STATUS_INVALID_URL) {
                qCDebug(VideoManagerLog) << "Invalid video URL. Not restarting";
            } else {
                _startReceiver(videoReceiver.index);
            }
        });

        // TODO: Create status variables for each receiver in VideoReceiverData
        (void) connect(videoReceiver.receiver, &VideoReceiver::streamingChanged, this, [this, &videoReceiver](bool active) {
            qCDebug(VideoManagerLog) << "Video" << videoReceiver.index << "streaming changed, active:" << (active ? "yes" : "no");
            if (videoReceiver.index == 0) {
                _streaming = active;
                emit streamingChanged();
            }
        });

        (void) connect(videoReceiver.receiver, &VideoReceiver::decodingChanged, this, [this, &videoReceiver](bool active) {
            qCDebug(VideoManagerLog) << "Video" << videoReceiver.index << "decoding changed, active:" << (active ? "yes" : "no");
            if (videoReceiver.index == 0) {
                _decoding = active;
                emit decodingChanged();
            }
        });

        (void) connect(videoReceiver.receiver, &VideoReceiver::recordingChanged, this, [this, &videoReceiver](bool active) {
            qCDebug(VideoManagerLog) << "Video" << videoReceiver.index << "recording changed, active:" << (active ? "yes" : "no");
            if (videoReceiver.index == 0) {
                _recording = active;
                if (!active) {
                    _subtitleWriter->stopCapturingTelemetry();
                }
                emit recordingChanged();
            }
        });

        (void) connect(videoReceiver.receiver, &VideoReceiver::recordingStarted, this, [this, &videoReceiver]() {
            qCDebug(VideoManagerLog) << "Video" << videoReceiver.index << "recording started";
            if (videoReceiver.index == 0) {
                _subtitleWriter->startCapturingTelemetry(_videoFile, videoSize());
            }
        });

        (void) connect(videoReceiver.receiver, &VideoReceiver::videoSizeChanged, this, [this, &videoReceiver](QSize size) {
            qCDebug(VideoManagerLog) << "Video" << videoReceiver.index << "resized. New resolution:" << size.width() << "x" << size.height();
            if (videoReceiver.index == 0) {
                _videoSize = (static_cast<quint32>(size.width()) << 16) | static_cast<quint32>(size.height());
                emit videoSizeChanged();
            }
        });

        (void) connect(videoReceiver.receiver, &VideoReceiver::onTakeScreenshotComplete, this, [&videoReceiver](VideoReceiver::STATUS status) {
            if (status == VideoReceiver::STATUS_OK) {
                qCDebug(VideoManagerLog) << "Video" << videoReceiver.index << "screenshot taken";
            } else {
                qCWarning(VideoManagerLog) << "Video" << videoReceiver.index << "screenshot failed";
            }
        });
    }

    _videoSourceChanged();

    startVideo();

    QQuickWindow *const rootWindow = qgcApp()->mainRootWindow();
    if (rootWindow) {
        rootWindow->scheduleRenderJob(new FinishVideoInitialization(), QQuickWindow::BeforeSynchronizingStage);
    }

    _initialized = true;
}

void VideoManager::cleanup()
{
    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        QGCCorePlugin::instance()->releaseVideoSink(videoReceiver.sink);
        delete videoReceiver.receiver;
        videoReceiver.receiver = nullptr;
    }
}

void VideoManager::startVideo()
{
    if (!_videoSettings->streamEnabled()->rawValue().toBool() || !hasVideo()) {
        qCDebug(VideoManagerLog) << "Stream not enabled/configured";
        return;
    }

    for (const VideoReceiverData &videoReceiver : _videoReceiverData) {
        _startReceiver(videoReceiver.index);
    }
}

void VideoManager::stopVideo()
{
    for (const VideoReceiverData &videoReceiver : _videoReceiverData) {
        _stopReceiver(videoReceiver.index);
    }
}

void VideoManager::startRecording(const QString &videoFile)
{
    const VideoReceiver::FILE_FORMAT fileFormat = static_cast<VideoReceiver::FILE_FORMAT>(_videoSettings->recordingFormat()->rawValue().toInt());
    if ((fileFormat < VideoReceiver::FILE_FORMAT_MIN) || (fileFormat >= VideoReceiver::FILE_FORMAT_MAX)) {
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
    const QString ext = kFileExtension[fileFormat - VideoReceiver::FILE_FORMAT_MIN];

    const QString videoFile1 = savePath + "/" + videoFileUrl + "." + ext;
    const QString videoFile2 = savePath + "/" + videoFileUrl + ".2." + ext;

    _videoFile = videoFile1;

    const QStringList videoFiles = {videoFile1, videoFile2};
    for (const VideoReceiverData &videoReceiver : _videoReceiverData) {
        if (videoReceiver.receiver && videoReceiver.started) {
            videoReceiver.receiver->startRecording(videoFiles.at(videoReceiver.index), fileFormat);
        } else {
            qCDebug(VideoManagerLog) << "Video receiver is not ready.";
        }
    }
}

void VideoManager::stopRecording()
{
    for (const VideoReceiverData &videoReceiver : _videoReceiverData) {
        videoReceiver.receiver->stopRecording();
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

    emit imageFileChanged();

    _videoReceiverData[0].receiver->takeScreenshot(_imageFile);
}

double VideoManager::aspectRatio() const
{
    if (_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo *const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if (pInfo) {
            qCDebug(VideoManagerLog) << "Primary AR:" << pInfo->aspectRatio();
            return pInfo->aspectRatio();
        }
    }

    // FIXME: AV: use _videoReceiver->videoSize() to calculate AR (if AR is not specified in the settings?)
    return _videoSettings->aspectRatio()->rawValue().toDouble();
}

double VideoManager::thermalAspectRatio() const
{
    if (_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo *const pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if (pInfo) {
            qCDebug(VideoManagerLog) << "Thermal AR:" << pInfo->aspectRatio();
            return pInfo->aspectRatio();
        }
    }

    return 1.0;
}

double VideoManager::hfov() const
{
    if (_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo *const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if (pInfo) {
            return pInfo->hfov();
        }
    }

    return 1.0;
}

double VideoManager::thermalHfov() const
{
    if (_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo *const pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if (pInfo) {
            return pInfo->aspectRatio();
        }
    }

    return _videoSettings->aspectRatio()->rawValue().toDouble();
}

bool VideoManager::hasThermal() const
{
    if (_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo *const pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if (pInfo) {
            return true;
        }
    }

    return false;
}

bool VideoManager::autoStreamConfigured() const
{
    if (_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo *const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if (pInfo) {
            return !pInfo->uri().isEmpty();
        }
    }

    return false;
}

bool VideoManager::hasVideo() const
{
    return (autoStreamConfigured() || _videoSettings->streamConfigured());
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

bool VideoManager::isUvc() const
{
    return (uvcEnabled() && (hasVideo() && !_uvcVideoSourceID.isEmpty()));
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
#ifndef QGC_DISABLE_UVC
    return UVCReceiver::enabled();
#else
    return false;
#endif
}

bool VideoManager::qtmultimediaEnabled()
{
#ifdef QGC_QT_STREAMING
    return true;
#else
    return false;
#endif
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

void VideoManager::_initVideo()
{
    const QQuickWindow *const root = qgcApp()->mainRootWindow();
    if (!root) {
        qCDebug(VideoManagerLog) << "mainRootWindow() failed. No root window";
        return;
    }

    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        QQuickItem *const widget = root->findChild<QQuickItem*>(videoReceiver.name);
        if (!widget || !videoReceiver.receiver) {
            qCDebug(VideoManagerLog) << videoReceiver.name << "receiver disabled";
            continue;
        }

        videoReceiver.sink = QGCCorePlugin::instance()->createVideoSink(this, widget);
        if (!videoReceiver.sink) {
            qCDebug(VideoManagerLog) << "createVideoSink() failed" << videoReceiver.index;
            continue;
        }

        if (videoReceiver.started) {
            qCDebug(VideoManagerLog) << videoReceiver.name << "receiver start decoding";
            videoReceiver.receiver->startDecoding(videoReceiver.sink);
        }
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
    for (const QFileInfo &video : vidList) {
        total += video.size();
    }

    const uint64_t maxSize = SettingsManager::instance()->videoSettings()->maxVideoSize()->rawValue().toUInt() * qPow(1024, 2);
    while ((total >= maxSize) && !vidList.isEmpty()) {
        total -= vidList.last().size();
        qCDebug(VideoManagerLog) << "Removing old video file:" << vidList.last().filePath();
        QFile file(vidList.last().filePath());
        (void) file.remove();
        vidList.removeLast();
    }
}

void VideoManager::_videoSourceChanged()
{
    for (const VideoReceiverData &videoReceiver : std::as_const(_videoReceiverData)) {
        (void) _updateSettings(videoReceiver.index);
    }

    emit hasVideoChanged();
    emit isStreamSourceChanged();
    emit isUvcChanged();
    emit isAutoStreamChanged();

    if (hasVideo()) {
        _restartAllVideos();
    } else {
        stopVideo();
    }

    qCDebug(VideoManagerLog) << "New Video Source:" << _videoSettings->videoSource()->rawValue().toString();
}

bool VideoManager::_updateUVC()
{
    bool result = false;

#ifndef QGC_DISABLE_UVC
    const QString oldUvcVideoSrcID = _uvcVideoSourceID;
    if (!hasVideo() || isStreamSource()) {
        _uvcVideoSourceID = "";
    } else {
        const QString uvcVideoSourceID = UVCReceiver::getSourceId();
        if (!uvcVideoSourceID.isEmpty()) {
            _uvcVideoSourceID = uvcVideoSourceID;
        }
    }

    if (oldUvcVideoSrcID != _uvcVideoSourceID) {
        qCDebug(VideoManagerLog) << "UVC changed from [" << oldUvcVideoSrcID << "] to [" << _uvcVideoSourceID << "]";
        UVCReceiver::checkPermission();
        result = true;
        emit uvcVideoSourceIDChanged();
        emit isUvcChanged();
    }
#endif

    return result;
}

bool VideoManager::_updateAutoStream(unsigned id)
{
    if (!_activeVehicle || !_activeVehicle->cameraManager()) {
        return false;
    }

    const QGCVideoStreamInfo *const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
    if (!pInfo) {
        return false;
    }

    bool settingsChanged = false;
    if (id == 0) {
        qCDebug(VideoManagerLog) << "Configure primary stream:" << pInfo->uri();
        switch(pInfo->type()) {
        case VIDEO_STREAM_TYPE_RTSP:
            settingsChanged = _updateVideoUri(id, pInfo->uri());
            if (settingsChanged) {
                _videoSettings->videoSource()->setRawValue(VideoSettings::videoSourceRTSP);
            }
            break;
        case VIDEO_STREAM_TYPE_TCP_MPEG:
            settingsChanged = _updateVideoUri(id, pInfo->uri());
            if (settingsChanged) {
                _videoSettings->videoSource()->setRawValue(VideoSettings::videoSourceTCP);
            }
            break;
        case VIDEO_STREAM_TYPE_RTPUDP: {
            const QString url = pInfo->uri().contains("udp://") ? pInfo->uri() : QStringLiteral("udp://0.0.0.0:%1").arg(pInfo->uri());
            settingsChanged = _updateVideoUri(id, url);
            if (settingsChanged) {
                _videoSettings->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
            }
            break;
        }
        case VIDEO_STREAM_TYPE_MPEG_TS:
            settingsChanged = _updateVideoUri(id, QStringLiteral("mpegts://0.0.0.0:%1").arg(pInfo->uri()));
            if (settingsChanged) {
                _videoSettings->videoSource()->setRawValue(VideoSettings::videoSourceMPEGTS);
            }
            break;
        default:
            settingsChanged = _updateVideoUri(id, pInfo->uri());
            break;
        }
    } else if (id == 1) {
        const QGCVideoStreamInfo *const pTinfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if (pTinfo) {
            qCDebug(VideoManagerLog) << "Configure secondary stream:" << pTinfo->uri();
            switch(pTinfo->type()) {
            case VIDEO_STREAM_TYPE_RTSP:
            case VIDEO_STREAM_TYPE_TCP_MPEG:
                settingsChanged = _updateVideoUri(id, pTinfo->uri());
                break;
            case VIDEO_STREAM_TYPE_RTPUDP:
                settingsChanged = _updateVideoUri(id, QStringLiteral("udp://0.0.0.0:%1").arg(pTinfo->uri()));
                break;
            case VIDEO_STREAM_TYPE_MPEG_TS:
                settingsChanged = _updateVideoUri(id, QStringLiteral("mpegts://0.0.0.0:%1").arg(pTinfo->uri()));
                break;
            default:
                settingsChanged = _updateVideoUri(id, pTinfo->uri());
                break;
            }
        }
    }

    return settingsChanged;
}

bool VideoManager::_updateSettings(unsigned id)
{
    if (!_videoSettings) {
        return false;
    }

    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return false;
    }

    bool settingsChanged = false;

    const bool lowLatencyStreaming = _videoSettings->lowLatencyMode()->rawValue().toBool();
    if (lowLatencyStreaming != _videoReceiverData[id].lowLatencyStreaming) {
        _videoReceiverData[id].lowLatencyStreaming = lowLatencyStreaming;
        settingsChanged = true;
    }

    settingsChanged |= _updateUVC();

    if (_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo *const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if (pInfo) {
            settingsChanged |= _updateAutoStream(id);
            return settingsChanged;
        }
    }

    if (id == 0) {
        const QString source = _videoSettings->videoSource()->rawValue().toString();
        if (source == VideoSettings::videoSourceUDPH264) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("udp://%1").arg(_videoSettings->udpUrl()->rawValue().toString()));
        } else if (source == VideoSettings::videoSourceUDPH265) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("udp265://%1").arg(_videoSettings->udpUrl()->rawValue().toString()));
        } else if (source == VideoSettings::videoSourceMPEGTS) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("mpegts://%1").arg(_videoSettings->udpUrl()->rawValue().toString()));
        } else if (source == VideoSettings::videoSourceRTSP) {
            settingsChanged |= _updateVideoUri(id, _videoSettings->rtspUrl()->rawValue().toString());
        } else if (source == VideoSettings::videoSourceTCP) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("tcp://%1").arg(_videoSettings->tcpUrl()->rawValue().toString()));
        } else if (source == VideoSettings::videoSource3DRSolo) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("udp://0.0.0.0:5600"));
        } else if (source == VideoSettings::videoSourceParrotDiscovery) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("udp://0.0.0.0:8888"));
        } else if (source == VideoSettings::videoSourceYuneecMantisG) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("rtsp://192.168.42.1:554/live"));
        } else if (source == VideoSettings::videoSourceHerelinkAirUnit) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("rtsp://192.168.0.10:8554/H264Video"));
        } else if (source == VideoSettings::videoSourceHerelinkHotspot) {
            settingsChanged |= _updateVideoUri(id, QStringLiteral("rtsp://192.168.43.1:8554/fpv_stream"));
        } else if (source == VideoSettings::videoDisabled || source == VideoSettings::videoSourceNoVideo) {
            settingsChanged |= _updateVideoUri(id, "");
        } else {
            settingsChanged |= _updateVideoUri(id, "");
            if (!isUvc()) {
                qCCritical(VideoManagerLog) << "Video source URI \"" << source << "\" is not supported. Please add support!";
            }
        }
    }

    return settingsChanged;
}

bool VideoManager::_updateVideoUri(unsigned id, const QString &uri)
{
    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return false;
    }

    if (uri == _videoReceiverData[id].uri) {
        return false;
    }

    qCDebug(VideoManagerLog) << "New Video URI" << uri;

    _videoReceiverData[id].uri = uri;

    return true;
}

void VideoManager::_restartVideo(unsigned id)
{
    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return;
    }

    qCDebug(VideoManagerLog) << "Restart video streaming" << id;

    if (_videoReceiverData[id].started) {
        _stopReceiver(id);
    }

    _startReceiver(id);
}

void VideoManager::_restartAllVideos()
{
    for (const VideoReceiverData &videoReceiver : _videoReceiverData) {
        _restartVideo(videoReceiver.index);
    }
}

void VideoManager::_startReceiver(unsigned id)
{
    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return;
    }

    if (!_videoReceiverData[id].receiver) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL" << id;
        return;
    }

    if (_videoReceiverData[id].uri.isEmpty()) {
        qCDebug(VideoManagerLog) << "VideoUri is NULL" << id;
        return;
    }

    const QString source = _videoSettings->videoSource()->rawValue().toString();
    const unsigned rtsptimeout = _videoSettings->rtspTimeout()->rawValue().toUInt();
    /* The gstreamer rtsp source will switch to tcp if udp is not available after 5 seconds.
       So we should allow for some negotiation time for rtsp */
    const unsigned timeout = (source == VideoSettings::videoSourceRTSP ? rtsptimeout : 15);

    _videoReceiverData[id].receiver->start(_videoReceiverData[id].uri, timeout, _videoReceiverData[id].lowLatencyStreaming ? -1 : 0);
}

void VideoManager::_stopReceiver(unsigned id)
{
    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return;
    }

    if (!_videoReceiverData[id].receiver) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL" << id;
        return;
    }

    _videoReceiverData[id].receiver->stop();
}

void VideoManager::_setActiveVehicle(Vehicle *vehicle)
{
    if (_activeVehicle) {
        (void) disconnect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, &VideoManager::_communicationLostChanged);
        if (_activeVehicle->cameraManager()) {
            MavlinkCameraControl *const pCamera = _activeVehicle->cameraManager()->currentCameraInstance();
            if (pCamera) {
                pCamera->stopStream();
            }
            (void) disconnect(_activeVehicle->cameraManager(), &QGCCameraManager::streamChanged, this, &VideoManager::_restartAllVideos);
        }
    }

    _activeVehicle = vehicle;
    if (_activeVehicle) {
        (void) connect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, &VideoManager::_communicationLostChanged);
        if (_activeVehicle->cameraManager()) {
            (void) connect(_activeVehicle->cameraManager(), &QGCCameraManager::streamChanged, this, &VideoManager::_restartAllVideos);
            MavlinkCameraControl *const pCamera = _activeVehicle->cameraManager()->currentCameraInstance();
            if (pCamera) {
                pCamera->resumeStream();
            }
        }
    } else {
        setfullScreen(false);
    }

    emit autoStreamConfiguredChanged();
    _restartAllVideos();
}

void VideoManager::_communicationLostChanged(bool connectionLost)
{
    if (connectionLost) {
        setfullScreen(false);
    }
}

/*===========================================================================*/

FinishVideoInitialization::FinishVideoInitialization()
    : QRunnable()
{
    // qCDebug(VideoManagerLog) << Q_FUNC_INFO << this;
}

FinishVideoInitialization::~FinishVideoInitialization()
{
    // qCDebug(VideoManagerLog) << Q_FUNC_INFO << this;
}

void FinishVideoInitialization::run()
{
    VideoManager::instance()->_initVideo();
}
