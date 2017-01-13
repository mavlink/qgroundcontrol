/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QUrl>

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

#include <VideoItem.h>

#include "ScreenToolsController.h"
#include "VideoManager.h"
#include "QGCToolbox.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"

static const char* kVideoSourceKey  = "VideoSource";
static const char* kVideoUDPPortKey = "VideoUDPPort";
static const char* kVideoRTSPUrlKey = "VideoRTSPUrl";
static const char* kVideoSavePathKey = "VideoSavePath";
#if defined(QGC_GST_STREAMING)
static const char* kUDPStream       = "UDP Video Stream";
static const char* kRTSPStream      = "RTSP Video Stream";
#endif
static const char* kNoVideo         = "No Video Available";

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app)
    : QGCTool(app)
    , _videoSurface(NULL)
    , _videoReceiver(NULL)
    , _videoRunning(false)
    , _udpPort(5600) //-- Defalut Port 5600 == Solo UDP Port
    , _init(false)
{
}

//-----------------------------------------------------------------------------
VideoManager::~VideoManager()
{

}

//-----------------------------------------------------------------------------
void
VideoManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<VideoManager>("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
   //-- Get saved settings
#if defined(QGC_GST_STREAMING)
   QSettings settings;
#if defined(NO_UDP_VIDEO)
   setVideoSource(settings.value(kVideoSourceKey, kRTSPStream).toString());
#else
   setVideoSource(settings.value(kVideoSourceKey, kUDPStream).toString());
#endif
   //-- Check if core plugin defines its own video requirements
   if(qgcApp()->toolbox()->corePlugin()->options()->definesVideo()) {
       if(qgcApp()->toolbox()->corePlugin()->options()->videoUDPPort()) {
           setUdpPort(qgcApp()->toolbox()->corePlugin()->options()->videoUDPPort());
           setVideoSource(kUDPStream);
       } else {
           setVideoSource(kRTSPStream);
           setRtspURL(qgcApp()->toolbox()->corePlugin()->options()->videoRSTPUrl());
       }
   } else {
       setUdpPort(settings.value(kVideoUDPPortKey, 5600).toUInt());
       setRtspURL(settings.value(kVideoRTSPUrlKey, "rtsp://192.168.42.1:554/live").toString()); //-- Example RTSP URL
   }
   setVideoSavePath(settings.value(kVideoSavePathKey, QDir::homePath()).toString());
#endif
   _init = true;
#if defined(QGC_GST_STREAMING)
   _updateVideo();
   connect(&_frameTimer, &QTimer::timeout, this, &VideoManager::_updateTimer);
   _frameTimer.start(1000);
#endif
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasVideo()
{
#if defined(QGC_GST_STREAMING)
    return true;
#endif
    return !_videoSource.isEmpty();
}

//-----------------------------------------------------------------------------
bool
VideoManager::isGStreamer()
{
#if defined(QGC_GST_STREAMING)
    return _videoSource == kUDPStream || _videoSource == kRTSPStream;
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
VideoManager::setVideoSource(QString vSource)
{
    if(vSource == kNoVideo)
        return;
    _videoSource = vSource;
    QSettings settings;
    settings.setValue(kVideoSourceKey, vSource);
    emit videoSourceChanged();
#ifndef QGC_DISABLE_UVC
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        if(cameraInfo.description() == vSource) {
            _videoSourceID = cameraInfo.deviceName();
            emit videoSourceIDChanged();
            qCDebug(VideoManagerLog) << "Found USB source:" << _videoSourceID << " Name:" << _videoSource;
            break;
        }
    }
#endif
    emit isGStreamerChanged();
    qCDebug(VideoManagerLog) << "New Video Source:" << vSource;
    /*
     * Not working. Requires restart for now. (Undef KRTSP/kUDP above when enabling this)
    if(isGStreamer())
        _updateVideo();
    */
    if(_videoReceiver) {
        if(isGStreamer()) {
            _videoReceiver->start();
        } else {
            _videoReceiver->stop();
        }
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::setUdpPort(quint16 port)
{
    _udpPort = port;
    QSettings settings;
    settings.setValue(kVideoUDPPortKey, port);
    emit udpPortChanged();
    /*
     * Not working. Requires restart for now. (Undef KRTSP/kUDP above when enabling this)
    if(_videoSource == kUDPStream)
        _updateVideo();
    */
}

//-----------------------------------------------------------------------------
void
VideoManager::setRtspURL(QString url)
{
    _rtspURL = url;
    QSettings settings;
    settings.setValue(kVideoRTSPUrlKey, url);
    emit rtspURLChanged();
    /*
     * Not working. Requires restart for now. (Undef KRTSP/kUDP above when enabling this)
    if(_videoSource == kRTSPStream)
        _updateVideo();
    */
}

void
VideoManager::setVideoSavePathByUrl(QUrl url) {
    setVideoSavePath(url.toLocalFile());
}

void
VideoManager::setVideoSavePath(QString path)
{
    _videoSavePath = path;
    QSettings settings;
    settings.setValue(kVideoSavePathKey, path);
    if(_videoReceiver)
        _videoReceiver->setVideoSavePath(_videoSavePath);
    emit videoSavePathChanged();
}

//-----------------------------------------------------------------------------
QStringList
VideoManager::videoSourceList()
{
    _videoSourceList.clear();
#if defined(QGC_GST_STREAMING)
    _videoSourceList.append(kUDPStream);
    _videoSourceList.append(kRTSPStream);
#endif
#ifndef QGC_DISABLE_UVC
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        qCDebug(VideoManagerLog) << "UVC Video source ID:" << cameraInfo.deviceName() << " Name:" << cameraInfo.description();
        _videoSourceList.append(cameraInfo.description());
    }
#endif
    if(_videoSourceList.count() == 0)
        _videoSourceList.append(kNoVideo);
    return _videoSourceList;
}

//-----------------------------------------------------------------------------
void VideoManager::_updateTimer()
{
#if defined(QGC_GST_STREAMING)
    if(_videoReceiver && _videoSurface) {
        if(_videoReceiver->stopping() || _videoReceiver->starting()) {
            return;
        }

        if(_videoReceiver->streaming()) {
            if(!_videoRunning) {
                _videoSurface->setLastFrame(0);
                _videoRunning = true;
                emit videoRunningChanged();
            }
        } else {
            if(_videoRunning) {
                _videoRunning = false;
                emit videoRunningChanged();
            }
        }

        if(_videoRunning) {
            time_t elapsed = 0;
            time_t lastFrame = _videoSurface->lastFrame();
            if(lastFrame != 0) {
                elapsed = time(0) - _videoSurface->lastFrame();
            }
            if(elapsed > 2 && _videoSurface) {
                _videoReceiver->stop();
            }
        } else {
            if(!_videoReceiver->running()) {
                _videoReceiver->start();
            }
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void VideoManager::_updateVideo()
{
    if(_init) {
        if(_videoReceiver)
            delete _videoReceiver;
        if(_videoSurface)
            delete _videoSurface;
        _videoSurface  = new VideoSurface;
        _videoReceiver = new VideoReceiver(this);
        #if defined(QGC_GST_STREAMING)
        _videoReceiver->setVideoSink(_videoSurface->videoSink());
        if(_videoSource == kUDPStream)
            _videoReceiver->setUri(QStringLiteral("udp://0.0.0.0:%1").arg(_udpPort));
        else
            _videoReceiver->setUri(_rtspURL);
        _videoReceiver->setVideoSavePath(_videoSavePath);
        #endif
        _videoReceiver->start();
    }
}
