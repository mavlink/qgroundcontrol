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

#include "MixerFacts.h"



MixerParameter::MixerParameter(mavlink_mixer_param_value_t* param_msg, QObject* parent)
    : QObject(parent)
    , _index(param_msg->index)
    , _mixerID(param_msg->mixer_index)
    , _submixerID(param_msg->mixer_sub_index)
    , _parameterID(param_msg->parameter_index)
    , _mixerType(param_msg->mixer_type)
    , _paramType(param_msg->param_type)
    , _paramArraySize(param_msg->param_array_size)
    , _param(new Fact(-1, param_msg->param_id, FactMetaData::valueTypeFloat, this))
{
    _param->setRawValue(param_msg->param_values[0]);
    _param->setObjectName(param_msg->param_id);
}

MixerParameter::MixerParameter(QObject* parent)
    : QObject(parent)
    , _index(-1)
    , _mixerID(-1)
    , _submixerID(-1)
    , _parameterID(-1)
    , _mixerType(-1)
    , _paramType(-1)
    , _paramArraySize(-1)
    , _param(nullptr)
{
}



MixerParameter::~MixerParameter(){

}




MixerGroup::MixerGroup(unsigned int groupID, QObject* parent)
    : QObject(parent)
    , _parameters()
    , _groupID(groupID)
    , _groupName("GROUP_DEFAULT_NAME")
    , _paramCount(-1)
{
};

MixerGroup::~MixerGroup(){
    deleteGroupParameters();
};


void MixerGroup::appendParameter(MixerParameter *param){
    param->setParent(this);
    _parameters.append(param);
}

void MixerGroup::deleteGroupParameters(void){
    //Delete and remove all mixers
    foreach(QObject *paramobj, _parameters){
        delete qobject_cast<MixerParameter *>(paramobj);
    }
    _parameters.clear();
}

MixerParameter* MixerGroup::getParameter(unsigned int paramID)
{
    MixerParameter *param;
    foreach(QObject *paramobj, _parameters){
        param = qobject_cast<MixerParameter *>(paramobj);
        if(param->index() == paramID)
            return param;
    }
    return nullptr;
}


MixerGroups::MixerGroups(QObject* parent)
    : QObject(parent)
    , _mixerGroups()
{
}

MixerGroups::~MixerGroups()
{
    qDeleteAll( _mixerGroups );
    _mixerGroups.clear();
}

void MixerGroups::addGroup(MixerGroup *group){
    unsigned int groupID = group->groupID();
    if(_mixerGroups.contains(groupID))
        delete _mixerGroups.value(groupID);
    group->setParent(this);
    _mixerGroups[groupID] = group;
}

void MixerGroups::deleteGroup(int groupID){
    if(_mixerGroups.contains(groupID)){
        MixerGroup *pgroup = _mixerGroups.value(groupID);
        delete pgroup;
        _mixerGroups.remove(groupID);
    }
}

MixerGroup* MixerGroups::getGroup(int groupID){
    if(_mixerGroups.contains(groupID))
        return _mixerGroups[groupID];
    else
        return nullptr;
}
