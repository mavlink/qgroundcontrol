/****************************************************************************
 *
 * Copyright (c) 2017, Intel Corporation
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC MAVLink video streaming Manager
 */

#ifndef MAVLINK_VIDEO_MANAGER_H
#define MAVLINK_VIDEO_MANAGER_H

#include <QObject>
#include <QStringList>

#include "MAVLinkProtocol.h"

class MAVLinkVideoManager : public QObject
{
    Q_OBJECT
public:
    MAVLinkVideoManager();
    ~MAVLinkVideoManager();

private slots:
    void _mavlinkMessageReceived(LinkInterface *link, mavlink_message_t message);
    void _videoHeartbeatInfo(LinkInterface *link, int systemId);

private:
    int _cameraId;
    MAVLinkProtocol *_mavlink;
    LinkInterface *_cameraLink;
};

#endif // MAVLINK_VIDEO_MANAGER_H
