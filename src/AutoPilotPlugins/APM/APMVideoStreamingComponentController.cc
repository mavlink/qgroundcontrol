/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMVideoStreamingComponentController.h"
#include "QGCMAVLink.h"
#include "QGCApplication.h"

APMVideoStreamingComponentController::APMVideoStreamingComponentController(void)
    : _mavlink(NULL)
    , _targetIp("None")
    , _targetPort(0)
    , _videoStatusFact          (0, "videoStatus",          FactMetaData::valueTypeUint8)
    , _resolutionFact           (0, "resolution",           FactMetaData::valueTypeUint8)
    , _resolutionHorizontalFact (0, "resolutionHorizontal", FactMetaData::valueTypeUint16)
    , _resolutionVerticalFact   (0, "resolutionVertical",   FactMetaData::valueTypeUint16)
    , _bitRateFact              (0, "bitRate",              FactMetaData::valueTypeUint32)
    , _fpsFact                  (0, "fps",                  FactMetaData::valueTypeFloat)
    , _rotationFact             (0, "rotation",             FactMetaData::valueTypeUint16)
    , _resolutionMetaData (FactMetaData::valueTypeUint8)
    , _bitRateMetaData    (FactMetaData::valueTypeUint32)
    , _fpsMetaData        (FactMetaData::valueTypeFloat)
    , _rotationMetaData   (FactMetaData::valueTypeUint16)
{
    QStringList  bitRateStrings, fpsStrings, rotationStrings;
    QVariantList resolutionValues, bitRateValues, fpsValues, rotationValues;

    _resolutionList << "640x480" << "720x576" << "1280x720";
    resolutionValues << 0 << 1 << 2;
    bitRateStrings << "Auto" << "10 Mb" << "15 Mb" << "25 Mb";
    bitRateValues << 0 << 10000000 << 15000000 << 25000000;
    fpsStrings << "Auto" << "15 Hz" << "30 Hz" << "60 Hz" << "90 Hz";
    fpsValues << 0.0 << 15.0 << 30.0 << 60.0 << 90.0;
    rotationStrings << "0°" << "90°" << "180°" << "270°";
    rotationValues << 0 << 90 << 180 << 270;

    _resolutionMetaData.setEnumInfo(_resolutionList, resolutionValues);
    _resolutionFact.setMetaData(&_resolutionMetaData);

    _bitRateMetaData.setEnumInfo(bitRateStrings, bitRateValues);
    _bitRateFact.setMetaData(&_bitRateMetaData);

    _fpsMetaData.setEnumInfo(fpsStrings, fpsValues);
    _fpsFact.setMetaData(&_fpsMetaData);

    _rotationMetaData.setEnumInfo(rotationStrings, rotationValues);
    _rotationFact.setMetaData(&_rotationMetaData);

    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    connect(_vehicle, &Vehicle::mavlinkVideoStreamTarget,   this, &APMVideoStreamingComponentController::_handleVideoStreamTarget);
    connect(_vehicle, &Vehicle::mavlinkCameraCaptureStatus, this, &APMVideoStreamingComponentController::_handleCameraCaptureStatus);
    // Request address of video stream target
    _vehicle->sendMavCommand(_mavlink->getComponentId(),
                             MAV_CMD_REQUEST_VIDEO_STREAM_TARGET,
                             true,
                             1.0f, // Camera ID
                             1.0f); //Request
    // Request state of camera and settings
    _vehicle->sendMavCommand(_mavlink->getComponentId(),
                             MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS,
                             true,
                             1.0f, // Camera ID
                             1.0f); // Request
}

APMVideoStreamingComponentController::~APMVideoStreamingComponentController()
{

}

void APMVideoStreamingComponentController::startVideo()
{
    _getResolution();

    //enable video streaming
    _vehicle->sendMavCommand(_mavlink->getComponentId(),
                             MAV_CMD_DO_CONTROL_VIDEO,
                             true,
                             1.0f, // Camera ID
                             1.0f, // Compressed
                             0.0f, // Video stream
                             0.0f); // Recording disabled
    //start capture
    _vehicle->sendMavCommand(_mavlink->getComponentId(),
                             MAV_CMD_VIDEO_START_CAPTURE,
                             true,
                             1.0f, // Camera ID
                             _fpsFact.rawValue().toFloat(),
                             0.0f, // Resolution in megapixels
                             _resolutionHorizontalFact.rawValue().toFloat(),
                             _resolutionVerticalFact.rawValue().toFloat(),
                             _bitRateFact.rawValue().toFloat(),
                             _rotationFact.rawValue().toFloat());
}

void APMVideoStreamingComponentController::stopVideo()
{
    _vehicle->sendMavCommand(_mavlink->getComponentId(), MAV_CMD_VIDEO_STOP_CAPTURE, true, 1.0f);
}

void APMVideoStreamingComponentController::saveAddress(const QString ip, const int port)
{
    mavlink_message_t msg;
    const char *mav_ip;

    _targetIp = ip;
    _targetPort = port;
    mav_ip = ip.toStdString().c_str();

    mavlink_msg_set_video_stream_target_pack(_mavlink->getSystemId(),
                                             _mavlink->getComponentId(),
                                             &msg, 1, 0, 1, mav_ip, port);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void APMVideoStreamingComponentController::_getResolution()
{
    int index;
    QStringList resolutions;

    index = _resolutionFact.rawValue().toInt();
    resolutions = _resolutionList[index].split("x");

    _resolutionHorizontalFact.setRawValue(resolutions[0].toInt());
    _resolutionVerticalFact.setRawValue(resolutions[1].toInt());
}

void APMVideoStreamingComponentController::_handleCameraCaptureStatus(mavlink_message_t message)
{
    QString resolution;
    int index=0;
    mavlink_camera_capture_status_t cameraCaptureStatus;
    mavlink_msg_camera_capture_status_decode(&message, &cameraCaptureStatus);

    _resolutionHorizontalFact.setRawValue(cameraCaptureStatus.video_resolution_h);
    _resolutionVerticalFact.setRawValue(cameraCaptureStatus.video_resolution_v);
    resolution = _resolutionHorizontalFact.rawValue().toString() + "x" + _resolutionVerticalFact.rawValue().toString();
    index = _resolutionList.indexOf(resolution);

    _videoStatusFact.setRawValue(cameraCaptureStatus.video_status);
    _resolutionFact.setRawValue(index);
    _bitRateFact.setRawValue(cameraCaptureStatus.video_bitrate);
    _fpsFact.setRawValue(cameraCaptureStatus.video_framerate);
    _rotationFact.setRawValue(cameraCaptureStatus.video_rotation);
}

void APMVideoStreamingComponentController::_handleVideoStreamTarget(mavlink_message_t message)
{
    mavlink_video_stream_target_t videoStreamTarget;
    mavlink_msg_video_stream_target_decode(&message, &videoStreamTarget);

    _targetIp = videoStreamTarget.ip;
    _targetPort = videoStreamTarget.port;

    emit addressChanged();
}
