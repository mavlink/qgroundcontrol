/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef MIXERFACTS_H
#define MIXERFACTS_H

#include <QObject>
#include <Fact.h>
#include <FactMetaData.h>
#include <QMap>

class MixerConnection : public QObject
{
    Q_OBJECT

public:
    MixerConnection(unsigned int connGroup, unsigned int connChannel);
    ~MixerConnection();

private:
    Fact    _connGroup;
    Fact    _connChannel;
};



class Mixer : public QObject
{
    Q_OBJECT

public:
    Mixer(unsigned int typeID);
    ~Mixer();
    void addSubmixer(unsigned int mixerID, Mixer *submixer);
    void addMixerParamFact(unsigned int paramID, Fact* paramFact);
    void addConnection(unsigned int connType, unsigned int connID, unsigned int connGroup, unsigned int connChannel);

private:
    unsigned _mixerTypeID;
//    MixerMetaData*  _mixerMetaData;
    QMap<int, Mixer*> _subMixers;

    //Map of mixer parameter Fact values
    QMap<int, Fact*> _mixerParamFacts;

    // Connections arranged as Map of types containing map of connection index
    QMap<int, QMap<int, MixerConnection*> > _mixerConnections;
};


class MixerGroup : public QObject
{
    Q_OBJECT

public:
    MixerGroup();
    ~MixerGroup();
    void addMixer(unsigned int mixerID, Mixer *mixer);
private:
    QMap<int, Mixer*> _mixers ;
};

class MixerGroups : public QObject
{
    Q_OBJECT

public:
    MixerGroups();
    ~MixerGroups();
    void deleteGroup(unsigned int groupID);
    void addGroup(unsigned int groupID, MixerGroup *group);

private:
    QMap<int, MixerGroup*> _mixerGroups;
};


#endif // MIXERFACTS_H
