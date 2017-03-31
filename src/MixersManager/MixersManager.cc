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
    , _mixerParameterMessages()
    , _ackTimeoutTimer(this)
    , _checkWriteParamTimer(this)
    , _expectedAck(AckNone)
    , _status(MIXERS_MANAGER_WAITING)
    , _actionGroup(0)
    , _groupsIdentified(false)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MixersManager::_mavlinkMessageReceived);
    
    _ackTimeoutTimer.setSingleShot(true);
    _ackTimeoutTimer.setInterval(_ackTimeoutMilliseconds);
    connect(&_ackTimeoutTimer, &QTimer::timeout, this, &MixersManager::_msgTimeout);

    _checkWriteParamTimer.setSingleShot(true);
    _checkWriteParamTimer.setInterval(100);
    connect(&_checkWriteParamTimer, &QTimer::timeout, this, &MixersManager::_checkWriteParamTimeout);
    
}

MixersManager::~MixersManager()
{
    _ackTimeoutTimer.stop();
}

MixerGroup* MixersManager::getMixerGroup(unsigned int groupID){
    return _mixerGroupsData.getGroup(groupID);
}


void MixersManager::_changedParamValue(MixerParameter* mixParam, Fact *value, int valueIndex)
{
    MixerGroup* mixGroup = qobject_cast<MixerGroup *>(QObject::sender());

    _dataMutex.lock();

    _waitingWriteParamMap[mixGroup->groupID()][mixParam->index()][value] = valueIndex;

    _dataMutex.unlock();

    _checkWriteParamTimer.start();

    //    if(_waitingWriteParamMap.contains(mixGroup)){
    //        if(_waitingWriteParamMap[mixGroup].contains(mixParam)){
    //            if(_waitingWriteParamMap[mixGroup][mixParam].contains(value)){

    //            }
    //        }
    //    } else {

    //    }
}


void MixersManager::_msgTimeout(void)
{
    switch(int(_status))
    {
    case MIXERS_MANAGER_WAITING:
        return;
        break;

    case MIXERS_MANAGER_DOWNLOADING_ALL: {
        qCDebug(MixersManagerLog) << "Mixer request all data - timeout.  Received " << _mixerParameterMessages.length() << " messages";
        _retryCount = 0;
        _requestMissingData(_actionGroup);
        break;
    }
    case MIXERS_MANAGER_DOWNLOADING_MISSING:{
        _retryCount++;
        if(_retryCount > _maxRetryCount) {
            _setStatus(MIXERS_MANAGER_WAITING);
            _expectedAck = AckNone;
            qCDebug(MixersManagerLog) << "Retry count exceeded while requesting missing data";
        } else {
            _requestMissingData(_actionGroup);
        }
        break;
    }
    case MIXERS_MANAGER_IDENTIFYING_SUPPORTED_GROUPS: {
        _retryCount++;
        if(_retryCount > _maxRetryCount){
            _searchNextMixerGroup();
            return;
        } else {
            _searchMixerGroup();
            return;
        }
        break;
    }
    case MIXERS_MANAGER_WRITING_PARAM : {
        _sendPendingWriteParam();
        break;
    }
    }
    _expectedAck = AckNone;
}


void MixersManager::_checkWriteParamTimeout(void){
    switch(int(_status))
    {
    case MIXERS_MANAGER_WAITING:
        _sendPendingWriteParam();
        break;

    case MIXERS_MANAGER_WRITING_PARAM:
        _checkWriteParamTimer.start();
        break;
    case MIXERS_MANAGER_DOWNLOADING_ALL:
    case MIXERS_MANAGER_DOWNLOADING_MISSING:
    case MIXERS_MANAGER_IDENTIFYING_SUPPORTED_GROUPS: {
        break;
    }
    }
}

void MixersManager::_sendPendingWriteParam(void){

    if(_waitingWriteParamMap.count() > 0){
//        _dataMutex.lock();
//        _dataMutex.unlock();

        int mixGroupID = _waitingWriteParamMap.firstKey();
        int paramIndex = _waitingWriteParamMap.first().firstKey();
        Fact* value = _waitingWriteParamMap.first().first().firstKey();
        int valueIndex = _waitingWriteParamMap.first().first().first();

        // TODO: Blend all value changes into one parameter change here

        Q_ASSERT(value);

        mavlink_message_t       messageOut;
        mavlink_mixer_param_set_t      set;

        _actionGroup = mixGroupID;

        set.mixer_group = _actionGroup;
        set.index = paramIndex;
        set.mixer_index = 0;
        set.mixer_sub_index = 0;
        set.mixer_type = 0;
        strcpy(set.param_id, "");
        set.param_array_index = valueIndex;
        set.param_type = value->type();
        memset(set.param_values, 0, sizeof(set.param_values));

        //Set each value in the array accoring to type
        int i = 0;
//        foreach(value, param->values()) {
            mavlink_param_union_t   union_value;
            switch (set.param_type) {
            case FactMetaData::valueTypeUint8:
                union_value.param_uint8 = (uint8_t)value->rawValue().toUInt();
                break;

            case FactMetaData::valueTypeInt8:
                union_value.param_int8 = (int8_t)value->rawValue().toInt();
                break;

            case FactMetaData::valueTypeUint16:
                union_value.param_uint16 = (uint16_t)value->rawValue().toUInt();
                break;

            case FactMetaData::valueTypeInt16:
                union_value.param_int16 = (int16_t)value->rawValue().toInt();
                break;

            case FactMetaData::valueTypeUint32:
                union_value.param_uint32 = (uint32_t)value->rawValue().toUInt();
                break;

            case FactMetaData::valueTypeFloat:
                union_value.param_float = value->rawValue().toFloat();
                break;

            case FactMetaData::valueTypeInt32:
                union_value.param_int32 = (int32_t)value->rawValue().toInt();
                break;
            default:
                qCritical() << "Unsupported fact type" << set.param_type;
                _setStatus(MIXERS_MANAGER_WAITING);
                _checkWriteParamTimer.stop();
                return;
            }
            set.param_values[i] = union_value.param_float;
//        }

        _dedicatedLink = _vehicle->priorityLink();
        mavlink_msg_mixer_param_set_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                             qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                             _dedicatedLink->mavlinkChannel(),
                                             &messageOut,
                                             &set);

        _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);

        _setStatus(MIXERS_MANAGER_WRITING_PARAM);
        _startAckTimeout(AckSetParameter);
    } else {
        _setStatus(MIXERS_MANAGER_WAITING);
        _checkWriteParamTimer.stop();
    }

}

void MixersManager::_startAckTimeout(AckType_t ack)
{
    _expectedAck = ack;
    _ackTimeoutTimer.start();
}

void MixersManager::_setStatus(MIXERS_MANAGER_STATUS_e newStatus){
    if(_status != newStatus) {
        _status = newStatus;
        emit(mixerManagerStatusChanged(_status));
    }
}


bool MixersManager::mixerDataReady() {
    if(!_groupsIdentified)
        return false;
    if(_status != MIXERS_MANAGER_WAITING){
        return false;
    }

//    MixerGroup* mixerGroup;
//    foreach(mixerGroup, _mixerGroupsData){
//        if(!mixerGroup->dataComplete())
//            return false;
//    }
    return true;
}

MixerGroup* MixersManager::mixerGroupStatus(void) {
    return _mixerGroupsData.getGroup(_actionGroup);
}


bool MixersManager::_requestParameter(unsigned int group, unsigned int index){
    mavlink_message_t       messageOut;

    mavlink_mixer_param_request_read_t req;

    req.mixer_group = group;
    req.index = index;

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mixer_param_request_read_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &req);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckGetParameter);
    return true;
}


//void MixersManager::_writeParameterRaw(MixerGroup* mixerGroup, MixerParameter*, Fact* value, int valueIndex)
//{
//    mavlink_param_set_t     p;
//    mavlink_param_union_t   union_value;

//    FactMetaData::ValueType_t factType = getParameter(componentId, paramName)->type();
//    p.param_type = _factTypeToMavType(factType);

//    switch (factType) {
//        case FactMetaData::valueTypeUint8:
//            union_value.param_uint8 = (uint8_t)value.toUInt();
//            break;

//        case FactMetaData::valueTypeInt8:
//            union_value.param_int8 = (int8_t)value.toInt();
//            break;

//        case FactMetaData::valueTypeUint16:
//            union_value.param_uint16 = (uint16_t)value.toUInt();
//            break;

//        case FactMetaData::valueTypeInt16:
//            union_value.param_int16 = (int16_t)value.toInt();
//            break;

//        case FactMetaData::valueTypeUint32:
//            union_value.param_uint32 = (uint32_t)value.toUInt();
//            break;

//        case FactMetaData::valueTypeFloat:
//            union_value.param_float = value.toFloat();
//            break;

//        default:
//            qCritical() << "Unsupported fact type" << factType;
//            // fall through

//        case FactMetaData::valueTypeInt32:
//            union_value.param_int32 = (int32_t)value.toInt();
//            break;
//    }

//    p.param_value = union_value.param_float;
//    p.target_system = (uint8_t)_vehicle->id();
//    p.target_component = (uint8_t)componentId;

//    strncpy(p.param_id, paramName.toStdString().c_str(), sizeof(p.param_id));

//    mavlink_message_t msg;
//    mavlink_msg_param_set_encode_chan(_mavlink->getSystemId(),
//                                      _mavlink->getComponentId(),
//                                      _vehicle->priorityLink()->mavlinkChannel(),
//                                      &msg,
//                                      &p);
//    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
//}


bool MixersManager::searchAllMixerGroupsAndDownload(void) {
    if(_status != MIXERS_MANAGER_WAITING)
        return false;
    _setStatus(MIXERS_MANAGER_IDENTIFYING_SUPPORTED_GROUPS);
    _actionGroup = 1;
    _retryCount = 0;
    _searchMixerGroup();
    return true;
}

bool MixersManager::_searchMixerGroup()
{
    return _requestParameter(_actionGroup, 0);
}

bool MixersManager::_searchNextMixerGroup()
{
    _retryCount = 0;
    _actionGroup++;
    if(_actionGroup > 1){
        _actionGroup = 0;
        return _downloadMixerGroup();
    }
    return _searchMixerGroup();
}

bool MixersManager::_downloadMixerGroup()
{
    if(getMixerGroup(_actionGroup) == nullptr) {
        return _downloadNextMixerGroup();
    }

    _retryCount = 0;
    return _requestMixerAll(_actionGroup);
}

bool MixersManager::_downloadNextMixerGroup()
{
    _retryCount = 0;
    _actionGroup++;
    if(_actionGroup > 1) {
            _setStatus(MIXERS_MANAGER_WAITING);
            emit mixerDataReadyChanged(true);
        return false;
    }
    return _downloadMixerGroup();
}


bool MixersManager::_requestMixerAll(unsigned int group){
    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    _actionGroup = group;

    command.command = MAV_CMD_REQUEST_MIXER_PARAM_LIST;
    command.param1 = group; //Group

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


void MixersManager::_clearMixerGroupMessages(unsigned int group){
    mavlink_mixer_param_value_t *msg;
    _actionGroup = group;
    QMutableListIterator<mavlink_mixer_param_value_t*> i(_mixerParameterMessages);
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
    _buildAll(group);
}

bool MixersManager::_buildAll(unsigned int group){
    if(!_buildStructureFromMessages(group))
        return false;
    return true;
}


//* Request a missing messages. true if there is missing data */
bool MixersManager::_requestMissingData(unsigned int group){
    _setStatus(MIXERS_MANAGER_DOWNLOADING_MISSING);
    _actionGroup = group;
    _retryCount = 0;



    _mixerDataDownloadComplete(group);
    _downloadNextMixerGroup();
    return false;
}


///* Build mixer structure from messages.  This only includes mixers and submixers with type facts
/// return true if successfull*/
bool MixersManager::_buildStructureFromMessages(unsigned int group){
    MixerGroup *mixer_group = _mixerGroupsData.getGroup(group);
    if(mixer_group == nullptr)
        return false;

    //Delete existing mixer group mixer data - not metadata
    mixer_group->deleteGroupParameters();

    int param_count = _mixerParameterMessages.count();
    for(int index=0; index<param_count; index++) {
        if(_mixerParameterMessages[index]->mixer_group == group)
        {
            mixer_group->appendParameter(new MixerParameter(_mixerParameterMessages[index]) );
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

bool MixersManager::_searchSupportedMixerGroup(unsigned int group)
{
    _setStatus(MixersManager::MIXERS_MANAGER_IDENTIFYING_SUPPORTED_GROUPS);
    _actionGroup = group;
    return _requestParameter(group, 0);
}

MixerGroup* MixersManager::_createMixerGroup(unsigned int group)
{
    MixerGroup *mixerGroup = getMixerGroup(group);
    if(mixerGroup == nullptr) {
        mixerGroup = new MixerGroup(group);
        _mixerGroupsData.addGroup(mixerGroup);
        connect(mixerGroup, &MixerGroup::mixerParamChanged, this, &MixersManager::_changedParamValue);
    }
    return mixerGroup;
}

bool MixersManager::_collectMixerData(const mavlink_mixer_param_value_t* param){
    //Check if this kind of message is in the list already. Add it if it is not there.    
    int dataIndex = -1;
    int count = _mixerParameterMessages.count();
    for(int i=0; i<count; i++){
        if(_mixerParameterMessages[i]->index == param->index){
            dataIndex = i;
        }
    }
    if(dataIndex == -1) {
        mavlink_mixer_param_value_t* newParam = (mavlink_mixer_param_value_t*) malloc(sizeof(mavlink_mixer_param_value_t));
        if(newParam == nullptr) return false;
        memcpy(newParam, param, sizeof(mavlink_mixer_param_value_t));
        _mixerParameterMessages.append(newParam);
    } else {
        memcpy(_mixerParameterMessages[dataIndex], param, sizeof(mavlink_mixer_param_value_t));
    }
    return true;
}

/// Called when a new mavlink message for out vehicle is received
void MixersManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MIXER_PARAM_VALUE: {

        qCDebug(MixersManagerLog) << "Received mixer data";
        mavlink_mixer_param_value_t param;
        mavlink_msg_mixer_param_value_decode(&message, &param);

        //Don't store error data
        if(param.mixer_group < 0){
            _setStatus(MIXERS_MANAGER_WAITING);
            return;
        }

        _collectMixerData(&param);

        // Add a new mixer group if required
        MixerGroup* mixGroup = _mixerGroupsData.getGroup(param.mixer_group);
        if(mixGroup == nullptr){
            mixGroup = _createMixerGroup(param.mixer_group);
        }

        if(param.count > 0) {
            mixGroup->setParamCount(param.count);
        }

        switch(_status){
        case MIXERS_MANAGER_DOWNLOADING_ALL:
            //Check if at end of download
            if(param.index < mixGroup->paramCount()){
                _ackTimeoutTimer.start();
                return;
            } else {
                _requestMissingData(_actionGroup);
            }
            break;
        case MIXERS_MANAGER_IDENTIFYING_SUPPORTED_GROUPS:
            _searchNextMixerGroup();
            break;
        case MIXERS_MANAGER_DOWNLOADING_MISSING:
            _requestMissingData(_actionGroup);
            break;
        case MIXERS_MANAGER_WRITING_PARAM:{
            _updateParamsFromRecievedMessage(&param);
            _setStatus(MIXERS_MANAGER_WAITING);
            _checkWriteParamTimer.start();
            }
            break;
        default:
            _expectedAck = AckNone;
            break;
        }

        _ackTimeoutTimer.stop();

        break;
    }
    default:
        break;
    }
}


void MixersManager::_updateParamsFromRecievedMessage(mavlink_mixer_param_value_t* msg)
{
    _dataMutex.lock();
    if(_waitingWriteParamMap.contains(msg->mixer_group)){
        MixerGroup* group = getMixerGroup(msg->mixer_group);
        if(_waitingWriteParamMap[msg->mixer_group].contains(msg->index)){
            MixerParameter* param = group->getParameter(msg->index);
            foreach(Fact* fact, _waitingWriteParamMap[msg->mixer_group][msg->index].keys() ){
                int arrayIndex = _waitingWriteParamMap[msg->mixer_group][msg->index][fact];
                fact->setRawValue( QVariant::fromValue(msg->param_values[arrayIndex]));
            }
            //Clean up received parameters
            _waitingWriteParamMap[msg->mixer_group].remove(msg->index);
            if(_waitingWriteParamMap[msg->mixer_group].empty()){
                _waitingWriteParamMap.remove(msg->mixer_group);
            }
            _setStatus(MIXERS_MANAGER_WAITING);
        }
    }
    _dataMutex.unlock();
}
