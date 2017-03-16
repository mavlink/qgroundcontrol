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
    ,_connGroup(connGroup)
    ,_connChannel(connChannel)
{
}

MixerConnection::~MixerConnection(){
}

////    connect(fact, &Fact::_containerRawValueChanged, this, &ParameterManager::_valueUpdated);



Mixer::Mixer(Fact *mixerFact, QObject* parent)
    : QObject(parent)
    , _parameters(this)
    , _submixers(this)
    , _inputConnections(this)
    , _outputConnections(this)
    , _mixer(mixerFact)
{
}



Mixer::~Mixer(){
    //Delete and remove all content
    _submixers.clearAndDeleteContents();
    _parameters.clearAndDeleteContents();
    _inputConnections.clearAndDeleteContents();
    _outputConnections.clearAndDeleteContents();
}


Mixer* Mixer::getSubmixer(int mixerID){
    if(mixerID > _submixers.count())
        return nullptr;
    if(mixerID == 0)
        return nullptr;
    return qobject_cast<Mixer *>(_submixers[mixerID-1]);
}

Fact* Mixer::getParameter(int paramIndex){
    if(paramIndex >= _parameters.count())
        return nullptr;
    return qobject_cast<Fact *>(_parameters[paramIndex]);
}


void Mixer::appendSubmixer(int mixerID, Mixer *submixer){
    Q_CHECK_PTR(submixer);
    submixer->setParent(&_submixers);
    _submixers.append(submixer);
    Q_ASSERT(mixerID == _submixers.count());
}

void Mixer::appendParamFact(Fact* paramFact){
    Q_CHECK_PTR(paramFact);
    paramFact->setParent(&_parameters);
    _parameters.append(paramFact);
}

void Mixer::appendInputConnection(MixerConnection *inputConn)
{
    Q_CHECK_PTR(inputConn);
    inputConn->setParent(&_inputConnections);
    _inputConnections.append(inputConn);
}

void Mixer::appendOutputConnection(MixerConnection *outputConn)
{
    Q_CHECK_PTR(outputConn);
    outputConn->setParent(&_outputConnections);
    _outputConnections.append(outputConn);
}


//void Mixer::addConnection(unsigned int connType, unsigned int connID, unsigned int connGroup, unsigned int connChannel){
//    if(_mixerConnections.contains(connType))
//        if(_mixerConnections[connType].contains(connID))
//        delete _mixerConnections[connType][connID];
//    _mixerConnections[connType][connID] = new MixerConnection(connGroup , connChannel);
//}

MixerGroup::MixerGroup(unsigned int groupID, QObject* parent)
    : QObject(parent)
    , _mixers()
    , _mixerMetaData()
    , _groupStatus(0)
    , _groupID(groupID)
{
};

MixerGroup::~MixerGroup(){
    deleteGroupMixers();
};

Mixer* MixerGroup::getMixer(int mixerID){
    if(mixerID >= _mixers.count())
        return nullptr;
    return qobject_cast<Mixer *>(_mixers[mixerID]);
}

void MixerGroup::appendMixer(int mixerID, Mixer *mixer){
    mixer->setParent(this);
    _mixers.append(mixer);
    Q_ASSERT(mixerID == _mixers.count()-1);
}

void MixerGroup::deleteGroupMixers(void){
    //Delete and remove all mixers
    foreach(QObject *mixobj, _mixers){
        delete qobject_cast<Fact *>(mixobj);
    }
    _mixers.clear();

    _groupStatus &= !(  MIXERGROUP_STRUCTURE_CREATED
                      | MIXERGROUP_PARAMETERS_CREATED
                      | MIXERGROUP_PARAMETER_VALUES_SET
                      | MIXERGROUP_CONNECTIONS_CREATED
                      | MIXERGROUP_CONNECTION_VALUES_SET
                      | MIXERGROUP_CONNECTION_ALIASES_SET
                      | MIXERGROUP_DATA_COMPLETE);
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
