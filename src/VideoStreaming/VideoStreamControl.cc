/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QDebug>

#include "LinkInterface.h"
#include "QGCApplication.h"
#include "VideoStreamControl.h"

QGC_LOGGING_CATEGORY(VideoStreamControlLog, "VideoStreamControlLog")

VideoStreamControl::VideoStreamControl()
    : QObject()
    , _systemId(-1)
    , _linkInterface(NULL)
    , _videoStreamSetupTimeout(0)
    , _checkResolutionTimeout(0)
    , _cameraServiceUid(0)
    , _cameraCount(0)
    , _cameraIdInUse(0)
    , _settingInProgress(false)
{
    _mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
    connect(_mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &VideoStreamControl::_mavlinkMessageReceived);
    _videoSettings = qgcApp()->toolbox()->settingsManager()->videoSettings();
    _cameraIdSetting = _videoSettings->cameraId()->rawValue().toUInt();
    connect(_videoSettings->cameraId(),   &Fact::rawValueChanged, this, &VideoStreamControl::_cameraIdChanged);
    connect(_videoSettings->videoShareEnable(),   &Fact::rawValueChanged, this, &VideoStreamControl::_videoShareChanged);
    connect(&_connectionLostTimer, &QTimer::timeout, this, &VideoStreamControl::_connectionLostTimeout);
    connect(&_settingInProgressTimer, &QTimer::timeout, this, &VideoStreamControl::_settingInProgressTimeout);
}

VideoStreamControl::~VideoStreamControl()
{
}

QString VideoStreamControl::videoStreamUrl()
{
    if (_videoSettings->videoShareEnable()->rawValue().toBool()) {
        // hard-code the url to proxy server
        return "rtsp://127.0.0.1:8554/fpv_stream";
    }
    return _videoStreamUrl;
}

QString VideoStreamControl::videoResolution()
{
    return _videoResolution;
}

uint VideoStreamControl::cameraCount()
{
    return _cameraCount;
}

bool VideoStreamControl::settingInProgress()
{
    return _settingInProgress;
}

void VideoStreamControl::fhdEnabledChanged(bool fhdEnabled)
{
    _setFhdEnabledLockUi(fhdEnabled, true);
}

void VideoStreamControl::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    if(message.msgid == MAVLINK_MSG_ID_HEARTBEAT && message.compid == MAV_COMP_ID_CAMERA) {
        _handleHeartbeatInfo(link, message);
        return;
    } else if (message.sysid != _systemId || message.compid != MAV_COMP_ID_CAMERA) {
        return;
    }

    qCDebug(VideoStreamControlLog) << "Camera message received" << message.msgid;
    switch(message.msgid) {
        case MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION: {
            _handleVideoStreamInfo(message);
            break;
        }
        case MAVLINK_MSG_ID_COMMAND_ACK: {
            mavlink_command_ack_t ack;
            mavlink_msg_command_ack_decode(&message, &ack);
            if (ack.command == MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION) {
                qCDebug(VideoStreamControlLog) << "ACK for REQUEST_VIDEO_STREAM_INFORMATION received" << ack.result;
            } else if (ack.command == MAV_CMD_VIDEO_START_STREAMING) {
                qCDebug(VideoStreamControlLog) << "ACK for VIDEO_START_STREAMING received" << ack.result;
            }
            break;
        }
    }
}

void VideoStreamControl::_connectionLostTimeout(void)
{
    if (_connectionLostTimer.isActive()){
        qCDebug(VideoStreamControlLog) << "Camera heartbeat lost, but do nothing";
    }
}

void VideoStreamControl::_settingInProgressTimeout()
{
    qCDebug(VideoStreamControlLog) << "Time out to setting camera, unlock UI!";
    _setSettingInProgress(false);
}

void VideoStreamControl::_cameraIdChanged()
{
    _setCameraIdLockUi(true);
}

void VideoStreamControl::_videoShareChanged()
{
    emit videoStreamUrlChanged();
}

void VideoStreamControl::_handleHeartbeatInfo(LinkInterface* link, mavlink_message_t& message)
{
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&message, &heartbeat);

    if (message.sysid == _systemId) {
        if (heartbeat.custom_mode == _cameraServiceUid) {
            _connectionActive();
            _checkResolution();
            _checkCameraId();
            return;
        } else {
            // customMode is a uid, the change means remote peer reset
            // need to restart the video streaming
            qCDebug(VideoStreamControlLog) << "remote peer reset";
            _resetVideoStreamInfo();
            emit videoStreamUrlChanged();
        }
    }

    qCDebug(VideoStreamControlLog) << "First camera heartbeat:" << message.sysid << heartbeat.system_status << heartbeat.custom_mode;

    _systemId = message.sysid;
    _cameraServiceUid = heartbeat.custom_mode;
     // customMode 32bits: bits 25-31: camera count, bits 16-24: timestamp, bits 0-15 remote peer pid
    _cameraCount = _cameraServiceUid >> 24;
    emit cameraCountChanged();
    qCDebug(VideoStreamControlLog) << "Camera found uid:" << _cameraServiceUid << "count:" << _cameraCount;

    _linkInterface = link;

    _videoStreamSetupTimeout = 0;
    _setupVideoStream();
}

void VideoStreamControl::_handleVideoStreamInfo(mavlink_message_t& message)
{
    mavlink_video_stream_information_t streamInfo;
    mavlink_msg_video_stream_information_decode(&message, &streamInfo);

    qCDebug(VideoStreamControlLog) << "Video stream URL:"  << streamInfo.uri;

    bool uriNotChange = false;
    if (_videoStreamUrl.compare(streamInfo.uri) == 0) {
        uriNotChange = true;
        qCDebug(VideoStreamControlLog) << "URL is not change";
    } else {
        _videoStreamUrl = streamInfo.uri;
    }

    QString newRes = QString::number(streamInfo.resolution_h) + "x" + QString::number(streamInfo.resolution_v);
    if (_videoResolution != newRes) {
        _videoResolution = newRes;
        emit videoResolutionChanged();
    }
    qCDebug(VideoStreamControlLog) << "Video stream resolution:" << _videoResolution;
    _cameraIdInUse = streamInfo.stream_id;
    qCDebug(VideoStreamControlLog) << "Camera id in use:" <<  _cameraIdInUse;

    if (!uriNotChange) {
        //this will trigger video receiver to setup video streaming
        emit videoStreamUrlChanged();
    }

    //Till now, video connection setup is ready, then begin to
    //monitor the connection lost after 10 seconds of missed heartbeat
    _connectionLostTimer.setInterval(10000);
    _connectionLostTimer.setSingleShot(false);
    _connectionLostTimer.start();

    //Video streaming info receivedmeans previous setting is done
    _setSettingInProgress(false);
}

void VideoStreamControl::_setupVideoStream()
{
    _setCameraId();
    uint res = _videoSettings->videoResolution()->rawValue().toUInt();
    if (res != 0) {
        if(res == 2) {
            _setVideoResolution(1920, 1080);
        } else {
            _setVideoResolution(1280, 720);
        }
    }
    _requestVideoStreamInfo();
}

void VideoStreamControl::_resetVideoStreamInfo()
{
    _systemId = 0;
    _videoStreamUrl = "";
    _videoResolution = "";
    _connectionLostTimer.stop();
}

void VideoStreamControl::_requestVideoStreamInfo()
{
    if (_linkInterface == NULL) {
        return;
    }

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    mavlink_message_t msg;

    mavlink_msg_command_long_pack(_mavlinkProtocol->getSystemId(), _mavlinkProtocol->getComponentId(), &msg, _systemId, MAV_COMP_ID_CAMERA, MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION, 0, 0, 0, 0, 0, 0, 0, 0);

    int len = mavlink_msg_to_send_buffer(buffer, &msg);
    _linkInterface->writeBytesSafe((const char*)buffer, len);
    qCDebug(VideoStreamControlLog) << "Request video stream info is sent";
}

void VideoStreamControl::_checkCameraId()
{
    if((_cameraCount > 1) && (_cameraIdSetting != _cameraIdInUse)) {
        _setCameraIdLockUi(false);
    }
}

void VideoStreamControl::_setCameraId()
{
    if(_cameraCount > 1) {
        _cameraIdSetting = _videoSettings->cameraId()->rawValue().toUInt();
        _startVideoStreaming();
    }
}

void VideoStreamControl::_setCameraIdLockUi(bool lockUi)
{
    if (_linkInterface == NULL) {
        return;
    }
    _cameraIdSetting = _videoSettings->cameraId()->rawValue().toUInt();

    _setCameraId();

    //Request video stream info to make sure setting is done
    _requestVideoStreamInfo();
    if (lockUi) {
        _setSettingInProgress(true);
    }
}

void VideoStreamControl::_startVideoStreaming() {
    if (_linkInterface == NULL) {
        return;
    }
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(_mavlinkProtocol->getSystemId(), _mavlinkProtocol->getComponentId(), &msg,
                                      _systemId, MAV_COMP_ID_CAMERA,
                                      MAV_CMD_VIDEO_START_STREAMING, 0, _cameraIdSetting, 0, 0, 0, 0, 0, 0);
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buffer, &msg);

    _linkInterface->writeBytesSafe((const char*)buffer, len);
    _requestVideoStreamInfo();
}

void VideoStreamControl::_checkResolution()
{
    if (_videoResolution.isEmpty()) {
            return;
    }
    if((_videoSettings->videoResolution()->rawValue().toUInt() == 2)
        && _videoResolution.compare("1920x1080") != 0) {
        qCDebug(VideoStreamControlLog) << "fhd set but res is" << _videoResolution;
        if (_checkResolutionTimeout++ > 3) {
            qCDebug(VideoStreamControlLog) << "resolution check failed, resend the request";
            _setFhdEnabledLockUi(true, false);
        }
    } else if ((_videoSettings->videoResolution()->rawValue().toUInt() == 1)
        && _videoResolution.compare("1920x1080") == 0) {
        qCDebug(VideoStreamControlLog) << "fhd not set but res is" << _videoResolution;
        if (_checkResolutionTimeout++ > 3) {
            qCDebug(VideoStreamControlLog) << "resolution check failed, resend the request";
            _setFhdEnabledLockUi(false, false);
        }
    }
}

void VideoStreamControl::_setFhdEnabledLockUi(bool fhdEnabled, bool lockUi)
{
    if(_videoSettings->videoResolution()->rawValue().toUInt() == 0) {
        if (fhdEnabled && QString::compare(_videoResolution, "1920x1080") != 0) {
            qCDebug(VideoStreamControlLog) << "fhdEnabled setting is changed to:" << fhdEnabled;
            _videoSettings->videoResolution()->setRawValue(2);
        } else if (!fhdEnabled && QString::compare(_videoResolution, "1920x1080") == 0) {
            qCDebug(VideoStreamControlLog) << "fhdEnabled setting is changed to:" << fhdEnabled;
            _videoSettings->videoResolution()->setRawValue(1);
        }
    } else {
        qCDebug(VideoStreamControlLog) << "fhdEnabled setting is changed to:" << fhdEnabled;
        _videoSettings->videoResolution()->setRawValue(fhdEnabled ? 2 : 1);
    }
    if (_linkInterface == NULL) {
        return;
    }
    if (fhdEnabled) {
         _setVideoResolution(1920, 1080);
    } else {
        _setVideoResolution(1280, 720);
    }

    //Request video stream info to make sure setting is done
    _requestVideoStreamInfo();
    if (lockUi) {
        _setSettingInProgress(true);
    }
}

void VideoStreamControl::_setVideoResolution(int h, int v)
{
    if (_linkInterface == NULL) {
        return;
    }

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    mavlink_message_t msg;
    mavlink_msg_set_video_stream_settings_pack(_mavlinkProtocol->getSystemId(), _mavlinkProtocol->getComponentId(), &msg, _systemId, MAV_COMP_ID_CAMERA, _cameraIdSetting, 0, h, v, 0, 0, "");
    int len = mavlink_msg_to_send_buffer(buffer, &msg);
    _linkInterface->writeBytesSafe((const char*)buffer, len);
     qCDebug(VideoStreamControlLog) << "set stream resolution sent for " << h << "x" << v;
     _checkResolutionTimeout = 0;
}

void VideoStreamControl::_setSettingInProgress(bool inProgress)
{
    _settingInProgress = inProgress;
    emit settingInProgressChanged();
    if (inProgress) {
        _settingInProgressTimer.setInterval(15000);
        _settingInProgressTimer.setSingleShot(true);
        _settingInProgressTimer.start();
        qCDebug(VideoStreamControlLog) << "Setup timer for setting camera, and lock UI";
    } else {
        if (_settingInProgressTimer.isActive()) {
            _settingInProgressTimer.stop();
            qCDebug(VideoStreamControlLog) << "Done for setting camera, unlock UI and clear timer";
        }
    }
    return;
}

void VideoStreamControl::_connectionActive(void)
{
    if (!_connectionLostTimer.isActive()){
        if(_videoStreamSetupTimeout++ > 3) {
            _resetVideoStreamInfo();
            _videoStreamSetupTimeout = 0;
        }
        return;
    }
    _videoStreamSetupTimeout = 0;
    _connectionLostTimer.start();
}
