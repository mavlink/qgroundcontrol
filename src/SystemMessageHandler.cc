/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QDebug>
#include <QDateTime>
#include "MAVLinkProtocol.h"
#include "SystemMessageHandler.h"
#include "QGCApplication.h"

SystemMessageHandler::SystemMessageHandler(MAVLinkProtocol* protocol)
    : QObject()
{
    _mavlinkProtocol  =  protocol;

    connect(protocol, &MAVLinkProtocol::messageReceived, this, &SystemMessageHandler::_receiveMessage);
}

void SystemMessageHandler::_receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    Q_UNUSED(link);

    if(message.compid != MAV_COMP_ID_SYSTEM_CONTROL) {
        return;
    }

    _activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    int _id = _activeVehicle ? _activeVehicle->id() : 0;

    if ((message.sysid != _id && message.sysid != 0) && (message.msgid == MAVLINK_MSG_ID_SCALED_PRESSURE))
    {
        _handleBoardTemperature(message);
    }

    if (message.msgid == MAVLINK_MSG_ID_TIMESYNC)
    {
        _handleTimeSyncMessage(link, message);
    }
}

void SystemMessageHandler::_handleBoardTemperature(mavlink_message_t& message)
{
    mavlink_scaled_pressure_t pressure;
    mavlink_msg_scaled_pressure_decode(&message, &pressure);
    QString temp = QString::number(pressure.temperature / 100.0, 'f', 2);
    emit boardTemputureChanged(temp);
}

void SystemMessageHandler::_handleTimeSyncMessage(LinkInterface* link, mavlink_message_t& message)
{
    if(link == NULL) return;

    mavlink_timesync_t timesync;
    mavlink_msg_timesync_decode(&message, &timesync);

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    mavlink_message_t msg;

    qint64 time = QDateTime::currentSecsSinceEpoch();
    mavlink_msg_timesync_pack(_mavlinkProtocol->getSystemId(), _mavlinkProtocol->getComponentId(), &msg, timesync.tc1, time);

    int len = mavlink_msg_to_send_buffer(buffer, &msg);
    link->writeBytesSafe((const char*)buffer, len);
}
