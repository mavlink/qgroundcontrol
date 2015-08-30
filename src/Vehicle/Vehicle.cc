/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "FirmwarePluginManager.h"
#include "LinkManager.h"
#include "FirmwarePlugin.h"
#include "AutoPilotPluginManager.h"

QGC_LOGGING_CATEGORY(VehicleLog, "VehicleLog")

Vehicle::Vehicle(LinkInterface* link, int vehicleId, MAV_AUTOPILOT firmwareType)
    : _id(vehicleId)
    , _firmwareType(firmwareType)
    , _uas(NULL)
{
    _addLink(link);
    
    connect(MAVLinkProtocol::instance(), &MAVLinkProtocol::messageReceived, this, &Vehicle::_mavlinkMessageReceived);
    connect(this, &Vehicle::_sendMessageOnThread, this, &Vehicle::_sendMessage, Qt::QueuedConnection);
    
    _uas = new UAS(MAVLinkProtocol::instance(), this);
    
    setLatitude(_uas->getLatitude());
    setLongitude(_uas->getLongitude());
    _setYaw(_uas->getYaw());
    
    connect(_uas, &UAS::latitudeChanged, this, &Vehicle::setLatitude);
    connect(_uas, &UAS::longitudeChanged, this, &Vehicle::setLongitude);
    connect(_uas, &UAS::yawChanged, this, &Vehicle::_setYaw);
    
    _firmwarePlugin = FirmwarePluginManager::instance()->firmwarePluginForAutopilot(firmwareType);
    _autopilotPlugin = AutoPilotPluginManager::instance()->newAutopilotPluginForVehicle(this);
}

void Vehicle::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    if (message.sysid != _id) {
        return;
    }
    
    if (!_containsLink(link)) {
        _addLink(link);
    }
    
    _uas->receiveMessage(message);
}

bool Vehicle::_containsLink(LinkInterface* link)
{
    foreach (SharedLinkInterface sharedLink, _links) {
        if (sharedLink.data() == link) {
            return true;
        }
    }
    
    return false;
}

void Vehicle::_addLink(LinkInterface* link)
{
    if (!_containsLink(link)) {
        _links += LinkManager::instance()->sharedPointerForLink(link);
        qCDebug(VehicleLog) << "_addLink:" << QString("%1").arg((ulong)link, 0, 16);
        connect(LinkManager::instance(), &LinkManager::linkDisconnected, this, &Vehicle::_linkDisconnected);
    }
}

void Vehicle::_linkDisconnected(LinkInterface* link)
{
    qCDebug(VehicleLog) << "_linkDisconnected:" << link->getName();
    qCDebug(VehicleLog) << "link count:" << _links.count();
    
    for (int i=0; i<_links.count(); i++) {
        if (_links[i].data() == link) {
            _links.removeAt(i);
            break;
        }
    }
    
    if (_links.count() == 0) {
        emit allLinksDisconnected();
    }
}

void Vehicle::sendMessage(mavlink_message_t message)
{
    emit _sendMessageOnThread(message);
}

void Vehicle::_sendMessage(mavlink_message_t message)
{
    // Emit message on all links that are currently connected
    foreach (SharedLinkInterface sharedLink, _links) {
        LinkInterface* link = sharedLink.data();
        Q_ASSERT(link);
        
        if (link->isConnected()) {
            MAVLinkProtocol* mavlink = MAVLinkProtocol::instance();
            
            // Write message into buffer, prepending start sign
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
            int len = mavlink_msg_to_send_buffer(buffer, &message);
            static uint8_t messageKeys[256] = MAVLINK_MESSAGE_CRCS;
            mavlink_finalize_message_chan(&message, mavlink->getSystemId(), mavlink->getComponentId(), link->getMavlinkChannel(), message.len, messageKeys[message.msgid]);
            
            if (link->isConnected()) {
                link->writeBytes((const char*)buffer, len);
            } else {
                qWarning() << "Link not connected";
            }
        }
    }
}

QList<LinkInterface*> Vehicle::links(void)
{
    QList<LinkInterface*> list;
    
    foreach (SharedLinkInterface sharedLink, _links) {
        list += sharedLink.data();
    }
    
    return list;
}

void Vehicle::setLatitude(double latitude)
{
    _geoCoordinate.setLatitude(latitude);
    emit coordinateChanged(_geoCoordinate);
}

void Vehicle::setLongitude(double longitude){
    _geoCoordinate.setLongitude(longitude);
    emit coordinateChanged(_geoCoordinate);
}

void Vehicle::_setYaw(double yaw)
{
    _heading = yaw * (180.0 / M_PI);
    emit headingChanged(_heading);
}