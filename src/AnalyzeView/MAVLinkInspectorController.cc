/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkInspectorController.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"

//-----------------------------------------------------------------------------
MAVLinkInspectorController::MAVLinkInspectorController()
{
    MultiVehicleManager* multiVehicleManager = qgcApp()->toolbox()->multiVehicleManager();
    connect(multiVehicleManager, &MultiVehicleManager::vehicleAdded,   this, &MAVLinkInspectorController::_vehicleAdded);
    connect(multiVehicleManager, &MultiVehicleManager::vehicleRemoved, this, &MAVLinkInspectorController::_vehicleRemoved);
    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
    connect(mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &MAVLinkInspectorController::_receiveMessage);
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::~MAVLinkInspectorController()
{
    _reset();
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_vehicleAdded(Vehicle* vehicle)
{
    _vehicleIDs.append(vehicle->id());
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_vehicleRemoved(Vehicle* vehicle)
{
    int idx = _vehicleIDs.indexOf(vehicle->id());
    if(idx >= 0) {
        _vehicleIDs.removeAt(idx);
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_receiveMessage(LinkInterface* link,mavlink_message_t message)
{
    Q_UNUSED(link);
    quint64 receiveTime;
    if (_selectedSystemID    != 0 && _selectedSystemID    != message.sysid)  return;
    if (_selectedComponentID != 0 && _selectedComponentID != message.compid) return;
    // Create dynamically an array to store the messages for each UAS
    if (!_uasMessageStorage.contains(message.sysid)) {
        mavlink_message_t* msg = new mavlink_message_t;
        *msg = message;
        _uasMessageStorage.insertMulti(message.sysid,msg);
    }
    bool msgFound = false;
    QMap<int, mavlink_message_t*>::const_iterator iteMsg = _uasMessageStorage.find(message.sysid);
    mavlink_message_t* uasMessage = iteMsg.value();
    while((iteMsg != _uasMessageStorage.end()) && (iteMsg.key() == message.sysid)) {
        if (iteMsg.value()->msgid == message.msgid) {
            msgFound   = true;
            uasMessage = iteMsg.value();
            break;
        }
        ++iteMsg;
    }
    if (!msgFound) {
        mavlink_message_t* msgIdMessage = new mavlink_message_t;
        *msgIdMessage = message;
        _uasMessageStorage.insertMulti(message.sysid,msgIdMessage);
    } else {
        *uasMessage = message;
    }
    // Looking if this message has already been received once
    msgFound = false;
    QMap<int, QMap<int, quint64>* >::const_iterator ite = _uasLastMessageUpdate.find(message.sysid);
    QMap<int, quint64>* lastMsgUpdate = ite.value();
    while((ite != _uasLastMessageUpdate.end()) && (ite.key() == message.sysid)) {
        if(ite.value()->contains(message.msgid)) {
            msgFound = true;
            //-- Point to the found message
            lastMsgUpdate = ite.value();
            break;
        }
        ++ite;
    }
    receiveTime = QGC::groundTimeMilliseconds();
    //-- If the message doesn't exist, create a map for the frequency, message count and time of reception
    if(!msgFound) {
        //-- Create a map for the message frequency
        QMap<int, float>* messageHz = new QMap<int,float>;
        messageHz->insert(message.msgid,0.0f);
        _uasMessageHz.insertMulti(message.sysid,messageHz);
        //-- Create a map for the message count
        QMap<int, unsigned int>* messagesCount = new QMap<int, unsigned int>;
        messagesCount->insert(message.msgid,0);
        _uasMessageCount.insertMulti(message.sysid,messagesCount);
        //-- Create a map for the time of reception of the message
        QMap<int, quint64>* lastMessage = new QMap<int, quint64>;
        lastMessage->insert(message.msgid,receiveTime);
        _uasLastMessageUpdate.insertMulti(message.sysid,lastMessage);
        //-- Point to the created message
        lastMsgUpdate = lastMessage;
    } else {
        //-- The message has been found/created
        if((lastMsgUpdate->contains(message.msgid)) && (_uasMessageCount.contains(message.sysid))) {
            //-- Looking for and updating the message count
            unsigned int count = 0;
            QMap<int, QMap<int, unsigned int>* >::const_iterator iter = _uasMessageCount.find(message.sysid);
            QMap<int, unsigned int> * uasMsgCount = iter.value();
            while((iter != _uasMessageCount.end()) && (iter.key() == message.sysid)) {
                if(iter.value()->contains(message.msgid)) {
                    uasMsgCount = iter.value();
                    count = uasMsgCount->value(message.msgid,0);
                    uasMsgCount->insert(message.msgid,count+1);
                    break;
                }
                ++iter;
            }
        }
        lastMsgUpdate->insert(message.msgid,receiveTime);
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_reset()
{
    QMap<int, mavlink_message_t* >::iterator ite;
    for(ite = _uasMessageStorage.begin(); ite != _uasMessageStorage.end(); ++ite) {
        delete ite.value();
        ite.value() = nullptr;
    }
    _uasMessageStorage.clear();
    QMap<int, QMap<int, float>* >::iterator iteHz;
    for(iteHz = _uasMessageHz.begin(); iteHz != _uasMessageHz.end(); ++iteHz) {
        iteHz.value()->clear();
        delete iteHz.value();
        iteHz.value() = nullptr;
    }
    _uasMessageHz.clear();
    QMap<int, QMap<int, unsigned int>*>::iterator iteCount;
    for(iteCount = _uasMessageCount.begin(); iteCount != _uasMessageCount.end(); ++iteCount) {
        iteCount.value()->clear();
        delete iteCount.value();
        iteCount.value() = nullptr;
    }
    _uasMessageCount.clear();
    QMap<int, QMap<int, quint64>* >::iterator iteLast;
    for(iteLast = _uasLastMessageUpdate.begin(); iteLast != _uasLastMessageUpdate.end(); ++iteLast) {
        iteLast.value()->clear();
        delete iteLast.value();
        iteLast.value() = nullptr;
    }
    _uasLastMessageUpdate.clear();
}
