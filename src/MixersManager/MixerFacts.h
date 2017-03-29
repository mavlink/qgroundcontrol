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

#include "QGCMAVLink.h"
//#include "LinkInterface.h"

#include <QObject>
#include <Fact.h>
#include <FactMetaData.h>
#include <QMap>
#include <QMetaType>


class MixerParameter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerParameter)

    Q_PROPERTY(int      index       READ index      CONSTANT)
    Q_PROPERTY(int      mixerID       READ mixerID      CONSTANT)
    Q_PROPERTY(int      submixerID       READ submixerID      CONSTANT)
    Q_PROPERTY(int      mixerType       READ mixerType      CONSTANT)
    Q_PROPERTY(int      paramType       READ paramType      CONSTANT)
    Q_PROPERTY(Fact*    param       READ param      CONSTANT)


public:
    MixerParameter(mavlink_mixer_param_value_t* param_msg, QObject* parent = NULL);
    MixerParameter(QObject* parent = NULL);
    ~MixerParameter();

    // Main mixer Fact describing mixer type    
    int index(void) {return _index;}
    int mixerID(void) {return _mixerID;}
    int submixerID(void) {return _submixerID;}
    int mixerType(void) {return _mixerType;}
    int paramType(void) {return _paramType;}
    Fact* param(void) {return _param;}

protected:
    int             _index;
    int             _mixerID;
    int             _submixerID;
    int             _parameterID;
    int             _mixerType;
    int             _paramType;
    int             _paramArraySize;
    Fact*           _param;
};

Q_DECLARE_METATYPE(MixerParameter*)


class MixerGroup : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerGroup)

    Q_PROPERTY(QString          groupName   READ groupName     CONSTANT)
    Q_PROPERTY(unsigned int     groupID     READ groupID       CONSTANT)
    Q_PROPERTY(int              paramCount  READ paramCount    CONSTANT)

public:
    MixerGroup(unsigned int groupID=0, QObject* parent = NULL);
    ~MixerGroup();

    // Parameters (MixerParameter objects)
    QObjectList parameters              (void) const { return _parameters; }

    void appendParameter(MixerParameter *param);
    void deleteGroupParameters(void);
    MixerParameter* getParameter(unsigned int paramID);

    unsigned int groupID(void) {return _groupID;}
    void setGroupID(unsigned int groupID) {_groupID = groupID;}

    QString groupName(void) {return _groupName;}
    void setGroupName(QString groupName) {_groupName = groupName;}

    int paramCount(void) {return _paramCount;}
    void setParamCount(int count) {_paramCount = count;}

private:
    QObjectList     _parameters ;
    unsigned int    _groupID;
    QString         _groupName;
    int             _paramCount;
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
