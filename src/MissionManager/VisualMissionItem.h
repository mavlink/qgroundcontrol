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

#ifndef VisualMissionItem_H
#define VisualMissionItem_H

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

// Abstract base class for all Simple and Complex visual mission objects.
class VisualMissionItem : public QObject
{
    Q_OBJECT
    
public:
    VisualMissionItem(Vehicle* vehicle, QObject* parent = NULL);
    VisualMissionItem(const VisualMissionItem& other, QObject* parent = NULL);

    ~VisualMissionItem();

    const VisualMissionItem& operator=(const VisualMissionItem& other);

    // The following properties are calulated/set by the MissionControll recalc methods

    Q_PROPERTY(double altDifference READ altDifference  WRITE setAltDifference  NOTIFY altDifferenceChanged)    ///< Change in altitude from previous waypoint
    Q_PROPERTY(double altPercent    READ altPercent     WRITE setAltPercent     NOTIFY altPercentChanged)       ///< Percent of total altitude change in mission altitude
    Q_PROPERTY(double azimuth       READ azimuth        WRITE setAzimuth        NOTIFY azimuthChanged)          ///< Azimuth to previous waypoint
    Q_PROPERTY(double distance      READ distance       WRITE setDistance       NOTIFY distanceChanged)         ///< Distance to previous waypoint

    // Visual mission items have two coordinates associated with them:

    /// This is the entry point for a waypoint line into the item. For a simple item it is also the location of the item
    Q_PROPERTY(QGeoCoordinate coordinate        READ coordinate     WRITE setCoordinate NOTIFY coordinateChanged)

    /// @return true: coordinate.latitude is relative to home altitude
    Q_PROPERTY(bool coordinateHasRelativeAltitude READ coordinateHasRelativeAltitude NOTIFY coordinateHasRelativeAltitudeChanged)

    /// This is the exit point for a waypoint line coming out of the item.
    Q_PROPERTY(QGeoCoordinate exitCoordinate    READ exitCoordinate                     NOTIFY exitCoordinateChanged)

    /// @return true: coordinate.latitude is relative to home altitude
    Q_PROPERTY(bool exitCoordinateHasRelativeAltitude READ exitCoordinateHasRelativeAltitude NOTIFY exitCoordinateHasRelativeAltitudeChanged)

    /// @return true: exitCoordinate and coordinate are the same value
    Q_PROPERTY(bool exitCoordinateSameAsEntry READ exitCoordinateSameAsEntry NOTIFY exitCoordinateSameAsEntryChanged)

    // General properties associated with all types of visual mission items

    Q_PROPERTY(QString  commandDescription      READ commandDescription                                 NOTIFY commandDescriptionChanged)
    Q_PROPERTY(QString  commandName             READ commandName                                        NOTIFY commandNameChanged)
    Q_PROPERTY(QString  abbreviation            READ abbreviation                                       NOTIFY abbreviationChanged)
    Q_PROPERTY(bool     dirty                   READ dirty                  WRITE setDirty              NOTIFY dirtyChanged)                    ///< Item is dirty and requires save/send
    Q_PROPERTY(bool     isCurrentItem           READ isCurrentItem          WRITE setIsCurrentItem      NOTIFY isCurrentItemChanged)
    Q_PROPERTY(int      sequenceNumber          READ sequenceNumber         WRITE setSequenceNumber     NOTIFY sequenceNumberChanged)
    Q_PROPERTY(bool     specifiesCoordinate     READ specifiesCoordinate                                NOTIFY specifiesCoordinateChanged)      ///< Item is associated with a coordinate position
    Q_PROPERTY(bool     isStandaloneCoordinate  READ isStandaloneCoordinate                             NOTIFY isStandaloneCoordinateChanged)   ///< Wayoint line does not go through item
    Q_PROPERTY(bool     isSimpleItem            READ isSimpleItem                                       NOTIFY isSimpleItemChanged)             ///< Simple or Complex MissionItem

    /// List of child mission items. Child mission item are subsequent mision items which do not specify a coordinate. They
    /// are shown next to the exitCoordinate indidcator in the ui.
    Q_PROPERTY(QmlObjectListModel*  childItems      READ childItems     CONSTANT)

    // Property accesors
    
    double altDifference    (void) const { return _altDifference; }
    double altPercent       (void) const { return _altPercent; }
    double azimuth          (void) const { return _azimuth; }
    double distance         (void) const { return _distance; }
    bool   isCurrentItem    (void) const { return _isCurrentItem; }

    QmlObjectListModel* childItems(void) { return &_childItems; }

    void setIsCurrentItem   (bool isCurrentItem);
    void setAltDifference   (double altDifference);
    void setAltPercent      (double altPercent);
    void setAzimuth         (double azimuth);
    void setDistance        (double distance);

    Vehicle* vehicle(void) { return _vehicle; }

    // Pure virtuals which must be provides by derived classes

    virtual bool            dirty                   (void) const = 0;
    virtual bool            isSimpleItem            (void) const = 0;
    virtual bool            isStandaloneCoordinate  (void) const = 0;
    virtual bool            specifiesCoordinate     (void) const = 0;
    virtual QString         commandDescription      (void) const = 0;
    virtual QString         commandName             (void) const = 0;
    virtual QString         abbreviation            (void) const = 0;
    virtual QGeoCoordinate  coordinate              (void) const = 0;
    virtual QGeoCoordinate  exitCoordinate          (void) const = 0;
    virtual int             sequenceNumber          (void) const = 0;

    virtual bool coordinateHasRelativeAltitude      (void) const = 0;
    virtual bool exitCoordinateHasRelativeAltitude  (void) const = 0;
    virtual bool exitCoordinateSameAsEntry          (void) const = 0;

    virtual void setDirty           (bool dirty) = 0;
    virtual void setCoordinate      (const QGeoCoordinate& coordinate) = 0;
    virtual void setSequenceNumber  (int sequenceNumber) = 0;

    /// Save the item(s) in Json format
    ///     @param saveObject Save the item to this json object
    virtual void save(QJsonObject& saveObject) const = 0;

signals:
    void altDifferenceChanged           (double altDifference);
    void altPercentChanged              (double altPercent);
    void azimuthChanged                 (double azimuth);
    void commandDescriptionChanged      (void);
    void commandNameChanged             (void);
    void abbreviationChanged            (void);
    void coordinateChanged              (const QGeoCoordinate& coordinate);
    void exitCoordinateChanged          (const QGeoCoordinate& exitCoordinate);
    void dirtyChanged                   (bool dirty);
    void distanceChanged                (double distance);
    void isCurrentItemChanged           (bool isCurrentItem);
    void sequenceNumberChanged          (int sequenceNumber);
    void isSimpleItemChanged            (bool isSimpleItem);
    void specifiesCoordinateChanged     (void);
    void isStandaloneCoordinateChanged  (void);

    void coordinateHasRelativeAltitudeChanged       (bool coordinateHasRelativeAltitude);
    void exitCoordinateHasRelativeAltitudeChanged   (bool exitCoordinateHasRelativeAltitude);
    void exitCoordinateSameAsEntryChanged           (bool exitCoordinateSameAsEntry);

protected:
    Vehicle*    _vehicle;
    bool        _isCurrentItem;
    bool        _dirty;
    double      _altDifference;             ///< Difference in altitude from previous waypoint
    double      _altPercent;                ///< Percent of total altitude change in mission
    double      _azimuth;                   ///< Azimuth to previous waypoint
    double      _distance;                  ///< Distance to previous waypoint

    /// This is used to reference any subsequent mission items which do not specify a coordinate.
    QmlObjectListModel  _childItems;
};

#endif
