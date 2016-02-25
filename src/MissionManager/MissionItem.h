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

#ifndef MissionItem_H
#define MissionItem_H

#include <QObject>
#include <QString>
#include <QtQml>
#include <QTextStream>
#include <QJsonObject>
#include <QGeoCoordinate>

#include "QGCMAVLink.h"
#include "QGC.h"
#include "MavlinkQmlSingleton.h"
#include "QmlObjectListModel.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "MissionCommands.h"

class ComplexMissionItem;
class SimpleMissionItem;
class MissionController;
#ifdef UNITTEST_BUILD
    class MissionItemTest;
#endif

// Represents a Mavlink mission command.
class MissionItem : public QObject
{
    Q_OBJECT
    
public:
    MissionItem(QObject* parent = NULL);

    MissionItem(int             sequenceNumber,
                MAV_CMD         command,
                MAV_FRAME       frame,
                double          param1,
                double          param2,
                double          param3,
                double          param4,
                double          param5,
                double          param6,
                double          param7,
                bool            autoContinue,
                bool            isCurrentItem,
                QObject*        parent = NULL);

    MissionItem(const MissionItem& other, QObject* parent = NULL);

    ~MissionItem();

    const MissionItem& operator=(const MissionItem& other);
    
    MAV_CMD         command         (void) const { return (MAV_CMD)_commandFact.rawValue().toInt(); }
    bool            isCurrentItem   (void) const { return _isCurrentItem; }
    int             sequenceNumber  (void) const { return _sequenceNumber; }
    MAV_FRAME       frame           (void) const { return (MAV_FRAME)_frameFact.rawValue().toInt(); }
    bool            autoContinue    (void) const { return _autoContinueFact.rawValue().toBool(); }
    double          param1          (void) const { return _param1Fact.rawValue().toDouble(); }
    double          param2          (void) const { return _param2Fact.rawValue().toDouble(); }
    double          param3          (void) const { return _param3Fact.rawValue().toDouble(); }
    double          param4          (void) const { return _param4Fact.rawValue().toDouble(); }
    double          param5          (void) const { return _param5Fact.rawValue().toDouble(); }
    double          param6          (void) const { return _param6Fact.rawValue().toDouble(); }
    double          param7          (void) const { return _param7Fact.rawValue().toDouble(); }
    QGeoCoordinate  coordinate      (void) const;

    void setCommand         (MAV_CMD command);
    void setSequenceNumber  (int sequenceNumber);
    void setIsCurrentItem   (bool isCurrentItem);
    void setFrame           (MAV_FRAME frame);
    void setAutoContinue    (bool autoContinue);
    void setParam1          (double param1);
    void setParam2          (double param2);
    void setParam3          (double param3);
    void setParam4          (double param4);
    void setParam5          (double param5);
    void setParam6          (double param6);
    void setParam7          (double param7);
    void setCoordinate      (const QGeoCoordinate& coordinate);
    
    void save(QJsonObject& json) const;
    bool load(QTextStream &loadStream);
    bool load(const QJsonObject& json, QString& errorString);

    bool relativeAltitude(void) const { return frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT; }

signals:
    void isCurrentItemChanged       (bool isCurrentItem);
    void sequenceNumberChanged      (int sequenceNumber);
    
private:
    int         _sequenceNumber;
    bool        _isCurrentItem;

    Fact    _autoContinueFact;
    Fact    _commandFact;
    Fact    _frameFact;
    Fact    _param1Fact;
    Fact    _param2Fact;
    Fact    _param3Fact;
    Fact    _param4Fact;
    Fact    _param5Fact;
    Fact    _param6Fact;
    Fact    _param7Fact;
    
    // Keys for Json save
    static const char*  _itemType;
    static const char*  _jsonTypeKey;
    static const char*  _jsonIdKey;
    static const char*  _jsonFrameKey;
    static const char*  _jsonCommandKey;
    static const char*  _jsonParam1Key;
    static const char*  _jsonParam2Key;
    static const char*  _jsonParam3Key;
    static const char*  _jsonParam4Key;
    static const char*  _jsonAutoContinueKey;
    static const char*  _jsonCoordinateKey;

    friend class ComplexMissionItem;
    friend class SimpleMissionItem;
    friend class MissionController;
#ifdef UNITTEST_BUILD
    friend class MissionItemTest;
#endif
};

#endif
