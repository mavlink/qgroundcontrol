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


MixerConnection::MixerConnection(int connGroup, int connChannel, QObject* parent)
    : QObject(parent)
    ,_connGroup(-1, QString("CONN_GROUP"), FactMetaData::valueTypeInt16, this)
    ,_connChannel(-1, QString("CONN_CHANNEL"), FactMetaData::valueTypeInt16, this)
{
    _connGroup.setRawValue(connGroup);
    _connChannel.setRawValue(connChannel);
}

MixerConnection::~MixerConnection(){
}

////    connect(fact, &Fact::_containerRawValueChanged, this, &ParameterManager::_valueUpdated);



Mixer::Mixer(QObject* parent)
    : QObject(parent)
    , _parameters()
    , _submixers()
    , _inputConnections()
    , _outputConnections()
    , _mixer()
{
}



Mixer::~Mixer(){
//    int index;
//    for(index=0; index<_submixers.count(); index++){

//    }
    //Delete and remove all submixers
    foreach(QVariant mixvar, _submixers){
        delete qobject_cast<Mixer *>(qvariant_cast<QObject *>(mixvar));
    }
    _submixers.clear();

    //Delete and remove all parameters
    foreach(QVariant paramvar, _parameters){
        delete qobject_cast<Fact *>(qvariant_cast<QObject *>(paramvar));
    }
    _parameters.clear();

    //Delete and remove all connetions
    foreach(QVariant connvar, _inputConnections){
        delete qobject_cast<MixerConnection *>(qvariant_cast<QObject *>(connvar));
    }
    _inputConnections.clear();
    foreach(QVariant connvar, _outputConnections){
        delete qobject_cast<MixerConnection *>(qvariant_cast<QObject *>(connvar));
    }
    _outputConnections.clear();
}


Mixer* Mixer::getSubmixer(unsigned int mixerID){
    if(!_submixers.contains(mixerID))
        return nullptr;
    QObject * obj = qvariant_cast<QObject *>(_submixers.value(mixerID));
    return qobject_cast<Mixer *>(obj);
}

void Mixer::addSubmixer(unsigned int mixerID, Mixer *submixer){
    QVariant var;
    if(_submixers.contains(mixerID))
        var = _submixers.value(mixerID);
        QObject * obj = qvariant_cast<QObject *>(var);
        Mixer * del = qobject_cast<Mixer *>(obj);
        delete del;
    var.fromValue(submixer);
    _submixers[mixerID] = var;
    submixer->setParent(this);
}

void Mixer::addMixerParamFact(unsigned int paramID, Fact* paramFact){
//    if(_parameters.contains(paramID))
//        delete _parameters.value(paramID);
//    _parameters[paramID] = paramFact;
}

//void Mixer::addConnection(unsigned int connType, unsigned int connID, unsigned int connGroup, unsigned int connChannel){
//    if(_mixerConnections.contains(connType))
//        if(_mixerConnections[connType].contains(connID))
//        delete _mixerConnections[connType][connID];
//    _mixerConnections[connType][connID] = new MixerConnection(connGroup , connChannel);
//}

MixerGroup::MixerGroup(QObject* parent)
    : QObject(parent)
    ,_mixers()
{
};

MixerGroup::~MixerGroup(){
    //Delete and remove all parameters
    foreach(QVariant mixvar, _mixers){
        delete qobject_cast<Fact *>(qvariant_cast<QObject *>(mixvar));
    }
    _mixers.clear();
};

Mixer* MixerGroup::getMixer(unsigned int mixerID){
    if(!_mixers.contains(mixerID))
        return nullptr;
    QObject * obj = qvariant_cast<QObject *>(_mixers.value(mixerID));
    return qobject_cast<Mixer *>(obj);
}

void MixerGroup::addMixer(unsigned int mixerID, Mixer *mixer){
    QVariant var;
    if(_mixers.contains(mixerID))
        var = _mixers.value(mixerID);
        QObject * obj = qvariant_cast<QObject *>(var);
        Mixer * del = qobject_cast<Mixer *>(obj);
        delete del;
    var.fromValue(mixer);
    _mixers[mixerID] = var;
    mixer->setParent(this);
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

MixerGroup* MixerGroups::getGroup(unsigned int groupID){
    if(_mixerGroups.contains(groupID))
        return _mixerGroups[groupID];
    else
        return nullptr;
}
