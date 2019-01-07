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
#include "JoystickManager.h"

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    _streamInfo = {};
    _lastZoomChange.start();
    _lastStreamChange.start();
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
   connect(_videoSettings->aspectRatio(),   &Fact::rawValueChanged, this, &VideoManager::_streamInfoChanged);
   MultiVehicleManager *pVehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
   connect(pVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);
   JoystickManager *pJoyMgr = qgcApp()->toolbox()->joystickManager();
   connect(pJoyMgr, &JoystickManager::activeJoystickChanged, this, &VideoManager::_activeJoystickChanged);

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
        videoSource == VideoSettings::videoSourceTCP ||
        videoSource == VideoSettings::videoSourceMPEGTS;
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
    QString source = _videoSettings->videoSource()->rawValue().toString();
    if (source == VideoSettings::videoSourceUDP)
        _videoReceiver->setUri(QStringLiteral("udp://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceMPEGTS)
        _videoReceiver->setUri(QStringLiteral("mpegts://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceRTSP)
        _videoReceiver->setUri(_videoSettings->rtspUrl()->rawValue().toString());
    else if (source == VideoSettings::videoSourceTCP)
        _videoReceiver->setUri(QStringLiteral("tcp://%1").arg(_videoSettings->tcpUrl()->rawValue().toString()));
    //-- Auto discovery
    else if (isAutoStream()) {
        switch(_streamInfo.type) {
            case VIDEO_STREAM_TYPE_RTSP:
            case VIDEO_STREAM_TYPE_TCP_MPEG:
                _videoReceiver->setUri(QString(_streamInfo.uri));
                break;
            case VIDEO_STREAM_TYPE_RTPUDP:
                _videoReceiver->setUri(QStringLiteral("udp://0.0.0.0:%1").arg(atoi(_streamInfo.uri)));
                break;
            case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                _videoReceiver->setUri(QStringLiteral("mpegts://0.0.0.0:%1").arg(atoi(_streamInfo.uri)));
                break;
            default:
                _videoReceiver->setUri(QString(_streamInfo.uri));
                break;
        }
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartVideo()
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoManagerLog) << "Restart video streaming";
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
            1);                                                   // Request video stream information
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_vehicleMessageReceived(const mavlink_message_t& message)
{
    //-- For now we only handle one stream. There is no UI to pick different streams.
    if(message.msgid == MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION) {
        _videoStreamCompID = message.compid;
        mavlink_msg_video_stream_information_decode(&message, &_streamInfo);
        _hasAutoStream = true;
        qCDebug(VideoManagerLog) << "Received video stream info:" << _streamInfo.uri;
        _restartVideo();
        emit streamInfoChanged();
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_activeJoystickChanged(Joystick* joystick)
{
    if(_activeJoystick) {
        disconnect(_activeJoystick, &Joystick::stepZoom,   this, &VideoManager::stepZoom);
        disconnect(_activeJoystick, &Joystick::stepStream, this, &VideoManager::stepStream);
    }
    _activeJoystick = joystick;
    if(_activeJoystick) {
        connect(_activeJoystick, &Joystick::stepZoom,   this, &VideoManager::stepZoom);
        connect(_activeJoystick, &Joystick::stepStream, this, &VideoManager::stepStream);
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::stepZoom(int direction)
{
    if(_lastZoomChange.elapsed() > 250) {
        _lastZoomChange.start();
        qCDebug(VideoManagerLog) << "Step Stream Zoom" << direction;
        if(_activeVehicle && hasZoom()) {
            _activeVehicle->sendMavCommand(
                _videoStreamCompID,                     // Target component
                MAV_CMD_SET_CAMERA_ZOOM,                // Command id
                false,                                  // ShowError
                ZOOM_TYPE_STEP,                         // Zoom type
                direction);                             // Direction (-1 wide, 1 tele)
        }
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::stepStream(int direction)
{
    if(_lastStreamChange.elapsed() > 250) {
        _lastStreamChange.start();
        int s = _currentStream + direction;
        if(s < 1) s = _streamInfo.count;
        if(s > _streamInfo.count) s = 1;
        setCurrentStream(s);
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::setCurrentStream(int stream)
{
    qCDebug(VideoManagerLog) << "Set Stream" << stream;
    //-- TODO: Handle multiple streams
    if(_hasAutoStream && stream <= _streamInfo.count && stream > 0 && _activeVehicle) {
        if(_currentStream != stream) {
            //-- Stop current stream
            _activeVehicle->sendMavCommand(
                _videoStreamCompID,                     // Target component
                MAV_CMD_VIDEO_STOP_STREAMING,           // Command id
                false,                                  // ShowError
                _currentStream);                        // Stream ID
            //-- Start new stream
            _currentStream = stream;
            _activeVehicle->sendMavCommand(
                _videoStreamCompID,                     // Target component
                MAV_CMD_VIDEO_START_STREAMING,          // Command id
                false,                                  // ShowError
                _currentStream);                        // Stream ID
            _currentStream = stream;
            emit currentStreamChanged();
        }
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_streamInfoChanged()
{
    emit streamInfoChanged();
}
