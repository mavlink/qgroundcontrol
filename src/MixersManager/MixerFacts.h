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
#include <QmlObjectListModel.h>
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
    Mixer(Fact *mixerFact = NULL, QObject* parent = NULL);
    ~Mixer();

    Q_PROPERTY(QmlObjectListModel*  parameters          READ parameters             CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  submixers           READ submixers              CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  inputConnections    READ inputConnections       CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  outputConnections   READ outputConnections      CONSTANT)

    Q_PROPERTY(Fact*        mixer                READ mixer              CONSTANT)

    // Parameters (Mixer private constants or variables as Fact object)
    QmlObjectListModel* parameters             (void) { return &_parameters; }

    // Submixers of object type MixerFact
    QmlObjectListModel* submixers              (void) { return &_submixers; }

    // Input connections
    QmlObjectListModel* inputConnections       (void) { return &_inputConnections; }

    // Output connections
    QmlObjectListModel* outputConnections      (void) { return &_outputConnections; }

    // Main mixer Fact describing mixer type
    Fact *mixer                         (void) const { return _mixer; }

    Mixer*  getSubmixer(unsigned int mixerID);
    Fact*   getParameter(unsigned int paramIndex);
    void appendSubmixer(unsigned int mixerID, Mixer *submixer);
    void appendParamFact(Fact* paramFact);
//    void addConnection(unsigned int connType, unsigned int connID, unsigned int connGroup, unsigned int connChannel);

protected:
    QmlObjectListModel    _parameters;
    QmlObjectListModel    _submixers;
    QmlObjectListModel    _inputConnections;
    QmlObjectListModel    _outputConnections;
    Fact*           _mixer;
};


Q_DECLARE_METATYPE(Mixer*)


class MixerGroup : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerGroup)

public:
    MixerGroup(QObject* parent = NULL);
    ~MixerGroup();

    // Parameters (Mixer private constants or variables)
    QObjectList mixers              (void) const { return _mixers; }

    Mixer* getMixer(unsigned int mixerID);
    void appendMixer(unsigned int mixerID, Mixer *mixer);

private:
    QObjectList _mixers ;
};


class MixerGroups : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerGroups)

public:
    MixerGroups(QObject* parent = NULL);
    ~MixerGroups();
    void deleteGroup(unsigned int groupID);
    void addGroup(unsigned int groupID, MixerGroup *group);
    MixerGroup* getGroup(unsigned int groupID);

private:
    QMap<int, MixerGroup*> _mixerGroups;
};


#endif // MIXERFACTS_H
