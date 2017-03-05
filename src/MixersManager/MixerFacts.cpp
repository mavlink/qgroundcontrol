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

//MixerFacts::MixerFacts(QObject *parent) : QObject(parent)
//{

////    Fact* fact = new Fact(componentId, parameterName, factType, this);

////    _mapParameterName2Variant[componentId][parameterName] = QVariant::fromValue(fact);

////    // We need to know when the fact changes from QML so that we can send the new value to the parameter manager
////    connect(fact, &Fact::_containerRawValueChanged, this, &ParameterManager::_valueUpdated);
//}



MixerConnection::MixerConnection(unsigned int connGroup, unsigned int connChannel)
    :_connGroup(-1, QString("CONN_GROUP"), FactMetaData::valueTypeInt16, this)
    ,_connChannel(-1, QString("CONN_CHANNEL"), FactMetaData::valueTypeInt16, this)
{
    _connGroup.setRawValue(connGroup);
    _connChannel.setRawValue(connChannel);
}

MixerConnection::~MixerConnection(){
}



Mixer::Mixer(unsigned int typeID)
    : _mixerTypeID(typeID)
    , _subMixers()
    , _mixerParamFacts()
    , _mixerConnections()
{
}

Mixer::~Mixer(){
    qDeleteAll( _subMixers );  //  deletes all the values stored in "map"
    _subMixers.clear();        //  removes all items from the map

    qDeleteAll( _mixerParamFacts );  //  deletes all the values stored in "map"
    _mixerParamFacts.clear();        //  removes all items from the map

    qDeleteAll( _mixerParamFacts );  //  deletes all the values stored in "map"
    _mixerParamFacts.clear();        //  removes all items from the map
}


void Mixer::addSubmixer(unsigned int mixerID, Mixer *submixer){
    if(_subMixers.contains(mixerID))
        delete _subMixers.value(mixerID);
    _subMixers[mixerID] = submixer;
}

void Mixer::addMixerParamFact(unsigned int paramID, Fact* paramFact){
    if(_mixerParamFacts.contains(paramID))
        delete _mixerParamFacts.value(paramID);
    _mixerParamFacts[paramID] = paramFact;
}

void Mixer::addConnection(unsigned int connType, unsigned int connID, unsigned int connGroup, unsigned int connChannel){
    if(_mixerConnections.contains(connType))
        if(_mixerConnections[connType].contains(connID))
        delete _mixerConnections[connType][connID];
    _mixerConnections[connType][connID] = new MixerConnection(connGroup , connChannel);
}

MixerGroup::MixerGroup()
    :_mixers()
{
};

MixerGroup::~MixerGroup(){
    qDeleteAll( _mixers );
    _mixers.clear();
};

void MixerGroup::addMixer(unsigned int mixerID, Mixer *mixer){
    if(_mixers.contains(mixerID))
        delete _mixers.value(mixerID);
    _mixers[mixerID] = mixer;
}



MixerGroups::MixerGroups()
    :_mixerGroups()
{

}

MixerGroups::~MixerGroups()
{
    qDeleteAll( _mixerGroups );
    _mixerGroups.clear();
}

void MixerGroups::addGroup(unsigned int groupID, MixerGroup *group){
    if(_mixerGroups.contains(groupID))
        delete _mixerGroups.value(groupID);
    _mixerGroups[groupID] = group;

}

void MixerGroups::deleteGroup(unsigned int groupID){
    if(_mixerGroups.contains(groupID)){
        MixerGroup *pgroup = _mixerGroups.value(groupID);
        delete pgroup;
        _mixerGroups.remove(groupID);
    }

}
