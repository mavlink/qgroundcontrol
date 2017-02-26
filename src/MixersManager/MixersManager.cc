/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "MixersManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(MixersManagerLog, "MixersManagerLog")

MixersManager::MixersManager(Vehicle* vehicle)
    : _vehicle(vehicle)
    , _dedicatedLink(NULL)
    , _ackTimeoutTimer(NULL)
    , _expectedAck(AckNone)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MixersManager::_mavlinkMessageReceived);
    
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);
    _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
    
    connect(_ackTimeoutTimer, &QTimer::timeout, this, &MixersManager::_ackTimeout);
}

MixersManager::~MixersManager()
{

}


void MixersManager::_ackTimeout(void)
{
}

//void MissionManager::_startAckTimeout(AckType_t ack)
//{
//    _expectedAck = ack;
//    _ackTimeoutTimer->start();
//}

///// Checks the received ack against the expected ack. If they match the ack timeout timer will be stopped.
///// @return true: received ack matches expected ack
//bool MixersManager::_checkForExpectedAck(AckType_t receivedAck)
//{
//    if (receivedAck == _expectedAck) {
//        _expectedAck = AckNone;
//        _ackTimeoutTimer->stop();
//        return true;
//    } else {
//        if (_expectedAck == AckNone) {
//            // Don't worry about unexpected mission commands, just ignore them; ArduPilot updates home position using
//            // spurious MISSION_ITEMs.
//        } else {
//            // We just warn in this case, this could be crap left over from a previous transaction or the vehicle going bonkers.
//            // Whatever it is we let the ack timeout handle any error output to the user.
//            qCDebug(MissionManagerLog) << QString("Out of sequence ack expected:received %1:%2").arg(_ackTypeToString(_expectedAck)).arg(_ackTypeToString(receivedAck));
//        }
//        return false;
//    }
//}

//void MissionManager::_readTransactionComplete(void)
//{
//    qCDebug(MissionManagerLog) << "_readTransactionComplete read sequence complete";
    
//    mavlink_message_t       message;
//    mavlink_mission_ack_t   missionAck;
    
//    missionAck.target_system =      _vehicle->id();
//    missionAck.target_component =   MAV_COMP_ID_MISSIONPLANNER;
//    missionAck.type =               MAV_MISSION_ACCEPTED;
    
//    mavlink_msg_mission_ack_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
//                                        qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
//                                        _dedicatedLink->mavlinkChannel(),
//                                        &message,
//                                        &missionAck);
    
//    _vehicle->sendMessageOnLink(_dedicatedLink, message);

//    _finishTransaction(true);
//    emit newMissionItemsAvailable();
//}



/// Called when a new mavlink message for out vehicle is received
void MixersManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_MISSION_COUNT:
            break;

//        case MAVLINK_MSG_ID_MISSION_ITEM:
//            _handleMissionItem(message);
//            break;
            
//        case MAVLINK_MSG_ID_MISSION_REQUEST:
//            _handleMissionRequest(message);
//            break;
            
//        case MAVLINK_MSG_ID_MISSION_ACK:
//            _handleMissionAck(message);
//            break;
            
//        case MAVLINK_MSG_ID_MISSION_ITEM_REACHED:
//            // FIXME: NYI
//            break;
            
//        case MAVLINK_MSG_ID_MISSION_CURRENT:
//            _handleMissionCurrent(message);
//            break;
    }
}
