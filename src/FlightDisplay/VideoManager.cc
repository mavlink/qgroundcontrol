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
#include "MultiVehicleManager.h"
#include "Settings/SettingsManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

//-----------------------------------------------------------------------------
VideoManager::~VideoManager()
{
    if(_videoReceiver) {
        delete _videoReceiver;
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<VideoManager> ("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
   qmlRegisterUncreatableType<VideoReceiver>("QGroundControl",              1, 0, "VideoReceiver","Reference only");
   qmlRegisterUncreatableType<VideoSurface> ("QGroundControl",              1, 0, "VideoSurface", "Reference only");
   _videoSettings = toolbox->settingsManager()->videoSettings();
   QString videoSource = _videoSettings->videoSource()->rawValue().toString();
   connect(_videoSettings->videoSource(),   &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_videoSettings->udpPort(),       &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged);
   connect(_videoSettings->rtspUrl(),       &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged);
   connect(_videoSettings->tcpUrl(),        &Fact::rawValueChanged, this, &VideoManager::_tcpUrlChanged);
   connect(_videoSettings->aspectRatio(),   &Fact::rawValueChanged, this, &VideoManager::_aspectRatioChanged);
   MultiVehicleManager *manager = qgcApp()->toolbox()->multiVehicleManager();
   connect(manager, &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);

#if defined(QGC_GST_STREAMING)
#ifndef QGC_DISABLE_UVC
   // If we are using a UVC camera setup the device name
   _updateUVC();
#endif

    emit isGStreamerChanged();
    qCDebug(VideoManagerLog) << "New Video Source:" << videoSource;
    _videoReceiver = toolbox->corePlugin()->createVideoReceiver(this);
    _updateSettings();
    if(isGStreamer()) {
        _videoReceiver->start();
    } else {
        _videoReceiver->stop();
    }

#endif
}

//-----------------------------------------------------------------------------
double VideoManager::aspectRatio()
{
    if(isAutoStream()) {
        if(_streamInfo.resolution_h && _streamInfo.resolution_v) {
            return static_cast<double>(_streamInfo.resolution_h) / static_cast<double>(_streamInfo.resolution_v);
        }
        return 1.0;
    } else {
        return _videoSettings->aspectRatio()->rawValue().toDouble();
    }
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
bool
VideoManager::hasVideo()
{
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
        videoSource == VideoSettings::videoSourceUDP ||
        videoSource == VideoSettings::videoSourceRTSP ||
        videoSource == VideoSettings::videoSourceAuto ||
        videoSource == VideoSettings::videoSourceTCP;
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
bool
VideoManager::isAutoStream()
{
#if defined(QGC_GST_STREAMING)
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return  videoSource == VideoSettings::videoSourceAuto;
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
VideoManager::_updateSettings()
{
    if(!_videoSettings || !_videoReceiver)
        return;
    if (_videoSettings->videoSource()->rawValue().toString() == VideoSettings::videoSourceUDP)
        _videoReceiver->setUri(QStringLiteral("udp://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (_videoSettings->videoSource()->rawValue().toString() == VideoSettings::videoSourceRTSP)
        _videoReceiver->setUri(_videoSettings->rtspUrl()->rawValue().toString());
    else if (_videoSettings->videoSource()->rawValue().toString() == VideoSettings::videoSourceTCP)
        _videoReceiver->setUri(QStringLiteral("tcp://%1").arg(_videoSettings->tcpUrl()->rawValue().toString()));
    else if (isAutoStream())
        _videoReceiver->setUri(QString(_streamInfo.uri));
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartVideo()
{
#if defined(QGC_GST_STREAMING)
    if(!_videoReceiver)
        return;
    _videoReceiver->stop();
    _updateSettings();
    _videoReceiver->start();
#endif
}

//----------------------------------------------------------------------------------------
void
VideoManager::_setActiveVehicle(Vehicle* vehicle)
{
    if(_activeVehicle) {
        disconnect(_activeVehicle, &Vehicle::mavlinkMessageReceived, this, &VideoManager::_vehicleMessageReceived);
    }
    _activeVehicle = vehicle;
    if(_activeVehicle) {
        if(isAutoStream()) {
            _videoReceiver->stop();
        }
        //-- Video Stream Discovery
        connect(_activeVehicle, &Vehicle::mavlinkMessageReceived, this, &VideoManager::_vehicleMessageReceived);
        qCDebug(VideoManagerLog) << "Requesting video stream info";
        _activeVehicle->sendMavCommand(
            MAV_COMP_ID_ALL,                                      // Target component
            MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION,             // Command id
            false,                                                // ShowError
            1,                                                    // First camera only
            0);                                                   // Reserved (Set to 0)
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_vehicleMessageReceived(const mavlink_message_t& message)
{
    //-- For now we only handle one stream. There is no UI to pick different streams.
    if(message.msgid == MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION) {
        mavlink_msg_video_stream_information_decode(&message, &_streamInfo);
        qCDebug(VideoManagerLog) << "Received video stream info:" << _streamInfo.uri;
        _restartVideo();
        emit aspectRatioChanged();
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_aspectRatioChanged()
{
    emit aspectRatioChanged();
}
