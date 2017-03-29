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
    if(!_groupsIndentified)
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
            mixGroup = new MixerGroup(param.mixer_group);
            Q_ASSERT(mixGroup != nullptr);
            _mixerGroupsData.addGroup(mixGroup);
        }

        if(param.count > 0) {
            mixGroup->setParamCount(param.count);
        }

        switch(_status){
        case MIXERS_MANAGER_DOWNLOADING_ALL:
            //Check if at end of download
            if(param.index < mixGroup->paramCount()){
                _ackTimeoutTimer->start();
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
        default:
            _expectedAck = AckNone;
            break;
        }

        _ackTimeoutTimer->stop();

        break;
    }
    default:
        break;
    }

}
