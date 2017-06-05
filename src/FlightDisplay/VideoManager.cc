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
#include <QDir>

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

#include <VideoItem.h>

#include "ScreenToolsController.h"
#include "VideoManager.h"
#include "QGCToolbox.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "Settings/SettingsManager.h"

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _videoSurface(NULL)
    , _videoReceiver(NULL)
    , _videoRunning(false)
    , _init(false)
    , _videoSettings(NULL)
    , _showFullScreen(false)
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

   _videoSettings = toolbox->settingsManager()->videoSettings();
   QString videoSource = _videoSettings->videoSource()->rawValue().toString();
   connect(_videoSettings->videoSource(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_videoSettings->udpPort(), &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged);
   connect(_videoSettings->rtspUrl(), &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged);


#if defined(QGC_GST_STREAMING)
#ifndef QGC_DISABLE_UVC
   // If we are using a UVC camera setup the device name
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        if(cameraInfo.description() == videoSource) {
            _videoSourceID = cameraInfo.deviceName();
            emit videoSourceIDChanged();
            qCDebug(VideoManagerLog) << "Found USB source:" << _videoSourceID << " Name:" << videoSource;
            break;
        }
    }
#endif

    emit isGStreamerChanged();
    qCDebug(VideoManagerLog) << "New Video Source:" << videoSource;

    if(_videoReceiver) {
        if(isGStreamer()) {
            _videoReceiver->start();
        } else {
            _videoReceiver->stop();
        }
    }
#endif

   _init = true;
#if defined(QGC_GST_STREAMING)
   _updateVideo();
   connect(&_frameTimer, &QTimer::timeout, this, &VideoManager::_updateTimer);
   _frameTimer.start(1000);
#endif
}

void VideoManager::_videoSourceChanged(void)
{
    emit hasVideoChanged();
    emit isGStreamerChanged();
    _restartVideo();
}

void VideoManager::_udpPortChanged(void)
{
    _restartVideo();
}

void VideoManager::_rtspUrlChanged(void)
{
    _restartVideo();
}

//-----------------------------------------------------------------------------
void
VideoManager::grabImage(QString imageFile)
{
    _imageFile = imageFile;
    emit imageFileChanged();
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasVideo()
{
#if defined(QGC_GST_STREAMING)
    return true;
#endif
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return !videoSource.isEmpty() && videoSource != VideoSettings::videoSourceNoVideo;
}

//-----------------------------------------------------------------------------
bool
VideoManager::isGStreamer()
{
#if defined(QGC_GST_STREAMING)
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return videoSource == VideoSettings::videoSourceUDP || videoSource == VideoSettings::videoSourceRTSP;
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
void VideoManager::_updateSettings()
{
    if(!_videoSettings || !_videoReceiver)
        return;

    if (_videoSettings->videoSource()->rawValue().toString() == VideoSettings::videoSourceUDP)
        _videoReceiver->setUri(QStringLiteral("udp://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else
        _videoReceiver->setUri(_videoSettings->rtspUrl()->rawValue().toString());
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
        _updateSettings();
#endif
        _videoReceiver->start();
    }
}

//-----------------------------------------------------------------------------
void VideoManager::_restartVideo()
{
    if(!_videoReceiver)
        return;
#if defined(QGC_GST_STREAMING)
    _videoReceiver->stop();
    _updateSettings();
    _videoReceiver->start();
#endif
}
