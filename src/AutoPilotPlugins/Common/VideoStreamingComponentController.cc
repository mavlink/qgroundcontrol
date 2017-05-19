/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "VideoStreamingComponentController.h"
#include "QGCMAVLink.h"
#include "QGCApplication.h"

const char* VideoStreamingFactGroup::_videoStatusFactName =          "videoStatus";
const char* VideoStreamingFactGroup::_resolutionFactName =           "resolution";
const char* VideoStreamingFactGroup::_resolutionHorizontalFactName = "resolutionHorizontal";
const char* VideoStreamingFactGroup::_resolutionVerticalFactName =   "resolutionVertical";
const char* VideoStreamingFactGroup::_bitRateFactName =              "bitRate";
const char* VideoStreamingFactGroup::_fpsFactName =                  "fps";
const char* VideoStreamingFactGroup::_rotationFactName =             "rotation";
const char* VideoStreamingFactGroup::_uriFactName =                  "uri";
const char* VideoStreamingFactGroup::_protocolFactName =             "protocol";

VideoStreamingFactGroup::VideoStreamingFactGroup(QString fileName, QObject* parent)
    : FactGroup(1000, fileName, parent)
    , _videoStatusFact          (0, _videoStatusFactName,          FactMetaData::valueTypeUint8)
    , _resolutionFact           (0, _resolutionFactName,           FactMetaData::valueTypeUint8)
    , _resolutionHorizontalFact (0, _resolutionHorizontalFactName, FactMetaData::valueTypeUint16)
    , _resolutionVerticalFact   (0, _resolutionVerticalFactName,   FactMetaData::valueTypeUint16)
    , _bitRateFact              (0, _bitRateFactName,              FactMetaData::valueTypeUint32)
    , _fpsFact                  (0, _fpsFactName,                  FactMetaData::valueTypeFloat)
    , _rotationFact             (0, _rotationFactName,             FactMetaData::valueTypeUint16)
    , _uriFact                  (0, _uriFactName,                  FactMetaData::valueTypeString)
    , _protocolFact             (0, _protocolFactName,             FactMetaData::valueTypeUint8)
{
    _addFact(&_videoStatusFact,          _videoStatusFactName);
    _addFact(&_resolutionFact,           _resolutionFactName);
    _addFact(&_resolutionHorizontalFact, _resolutionHorizontalFactName);
    _addFact(&_resolutionVerticalFact,   _resolutionVerticalFactName);
    _addFact(&_bitRateFact,              _bitRateFactName);
    _addFact(&_fpsFact,                  _fpsFactName);
    _addFact(&_rotationFact,             _rotationFactName);
    _addFact(&_uriFact,                  _uriFactName);
    _addFact(&_protocolFact,             _protocolFactName);
}

VideoStreamingComponentController::VideoStreamingComponentController(void)
    : _mavlink(NULL)
    , _resolutionMetaData (FactMetaData::valueTypeUint8)
    , _bitRateMetaData    (FactMetaData::valueTypeUint32)
    , _fpsMetaData        (FactMetaData::valueTypeFloat)
    , _rotationMetaData   (FactMetaData::valueTypeUint16)
{
    for (int i=0; i<_vehicle->cameraCount(); i++)
    {
        QString fileName;
        CameraInformationFactGroup *cameraInfo = _vehicle->_cameras[i];
        _cameraNames << cameraInfo->vendorNameFact()->rawValueString() + " " + cameraInfo->modelNameFact()->rawValueString();
        _cameraIds << cameraInfo->cameraIdFact()->rawValue().toInt();
        if (cameraInfo->fileNameFact()->rawValueString() == "0") {
            qWarning() << "File empty";
            fileName = ":/json/Vehicle/VideoStreamingFact.json";
        } else {
            fileName = cameraInfo->fileNameFact()->rawValueString();
        }
        VideoStreamingFactGroup* camera = new VideoStreamingFactGroup(fileName);
        _cameras.append(QVariant::fromValue(camera));
    }

    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    connect(_vehicle, &Vehicle::mavlinkVideoStreamInformation, this, &VideoStreamingComponentController::_handleVideoStreamInformation);

    // Request settings of video stream for all available cameras
    for (int i=0; i<_vehicle->cameraCount(); i++)
    {
        _vehicle->sendMavCommand(_mavlink->getComponentId(),
                                 MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION,
                                 false,
                                 _vehicle->_cameras[i]->cameraIdFact()->rawValue().toInt(),
                                 1.0f); //Request
    }
}

void VideoStreamingComponentController::startVideo(int index)
{
    int cameraId = _cameraIds[index];
    _vehicle->sendMavCommand(_mavlink->getComponentId(), MAV_CMD_VIDEO_START_STREAMING, false, cameraId);
}

void VideoStreamingComponentController::stopVideo(int index)
{
    int cameraId = _cameraIds[index];
    _vehicle->sendMavCommand(_mavlink->getComponentId(), MAV_CMD_VIDEO_STOP_STREAMING, false, cameraId);
}

void VideoStreamingComponentController::saveSettings(int index)
{
    mavlink_message_t msg;
    VideoStreamingFactGroup *camera = qvariant_cast<VideoStreamingFactGroup*>(_cameras[index]);
    int cameraId = _cameraIds[index];

    // Get horizontal and vertical resolution
    QStringList resolutions = camera->resolutionFact()->enumStringValue().split("x");
    if (resolutions.count() == 2) {
        camera->resolutionHorizontalFact()->setRawValue(resolutions[0].toInt());
        camera->resolutionVerticalFact()->setRawValue(resolutions[1].toInt());
    } else if (resolutions[0].contains("auto")) {
        camera->resolutionHorizontalFact()->setRawValue(0);
        camera->resolutionVerticalFact()->setRawValue(0);
    }

    mavlink_msg_set_video_stream_settings_pack(_mavlink->getSystemId(),
                                               _mavlink->getComponentId(),
                                               &msg, _vehicle->id(), 0,
                                               cameraId,
                                               camera->fpsFact()->rawValue().toFloat(),
                                               camera->resolutionHorizontalFact()->rawValue().toInt(),
                                               camera->resolutionVerticalFact()->rawValue().toInt(),
                                               camera->bitRateFact()->rawValue().toInt(),
                                               camera->rotationFact()->rawValue().toInt(),
                                               camera->uriFact()->rawValue().toByteArray());

    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void VideoStreamingComponentController::_handleVideoStreamInformation(mavlink_message_t message)
{
    mavlink_video_stream_information_t videoStreamInformation;
    mavlink_msg_video_stream_information_decode(&message, &videoStreamInformation);
    int cam_id = videoStreamInformation.camera_id;
    int index = _cameraIds.indexOf(cam_id);

    if (index == -1) {
        qWarning() << "No camera with such id: " << cam_id;
        return;
    }

    VideoStreamingFactGroup *camera = qvariant_cast<VideoStreamingFactGroup*>(_cameras[index]);

    camera->videoStatusFact()->setRawValue(videoStreamInformation.status);
    camera->resolutionHorizontalFact()->setRawValue(videoStreamInformation.resolution_h);
    camera->resolutionVerticalFact()->setRawValue(videoStreamInformation.resolution_v);
    camera->bitRateFact()->setRawValue(videoStreamInformation.bitrate);
    camera->fpsFact()->setRawValue(videoStreamInformation.framerate);
    camera->rotationFact()->setRawValue(videoStreamInformation.rotation);
    camera->uriFact()->setRawValue(videoStreamInformation.uri);

    QString resolution = camera->resolutionHorizontalFact()->rawValueString() + "x" + camera->resolutionVerticalFact()->rawValueString();
    int resolutionIndex = camera->resolutionFact()->enumStrings().indexOf(resolution);
    camera->resolutionFact()->setRawValue(resolutionIndex);
}
