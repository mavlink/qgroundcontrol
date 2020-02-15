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

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

//-----------------------------------------------------------------------------
VideoManager::~VideoManager()
{
    delete _videoReceiver;
    _videoReceiver = nullptr;
    delete _thermalVideoReceiver;
    _thermalVideoReceiver = nullptr;
}

//-----------------------------------------------------------------------------
void
VideoManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<VideoManager> ("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
   qmlRegisterUncreatableType<VideoReceiver>("QGroundControl",              1, 0, "VideoReceiver","Reference only");
   _videoSettings = toolbox->settingsManager()->videoSettings();
   QString videoSource = _videoSettings->videoSource()->rawValue().toString();
   connect(_videoSettings->videoSource(),   &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_videoSettings->udpPort(),       &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged);
   connect(_videoSettings->rtspUrl(),       &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged);
   connect(_videoSettings->tcpUrl(),        &Fact::rawValueChanged, this, &VideoManager::_tcpUrlChanged);
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
    _videoReceiver = toolbox->corePlugin()->createVideoReceiver(this);
    _thermalVideoReceiver = toolbox->corePlugin()->createVideoReceiver(this);
    _updateSettings();
    if(isGStreamer()) {
        startVideo();
        _subtitleWriter.setVideoReceiver(_videoReceiver);
    } else {
        stopVideo();
    }

#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::startVideo()
{
    if(_videoReceiver) _videoReceiver->start();
    if(_thermalVideoReceiver) _thermalVideoReceiver->start();
}

//-----------------------------------------------------------------------------
void
VideoManager::stopVideo()
{
    if(_videoReceiver) _videoReceiver->stop();
    if(_thermalVideoReceiver) _thermalVideoReceiver->stop();
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
    restartVideo();
}

//-----------------------------------------------------------------------------
void
VideoManager::_udpPortChanged()
{
    restartVideo();
}

//-----------------------------------------------------------------------------
void
VideoManager::_rtspUrlChanged()
{
    restartVideo();
}

//-----------------------------------------------------------------------------
void
VideoManager::_tcpUrlChanged()
{
    restartVideo();
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

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
gboolean
VideoManager::_videoSinkQuery(GstPad* pad, GstObject* parent, GstQuery* query)
{
    GstElement* element;

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CAPS:
        element = gst_bin_get_by_name(GST_BIN(parent), "glupload");
        break;
    case GST_QUERY_CONTEXT:
        element = gst_bin_get_by_name(GST_BIN(parent), "qmlglsink");
        break;
    default:
        return gst_pad_query_default (pad, parent, query);
    }

    if (element == nullptr) {
        qWarning() << "VideoManager::_videoSinkQuery(): No element found";
        return FALSE;
    }

    GstPad* sinkpad = gst_element_get_static_pad(element, "sink");

    if (sinkpad == nullptr) {
        qWarning() << "VideoManager::_videoSinkQuery(): No sink pad found";
        return FALSE;
    }

    const gboolean ret = gst_pad_query(sinkpad, query);

    gst_object_unref(sinkpad);
    sinkpad = nullptr;

    return ret;
}

GstElement*
VideoManager::_makeVideoSink(gpointer widget)
{
    GstElement* glupload        = nullptr;
    GstElement* glcolorconvert  = nullptr;
    GstElement* qmlglsink       = nullptr;
    GstElement* bin             = nullptr;
    GstElement* sink            = nullptr;

    do {
        if ((glupload = gst_element_factory_make("glupload", "glupload")) == nullptr) {
            qCritical() << "VideoManager::_makeVideoSink() failed. Error with gst_element_factory_make('glupload')";
            break;
        }

        if ((glcolorconvert = gst_element_factory_make("glcolorconvert", "glcolorconvert")) == nullptr) {
            qCritical() << "VideoManager::_makeVideoSink() failed. Error with gst_element_factory_make('glcolorconvert')";
            break;
        }

        if ((qmlglsink = gst_element_factory_make("qmlglsink", "qmlglsink")) == nullptr) {
            qCritical() << "VideoManager::_makeVideoSink() failed. Error with gst_element_factory_make('qmlglsink')";
            break;
        }

        g_object_set(qmlglsink, "widget", widget, NULL);

        if ((bin = gst_bin_new("videosink")) == nullptr) {
            qCritical() << "VideoManager::_makeVideoSink() failed. Error with gst_bin_new('videosink')";
            break;
        }

        GstPad* pad;

        if ((pad = gst_element_get_static_pad(glupload, "sink")) == nullptr) {
            qCritical() << "VideoManager::_makeVideoSink() failed. Error with gst_element_get_static_pad(glupload, 'sink')";
            break;
        }

        gst_bin_add_many(GST_BIN(bin), glupload, glcolorconvert, qmlglsink, nullptr);

        gboolean ret = gst_element_link_many(glupload, glcolorconvert, qmlglsink, nullptr);

        qmlglsink = glcolorconvert = glupload = nullptr;

        if (!ret) {
            qCritical() << "VideoManager::_makeVideoSink() failed. Error with gst_element_link_many()";
            break;
        }

        GstPad* ghostpad = gst_ghost_pad_new("sink", pad);

        gst_pad_set_query_function(ghostpad, _videoSinkQuery);

        gst_element_add_pad(bin, ghostpad);

        gst_object_unref(pad);
        pad = nullptr;

        sink = bin;
        bin = nullptr;
    } while(0);

    if (bin != nullptr) {
        gst_object_unref(bin);
        bin = nullptr;
    }

    if (qmlglsink != nullptr) {
        gst_object_unref(qmlglsink);
        qmlglsink = nullptr;
    }

    if (glcolorconvert != nullptr) {
        gst_object_unref(glcolorconvert);
        glcolorconvert = nullptr;
    }

    if (glupload != nullptr) {
        gst_object_unref(glupload);
        glupload = nullptr;
    }

    return sink;
}
#endif

//-----------------------------------------------------------------------------
void
VideoManager::_initVideo()
{
#if defined(QGC_GST_STREAMING)
    QQuickItem* root = qgcApp()->mainRootWindow();

    if (root == nullptr) {
        qCDebug(VideoManagerLog) << "VideoManager::_makeVideoSink() failed. No root window";
        return;
    }

    QQuickItem* widget = root->findChild<QQuickItem*>("videoContent");

    if (widget != nullptr) {
        _videoReceiver->setVideoSink(_makeVideoSink(widget));
    } else {
        qCDebug(VideoManagerLog) << "VideoManager::_makeVideoSink() failed. 'videoContent' widget not found";
    }

    widget = root->findChild<QQuickItem*>("thermalVideo");

    if (widget != nullptr) {
        _thermalVideoReceiver->setVideoSink(_makeVideoSink(widget));
    } else {
        qCDebug(VideoManagerLog) << "VideoManager::_makeVideoSink() failed. 'thermalVideo' widget not found";
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_updateSettings()
{
    if(!_videoSettings || !_videoReceiver)
        return;
    //-- Auto discovery
    if(_activeVehicle && _activeVehicle->dynamicCameras()) {
        QGCVideoStreamInfo* pInfo = _activeVehicle->dynamicCameras()->currentStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Configure primary stream: " << pInfo->uri();
            switch(pInfo->type()) {
                case VIDEO_STREAM_TYPE_RTSP:
                    _videoReceiver->setUri(pInfo->uri());
                    _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceRTSP);
                    break;
                case VIDEO_STREAM_TYPE_TCP_MPEG:
                    _videoReceiver->setUri(pInfo->uri());
                    _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceTCP);
                    break;
                case VIDEO_STREAM_TYPE_RTPUDP:
                    _videoReceiver->setUri(QStringLiteral("udp://0.0.0.0:%1").arg(pInfo->uri()));
                    _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
                    break;
                case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                    _videoReceiver->setUri(QStringLiteral("mpegts://0.0.0.0:%1").arg(pInfo->uri()));
                    _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceMPEGTS);
                    break;
                default:
                    _videoReceiver->setUri(pInfo->uri());
                    break;
            }
            //-- Thermal stream (if any)
            QGCVideoStreamInfo* pTinfo = _activeVehicle->dynamicCameras()->thermalStreamInstance();
            if(pTinfo) {
                qCDebug(VideoManagerLog) << "Configure secondary stream: " << pTinfo->uri();
                switch(pTinfo->type()) {
                    case VIDEO_STREAM_TYPE_RTSP:
                    case VIDEO_STREAM_TYPE_TCP_MPEG:
                        _thermalVideoReceiver->setUri(pTinfo->uri());
                        break;
                    case VIDEO_STREAM_TYPE_RTPUDP:
                        _thermalVideoReceiver->setUri(QStringLiteral("udp://0.0.0.0:%1").arg(pTinfo->uri()));
                        break;
                    case VIDEO_STREAM_TYPE_MPEG_TS_H264:
                        _thermalVideoReceiver->setUri(QStringLiteral("mpegts://0.0.0.0:%1").arg(pTinfo->uri()));
                        break;
                    default:
                        _thermalVideoReceiver->setUri(pTinfo->uri());
                        break;
                }
            }
            return;
        }
    }
    QString source = _videoSettings->videoSource()->rawValue().toString();
    if (source == VideoSettings::videoSourceUDPH264)
        _videoReceiver->setUri(QStringLiteral("udp://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceUDPH265)
        _videoReceiver->setUri(QStringLiteral("udp265://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceMPEGTS)
        _videoReceiver->setUri(QStringLiteral("mpegts://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceRTSP)
        _videoReceiver->setUri(_videoSettings->rtspUrl()->rawValue().toString());
    else if (source == VideoSettings::videoSourceTCP)
        _videoReceiver->setUri(QStringLiteral("tcp://%1").arg(_videoSettings->tcpUrl()->rawValue().toString()));
}

//-----------------------------------------------------------------------------
void
VideoManager::restartVideo()
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoManagerLog) << "Restart video streaming";
    stopVideo();
    _updateSettings();
    startVideo();
    emit aspectRatioChanged();
#endif
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
            disconnect(_activeVehicle->dynamicCameras(), &QGCCameraManager::streamChanged, this, &VideoManager::restartVideo);
        }
    }
    _activeVehicle = vehicle;
    if(_activeVehicle) {
        connect(_activeVehicle, &Vehicle::connectionLostChanged, this, &VideoManager::_connectionLostChanged);
        if(_activeVehicle->dynamicCameras()) {
            connect(_activeVehicle->dynamicCameras(), &QGCCameraManager::streamChanged, this, &VideoManager::restartVideo);
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
    restartVideo();
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
