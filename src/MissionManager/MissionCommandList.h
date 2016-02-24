/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef MissionCommandList_H
#define MissionCommandList_H

#include "QGCToolbox.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "MavlinkQmlSingleton.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonValue>

class MissionCommandList;
class Vehicle;

/// The information associated with a mission command parameter.
class MavCmdParamInfo : public QObject {

    Q_OBJECT

public:
    MavCmdParamInfo(QObject* parent = NULL)
        : QObject(parent)
    {

    }

    Q_PROPERTY(int          decimalPlaces   READ decimalPlaces  CONSTANT)
    Q_PROPERTY(double       defaultValue    READ defaultValue   CONSTANT)
    Q_PROPERTY(QStringList  enumStrings     READ enumStrings    CONSTANT)
    Q_PROPERTY(QVariantList enumValues      READ enumValues     CONSTANT)
    Q_PROPERTY(QString      label           READ label          CONSTANT)
    Q_PROPERTY(int          param           READ param          CONSTANT)
    Q_PROPERTY(QString      units           READ units          CONSTANT)

    int             decimalPlaces   (void) const { return _decimalPlaces; }
    double          defaultValue    (void) const { return _defaultValue; }
    QStringList     enumStrings     (void) const { return _enumStrings; }
    QVariantList    enumValues      (void) const { return _enumValues; }
    QString         label           (void) const { return _label; }
    int             param           (void) const { return _param; }
    QString         units           (void) const { return _units; }

private:
    int             _decimalPlaces;
    double          _defaultValue;
    QStringList     _enumStrings;
    QVariantList    _enumValues;
    QString         _label;
    int             _param;
    QString         _units;

    friend class MissionCommandList;
};

// The information associated with a mission command.
class MavCmdInfo : public QObject {
    Q_OBJECT

public:
    MavCmdInfo(QObject* parent = NULL)
        : QObject(parent)
    {

    }

    Q_PROPERTY(QString  category                READ category               CONSTANT)
    Q_PROPERTY(MavlinkQmlSingleton::Qml_MAV_CMD command READ qmlCommand     CONSTANT)
    Q_PROPERTY(QString  description             READ description            CONSTANT)
    Q_PROPERTY(bool     friendlyEdit            READ friendlyEdit           CONSTANT)
    Q_PROPERTY(QString  friendlyName            READ friendlyName           CONSTANT)
    Q_PROPERTY(QString  rawName                 READ rawName                CONSTANT)
    Q_PROPERTY(bool     isStandaloneCoordinate    READ isStandaloneCoordinate   CONSTANT)
    Q_PROPERTY(bool     specifiesCoordinate     READ specifiesCoordinate    CONSTANT)

    MavlinkQmlSingleton::Qml_MAV_CMD qmlCommand(void) const { return (MavlinkQmlSingleton::Qml_MAV_CMD)_command; }
    MAV_CMD command(void) const { return _command; }

    QString category            (void) const { return _category; }
    QString description         (void) const { return _description; }
    bool    friendlyEdit        (void) const { return _friendlyEdit; }
    QString friendlyName        (void) const { return _friendlyName; }
    QString rawName             (void) const { return _rawName; }
    bool    isStandaloneCoordinate(void) const { return _standaloneCoordinate; }
    bool    specifiesCoordinate (void) const { return _specifiesCoordinate; }

    const QMap<int, MavCmdParamInfo*>& paramInfoMap(void) const { return _paramInfoMap; }

private:
    QString                     _category;
    MAV_CMD                     _command;
    QString                     _description;
    bool                        _friendlyEdit;
    QString                     _friendlyName;
    QMap<int, MavCmdParamInfo*> _paramInfoMap;
    QString                     _rawName;
    bool                        _standaloneCoordinate;
    bool                        _specifiesCoordinate;

    friend class MissionCommandList;
};

// A list of mission command info loaded from a json file.
class MissionCommandList : public QObject
{
    Q_OBJECT
    
public:
    MissionCommandList(const QString& jsonFilename, QObject* parent = NULL);

    Q_INVOKABLE bool contains(MavlinkQmlSingleton::Qml_MAV_CMD command) const { return contains((MAV_CMD)command); }
    bool contains(MAV_CMD command) const;

    Q_INVOKABLE QVariant getMavCmdInfo(MavlinkQmlSingleton::Qml_MAV_CMD command) const { return QVariant::fromValue(getMavCmdInfo((MAV_CMD)command)); }
    MavCmdInfo* getMavCmdInfo(MAV_CMD command) const;

    QList<MAV_CMD> commandsIds(void) const;
    
private:
    void _loadMavCmdInfoJson(const QString& jsonFilename);

private:
    QMap<MAV_CMD, MavCmdInfo*> _mavCmdInfoMap;

    static const QString _categoryJsonKey;
    static const QString _decimalPlacesJsonKey;
    static const QString _defaultJsonKey;
    static const QString _descriptionJsonKey;
    static const QString _enumStringsJsonKey;
    static const QString _enumValuesJsonKey;
    static const QString _friendlyNameJsonKey;
    static const QString _friendlyEditJsonKey;
    static const QString _idJsonKey;
    static const QString _labelJsonKey;
    static const QString _mavCmdInfoJsonKey;
    static const QString _param1JsonKey;
    static const QString _param2JsonKey;
    static const QString _param3JsonKey;
    static const QString _param4JsonKey;
    static const QString _paramJsonKeyFormat;
    static const QString _rawNameJsonKey;
    static const QString _standaloneCoordinateJsonKey;
    static const QString _specifiesCoordinateJsonKey;
    static const QString _unitsJsonKey;
    static const QString _versionJsonKey;
};

#endif
