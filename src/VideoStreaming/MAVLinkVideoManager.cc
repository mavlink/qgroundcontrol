/****************************************************************************
 *
 * Copyright (c) 2017, Intel Corporation
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QDebug>

#include "MAVLinkVideoManager.h"
#include "LinkInterface.h"
#include "QGCApplication.h"

/**
 * @file
 *   @brief QGC MAVLink video streaming Manager
 */


//-----------------------------------------------------------------------------
MAVLinkVideoManager::MAVLinkVideoManager()
    : QObject()
    , _cameraId(0)
    , _cameraLink(NULL)

{
    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    connect(_mavlink, &MAVLinkProtocol::videoHeartbeatInfo, this, &MAVLinkVideoManager::_videoHeartbeatInfo);
    connect(_mavlink, &MAVLinkProtocol::messageReceived, this, &MAVLinkVideoManager::_mavlinkMessageReceived);
}

//-----------------------------------------------------------------------------
MAVLinkVideoManager::~MAVLinkVideoManager()
{
}

//-----------------------------------------------------------------------------
void
MAVLinkVideoManager::_videoHeartbeatInfo(LinkInterface *link, int systemId)
{
    if (systemId == _cameraId)
        return;

    qDebug() << "MAVLinkVideoManager: First camera heartbeat info received";
    _cameraId = systemId;
    _cameraLink = link;

    //TODO: Send message requesting cameras (MAV_CMD_REQUEST_CAMERA_INFORMATION)
}

//-----------------------------------------------------------------------------
void
MAVLinkVideoManager::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    if (message.sysid != _cameraId)
        return;

    qDebug() << "Message received";

    //TODO: Handle received messages like CAMERA_INFORMATION and VIDEO_STREAM_INFORMATION
}
