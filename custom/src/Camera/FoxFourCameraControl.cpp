

#include "FoxFourCameraControl.h"
#include "QGCCameraIO.h"
#include "VideoManager.h"
#include "FTPManager.h"
#include "QGCCorePlugin.h"
#include "Vehicle.h"
#include "MissionCommandTree.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtXml/QDomDocument>
#include <QtXml/QDomNodeList>
#include <QtQml/QQmlEngine>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkReply>



//-----------------------------------------------------------------------------
static bool
read_attribute(QDomNode& node, const char* tagName, bool& target)
{
    QDomNamedNodeMap attrs = node.attributes();
    if(!attrs.count()) {
        return false;
    }
    QDomNode subNode = attrs.namedItem(tagName);
    if(subNode.isNull()) {
        return false;
    }
    target = subNode.nodeValue() != "0";
    return true;
}

//-----------------------------------------------------------------------------
static bool
read_attribute(QDomNode& node, const char* tagName, int& target)
{
    QDomNamedNodeMap attrs = node.attributes();
    if(!attrs.count()) {
        return false;
    }
    QDomNode subNode = attrs.namedItem(tagName);
    if(subNode.isNull()) {
        return false;
    }
    target = subNode.nodeValue().toInt();
    return true;
}

//-----------------------------------------------------------------------------
FoxFourCameraControl::FoxFourCameraControl(const mavlink_camera_information_t *info, Vehicle* vehicle, int compID, QObject* parent)
    : VehicleCameraControl(info,vehicle, compID, parent)
{
    _cameraMode      = CAM_MODE_VIDEO;
    connect(VideoManager::instance(), &VideoManager::recordingChanged,this,&FoxFourCameraControl::_processRecordingChanged);
    qCDebug(CameraControlLog)<< "FoxFour camera control initialized";
    _info.flags |= CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM;
}

//-----------------------------------------------------------------------------
FoxFourCameraControl::~FoxFourCameraControl()
{
    // Stop all timers to prevent them from firing during or after destruction
    _captureStatusTimer.stop();
    _videoRecordTimeUpdateTimer.stop();
    _streamInfoTimer.stop();
    _streamStatusTimer.stop();
    _cameraSettingsTimer.stop();
    _storageInfoTimer.stop();

    delete _netManager;
    _netManager = nullptr;
}

//-----------------------------------------------------------------------------
bool
FoxFourCameraControl::startVideoRecording()
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "startVideoRecording()";
        //-- Check if camera can capture videos or if it can capture it while in Photo Mode
        if((cameraMode() == CAM_MODE_PHOTO && !videoInPhotoMode())) {
            return false;
        } else if ( !capturesVideo()) {
            // If camera can't record video, just do the normal ground station recording here
            VideoManager::instance()->startRecording();

            if(videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
                qCWarning(CameraControlLog) << "startVideoRecording: Camera already recording";
                return false;
            }

            _videoRecordTimeUpdateTimer.start();
            _videoRecordTimeElapsedTimer.start();
            VideoManager::instance()->startRecording();
            _setVideoStatus(VIDEO_CAPTURE_STATUS_RUNNING);
            return true;
        }

        if(videoCaptureStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void
FoxFourCameraControl::handleSettings(const mavlink_camera_settings_t& settings)
{
    qCDebug(CameraControlLog) << "Received CAMERA_SETTINGS Mode:" << settings.mode_id << "- stopping timer, resetting retries";
    _cameraSettingsTimer.stop();
    _cameraSettingsRetries = 0;
    // FIXME: for now just hardcoding to support older VGM firmware with mode_id hardcoded to photo in mavsdk
    int mode_id = 1;
    _setCameraMode(static_cast<CameraMode>(mode_id));

    // qreal z = static_cast<qreal>(settings.zoomLevel);
    qreal f = static_cast<qreal>(settings.focusLevel);
    // if(std::isfinite(z) && z != _zoomLevel) {
    //     _zoomLevel = z;
    //     emit zoomLevelChanged();
    // }
    if(std::isfinite(f) && f != _focusLevel) {
        _focusLevel = f;
        emit focusLevelChanged();
    }
}

//-----------------------------------------------------------------------------
void
FoxFourCameraControl::_processRecordingChanged()
{
    bool isRecording = VideoManager::instance()->recording();
    if(!isRecording &&
       !capturesVideo() &&
        _videoCaptureStatus == VIDEO_CAPTURE_STATUS_RUNNING) {
        _videoRecordTimeUpdateTimer.stop();
        _setVideoStatus(VIDEO_CAPTURE_STATUS_STOPPED);
    }
}

//-----------------------------------------------------------------------------
bool
FoxFourCameraControl::stopVideoRecording()
{
    if(!_resetting) {
        if (!capturesVideo()) {
            // Again, if camera doesn't have the recording, do one on the ground station
            if(videoCaptureStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
                qCWarning(CameraControlLog) << "stopVideoRecording: Camera not recording";
                return false;
            }

            _videoRecordTimeUpdateTimer.stop();
            VideoManager::instance()->stopRecording();
            _setVideoStatus(VIDEO_CAPTURE_STATUS_STOPPED);
            return true;
        }

        // Firstly, stop video recording on the UAV
        qCDebug(CameraControlLog) << "stopVideoRecording()";
        if(videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
            _vehicle->sendMavCommand(
                _compID,                                    // Target component
                MAV_CMD_VIDEO_STOP_CAPTURE,                 // Command id
                false,                                      // Don't Show Error (handle locally)
                0);                                         // Reserved (Set to 0)

            VideoManager::instance()->stopRecording();

            return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
void FoxFourCameraControl::startTracking(QRectF rec, QString timestamp, bool zoom)
{
    uint64_t time = timestamp.toULongLong();
    qDebug()<<"starting tracking";
    if(_trackingMarquee != rec) {
        _trackingMarquee = rec;

        qCDebug(CameraControlLog) << "Start Tracking (Rectangle: ["
                                  << static_cast<float>(rec.x()) << ", "
                                  << static_cast<float>(rec.y()) << "] - ["
                                  << static_cast<float>(rec.x() + rec.width()) << ", "
                                  << static_cast<float>(rec.y() + rec.height()) << "]"
                                  << ", Timestamp: "<< timestamp;

        if(zoom){
            qDebug()<<"zooming";
            time = time | (1ULL<<63);
        }
        // qDebug()<<time;
        uint32_t timestampLow = static_cast<uint32_t>(time);
        uint32_t timestampHight = static_cast<uint32_t>(time >> 32);

        float param5,param6;

        std::memcpy(&param5, &timestampLow, sizeof(param5));
        std::memcpy(&param6, &timestampHight, sizeof(param6));

        _vehicle->sendMavCommand(_compID,
                                 MAV_CMD_CAMERA_TRACK_RECTANGLE,
                                 true,
                                 static_cast<float>(rec.x()),
                                 static_cast<float>(rec.y()),
                                 static_cast<float>(rec.x() + rec.width()),
                                 static_cast<float>(rec.y() + rec.height()),
                                 param5,
                                 param6);

        // Request tracking status
        _requestTrackingStatus();
    }
}

//-----------------------------------------------------------------------------
void FoxFourCameraControl::stopTracking(uint64_t timestamp)
{
    qCDebug(CameraControlLog) << "Stop Tracking";

    uint32_t timestampLow = static_cast<uint32_t>(timestamp);
    uint32_t timestampHight = static_cast<uint32_t>(timestamp >> 32);

    float param1,param2;

    std::memcpy(&param1, &timestampLow, sizeof(param1));
    std::memcpy(&param2, &timestampLow, sizeof(param2));


    //-- Stop Tracking
    _vehicle->sendMavCommand(_compID,
                             MAV_CMD_CAMERA_STOP_TRACKING,
                             true);

    //-- Stop Sending Tracking Status
    _vehicle->sendMavCommand(_compID,
                             MAV_CMD_SET_MESSAGE_INTERVAL,
                             true,
                             MAVLINK_MSG_ID_CAMERA_TRACKING_IMAGE_STATUS,
                             -1);

    // reset tracking image rectangle
    _trackingImageRect = {};
}

void FoxFourCameraControl::setZoomLevel(qreal level)
{
    VehicleCameraControl::setZoomLevel(level);
    _zoomLevel = level;
    if((_zoomLevel > 1) != _zoomEnabled){
        _zoomEnabled = !_zoomEnabled;
        emit zoomEnabledChanged();
    }
    emit zoomLevelChanged();
}
