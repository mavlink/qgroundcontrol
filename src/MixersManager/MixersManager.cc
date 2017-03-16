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

#include "MixerMetaData.h"

QGC_LOGGING_CATEGORY(MixersManagerLog, "MixersManagerLog")

MixersManager::MixersManager(Vehicle* vehicle)
    : _vehicle(vehicle)
    , _dedicatedLink(NULL)
    , _mixerGroupsData(this)
    , _mixerDataMessages()
    , _ackTimeoutTimer(NULL)
    , _expectedAck(AckNone)
    , _status(MIXERS_MANAGER_WAITING)
    , _actionGroup(0)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MixersManager::_mavlinkMessageReceived);
    
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);
    _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
    
    connect(_ackTimeoutTimer, &QTimer::timeout, this, &MixersManager::_msgTimeout);
}

MixersManager::~MixersManager()
{
    _ackTimeoutTimer->stop();
}

MixerGroup* MixersManager::getMixerGroup(unsigned int groupID){
    return _mixerGroupsData.getGroup(groupID);
}

void MixersManager::_paramValueUpdated(const QVariant& value){
    Q_UNUSED(value)
}

void MixersManager::_msgTimeout(void)
{
    switch(int(_status))
    {
        case MIXERS_MANAGER_WAITING:
            return;
            break;

        case MIXERS_MANAGER_DOWNLOADING_ALL:
            qCDebug(MixersManagerLog) << "Mixer request all data - timeout.  Received " << _mixerDataMessages.length() << " messages";
            _retryCount = 0;
            _requestMissingData(_actionGroup);
            break;

        case MIXERS_MANAGER_DOWNLOADING_MISSING:{
            _retryCount++;
            if(_retryCount > _maxRetryCount) {
                _setStatus(MIXERS_MANAGER_WAITING);
                _expectedAck = AckNone;
                qDebug("Retry count exceeded while requesting missing data");
            } else {
                _requestMissingData(_actionGroup);
            }
            break;
        }
    }
    _expectedAck = AckNone;
}

void MixersManager::_startAckTimeout(AckType_t ack)
{
    _expectedAck = ack;
    _ackTimeoutTimer->start();
}

void MixersManager::_setStatus(MIXERS_MANAGER_STATUS_e newStatus){
    if(_status != newStatus) {
        _status = newStatus;
        emit(mixerManagerStatusChanged(_status));
    }
}


bool MixersManager::mixerDataReady() {

    MixerGroup *group = _mixerGroupsData.getGroup(_actionGroup);
    if(group==nullptr)
        return false;
    return group->dataComplete();
}

MixerGroup* MixersManager::mixerGroupStatus(void) {
    return _mixerGroupsData.getGroup(_actionGroup);
}

bool MixersManager::_requestMixerCount(unsigned int group){
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

bool MixersManager::_requestSubmixerCount(unsigned int group, unsigned int mixer){
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


bool MixersManager::_requestMixerType(unsigned int group, unsigned int mixer, unsigned int submixer){
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


bool MixersManager::_requestParameterCount(unsigned int group, unsigned int mixer, unsigned int submixer){
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

bool MixersManager::_requestParameter(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int parameter){
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

bool MixersManager::_requestConnectionCount(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned connType){
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

bool MixersManager::_requestConnection(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned connType, unsigned conn){
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


bool MixersManager::requestMixerDownload(unsigned int group){
    if(_status != MIXERS_MANAGER_WAITING)
        return false;

    MixerGroup* mixerGroup = getMixerGroup(group);
    if(mixerGroup == nullptr) {
        mixerGroup = new MixerGroup(group);
        _mixerGroupsData.addGroup(mixerGroup);
    }

    _requestMixerAll(group);
    return true;
}


bool MixersManager::_requestMixerAll(unsigned int group){
    if(_status != MIXERS_MANAGER_WAITING)
        return false;

    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    _actionGroup = group;
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
    _setStatus(MIXERS_MANAGER_DOWNLOADING_ALL);
    _startAckTimeout(AckAll);
    return true;
}


void MixersManager::clearMixerGroupMessages(unsigned int group){
    mavlink_mixer_data_t *msg;
    _actionGroup = group;
    QMutableListIterator<mavlink_mixer_data_t*> i(_mixerDataMessages);
    while (i.hasNext()) {
        msg = i.value();
        Q_CHECK_PTR(msg);
        if(msg->mixer_group == group){
            delete msg;
            i.remove();
        }
    }
}

void MixersManager::_mixerDataDownloadComplete(unsigned int group){
    _setStatus(MIXERS_MANAGER_WAITING);

    if(_buildAll(group)){
        emit mixerDataReadyChanged(true);
    }
}

bool MixersManager::_buildAll(unsigned int group){
    if(!_buildStructureFromMessages(group))
        return false;
    if(!_buildParametersFromHeaders(group))
        return false;
    if(!_buildConnections(group))
        return false;
    if(!_parameterValuesFromMessages(group))
        return false;
//    if(!_connectionsFromMessages(group))
//        return false;
    return true;
}


//* Request a missing messages. true if there is missing data */
bool MixersManager::_requestMissingData(unsigned int group){
    mavlink_mixer_data_t chk;
    _setStatus(MIXERS_MANAGER_DOWNLOADING_MISSING);
    _actionGroup = group;
    _retryCount = 0;

    int found_index;
    int mixer_count;
    int submixer_count;
    int parameter_count;
//    int mixer_type;
    int mixer_input_conn_count;
    int mixer_output_conn_count;

    memset(&chk, 0, sizeof(mavlink_mixer_data_t));
    chk.mixer_group = group;

    chk.data_type = MIXER_DATA_TYPE_MIXER_COUNT;
    found_index = _getMessageOfKind(&chk);
    if(found_index == -1){
        _requestMixerCount(group);
        return true;
    } else {
        mixer_count = _mixerDataMessages[found_index]->data_value;
    }

    for(chk.mixer_index=0; chk.mixer_index<mixer_count; chk.mixer_index++){
        chk.data_type = MIXER_DATA_TYPE_SUBMIXER_COUNT;
        found_index = _getMessageOfKind(&chk);
        if(found_index == -1){
            _requestSubmixerCount(group, chk.mixer_index);
            return true;
        } else {
            submixer_count = _mixerDataMessages[found_index]->data_value;
        }
        for(chk.mixer_sub_index=0; chk.mixer_sub_index<=submixer_count; chk.mixer_sub_index++){

            //Check for mixer type
            chk.data_type = MIXER_DATA_TYPE_MIXTYPE;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                _requestMixerType(group, chk.mixer_index, chk.mixer_sub_index);
                return true;
            }
//            else {
//                mixer_type = _mixerDataMessages[found_index]->data_value;
//                //TODO Use mixer type instead of requesting counts
//            }

            //Check for mixer parameter count
            //TODO Change this to choose data source depending on situation
            chk.data_type = MIXER_DATA_TYPE_PARAMETER_COUNT;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                _requestParameterCount(group, chk.mixer_index, chk.mixer_sub_index);
                return true;
            } else {
                parameter_count = _mixerDataMessages[found_index]->data_value;
            }

            //Check for parameters
            for(chk.parameter_index=0; chk.parameter_index<parameter_count; chk.parameter_index++){
                chk.data_type = MIXER_DATA_TYPE_PARAMETER;
                found_index = _getMessageOfKind(&chk);
                if(found_index == -1){
                    _requestParameter(group, chk.mixer_index, chk.mixer_sub_index, chk.parameter_index);
                    return true;
                }
            }

            //Check for mixer input connection count
            chk.parameter_index=0;
            chk.connection_type=1;
            chk.data_type = MIXER_DATA_TYPE_CONNECTION_COUNT;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                _requestConnectionCount(group, chk.mixer_index, chk.mixer_sub_index, chk.connection_type);
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
                    _requestConnection(group, chk.mixer_index, chk.mixer_sub_index, chk.connection_type, chk.parameter_index);
                    return true;
                }
            }

            //Check for mixer output connection count
            chk.parameter_index=0;
            chk.connection_type=0;
            chk.data_type = MIXER_DATA_TYPE_CONNECTION_COUNT;
            found_index = _getMessageOfKind(&chk);
            if(found_index == -1){
                _requestConnectionCount(group, chk.mixer_index, chk.mixer_sub_index, chk.connection_type);
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
                    _requestConnection(group, chk.mixer_index, chk.mixer_sub_index, chk.connection_type, chk.parameter_index);
                    return true;
                }
            }
        }
    }

    _mixerDataDownloadComplete(group);
    _setStatus(MIXERS_MANAGER_WAITING);

    return false;
}


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
    MixerGroup *mixer_group = _mixerGroupsData.getGroup(group);
    if(mixer_group == nullptr)
    {
        mixer_group->deleteGroupMixers();
    } else {
        mixer_group = new MixerGroup(group);
        _mixerGroupsData.addGroup(mixer_group);
    }

    MixerMetaData *mixer_metadata = mixer_group->getMixerMetaData();

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
        mixer = new Mixer(mixer_metadata->getMixerType(mixer_type));
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
                submixer = new Mixer(mixer_metadata->getMixerType(mixer_type));
                mixer->appendSubmixer(msg.mixer_sub_index, submixer);
            }

        }
    }

    return true;
}


bool MixersManager::_buildParametersFromHeaders(unsigned int group){
    Mixer *mixer;
    Mixer *submixer;
    Fact  *param;
    int mixType, subType, mixIndex, subIndex, paramCount, subCount;
    bool convOK;
    FactMetaData *metaData;

    MixerGroup *mixer_group = _mixerGroupsData.getGroup(group);
    if(mixer_group == nullptr)
        return false;

    MixerMetaData *mixer_metadata = mixer_group->getMixerMetaData();

    QObjectList mixers = mixer_group->mixers();

    for(mixIndex = 0; mixIndex<mixers.count(); mixIndex++){
        mixer = mixer_group->getMixer(mixIndex);
        mixType = mixer->mixer()->rawValue().toInt(&convOK);
        Q_ASSERT(convOK==true);

        paramCount = mixer_metadata->getMixerParameterCount(mixType);
        for(int paramIndex=0; paramIndex<paramCount; paramIndex++){
            metaData = mixer_metadata->getMixerParameterMetaData(mixType, paramIndex);
            Q_CHECK_PTR(metaData);
            param = new Fact(-1, metaData->name(), FactMetaData::valueTypeFloat, mixer->parameters());
            param->setMetaData(metaData);
            mixer->appendParamFact(param);
        }

        //Submixers indexed from 1
        subCount = mixer->submixers()->count();
        for(subIndex=1; subIndex<=subCount; subIndex++){
            submixer = mixer->getSubmixer(subIndex);
            Q_CHECK_PTR(submixer);
            subType = submixer->mixer()->rawValue().toInt(&convOK);
            Q_ASSERT(convOK==true);

            paramCount = mixer_metadata->getMixerParameterCount(mixType);
            for(int paramIndex=0; paramIndex<paramCount; paramIndex++){
                metaData = mixer_metadata->getMixerParameterMetaData(subType, paramIndex);
                Q_CHECK_PTR(metaData);
                param = new Fact(-1, metaData->name(), FactMetaData::valueTypeFloat, submixer->parameters());
                param->setMetaData(metaData);
                submixer->appendParamFact(param);
            }
        }
    }
    return true;
}

int MixersManager::_getMixerConnCountFromVehicle(unsigned int group, int mixerType, int connType)
{
    MixerGroup *mixer_group = _mixerGroupsData.getGroup(group);
    if(mixer_group == nullptr)
        return false;
    MixerMetaData *mixer_metadata = mixer_group->getMixerMetaData();

    return mixer_metadata->getMixerConnCount(mixerType, connType);
}

void MixersManager::_setMixerConnectionFromVehicle(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int connType, unsigned int connIndex, MixerConnection* conn )
{
    _setMixerConnectionFromMessage(group, mixer, submixer, connType, connIndex, conn);
}

void MixersManager::_setMixerConnectionFromMessage(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int connType, unsigned int connIndex, MixerConnection* conn )
{
    mavlink_mixer_data_t msg;
    int msgIndex;

    memset(&msg, 0, sizeof(msg));
    msg.mixer_group = group;
    msg.mixer_index = mixer;
    msg.mixer_sub_index = submixer;
    msg.connection_type = connType;
    msg.parameter_index = connIndex;
    msg.data_type = MIXER_DATA_TYPE_CONNECTION;

    msgIndex = _getMessageOfKind(&msg);
    if(msgIndex == -1) {
        conn->setGroup(-1);
        conn->setChannel(-1);
    }
    else {
        conn->setGroup(_mixerDataMessages[msgIndex]->connection_group);
        conn->setChannel(_mixerDataMessages[msgIndex]->data_value);
    }
}




bool MixersManager::_buildConnections(unsigned int group){
    Mixer *mixer;
    Mixer *submixer;
    MixerConnection  *conn;
    int mixType, subType, mixIndex, subIndex, connCount, subCount;
    bool convOK;

    MixerGroup *mixer_group = _mixerGroupsData.getGroup(group);
    if(mixer_group == nullptr)
        return false;

    QObjectList mixers = mixer_group->mixers();

    for(mixIndex = 0; mixIndex<mixers.count(); mixIndex++){
        mixer = mixer_group->getMixer(mixIndex);
        mixType = mixer->mixer()->rawValue().toInt(&convOK);
        Q_ASSERT(convOK==true);

        //Output connections
        connCount = _getMixerConnCountFromVehicle(group, mixType, 0);
        for(int connIndex=0; connIndex<connCount; connIndex++){
            conn = new MixerConnection();
            _setMixerConnectionFromVehicle(group, mixIndex, 0, 0, connIndex, conn);
            mixer->appendOutputConnection(conn);
        }

        //Input connections
        connCount = _getMixerConnCountFromVehicle(group, mixType, 1);
        for(int connIndex=0; connIndex<connCount; connIndex++){
            conn = new MixerConnection();
            _setMixerConnectionFromVehicle(group, mixIndex, 0, 1, connIndex, conn);
            mixer->appendInputConnection(conn);
        }

        //Submixers indexed from 1
        subCount = mixer->submixers()->count();
        for(subIndex=1; subIndex<=subCount; subIndex++){
            submixer = mixer->getSubmixer(subIndex);
            Q_CHECK_PTR(submixer);
            subType = submixer->mixer()->rawValue().toInt(&convOK);
            Q_ASSERT(convOK==true);

            //Output connections
            connCount = _getMixerConnCountFromVehicle(group, subType, 0);
            for(int connIndex=0; connIndex<connCount; connIndex++){
                conn = new MixerConnection();
                _setMixerConnectionFromVehicle(group, mixIndex, subIndex, 0, connIndex, conn);
                submixer->appendOutputConnection(conn);
            }

            //Input connections
            connCount = _getMixerConnCountFromVehicle(group, subType, 1);
            for(int connIndex=0; connIndex<connCount; connIndex++){
                conn = new MixerConnection();
                _setMixerConnectionFromVehicle(group, mixIndex, subIndex, 1, connIndex, conn);
                submixer->appendInputConnection(conn);
            }
        }
    }
    return true;
}


void MixersManager::_setParameterFactFromVehicle(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int param, Fact* paramFact )
{
    _setParameterFactFromMessage(group, mixer, submixer, param, paramFact);
}


void MixersManager::_setParameterFactFromMessage(unsigned int group, unsigned int mixer, unsigned int submixer, unsigned int param, Fact* paramFact  )
{
    mavlink_mixer_data_t msg;
    int msgIndex;

    memset(&msg, 0, sizeof(msg));
    msg.mixer_group = group;
    msg.mixer_index = mixer;
    msg.mixer_sub_index = submixer;
    msg.parameter_index = param;
    msg.data_type = MIXER_DATA_TYPE_PARAMETER;

    msgIndex = _getMessageOfKind(&msg);
    if(msgIndex == -1)
        paramFact->setRawValue(0.0);
    else
        paramFact->setRawValue(_mixerDataMessages[msgIndex]->param_value);
}


///* Set parameter values from mixer data messages
/// return true if successfull*/
bool MixersManager::_parameterValuesFromMessages(unsigned int group){
    Mixer *mixer;
    Mixer *submixer;
    Fact *parameter;
    int mixIndex, subIndex, paramCount, subCount, paramIndex;

    MixerGroup *mixer_group = _mixerGroupsData.getGroup(group);
    if(mixer_group == nullptr)
        return false;

    QObjectList mixers = mixer_group->mixers();

    for(mixIndex = 0; mixIndex<mixers.count(); mixIndex++){
        mixer = mixer_group->getMixer(mixIndex);

        paramCount = mixer->parameters()->count();
        for(paramIndex=0; paramIndex<paramCount; paramIndex++){
            parameter = mixer->getParameter(paramIndex);
            Q_CHECK_PTR(parameter);
            _setParameterFactFromVehicle(group, mixIndex, 0, paramIndex, parameter);
        }

        //Submixers indexed from 1
        subCount = mixer->submixers()->count();
        for(subIndex=1; subIndex<=subCount; subIndex++){
            submixer = mixer->getSubmixer(subIndex);
            Q_CHECK_PTR(submixer);

            paramCount = submixer->parameters()->count();
            for(paramIndex=0; paramIndex<paramCount; paramIndex++){
                parameter = submixer->getParameter(paramIndex);
                Q_CHECK_PTR(parameter);
                _setParameterFactFromVehicle(group, mixIndex, subIndex, paramIndex, parameter);
            }
        }
    }
    return true;
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
            if(_status == MIXERS_MANAGER_DOWNLOADING_ALL)
                _ackTimeoutTimer->start();
            else {
                _ackTimeoutTimer->stop();
                _expectedAck = AckNone;
            }

            qCDebug(MixersManagerLog) << "Received mixer data";
            mavlink_mixer_data_t mixerData;
            mavlink_msg_mixer_data_decode(&message, &mixerData);

            _collectMixerData(&mixerData);

            if(_status == MIXERS_MANAGER_DOWNLOADING_MISSING) {
                _requestMissingData(_actionGroup);
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
                qCDebug(MixersManagerLog) << "Received submixer count from group:"
                          << mixerData.mixer_group
                          << " mixer:"
                          << mixerData.mixer_index
                          << " count:"
                          << mixerData.data_value;
                break;
            }
            case MIXER_DATA_TYPE_MIXTYPE: {
                qCDebug(MixersManagerLog) << "Received mixer type from group:"
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
                qCDebug(MixersManagerLog) << "Received parameter count from group:"
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
                qCDebug(MixersManagerLog) << "Received parameter from group:"
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
                qCDebug(MixersManagerLog) << "Received connection count from group:"
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
                qCDebug(MixersManagerLog) << "Received connection from group:"
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
