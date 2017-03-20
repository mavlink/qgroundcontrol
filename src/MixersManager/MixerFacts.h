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
#include <MixerMetaData.h>
#include <QMap>
#include <QmlObjectListModel.h>
#include <QMetaType>

class MixerConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerConnection)

    Q_PROPERTY(int group       READ group         CONSTANT)
    Q_PROPERTY(int channel     READ channel       CONSTANT)

public:
    MixerConnection(int connGroup = -1, int connChannel = -1, QObject* parent = NULL);
    ~MixerConnection();

    int group              (void) const { return _connGroup; }
    int channel            (void) const { return _connChannel; }

    void setGroup(int group) {_connGroup = group;}
    void setChannel(int channel) {_connChannel = channel;}

protected:
    int     _connGroup;
    int     _connChannel;
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

    Mixer*  getSubmixer(int mixerID);
    Fact*   getParameter(int paramIndex);
    void appendSubmixer(int mixerID, Mixer *submixer);
    void appendParamFact(Fact* paramFact);
    void appendInputConnection(MixerConnection *inputConn);
    void appendOutputConnection(MixerConnection *outputConn);
//    void addConnection(unsigned int connType, unsigned int connID, unsigned int connGroup, unsigned int connChannel);

protected:
    QmlObjectListModel    _parameters;
    QmlObjectListModel    _submixers;
    QmlObjectListModel    _inputConnections;
    QmlObjectListModel    _outputConnections;
    Fact*                 _mixer;
};


Q_DECLARE_METATYPE(Mixer*)


class MixerGroup : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerGroup)

    Q_PROPERTY(QString          groupName   READ groupName     CONSTANT)
    Q_PROPERTY(unsigned int     groupID     READ groupID       CONSTANT)

    enum {
        MIXERGROUP_STRUCTURE_CREATED = 0x01,
        MIXERGROUP_PARAMETERS_CREATED = 0x02,
        MIXERGROUP_PARAMETER_VALUES_SET = 0x04,
        MIXERGROUP_CONNECTIONS_CREATED = 0x08,
        MIXERGROUP_CONNECTION_VALUES_SET = 0x10,
        MIXERGROUP_CONNECTION_ALIASES_SET = 0x20,
        MIXERGROUP_DATA_COMPLETE = 0x40,

//        MIXERGROUP_DOWNLOADED_CAPABILITIES = 0x100,
        MIXERGROUP_DOWNLOADED_STREAM_ALL = 0x200,
        MIXERGROUP_DOWNLOADED_MISSING = 0x400,
//        MIXERGROUP_DOWNLOADED_MIXER_SCRIPT = 0x800,
//        MIXERGROUP_DOWNLOADED_MIXER_DESCRIPTIONS = 0x1000,
//        MIXERGROUP_DOWNLOADED_CONNECTION_DESCIPTIONS = 0x2000,
//        MIXERGROUP_DOWNLOADED_DEBUG_DATA = 0x4000
        MIXERGROUP_GROUP_EXISTS = 0x8000,
    };

public:
    MixerGroup(unsigned int groupID=0, QObject* parent = NULL);
    ~MixerGroup();

    // Parameters (Mixer private constants or variables)
    QObjectList mixers              (void) const { return _mixers; }
    MixerMetaData* getMixerMetaData(void) {return &_mixerMetaData;}

    Mixer* getMixer(int mixerID);
    void appendMixer(int mixerID, Mixer *mixer);
    unsigned int getGroupStatus(void) {return _groupStatus;}
    void setGroupStatusFlags(unsigned int flags) {_groupStatus |= flags;}
    void deleteGroupMixers(void);
    bool dataComplete(void) {return ((_groupStatus & MIXERGROUP_DATA_COMPLETE) != 0);}

    unsigned int groupID(void) {return _groupID;}
    void setGroupID(unsigned int groupID) {_groupID = groupID;}

    QString groupName(void) {return _groupName;}
    void setGroupName(QString groupName) {_groupName = groupName;}

private:
    QObjectList     _mixers ;
    MixerMetaData   _mixerMetaData;
    unsigned int    _groupStatus;
    unsigned int    _groupID;
    QString         _groupName;
};


class MixerGroups : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerGroups)

public:
    MixerGroups(QObject* parent = NULL);
    ~MixerGroups();
    void deleteGroup(int groupID);
    void addGroup(MixerGroup *group);
    MixerGroup* getGroup(int groupID);
    QMap<int, MixerGroup*>* getMixerGroups(void) {return &_mixerGroups;}

private:
    QMap<int, MixerGroup*> _mixerGroups;
};


#endif // MIXERFACTS_H
