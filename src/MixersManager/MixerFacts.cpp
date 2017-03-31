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
    , _paramName(param_msg->param_id)
    , _values(new QmlObjectListModel(this))
{
    Fact* newValue;
    QString valueName;
    for(int i=0; i<_paramArraySize; i++){
        valueName = "%1[%2]";
        newValue = new Fact(-1, valueName.arg(_paramName).arg(i), FactMetaData::valueTypeFloat, _values);
        newValue->setRawValue(param_msg->param_values[i]);
        _values->append(newValue);

        //Do this afer setting initial raw value
        connect(newValue, &Fact::rawValueChanged, this, &MixerParameter::_changedParamValue);
    }
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
    , _paramName("NONE")
    , _values(nullptr)
{
}

void MixerParameter::_changedParamValue(QVariant value){
    if(_values->contains(QObject::sender())){
        int index = _values->indexOf(QObject::sender());
        Fact* fact = qobject_cast<Fact *>(QObject::sender());
        emit (mixerParamChanged(fact , index));
    }
}


MixerParameter::~MixerParameter(){
    delete _values;
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
    connect(param, &MixerParameter::mixerParamChanged, this, &MixerGroup::_changedParamValue);
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


void MixerGroup::_changedParamValue(Fact *value, int valueIndex)
{
    if(!_parameters.contains(QObject::sender()))
        return;
    MixerParameter *param = qobject_cast<MixerParameter *>(QObject::sender());
     emit (mixerParamChanged(param, value, valueIndex));
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
