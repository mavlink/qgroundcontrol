/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Matthew Coleman <uavflightdirector@gmail.com>

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
    , _mixerGroupsData(this)
    , _mixerMetaData()
    , _mixerDataMessages()
    , _ackTimeoutTimer(NULL)
    , _expectedAck(AckNone)
    , _getMissing(false)
    , _requestGroup(0)
    , _mixerDataReady(false)
    , _missingMixerData(true)
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

MixerGroup* MixersManager::getMixerGroup(unsigned int groupID){
    return _mixerGroupsData.getGroup(groupID);
}

void MixersManager::_paramValueUpdated(const QVariant& value){

}


void MixersManager::_ackTimeout(void)
{
    if (_expectedAck == AckNone) {
        return;
    }

    switch(_expectedAck){
    case AckAll:
        qCDebug(MixersManagerLog) << "Mixer request all data - timeout.  Received " << _mixerDataMessages.length() << " messages";
        _retryCount = 0;
        _requestMissingData(_requestGroup);
        break;
    default:
        break;
    }

    if(_getMissing) {
        _retryCount++;
        if(_retryCount > _maxRetryCount) {
            _getMissing = false;
            _expectedAck = AckNone;
            qDebug("Retry count exceeded while requesting missing data");
        } else {
            _requestMissingData(_requestGroup);
        }
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


bool MixersManager::requestParameterCount(unsigned int group, unsigned int mixer, unsigned int submixer){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    command.command = MAV_CMD_REQUEST_MIXER_DATA;
    command.param1 = group; //Group
    command.param2 = mixer; //Mixer
    command.param3 = submixer; //SubMixer
    command.param4 = 0; //Parameter
    command.param5 = MIXER_DATA_TYPE_PARAMETER_COUNT; //Type

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckParameterCount);
    return true;
}

bool MixersManager::requestParameter(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int parameter){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    command.command = MAV_CMD_REQUEST_MIXER_DATA;
    command.param1 = group; //Group
    command.param2 = mixer; //Mixer
    command.param3 = submixer; //SubMixer
    command.param4 = parameter; //Parameter
    command.param5 = MIXER_DATA_TYPE_PARAMETER; //Type

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckGetParameter);
    return true;
}

bool MixersManager::requestConnectionCount(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned connType){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    command.command = MAV_CMD_REQUEST_MIXER_DATA;
    command.param1 = group; //Group
    command.param2 = mixer; //Mixer
    command.param3 = submixer; //SubMixer
    command.param4 = 0;         //Parameter / Connection index
    command.param5 = MIXER_DATA_TYPE_CONNECTION_COUNT; //Type
    command.param6 = connType;  //Connection type

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckGetParameter);
    return true;
}

bool MixersManager::requestConnection(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned connType, unsigned conn){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    command.command = MAV_CMD_REQUEST_MIXER_CONN;
    command.param1 = group; //Group
    command.param2 = mixer; //Mixer
    command.param3 = submixer; //SubMixer
    command.param4 = connType; //Connection type
    command.param5 = conn; //Connection index
    command.param6 = 0;

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckGetParameter);
    return true;
}



bool MixersManager::requestMixerAll(unsigned int group){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    _requestGroup = group;
    _retryCount = 0;

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
    _mixerDataReady = false;
    emit mixerDataReadyChanged(false);
    return true;
}

bool MixersManager::requestMissingData(unsigned int group){
    _retryCount = 0;
    _requestGroup = group;
    if(!_requestMissingData(group)){
        mixerDataDownloadComplete(group);
    }
    emit mixerDataReadyChanged(false);
    return false;
}

void MixersManager::mixerDataDownloadComplete(unsigned int group){
    if(_buildAll(group)){
        _mixerDataReady = true;
        emit mixerDataReadyChanged(true);
    }
}

bool MixersManager::_buildAll(unsigned int group){
    if(!_buildStructureFromMessages(group))
        return false;
//    if(!_buildParametersFromHeaders(group))
//        return false;
//    if(!_buildConnectionsFromHeaders(group))
//        return false;
//    if(!_parameterValuesFromMessages(group))
//        return false;
//    if(!_connectionsFromMessages(group))
//        return false;
    return true;
}


//* Request a missing messages. true if there is missing data */
bool MixersManager::_requestMissingData(unsigned int group){
    mavlink_mixer_data_t chk;
    _getMissing = true;
    _retryCount = 0;

    int found_index;
    int mixer_count;
    int submixer_count;
    int parameter_count;
    int mixer_type;
    int mixer_input_conn_count;
    int mixer_output_conn_count;
    chk.mixer_group = group;

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

            //Check for mixer type
            chk.data_type = MIXER_DATA_TYPE_MIXTYPE;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                requestMixerType(group, chk.mixer_index, chk.mixer_sub_index);
                return true;
            } else {
                mixer_type = _mixerDataMessages[found_index]->data_value;
            }

            //Check for mixer parameter count
            chk.data_type = MIXER_DATA_TYPE_PARAMETER_COUNT;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                requestParameterCount(group, chk.mixer_index, chk.mixer_sub_index);
                return true;
            } else {
                parameter_count = _mixerDataMessages[found_index]->data_value;
            }

            //Check for parameters
            for(chk.parameter_index=0; chk.parameter_index<parameter_count; chk.parameter_index++){
                chk.data_type = MIXER_DATA_TYPE_PARAMETER;
                found_index = _getMessageOfKind(&chk);
                if(found_index == -1){
                    requestParameter(group, chk.mixer_index, chk.mixer_sub_index, chk.parameter_index);
                    return true;
                }
            }

            //Check for mixer input connection count
            chk.parameter_index=0;
            chk.connection_type=1;
            chk.data_type = MIXER_DATA_TYPE_CONNECTION_COUNT;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                requestConnectionCount(group, chk.mixer_index, chk.mixer_sub_index, chk.connection_type);
                return true;
            } else {
                mixer_input_conn_count = _mixerDataMessages[found_index]->data_value;
            }

            //Check for input connections
            chk.connection_type=1;
            for(chk.parameter_index=0; chk.parameter_index<mixer_input_conn_count; chk.parameter_index++){
                chk.data_type = MIXER_DATA_TYPE_CONNECTION;
                found_index = _getMessageOfKind(&chk);
                if(found_index == -1){
                    requestConnection(group, chk.mixer_index, chk.mixer_sub_index, chk.connection_type, chk.parameter_index);
                    return true;
                }
            }

            //Check for mixer output connection count
            chk.parameter_index=0;
            chk.connection_type=0;
            chk.data_type = MIXER_DATA_TYPE_CONNECTION_COUNT;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                requestConnectionCount(group, chk.mixer_index, chk.mixer_sub_index, chk.connection_type);
                return true;
            } else {
                mixer_output_conn_count = _mixerDataMessages[found_index]->data_value;
            }

            //Check for output connections
            chk.connection_type=0;
            for(chk.parameter_index=0; chk.parameter_index<mixer_output_conn_count; chk.parameter_index++){
                chk.data_type = MIXER_DATA_TYPE_CONNECTION;
                found_index = _getMessageOfKind(&chk);
                if(found_index == -1){
                    requestConnection(group, chk.mixer_index, chk.mixer_sub_index, chk.connection_type, chk.parameter_index);
                    return true;
                }
            }
        }
    }
    _getMissing = false;
    return false;
}

//bool MixersManager::_buildFactsFromMessages(unsigned int group){
//    mavlink_mixer_data_t msg;

//    int found_index, mix_count, submixer_count, parameter_count, mixer_type;
//    int mixer_conn_count;

//    Mixer *mixer;
//    Mixer *submixer;
//    Fact *fact;
//    MixerConnection *mixConn;

//    _mixerGroupsData.deleteGroup(group);
//    MixerGroup *mixer_group = new MixerGroup();
//    _mixerGroupsData.addGroup(group, mixer_group);
//    FactMetaData *paramMetaData;

//    msg.mixer_group = group;
//    msg.data_type = MIXER_DATA_TYPE_MIXER_COUNT;
//    found_index = _getMessageOfKind(&msg);
//    if(found_index == -1){
//        return false;
//    }

//    mix_count = _mixerDataMessages[found_index]->data_value;

//    for(msg.mixer_index=0; msg.mixer_index<mix_count; msg.mixer_index++) {

//        //Get type of main mixer
//        msg.mixer_sub_index=0;
//        msg.data_type = MIXER_DATA_TYPE_MIXTYPE;
//        found_index = _getMessageOfKind(&msg);
//        if(found_index == -1){
//            return false;
//        }
//        mixer_type = _mixerDataMessages[found_index]->data_value;

//        msg.data_type = MIXER_DATA_TYPE_SUBMIXER_COUNT;
//        found_index = _getMessageOfKind(&msg);
//        if(found_index == -1){
//            return false;
//        }
//        submixer_count = _mixerDataMessages[found_index]->data_value;

//        for(msg.mixer_sub_index=0; msg.mixer_sub_index<=submixer_count; msg.mixer_sub_index++){

//            //mixer type
//            msg.data_type = MIXER_DATA_TYPE_MIXTYPE;
//            found_index = _getMessageOfKind(&msg);
//            if(found_index == -1){
//                return false;
//            }
//            mixer_type = _mixerDataMessages[found_index]->data_value;

//            //Mixer or submixer
//            if(msg.mixer_sub_index == 0){
//                //Add mixer to the group
//                mixer = new Mixer(mixer_type);
//                mixer_group->addMixer(msg.mixer_index, mixer);

//            } else {
//                submixer = new Mixer(mixer_type);
//                mixer->addSubmixer(msg.mixer_sub_index, submixer);
//            }

//            //parameter count
//            msg.data_type = MIXER_DATA_TYPE_PARAMETER_COUNT;
//            found_index = _getMessageOfKind(&msg);
//            if(found_index == -1){
//                return false;
//            }
//            parameter_count = _mixerDataMessages[found_index]->data_value;

//            //Parameters
//            msg.data_type = MIXER_DATA_TYPE_PARAMETER;
//            for(msg.parameter_index=0; msg.parameter_index<parameter_count; msg.parameter_index++){
//                found_index = _getMessageOfKind(&msg);
//                if(found_index == -1){
//                    return false;
//                }

//                //Mixer or submixer
//                if(msg.mixer_sub_index == 0){
//                    fact = new Fact(-1, QString("MIX_PARAM"), FactMetaData::valueTypeFloat, mixer_group);
//                    mixer->addMixerParamFact(msg.parameter_index, fact);
//                } else {
//                    fact = new Fact(-1, QString("SUBMIX_PARAM"), FactMetaData::valueTypeFloat, mixer);
//                    submixer->addMixerParamFact(msg.parameter_index, fact);
//                }
//                float param_value = _mixerDataMessages[found_index]->param_value;

//                //Set the parameter Fact meta data and value
//                paramMetaData = _mixerMetaData.GetMixerParameterMetaData(mixer_type, msg.parameter_index);
//                if(paramMetaData != nullptr){
//                        fact->setMetaData(paramMetaData);
//                }
//                fact->setRawValue(QVariant(param_value));

//                connect(fact, &Fact::_containerRawValueChanged, this, &MixersManager::_paramValueUpdated);
//            }

//            //Input connection count
//            msg.parameter_index=0;
//            msg.connection_type=1;
//            msg.data_type = MIXER_DATA_TYPE_CONNECTION_COUNT;
//            found_index = _getMessageOfKind(&msg);
//            if(found_index == -1){
//                return false;
//            }
//            mixer_conn_count = _mixerDataMessages[found_index]->data_value;

//            //Input connections
//            msg.connection_type=1;
//            msg.data_type = MIXER_DATA_TYPE_CONNECTION;
//            for(msg.parameter_index=0; msg.parameter_index<mixer_conn_count; msg.parameter_index++){
//                found_index = _getMessageOfKind(&msg);
//                if(found_index == -1){
//                    return false;
//                }

//                //Mixer or submixer
//                if(msg.mixer_sub_index == 0){
//                    mixer->addConnection(_mixerDataMessages[found_index]->connection_type,
//                                         _mixerDataMessages[found_index]->parameter_index,
//                                         _mixerDataMessages[found_index]->connection_group,
//                                         _mixerDataMessages[found_index]->data_value );
//                } else {
//                    submixer->addConnection(_mixerDataMessages[found_index]->connection_type,
//                                         _mixerDataMessages[found_index]->parameter_index,
//                                         _mixerDataMessages[found_index]->connection_group,
//                                         _mixerDataMessages[found_index]->data_value );
//                }
//            }

//            //Output connection count
//            msg.parameter_index=0;
//            msg.connection_type=0;
//            msg.data_type = MIXER_DATA_TYPE_CONNECTION_COUNT;
//            found_index = _getMessageOfKind(&msg);
//            if(found_index == -1){
//                return false;
//            }
//            mixer_conn_count = _mixerDataMessages[found_index]->data_value;

//            //Output connections
//            msg.connection_type=0;
//            msg.data_type = MIXER_DATA_TYPE_CONNECTION;
//            for(msg.parameter_index=0; msg.parameter_index<mixer_conn_count; msg.parameter_index++){
//                found_index = _getMessageOfKind(&msg);
//                if(found_index == -1){
//                    return false;
//                }

//                //Mixer or submixer
//                if(msg.mixer_sub_index == 0){
//                    mixer->addConnection(_mixerDataMessages[found_index]->connection_type,
//                                         _mixerDataMessages[found_index]->parameter_index,
//                                         _mixerDataMessages[found_index]->connection_group,
//                                         _mixerDataMessages[found_index]->data_value );
//                } else {
//                    submixer->addConnection(_mixerDataMessages[found_index]->connection_type,
//                                         _mixerDataMessages[found_index]->parameter_index,
//                                         _mixerDataMessages[found_index]->connection_group,
//                                         _mixerDataMessages[found_index]->data_value );
//                }
//            }
//        }
//    }

//    _mixerDataReady = true;
//    emit mixerDataReadyChanged(true);
//    return true;
//}



///* Build mixer structure from messages.  This only includes mixers and submixers with type facts
/// return true if successfull*/
bool MixersManager::_buildStructureFromMessages(unsigned int group){
    mavlink_mixer_data_t msg;

    int found_index, mix_count, submixer_count, mixer_type;

    Mixer *mixer;
    Mixer *submixer;

    //TODO Put this back
//    _mixerDataReady = false;
//    emit mixerDataReadyChanged(false);

    //Delete existing mixer group data
    _mixerGroupsData.deleteGroup(group);
    MixerGroup *mixer_group = new MixerGroup();
    _mixerGroupsData.addGroup(group, mixer_group);

    msg.mixer_group = group;
    msg.data_type = MIXER_DATA_TYPE_MIXER_COUNT;
    found_index = _getMessageOfKind(&msg);
    if(found_index == -1){
        return false;
    }

    mix_count = _mixerDataMessages[found_index]->data_value;

    for(msg.mixer_index=0; msg.mixer_index<mix_count; msg.mixer_index++) {

        //Get type of main mixer
        msg.mixer_sub_index=0;
        msg.data_type = MIXER_DATA_TYPE_MIXTYPE;
        found_index = _getMessageOfKind(&msg);
        if(found_index == -1){
            return false;
        }
        mixer_type = _mixerDataMessages[found_index]->data_value;

        //Add mixer to the group
        mixer = new Mixer(_mixerMetaData.GetMixerType(mixer_type) );
        mixer_group->appendMixer(msg.mixer_index, mixer);

        msg.data_type = MIXER_DATA_TYPE_SUBMIXER_COUNT;
        found_index = _getMessageOfKind(&msg);
        if(found_index == -1){
            return false;
        }
        submixer_count = _mixerDataMessages[found_index]->data_value;

        for(msg.mixer_sub_index=1; msg.mixer_sub_index<=submixer_count; msg.mixer_sub_index++){

            //mixer type
            msg.data_type = MIXER_DATA_TYPE_MIXTYPE;
            found_index = _getMessageOfKind(&msg);
            if(found_index == -1){
                return false;
            }
            mixer_type = _mixerDataMessages[found_index]->data_value;

            //Mixer or submixer
            if(msg.mixer_sub_index == 0){


            } else {
                submixer = new Mixer(_mixerMetaData.GetMixerType(mixer_type));
                mixer->appendSubmixer(msg.mixer_sub_index, submixer);
            }

        }
    }

    return true;
}


///* Build parameters from included headers.  TODO: DEPRECIATE AND CHANGE TO FILE INSTEAD OF HEADERS
///  return true if successfull*/
bool MixersManager::_buildParametersFromHeaders(unsigned int group){
    return false;
}

///* Build connections from included headers.  TODO: DEPRECIATE AND CHANGE TO FILE INSTEAD OF HEADERS
/// return true if successfull*/
bool MixersManager::_buildConnectionsFromHeaders(unsigned int group){
    return false;
}

///* Set parameter values from mixer data messages
/// return true if successfull*/
bool MixersManager::_parameterValuesFromMessages(unsigned int group){
    return false;
}

///* Set connection points from mixer data messages
/// return true if successfull*/
bool MixersManager::_connectionsFromMessages(unsigned int group){
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
                case MIXER_DATA_TYPE_CONNECTION_COUNT:
                    if( (scandata->mixer_index == data->mixer_index) &&
                        (scandata->mixer_sub_index == data->mixer_sub_index) &&
                        (scandata->connection_type == data->connection_type) )
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

            if(_getMissing) {
                _requestMissingData(_requestGroup);
            }

            switch(mixerData.data_type){
            case MIXER_DATA_TYPE_MIXER_COUNT: {
                qDebug() << "Received mixer count from group:"
                                          << mixerData.mixer_group
                                          << " count:"
                                          << mixerData.data_value;
//                qCDebug(MixersManagerLog) << "Received mixer count from group:"
//                                          << mixerData.mixer_group
//                                          << " count:"
//                                          << mixerData.data_value;
                break;
            }
            case MIXER_DATA_TYPE_SUBMIXER_COUNT: {
                qDebug() << "Received submixer count from group:"
                          << mixerData.mixer_group
                          << " mixer:"
                          << mixerData.mixer_index
                          << " count:"
                          << mixerData.data_value;
                break;
            }
            case MIXER_DATA_TYPE_MIXTYPE: {
                qDebug() << "Received mixer type from group:"
                          << mixerData.mixer_group
                          << " mixer:"
                          << mixerData.mixer_index
                          << " submixer:"
                          << mixerData.mixer_sub_index
                          << " type:"
                          << mixerData.data_value;
                break;
            }
            case MIXER_DATA_TYPE_PARAMETER_COUNT: {
                qDebug() << "Received parameter count from group:"
                          << mixerData.mixer_group
                          << " mixer:"
                          << mixerData.mixer_index
                          << " submixer:"
                          << mixerData.mixer_sub_index
                          << " param_count:"
                          << mixerData.data_value;
                break;
            }
            case MIXER_DATA_TYPE_PARAMETER: {
                qDebug() << "Received parameter from group:"
                          << mixerData.mixer_group
                          << " mixer:"
                          << mixerData.mixer_index
                          << " submixer:"
                          << mixerData.mixer_sub_index
                          << " param_index:"
                          << mixerData.parameter_index
                          << " param:"
                          << mixerData.param_value;
                break;
            }
            case MIXER_DATA_TYPE_CONNECTION_COUNT: {
                qDebug() << "Received connection count from group:"
                          << mixerData.mixer_group
                          << " mixer:"
                          << mixerData.mixer_index
                          << " submixer:"
                          << mixerData.mixer_sub_index
                          << " conn_type:"
                          << mixerData.connection_type
                          << " conn_count:"
                          << mixerData.data_value;
                break;
            }
            case MIXER_DATA_TYPE_CONNECTION: {
                qDebug() << "Received connection from group:"
                          << mixerData.mixer_group
                          << " mixer:"
                          << mixerData.mixer_index
                          << " submixer:"
                          << mixerData.mixer_sub_index
                          << " conn_type:"
                          << mixerData.connection_type
                          << " conn_index:"
                          << mixerData.parameter_index
                        << " conn_group:"
                        << mixerData.connection_group
                        << " conn_channel:"
                        << mixerData.data_value;
                break;
            }
            default:
                break;
           break;
           }
        }
        default:
            break;

    }
}
