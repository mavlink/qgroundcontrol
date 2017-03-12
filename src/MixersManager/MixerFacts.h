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
#include <QVariantList>
#include <QMetaType>

class MixerConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerConnection)

    Q_PROPERTY(Fact group       READ group         CONSTANT)
    Q_PROPERTY(Fact channel     READ channel       CONSTANT)

public:
    MixerConnection(int connGroup = -1, int connChannel = -1, QObject* parent = NULL);
    ~MixerConnection();

    Fact group              (void) const { return _connGroup; }
    Fact channel            (void) const { return _connChannel; }

protected:
    Fact    _connGroup;
    Fact    _connChannel;
};

Q_DECLARE_METATYPE(MixerConnection*)

class Mixer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Mixer)

public:
    Mixer(QObject* parent = NULL);
    ~Mixer();

    Q_PROPERTY(QVariantList parameters           READ parameters         CONSTANT)
    Q_PROPERTY(QVariantList submixers            READ submixers          CONSTANT)
    Q_PROPERTY(QVariantList inputConnections     READ inputConnections   CONSTANT)
    Q_PROPERTY(QVariantList outputConnections    READ outputConnections  CONSTANT)
    Q_PROPERTY(Fact mixer                        READ mixer              CONSTANT)

    // Parameters (Mixer private constants or variables as Fact object)
    QVariantList parameters             (void) const { return _parameters; }

    // Submixers of object type MixerFact
    QVariantList submixers              (void) const { return _submixers; }

    // Input connections
    QVariantList inputConnections       (void) const { return _inputConnections; }

    // Output connections
    QVariantList outputConnections      (void) const { return _outputConnections; }

    // Output connections
    Fact mixer                          (void) const { return _mixer; }

    Mixer* getSubmixer(unsigned int mixerID);
    void addSubmixer(unsigned int mixerID, Mixer *submixer);
    void addMixerParamFact(unsigned int paramID, Fact* paramFact);
//    void addConnection(unsigned int connType, unsigned int connID, unsigned int connGroup, unsigned int connChannel);

protected:
    QVariantList    _parameters;
    QVariantList    _submixers;
    QVariantList    _inputConnections;
    QVariantList    _outputConnections;
    Fact            _mixer;
};


Q_DECLARE_METATYPE(Mixer*)


class MixerGroup : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerGroup)

public:
    MixerGroup(QObject* parent = NULL);
    ~MixerGroup();

    Q_PROPERTY(QVariantList mixers    READ mixers  CONSTANT)

    // Parameters (Mixer private constants or variables)
    QVariantList mixers              (void) const { return _mixers; }

    Mixer* getMixer(unsigned int mixerID);
    void addMixer(unsigned int mixerID, Mixer *mixer);

private:
    QVariantList _mixers ;
};

Q_DECLARE_METATYPE(MixerGroup*)


class MixerGroups : public QObject
{
    Q_OBJECT

public:
    MixerGroups();
    ~MixerGroups();
    void deleteGroup(unsigned int groupID);
    void addGroup(unsigned int groupID, MixerGroup *group);
    MixerGroup* getGroup(unsigned int groupID);

private:
    QMap<int, MixerGroup*> _mixerGroups;
};


#endif // MIXERFACTS_H
