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
#include <QmlObjectListModel.h>


class MixerParameter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerParameter)

    Q_PROPERTY(unsigned int         index           READ index          CONSTANT)
    Q_PROPERTY(unsigned int         mixerID         READ mixerID        CONSTANT)
    Q_PROPERTY(unsigned int         submixerID      READ submixerID     CONSTANT)
    Q_PROPERTY(unsigned int         mixerType       READ mixerType      CONSTANT)
    Q_PROPERTY(unsigned int         paramType       READ paramType      CONSTANT)
    Q_PROPERTY(QString              paramName       READ paramName      CONSTANT)
    Q_PROPERTY(bool                 readOnly        READ readOnly       CONSTANT)
    Q_PROPERTY(unsigned int         arraySize       READ arraySize      CONSTANT)
    Q_PROPERTY(QString              valuesString    READ valuesString   CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  values          READ values         CONSTANT)


public:
    MixerParameter(mavlink_mixer_param_value_t* param_msg, QObject* parent = NULL);
    MixerParameter(QObject* parent = NULL);
    ~MixerParameter();

    // Main mixer Fact describing mixer type    
    unsigned int index(void) {return _index;}
    unsigned int mixerID(void) {return _mixerID;}
    unsigned int submixerID(void) {return _submixerID;}
    unsigned int mixerType(void) {return _mixerType;}
    unsigned int paramType(void) {return _paramType;}
    unsigned int arraySize(void) {return _paramArraySize;}
    QString valuesString(void);
    bool readOnly(void) {return _flags!=0;}
    QString  paramName(void) {return _paramName;}
    QmlObjectListModel* values(void) {return _values;}

signals:
    void mixerParamChanged(Fact* value, int valueIndex);

private slots:
    void _changedParamValue(QVariant value);

protected:
    unsigned int    _index;
    unsigned int    _mixerID;
    unsigned int    _submixerID;
    unsigned int    _parameterID;
    unsigned int    _mixerType;
    unsigned int    _paramType;
    unsigned int    _paramArraySize;
    unsigned int    _flags;
    QString         _paramName;
    QmlObjectListModel* _values;
};

Q_DECLARE_METATYPE(MixerParameter*)



class MixerGroup : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MixerGroup)

    Q_PROPERTY(QString          groupName   READ groupName     CONSTANT)
    Q_PROPERTY(unsigned int     groupID     READ groupID       CONSTANT)
    Q_PROPERTY(unsigned int     paramCount  READ paramCount    CONSTANT)
    Q_PROPERTY(bool             complete    READ isComplete    CONSTANT)

public:
    MixerGroup(unsigned int groupID=0, QObject* parent = NULL);
    ~MixerGroup();

    // Parameters (MixerParameter objects)
    QObjectList parameters              (void) const { return _parameters; }

    void appendParameter(MixerParameter *param);
    void deleteGroupParameters(void);
    MixerParameter* getParameter(unsigned int index);

    unsigned int groupID(void) {return _groupID;}
    void setGroupID(unsigned int groupID) {_groupID = groupID;}

    QString groupName(void) {return _groupName;}
    void setGroupName(QString groupName) {_groupName = groupName;}

    unsigned int paramCount(void) {return _paramCount;}
    void setParamCount(unsigned int count) {_paramCount = count;}

    bool isComplete(void) {return _isComplete;}
    void setComplete(void) {_isComplete = true;}

signals:
    void mixerParamChanged(MixerParameter* param, Fact *value, int valueIndex);

private slots:
    void _changedParamValue(Fact *value, int valueIndex);

private:
    QObjectList     _parameters ;
    unsigned int    _groupID;
    unsigned int    _paramCount;
    QString         _groupName;
    bool            _isComplete;
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
