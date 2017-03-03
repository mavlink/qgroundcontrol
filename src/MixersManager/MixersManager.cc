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
#include <cstring>

QGC_LOGGING_CATEGORY(MixersManagerLog, "MixersManagerLog")

MixersManager::MixersManager(Vehicle* vehicle)
    : _vehicle(vehicle)
    , _dedicatedLink(NULL)
    , _mixerDataMessages()
    , _ackTimeoutTimer(NULL)
    , _expectedAck(AckNone)
    , _getMissing(false)
    , _requestGroup(0)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MixersManager::_mavlinkMessageReceived);
    
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);
    _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
    
    connect(_ackTimeoutTimer, &QTimer::timeout, this, &MixersManager::_ackTimeout);
}

MixersManager::~MixersManager()
{
    _ackTimeoutTimer->stop();
}


void MixersManager::_ackTimeout(void)
{
    if (_expectedAck == AckNone) {
        return;
    }

    switch(_expectedAck){
    case AckAll:
        qCDebug(MixersManagerLog) << "Mixer request all data - timeout.  Received " << _mixerDataMessages.length() << " messages";
        _requestMissingData(0);
        break;
    default:
        break;
    }

    _expectedAck = AckNone;
}

void MixersManager::_startAckTimeout(AckType_t ack)
{
    _expectedAck = ack;
    _ackTimeoutTimer->start();
}

bool MixersManager::requestMixerCount(unsigned int group){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    command.command = MAV_CMD_REQUEST_MIXER_DATA;
    command.param1 = group; //Group
    command.param2 = 0; //Mixer
    command.param3 = 0; //SubMixer
    command.param4 = 0; //Parameter
    command.param5 = MIXER_DATA_TYPE_MIXER_COUNT; //Type

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckMixersCount);
    return true;
}

bool MixersManager::requestSubmixerCount(unsigned int group, unsigned int mixer){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    command.command = MAV_CMD_REQUEST_MIXER_DATA;
    command.param1 = group; //Group
    command.param2 = mixer; //Mixer
    command.param3 = 0; //SubMixer
    command.param4 = 0; //Parameter
    command.param5 = MIXER_DATA_TYPE_SUBMIXER_COUNT; //Type

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckSubmixersCount);
    return true;
}


bool MixersManager::requestMixerType(unsigned int group, unsigned int mixer, unsigned int submixer){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    command.command = MAV_CMD_REQUEST_MIXER_DATA;
    command.param1 = group; //Group
    command.param2 = mixer; //Mixer
    command.param3 = submixer; //SubMixer
    command.param4 = 0; //Parameter
    command.param5 = MIXER_DATA_TYPE_MIXTYPE; //Type

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckSubmixersCount);
    return true;
}



bool MixersManager::requestMixerAll(unsigned int group){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    _requestGroup = group;

    command.command = MAV_CMD_REQUEST_MIXER_SEND_ALL;
    command.param1 = group; //Group
    command.param2 = 0; //Mixer
    command.param3 = 0; //SubMixer
    command.param4 = 0; //Parameter
    command.param5 = 0; //Type

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckAll);
    return true;
}

bool MixersManager::requestMissingData(unsigned int group){
    _requestGroup = group;
    _requestMissingData(group);
    return true;
}

//* Request a missing messages. true if there is missing data */
bool MixersManager::_requestMissingData(unsigned int group){
    mavlink_mixer_data_t chk;
    _getMissing = true;

    int found_index;
    int mixer_count;
    int submixer_count;
    int mixer_type;
    chk.connection_group = group;

    chk.data_type = MIXER_DATA_TYPE_MIXER_COUNT;
    found_index = _getMessageOfKind(&chk);
    if(found_index == -1){
        requestMixerCount(group);
        return true;
    } else {
        mixer_count = _mixerDataMessages[found_index]->data_value;
    }

    for(chk.mixer_index=0; chk.mixer_index<mixer_count; chk.mixer_index++){
        chk.data_type = MIXER_DATA_TYPE_SUBMIXER_COUNT;
        found_index = _getMessageOfKind(&chk);
        if(found_index == -1){
            requestSubmixerCount(group, chk.mixer_index);
            return true;
        } else {
            submixer_count = _mixerDataMessages[found_index]->data_value;
        }
        for(chk.mixer_sub_index=0; chk.mixer_sub_index<=submixer_count; chk.mixer_sub_index++){
            chk.data_type = MIXER_DATA_TYPE_MIXTYPE;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                requestMixerType(group, chk.mixer_index, chk.mixer_sub_index);
                return true;
            } else {
                mixer_type = _mixerDataMessages[found_index]->data_value;
            }
        }

    }
    _getMissing = false;
    return false;
}

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

int MixersManager::_getMessageOfKind(const mavlink_mixer_data_t* data){

    mavlink_mixer_data_t* scandata;
    int index = 0;
    for (index=0; index<_mixerDataMessages.length(); index++){
        scandata = _mixerDataMessages[index];
        if(scandata != nullptr){
            if( (scandata->connection_group == data->connection_group) &&
                (scandata->data_type == data->data_type) ){
                switch(data->data_type){
                case MIXER_DATA_TYPE_MIXER_COUNT:
                    return index;
                    break;
                case MIXER_DATA_TYPE_SUBMIXER_COUNT:
                    if(scandata->mixer_index == data->mixer_index)
                        return index;
                    break;
                case MIXER_DATA_TYPE_MIXTYPE:
                case MIXER_DATA_TYPE_CONNECTION_COUNT:
                case MIXER_DATA_TYPE_PARAMETER_COUNT:
                    if( (scandata->mixer_index == data->mixer_index) &&
                        (scandata->mixer_sub_index == data->mixer_sub_index) )
                        return index;
                    break;
                case MIXER_DATA_TYPE_PARAMETER:
                    if( (scandata->mixer_index == data->mixer_index) &&
                        (scandata->mixer_sub_index == data->mixer_sub_index) &&
                        (scandata->parameter_index == data->parameter_index) )
                        return index;
                    break;
                case MIXER_DATA_TYPE_CONNECTION:
                    if( (scandata->mixer_index == data->mixer_index) &&
                        (scandata->mixer_sub_index == data->mixer_sub_index) &&
                        (scandata->parameter_index == data->parameter_index) &&
                        (scandata->connection_type == data->connection_type) )
                        return index;
                    break;
                }
            }
        }
    }
    return -1;
}


bool MixersManager::_collectMixerData(const mavlink_mixer_data_t* data){
    //Check if this kind of message is in the list already. Add it if it is not there.
    int dataIndex = _getMessageOfKind(data);
    if(dataIndex == -1) {
        mavlink_mixer_data_t* newMixerData = (mavlink_mixer_data_t*) malloc(sizeof(mavlink_mixer_data_t));
        if(newMixerData == nullptr) return false;
        memcpy(newMixerData, data, sizeof(mavlink_mixer_data_t));
        _mixerDataMessages.append(newMixerData);
        dataIndex=_mixerDataMessages.length()-1;
    } else {
        memcpy(_mixerDataMessages[dataIndex], data, sizeof(mavlink_mixer_data_t));
    }
    return true;
}

/// Called when a new mavlink message for out vehicle is received
void MixersManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_MIXER_DATA: {
        if(_expectedAck == AckAll)
            _ackTimeoutTimer->start();
        else {
            _ackTimeoutTimer->stop();
            _expectedAck = AckNone;
        }

        qCDebug(MixersManagerLog) << "Received mixer data";
            mavlink_mixer_data_t mixerData;
            mavlink_msg_mixer_data_decode(&message, &mixerData);

            _collectMixerData(&mixerData);

            switch(mixerData.data_type){
            case MIXER_DATA_TYPE_MIXER_COUNT:
                qCDebug(MixersManagerLog) << "Received mixer count from group:"
                                          << mixerData.mixer_group
                                          << " count:"
                                          << mixerData.data_value;
                break;
            default:
                break;
            break;
           }
        }
        default:
            break;
    }

    if(_getMissing)
        _requestMissingData(_requestGroup);


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
