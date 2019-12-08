/****************************************************************************
 *
 *   (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<VideoManager> ("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
    qmlRegisterUncreatableType<VideoReceiver>("QGroundControl",              1, 0, "VideoReceiver","Reference only");
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
   _videoSettings = toolbox->settingsManager()->videoSettings();
   QString videoSource = _videoSettings->videoSource()->rawValue().toString();
   connect(_videoSettings->videoSource(),   &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_videoSettings->aspectRatio(),   &Fact::rawValueChanged, this, &VideoManager::_aspectRatioChanged);
   MultiVehicleManager *pVehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
   connect(pVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);

#if defined(QGC_GST_STREAMING)
#ifndef QGC_DISABLE_UVC
   // If we are using a UVC camera setup the device name
   _updateUVC();
#endif

    emit isGStreamerChanged();
    qCDebug(VideoManagerLog) << "New Video Source:" << videoSource;
    _videoReceiver = new VideoReceiver();
    _updateSettings();
    _videoReceiver->start();
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
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasVideo()
{
    if(autoStreamConfigured()) {
        return true;
    }
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return !videoSource.isEmpty()
            && videoSource != VideoSettings::videoSourceNoVideo
            && videoSource != VideoSettings::videoDisabled;
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

//-----------------------------------------------------------------------------
void
VideoManager::_updateSettings()
{
    //TODO: This should be done in the Video Stream
    if(!_videoSettings)
        return;
    //-- Auto discovery
    QString uri;
    QString thermalUri;
    auto videoSource = _toolbox->settingsManager()->videoSettings()->videoSource();

    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->currentStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Configure primary stream: " << pInfo->uri();
            switch(pInfo->type()) {
                case VIDEO_STREAM_TYPE_RTSP:
                    uri = pInfo->uri();
                    videoSource->setRawValue(VideoSettings::videoSourceRTSP);
                    break;
                case VIDEO_STREAM_TYPE_TCP_MPEG:
                    uri = (pInfo->uri());
                    videoSource->setRawValue(VideoSettings::videoSourceTCP);
                    break;
                case VIDEO_STREAM_TYPE_RTPUDP:
                    uri = QStringLiteral("udp://0.0.0.0:%1").arg(pInfo->uri());
                    videoSource->setRawValue(VideoSettings::videoSourceUDPH264);
                    break;
                case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                    uri = QStringLiteral("mpegts://0.0.0.0:%1").arg(pInfo->uri());
                    videoSource->setRawValue(VideoSettings::videoSourceMPEGTS);
                    break;
                default:
                    uri = pInfo->uri();
                    break;
            }
            //-- Thermal stream (if any)
            QGCVideoStreamInfo* pTinfo = _activeVehicle->dynamicCameras()->thermalStreamInstance();
            if(pTinfo) {
                qCDebug(VideoManagerLog) << "Configure secondary stream: " << pTinfo->uri();
                switch(pTinfo->type()) {
                    case VIDEO_STREAM_TYPE_RTSP:
                    case VIDEO_STREAM_TYPE_TCP_MPEG:
                        thermalUri = pTinfo->uri();
                        break;
                    case VIDEO_STREAM_TYPE_RTPUDP:
                        thermalUri = QStringLiteral("udp://0.0.0.0:%1").arg(pTinfo->uri());
                        break;
                    case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                        thermalUri = QStringLiteral("mpegts://0.0.0.0:%1").arg(pTinfo->uri());
                        break;
                    default:
                        thermalUri = pTinfo->uri();
                        break;
                }
            }
            return;
        }
    }
    QString source = _videoSettings->videoSource()->rawValue().toString();
    const int udpPort = _videoSettings->udpPort()->rawValue().toInt();
    const QString tcpUrl = _videoSettings->tcpUrl()->rawValue().toString();

    uri = source == VideoSettings::videoSourceUDPH264 ? QStringLiteral("udp://0.0.0.0:%1").arg(udpPort)
        : source == VideoSettings::videoSourceUDPH265 ? QStringLiteral("udp265://0.0.0.0:%1").arg(udpPort)
        : source == VideoSettings::videoSourceMPEGTS  ? QStringLiteral("mpegts://0.0.0.0:%1").arg(udpPort)
        : source == VideoSettings::videoSourceTCP     ? QStringLiteral("tcp://%1").arg(tcpUrl)
        : source == VideoSettings::videoSourceRTSP    ? _videoSettings->rtspUrl()->rawValue().toString()
        : uri;

    _videoReceiver->setUri(uri);
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
        }
    }
    _activeVehicle = vehicle;
    if(_activeVehicle) {
        connect(_activeVehicle, &Vehicle::connectionLostChanged, this, &VideoManager::_connectionLostChanged);
        if(_activeVehicle->dynamicCameras()) {
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
