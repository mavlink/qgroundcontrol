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
    , _actionParameter(0)
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
    //Not used yet.  Used for writing individual array items instead of whole parameter
    Q_UNUSED(value);
    Q_UNUSED(valueIndex);

    MixerGroup* mixGroup = qobject_cast<MixerGroup *>(QObject::sender());

    // Put write request in list and reset retry counter
    _dataMutex.lock();
    _waitingWriteParamMap[mixGroup->groupID()][mixParam->index()] = 0;
    _dataMutex.unlock();

    _checkWriteParamTimer.start();
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
        _sendPendingWriteTimeout();
        break;
    }
    case MIXERS_MANAGER_STORING_PARAMS : {
        _setStatus(MIXERS_MANAGER_WAITING);
        _expectedAck = AckNone;
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

void MixersManager::_sendPendingWriteTimeout(void){
    //Check if the expected group and paraemter is expected.
    // Increment retry and remove item it it has exceeded maximum
    _dataMutex.lock();
    if(_waitingWriteParamMap.contains(_actionGroup)){
        if(_waitingWriteParamMap[_actionGroup].contains(_actionParameter)){
            _waitingWriteParamMap[_actionGroup][_actionParameter]++;
            if(_waitingWriteParamMap[_actionGroup][_actionParameter] > _maxRetryCount){
                _waitingWriteParamMap[_actionGroup].remove(_actionParameter);
                if(_waitingWriteParamMap[_actionGroup].empty()){
                    _waitingWriteParamMap.remove(_actionGroup);
                }
            }
        }
    }
    _dataMutex.unlock();

    _sendPendingWriteParam();
}

void MixersManager::_sendPendingWriteParam(void){

    if(_waitingWriteParamMap.count() > 0){

        _dataMutex.lock();
        int mixGroupID = _waitingWriteParamMap.firstKey();
        int paramIndex = _waitingWriteParamMap.first().firstKey();
        _dataMutex.unlock();

        mavlink_message_t       messageOut;
        mavlink_mixer_param_set_t      set;

        MixerParameter* param = getMixerGroup(mixGroupID)->getParameter(paramIndex);
        Q_ASSERT(param);

        _actionGroup = mixGroupID;
        _actionParameter = paramIndex;

        set.mixer_group = mixGroupID;
        set.index = paramIndex;
        set.mixer_index = 0;
        set.mixer_sub_index = 0;
        set.mixer_type = 0;
        strcpy(set.param_id, "");
        set.param_type = param->paramType();
        memset(set.param_values, 0, sizeof(set.param_values));

        //Set each value in the array accoring to type
        // TODO Handle different parameter types here. Now hard coded to float.
        Fact* value;
        for(int i=0; i<param->values()->count(); i++) {
            value = qobject_cast<Fact*>(param->values()->get(i));
            set.param_values[i] = value->rawValue().toFloat();
        }

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

    MixerGroup* mixerGroup;
    foreach(mixerGroup, _mixerGroupsData.getMixerGroups()->values()){
        if(!mixerGroup->isComplete())
            return false;
    }
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

bool MixersManager::requestStoreParams(unsigned int group){
    if(_status != MIXERS_MANAGER_WAITING)
        return false;

    mavlink_message_t       messageOut;
    mavlink_command_long_t  command;

    _actionGroup = group;
    _retryCount = 0;

    command.command = MAV_CMD_SAVE_MIXER_PARAMS;
    command.param1 = group; //Group

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &command);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _setStatus(MIXERS_MANAGER_STORING_PARAMS);
    _startAckTimeout(AckStore);
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
    if(_actionGroup > 2){
        _actionGroup = 1;
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
    if(_actionGroup > 2) {
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

    MixerGroup* mixerGroup = getMixerGroup(group);
    //Check if group exists yet.  If not just stop.
    if(mixerGroup==nullptr){
        _setStatus(MIXERS_MANAGER_WAITING);
        return false;
    }
    bool found;
    for(int i=0; i<mixerGroup->paramCount(); i++ ){
        found = false;
        foreach(mavlink_mixer_param_value_t* msg, _mixerParameterMessages){
            if(msg->index == i) {
                found = true;
                break;
            }
        }
        if(!found)
            return _requestParameter(_actionGroup, i);
    }

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

    int param_count = mixer_group->paramCount();
    int check = 0;
    int scanIndex = 0;
    while(scanIndex < param_count)
    {
        foreach(mavlink_mixer_param_value_t* msg ,_mixerParameterMessages){
            if( (msg->mixer_group == group) && (msg->index == scanIndex)){
                mixer_group->appendParameter(new MixerParameter(msg));
                scanIndex++;
            }
        }
        check++;
        if(check > _mixerParameterMessages.count())
            return false;
    }

    mixer_group->setComplete();
    return true;
}


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
        if( (_mixerParameterMessages[i]->index == param->index) &&
            (_mixerParameterMessages[i]->mixer_group == param->mixer_group) ) {
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
    //Update percentage done
    int total_params = 0;
    foreach(MixerGroup* group, getMixerGroups()->getMixerGroups()->values()){
        total_params += group->paramCount();
    }
    if(total_params <= 0){
        emit downloadPercentChanged(0.0);
    } else {
        float percent = 100.0 * (float) _mixerParameterMessages.count() / (float) total_params;
        emit downloadPercentChanged(percent);
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
        if(param.count < 0){
            _removeParamFromWaiting(&param);
            _setStatus(MIXERS_MANAGER_WAITING);
            _sendPendingWriteParam();
            return;
        }

        // Test case to miss some data on purpose
//        if(_status != MIXERS_MANAGER_DOWNLOADING_ALL){
//            _collectMixerData(&param);
//        } else {
//            if(param.index%4 != 1){
//                _collectMixerData(&param);
//            }
//        }
        _collectMixerData(&param);

        // Add a new mixer group if required
        MixerGroup* mixGroup = _mixerGroupsData.getGroup(param.mixer_group);
        if(mixGroup == nullptr){
            mixGroup = _createMixerGroup(param.mixer_group);
        }

        if(param.count > 0){
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
        //Check if group is complete.  Don't update if not complete.
        if(group->isComplete()){
            if(_waitingWriteParamMap[msg->mixer_group].contains(msg->index)){
                MixerParameter* param = group->getParameter(msg->index);
                Q_ASSERT(param);
                Fact* value;
//                Q_ASSERT(param->arraySize() == msg->param_array_size);
//                Q_ASSERT(param->paramType() == msg->mixer_type);
                for(int i=0; i<msg->param_array_size; i++){
                    value = qobject_cast<Fact *>(param->values()->get(i));
                    //TODO Fixed float value for now.
                    value->setSendValueChangedSignals(false);
                    value->setRawValue(QVariant::fromValue(msg->param_values[i]));
                    value->sendDeferredValueChangedSignal();
                    value->setSendValueChangedSignals(true);
                }
                _setStatus(MIXERS_MANAGER_WAITING);
            }
        }
    }
    _dataMutex.unlock();
    _removeParamFromWaiting(msg);
}

void MixersManager::_removeParamFromWaiting(mavlink_mixer_param_value_t* msg){
    //Clean up received parameters
    _dataMutex.lock();
    if(_waitingWriteParamMap.contains(msg->mixer_group)){
        _waitingWriteParamMap[msg->mixer_group].remove(msg->index);
        if(_waitingWriteParamMap[msg->mixer_group].empty()){
            _waitingWriteParamMap.remove(msg->mixer_group);
        }
    }
    _dataMutex.unlock();
}
