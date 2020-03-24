/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QUrl>
#include <QDir>

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

#include "ScreenToolsController.h"
#include "VideoManager.h"
#include "QGCToolbox.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "MultiVehicleManager.h"
#include "Settings/SettingsManager.h"
#include "Vehicle.h"
#include "QGCCameraManager.h"

#if defined(QGC_GST_STREAMING)
#include "GStreamer.h"
#else
#include "GLVideoItemStub.h"
#endif

#ifdef QGC_GST_TAISYNC_ENABLED
#include "TaisyncHandler.h"
#endif

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

#if defined(QGC_GST_STREAMING)
static const char* kFileExtension[VideoReceiver::FILE_FORMAT_MAX - VideoReceiver::FILE_FORMAT_MIN] = {
    "mkv",
    "mov",
    "mp4"
};
#endif

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
#if !defined(QGC_GST_STREAMING)
    static bool once = false;
    if (!once) {
        qmlRegisterType<GLVideoItemStub>("org.freedesktop.gstreamer.GLVideoItem", 1, 0, "GstGLVideoItem");
        once = true;
    }
#endif
}

//-----------------------------------------------------------------------------
VideoManager::~VideoManager()
{
    delete _videoReceiver;
    _videoReceiver = nullptr;
    delete _thermalVideoReceiver;
    _thermalVideoReceiver = nullptr;
#if defined(QGC_GST_STREAMING)
    GStreamer::releaseVideoSink(_thermalVideoSink);
    _thermalVideoSink = nullptr;
    GStreamer::releaseVideoSink(_videoSink);
    _videoSink = nullptr;
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<VideoManager> ("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
   qmlRegisterUncreatableType<VideoReceiver>("QGroundControl",              1, 0, "VideoReceiver","Reference only");

   // TODO: Those connections should be Per Video, not per VideoManager.
   _videoSettings = toolbox->settingsManager()->videoSettings();
   QString videoSource = _videoSettings->videoSource()->rawValue().toString();
   connect(_videoSettings->videoSource(),   &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_videoSettings->udpPort(),       &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged);
   connect(_videoSettings->rtspUrl(),       &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged);
   connect(_videoSettings->tcpUrl(),        &Fact::rawValueChanged, this, &VideoManager::_tcpUrlChanged);
   connect(_videoSettings->aspectRatio(),   &Fact::rawValueChanged, this, &VideoManager::_aspectRatioChanged);
   connect(_videoSettings->lowLatencyMode(),&Fact::rawValueChanged, this, &VideoManager::_lowLatencyModeChanged);
   MultiVehicleManager *pVehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
   connect(pVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);

#if defined(QGC_GST_STREAMING)
#ifndef QGC_DISABLE_UVC
   // If we are using a UVC camera setup the device name
   _updateUVC();
#endif

    emit isGStreamerChanged();
    qCDebug(VideoManagerLog) << "New Video Source:" << videoSource;
#if defined(QGC_GST_STREAMING)
    _videoReceiver = toolbox->corePlugin()->createVideoReceiver(this);
    _thermalVideoReceiver = toolbox->corePlugin()->createVideoReceiver(this);

    connect(_videoReceiver, &VideoReceiver::timeout, this, &VideoManager::_restartVideo);
    connect(_videoReceiver, &VideoReceiver::streamingChanged, this, &VideoManager::_streamingChanged);
    connect(_videoReceiver, &VideoReceiver::recordingStarted, this,  &VideoManager::_recordingStarted);
    connect(_videoReceiver, &VideoReceiver::recordingChanged, this,  &VideoManager::_recordingChanged);
    connect(_videoReceiver, &VideoReceiver::screenshotComplete, this,  &VideoManager::_screenshotComplete);

    // FIXME: AV: I believe _thermalVideoReceiver should be handled just like _videoReceiver in terms of event
    // and I expect that it will be changed during multiple video stream activity
    connect(_thermalVideoReceiver, &VideoReceiver::timeout, this, &VideoManager::_restartVideo);
    connect(_thermalVideoReceiver, &VideoReceiver::streamingChanged, this, &VideoManager::_streamingChanged);
#endif
    _updateSettings();
    if(isGStreamer()) {
        startVideo();
    } else {
        stopVideo();
    }

#endif
}

void VideoManager::_cleanupOldVideos()
{
#if defined(QGC_GST_STREAMING)
    //-- Only perform cleanup if storage limit is enabled
    if(!_videoSettings->enableStorageLimit()->rawValue().toBool()) {
        return;
    }
    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->videoSavePath();
    QDir videoDir = QDir(savePath);
    videoDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    videoDir.setSorting(QDir::Time);

    QStringList nameFilters;

    for(size_t i = 0; i < sizeof(kFileExtension) / sizeof(kFileExtension[0]); i += 1) {
        nameFilters << QString("*.") + kFileExtension[i];
    }

    videoDir.setNameFilters(nameFilters);
    //-- get the list of videos stored
    QFileInfoList vidList = videoDir.entryInfoList();
    if(!vidList.isEmpty()) {
        uint64_t total   = 0;
        //-- Settings are stored using MB
        uint64_t maxSize = _videoSettings->maxVideoSize()->rawValue().toUInt() * 1024 * 1024;
        //-- Compute total used storage
        for(int i = 0; i < vidList.size(); i++) {
            total += vidList[i].size();
        }
        //-- Remove old movies until max size is satisfied.
        while(total >= maxSize && !vidList.isEmpty()) {
            total -= vidList.last().size();
            qCDebug(VideoManagerLog) << "Removing old video file:" << vidList.last().filePath();
            QFile file (vidList.last().filePath());
            file.remove();
            vidList.removeLast();
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::startVideo()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }

    if(!_videoSettings->streamEnabled()->rawValue().toBool() || !_videoSettings->streamConfigured()) {
        qCDebug(VideoManagerLog) << "Stream not enabled/configured";
        return;
    }

#if defined(QGC_GST_STREAMING)
    const unsigned timeout = _videoSettings->rtspTimeout()->rawValue().toUInt();

    if(_videoReceiver != nullptr) {
        _videoReceiver->start(_videoUri, timeout);
        if (_videoSink != nullptr) {
            _videoReceiver->startDecoding(_videoSink);
        }
    }

    if(_thermalVideoReceiver != nullptr) {
        _thermalVideoReceiver->start(_thermalVideoUri, timeout);
        if (_thermalVideoSink != nullptr) {
            _thermalVideoReceiver->startDecoding(_thermalVideoSink);
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::stopVideo()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if(_videoReceiver) _videoReceiver->stop();
    if(_thermalVideoReceiver) _thermalVideoReceiver->stop();
#endif
}

void
VideoManager::startRecording(const QString& videoFile)
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if (!_videoReceiver) {
        qgcApp()->showAppMessage(tr("Video receiver is not ready."));
        return;
    }

    const VideoReceiver::FILE_FORMAT fileFormat = static_cast<VideoReceiver::FILE_FORMAT>(_videoSettings->recordingFormat()->rawValue().toInt());

    if(fileFormat < VideoReceiver::FILE_FORMAT_MIN || fileFormat >= VideoReceiver::FILE_FORMAT_MAX) {
        qgcApp()->showAppMessage(tr("Invalid video format defined."));
        return;
    }

    //-- Disk usage maintenance
    _cleanupOldVideos();

    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->videoSavePath();

    if(savePath.isEmpty()) {
        qgcApp()->showAppMessage(tr("Unabled to record video. Video save path must be specified in Settings."));
        return;
    }

    _videoFile = savePath + "/"
            + (videoFile.isEmpty() ? QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss") : videoFile)
            + "." + kFileExtension[fileFormat - VideoReceiver::FILE_FORMAT_MIN];

    _videoReceiver->startRecording(_videoFile, fileFormat);
#else
    Q_UNUSED(videoFile)
#endif
}

void
VideoManager::stopRecording()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if (!_videoReceiver) {
        return;
    }

    _videoReceiver->stopRecording();
#endif
}

void
VideoManager::grabImage(const QString& imageFile)
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if (!_videoReceiver) {
        return;
    }

    _imageFile = imageFile;

    emit imageFileChanged();

    _videoReceiver->takeScreenshot(_imageFile);
#else
    Q_UNUSED(imageFile)
#endif
}

//-----------------------------------------------------------------------------
double VideoManager::aspectRatio()
{
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->currentStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Primary AR: " << pInfo->aspectRatio();
            return pInfo->aspectRatio();
        }
    }
    // FIXME: AV: use _videoReceiver->videoSize() to calculate AR (if AR is not specified in the settings?)
    return _videoSettings->aspectRatio()->rawValue().toDouble();
}

//-----------------------------------------------------------------------------
double VideoManager::thermalAspectRatio()
{
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->thermalStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Thermal AR: " << pInfo->aspectRatio();
            return pInfo->aspectRatio();
        }
    }
    return 1.0;
}

//-----------------------------------------------------------------------------
double VideoManager::hfov()
{
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->currentStreamInstance();
        if(pInfo) {
            return pInfo->hfov();
        }
    }
    return 1.0;
}

//-----------------------------------------------------------------------------
double VideoManager::thermalHfov()
{
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->thermalStreamInstance();
        if(pInfo) {
            return pInfo->aspectRatio();
        }
    }
    return _videoSettings->aspectRatio()->rawValue().toDouble();
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasThermal()
{
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->thermalStreamInstance();
        if(pInfo) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
QString
VideoManager::imageFile()
{
    return _imageFile;
}

//-----------------------------------------------------------------------------
bool
VideoManager::autoStreamConfigured()
{
#if defined(QGC_GST_STREAMING)
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->currentStreamInstance();
        if(pInfo) {
            return !pInfo->uri().isEmpty();
        }
    }
#endif
    return false;
}

//-----------------------------------------------------------------------------
void
VideoManager::_updateUVC()
{
#ifndef QGC_DISABLE_UVC
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo: cameras) {
        if(cameraInfo.description() == videoSource) {
            _videoSourceID = cameraInfo.deviceName();
            emit videoSourceIDChanged();
            qCDebug(VideoManagerLog) << "Found USB source:" << _videoSourceID << " Name:" << videoSource;
            break;
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_videoSourceChanged()
{
    _updateUVC();
    emit hasVideoChanged();
    emit isGStreamerChanged();
    emit isAutoStreamChanged();
    _restartVideo();
}

//-----------------------------------------------------------------------------
void
VideoManager::_udpPortChanged()
{
    _restartVideo();
}

//-----------------------------------------------------------------------------
void
VideoManager::_rtspUrlChanged()
{
    _restartVideo();
}

//-----------------------------------------------------------------------------
void
VideoManager::_tcpUrlChanged()
{
    _restartVideo();
}

//-----------------------------------------------------------------------------
void
VideoManager::_lowLatencyModeChanged()
{
    //restartVideo();
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasVideo()
{
    if(autoStreamConfigured()) {
        return true;
    }
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return !videoSource.isEmpty() && videoSource != VideoSettings::videoSourceNoVideo && videoSource != VideoSettings::videoDisabled;
}

//-----------------------------------------------------------------------------
bool
VideoManager::isGStreamer()
{
#if defined(QGC_GST_STREAMING)
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return
        videoSource == VideoSettings::videoSourceUDPH264 ||
        videoSource == VideoSettings::videoSourceUDPH265 ||
        videoSource == VideoSettings::videoSourceRTSP ||
        videoSource == VideoSettings::videoSourceTCP ||
        videoSource == VideoSettings::videoSourceMPEGTS ||
        autoStreamConfigured();
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
#ifndef QGC_DISABLE_UVC
bool
VideoManager::uvcEnabled()
{
    return QCameraInfo::availableCameras().count() > 0;
}
#endif

//-----------------------------------------------------------------------------
void
VideoManager::setfullScreen(bool f)
{
    if(f) {
        //-- No can do if no vehicle or connection lost
        if(!_activeVehicle || _activeVehicle->connectionLost()) {
            f = false;
        }
    }
    _fullScreen = f;
    emit fullScreenChanged();
}

void
VideoManager::_initVideo()
{
#if defined(QGC_GST_STREAMING)
    QQuickItem* root = qgcApp()->mainRootWindow();

    if (root == nullptr) {
        qCDebug(VideoManagerLog) << "mainRootWindow() failed. No root window";
        return;
    }

    QQuickItem* widget = root->findChild<QQuickItem*>("videoContent");

    if (widget != nullptr && _videoReceiver != nullptr) {
        if ((_videoSink = qgcApp()->toolbox()->corePlugin()->createVideoSink(this, widget)) != nullptr) {
            _videoReceiver->startDecoding(_videoSink);
        } else {
            qCDebug(VideoManagerLog) << "createVideoSink() failed";
        }
    } else {
        qCDebug(VideoManagerLog) << "video receiver disabled";
    }

    widget = root->findChild<QQuickItem*>("thermalVideo");

    if (widget != nullptr && _thermalVideoReceiver != nullptr) {
        if ((_thermalVideoSink = qgcApp()->toolbox()->corePlugin()->createVideoSink(this, widget)) != nullptr) {
            _thermalVideoReceiver->startDecoding(_thermalVideoSink);
        } else {
            qCDebug(VideoManagerLog) << "createVideoSink() failed";
        }
    } else {
        qCDebug(VideoManagerLog) << "thermal video receiver disabled";
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_updateSettings()
{
    if(!_videoSettings)
        return;
    //-- Auto discovery
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->currentStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Configure primary stream: " << pInfo->uri();
            switch(pInfo->type()) {
                case VIDEO_STREAM_TYPE_RTSP:
                    _setVideoUri(pInfo->uri());
                    _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceRTSP);
                    break;
                case VIDEO_STREAM_TYPE_TCP_MPEG:
                    _setVideoUri(pInfo->uri());
                    _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceTCP);
                    break;
                case VIDEO_STREAM_TYPE_RTPUDP:
                    _setVideoUri(QStringLiteral("udp://0.0.0.0:%1").arg(pInfo->uri()));
                    _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
                    break;
                case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                    _setVideoUri(QStringLiteral("mpegts://0.0.0.0:%1").arg(pInfo->uri()));
                    _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceMPEGTS);
                    break;
                default:
                    _setVideoUri(pInfo->uri());
                    break;
            }
            //-- Thermal stream (if any)
            QGCVideoStreamInfo* pTinfo = _activeVehicle->dynamicCameras()->thermalStreamInstance();
            if(pTinfo) {
                qCDebug(VideoManagerLog) << "Configure secondary stream: " << pTinfo->uri();
                switch(pTinfo->type()) {
                    case VIDEO_STREAM_TYPE_RTSP:
                    case VIDEO_STREAM_TYPE_TCP_MPEG:
                        _setThermalVideoUri(pTinfo->uri());
                        break;
                    case VIDEO_STREAM_TYPE_RTPUDP:
                        _setThermalVideoUri(QStringLiteral("udp://0.0.0.0:%1").arg(pTinfo->uri()));
                        break;
                    case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                        _setThermalVideoUri(QStringLiteral("mpegts://0.0.0.0:%1").arg(pTinfo->uri()));
                        break;
                    default:
                        _setThermalVideoUri(pTinfo->uri());
                        break;
                }
            }
            return;
        }
    }
    QString source = _videoSettings->videoSource()->rawValue().toString();
    if (source == VideoSettings::videoSourceUDPH264)
        _setVideoUri(QStringLiteral("udp://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceUDPH265)
        _setVideoUri(QStringLiteral("udp265://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceMPEGTS)
        _setVideoUri(QStringLiteral("mpegts://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceRTSP)
        _setVideoUri(_videoSettings->rtspUrl()->rawValue().toString());
    else if (source == VideoSettings::videoSourceTCP)
        _setVideoUri(QStringLiteral("tcp://%1").arg(_videoSettings->tcpUrl()->rawValue().toString()));
}

void
VideoManager::_setVideoUri(const QString& uri)
{
#if defined(QGC_GST_TAISYNC_ENABLED) && (defined(__android__) || defined(__ios__))
    //-- Taisync on iOS or Android sends a raw h.264 stream
    if (isTaisync()) {
        _videoUri = QString("tsusb://0.0.0.0:%1").arg(TAISYNC_VIDEO_UDP_PORT);
        return;
    }
#endif
    _videoUri = uri;
}

void
VideoManager::_setThermalVideoUri(const QString& uri)
{
#if defined(QGC_GST_TAISYNC_ENABLED) && (defined(__android__) || defined(__ios__))
    //-- Taisync on iOS or Android sends a raw h.264 stream
    if (isTaisync()) {
        // FIXME: AV: TAISYNC_VIDEO_UDP_PORT is used by video stream, thermal stream should go via its own proxy
        _thermalVideoUri = QString("tsusb://0.0.0.0:%1").arg(TAISYNC_VIDEO_UDP_PORT);
        return;
    }
#endif
    _thermalVideoUri = uri;
}

//-----------------------------------------------------------------------------
void
VideoManager::_streamingChanged()
{
#if defined(QGC_GST_STREAMING)
    // FIXME: AV: we need VideoReceiver::running() to avoid restarting if one of streams is not active
    // but since VideoManager is going to be relpaced by Video Model during multiple video streaming development activity
    // I'll leave it as is for week or two
    if ((_videoReceiver && !_videoReceiver->streaming())
            /*|| (_thermalVideoReceiver && !_thermalVideoReceiver->streaming())*/) {
        _restartVideo();
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartVideo()
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoManagerLog) << "Restart video streaming";
    stopVideo();
    _updateSettings();
    startVideo();
    emit aspectRatioChanged();
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_recordingStarted()
{
    _subtitleWriter.startCapturingTelemetry(_videoFile);
}

//-----------------------------------------------------------------------------
void
VideoManager::_recordingChanged()
{
#if defined(QGC_GST_STREAMING)
    if (_videoReceiver && !_videoReceiver->recording()) {
        _subtitleWriter.stopCapturingTelemetry();
    }
#endif
}

//----------------------------------------------------------------------------------------
void
VideoManager::_screenshotComplete()
{
}

//----------------------------------------------------------------------------------------
void
VideoManager::_setActiveVehicle(Vehicle* vehicle)
{
    if(_activeVehicle) {
        disconnect(_activeVehicle, &Vehicle::connectionLostChanged, this, &VideoManager::_connectionLostChanged);
        if(_activeVehicle->dynamicCameras()) {
            QGCCameraControl* pCamera = _activeVehicle->dynamicCameras()->currentCameraInstance();
            if(pCamera) {
                pCamera->stopStream();
            }
            disconnect(_activeVehicle->dynamicCameras(), &QGCCameraManager::streamChanged, this, &VideoManager::_restartVideo);
        }
    }
    _activeVehicle = vehicle;
    if(_activeVehicle) {
        connect(_activeVehicle, &Vehicle::connectionLostChanged, this, &VideoManager::_connectionLostChanged);
        if(_activeVehicle->dynamicCameras()) {
            connect(_activeVehicle->dynamicCameras(), &QGCCameraManager::streamChanged, this, &VideoManager::_restartVideo);
            QGCCameraControl* pCamera = _activeVehicle->dynamicCameras()->currentCameraInstance();
            if(pCamera) {
                pCamera->resumeStream();
            }
        }
    } else {
        //-- Disable full screen video if vehicle is gone
        setfullScreen(false);
    }
    emit autoStreamConfiguredChanged();
    _restartVideo();
}

//----------------------------------------------------------------------------------------
void
VideoManager::_connectionLostChanged(bool connectionLost)
{
    if(connectionLost) {
        //-- Disable full screen video if connection is lost
        setfullScreen(false);
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_aspectRatioChanged()
{
    emit aspectRatioChanged();
}
