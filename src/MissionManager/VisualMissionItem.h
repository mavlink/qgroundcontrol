/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include <QtQml>
#include <QTextStream>
#include <QJsonObject>
#include <QGeoCoordinate>

#include "QGCMAVLink.h"
#include "QGC.h"
#include "QmlObjectListModel.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "MissionController.h"

class MissionItem;
class PlanMasterController;
class MissionController;
class TerrainAtCoordinateQuery;

// Abstract base class for all Simple and Complex visual mission objects.
class VisualMissionItem : public QObject
{
    Q_OBJECT

public:
    VisualMissionItem(PlanMasterController* masterController, bool flyView);
    VisualMissionItem(const VisualMissionItem& other, bool flyView);

    ~VisualMissionItem();

    const VisualMissionItem& operator=(const VisualMissionItem& other);

    enum ReadyForSaveState {
        ReadyForSave,
        NotReadyForSaveTerrain,
        NotReadyForSaveData,
    };
    Q_ENUM(ReadyForSaveState)

    Q_PROPERTY(bool             homePosition                        READ homePosition                                                       CONSTANT)                                           ///< true: This item is being used as a home position indicator
    Q_PROPERTY(QGeoCoordinate   coordinate                          READ coordinate                         WRITE setCoordinate             NOTIFY coordinateChanged)                           ///< Does not include altitude
    Q_PROPERTY(double           amslEntryAlt                        READ amslEntryAlt                                                       NOTIFY amslEntryAltChanged)
    Q_PROPERTY(double           terrainAltitude                     READ terrainAltitude                                                    NOTIFY terrainAltitudeChanged)                      ///< The altitude of terrain at the coordinate position, NaN if not known
    Q_PROPERTY(QGeoCoordinate   exitCoordinate                      READ exitCoordinate                                                     NOTIFY exitCoordinateChanged)                       ///< Does not include altitude
    Q_PROPERTY(double           amslExitAlt                         READ amslExitAlt                                                        NOTIFY amslExitAltChanged)
    Q_PROPERTY(bool             exitCoordinateSameAsEntry           READ exitCoordinateSameAsEntry                                          NOTIFY exitCoordinateSameAsEntryChanged)            ///< true: exitCoordinate and coordinate are the same value
    Q_PROPERTY(QString          commandDescription                  READ commandDescription                                                 NOTIFY commandDescriptionChanged)
    Q_PROPERTY(QString          commandName                         READ commandName                                                        NOTIFY commandNameChanged)
    Q_PROPERTY(QString          abbreviation                        READ abbreviation                                                       NOTIFY abbreviationChanged)
    Q_PROPERTY(bool             dirty                               READ dirty                              WRITE setDirty                  NOTIFY dirtyChanged)                                ///< Item is dirty and requires save/send
    Q_PROPERTY(bool             isCurrentItem                       READ isCurrentItem                      WRITE setIsCurrentItem          NOTIFY isCurrentItemChanged)
    Q_PROPERTY(bool             hasCurrentChildItem                 READ hasCurrentChildItem                WRITE setHasCurrentChildItem    NOTIFY hasCurrentChildItemChanged)                  ///< true: On of this items children is current
    Q_PROPERTY(int              sequenceNumber                      READ sequenceNumber                     WRITE setSequenceNumber         NOTIFY sequenceNumberChanged)
    Q_PROPERTY(int              lastSequenceNumber                  READ lastSequenceNumber                                                 NOTIFY lastSequenceNumberChanged)
    Q_PROPERTY(bool             specifiesCoordinate                 READ specifiesCoordinate                                                NOTIFY specifiesCoordinateChanged)                  ///< true: Item is associated with a coordinate position
    Q_PROPERTY(bool             isStandaloneCoordinate              READ isStandaloneCoordinate                                             NOTIFY isStandaloneCoordinateChanged)               ///< true: Waypoint line does not go through item
    Q_PROPERTY(bool             specifiesAltitudeOnly               READ specifiesAltitudeOnly                                              NOTIFY specifiesAltitudeOnlyChanged)                ///< true: Item has altitude only, no full coordinate
    Q_PROPERTY(bool             isSimpleItem                        READ isSimpleItem                                                       NOTIFY isSimpleItemChanged)                         ///< Simple or Complex MissionItem
    Q_PROPERTY(bool             isTakeoffItem                       READ isTakeoffItem                                                      NOTIFY isTakeoffItemChanged)                        ///< true: Takeoff item special case
    Q_PROPERTY(bool             isLandCommand                       READ isLandCommand                                                      NOTIFY isLandCommandChanged)
    Q_PROPERTY(bool             isSurveyItem                        READ isSurveyItem                                                       )                                                   ///< true: Survey item special case for editing center position through mission item list menue
    Q_PROPERTY(QString          editorQml                           MEMBER _editorQml                                                       CONSTANT)                                           ///< Qml code for editing this item
    Q_PROPERTY(QString          mapVisualQML                        READ mapVisualQML                                                       CONSTANT)                                           ///< QMl code for map visuals
    Q_PROPERTY(double           specifiedFlightSpeed                READ specifiedFlightSpeed                                               NOTIFY specifiedFlightSpeedChanged)                 ///< NaN for not specified
    Q_PROPERTY(double           specifiedGimbalYaw                  READ specifiedGimbalYaw                                                 NOTIFY specifiedGimbalYawChanged)                   ///< NaN for not specified
    Q_PROPERTY(double           specifiedGimbalPitch                READ specifiedGimbalPitch                                               NOTIFY specifiedGimbalPitchChanged)                 ///< NaN for not specified
    Q_PROPERTY(double           specifiedVehicleYaw                 READ specifiedVehicleYaw                                                NOTIFY specifiedVehicleYawChanged)                  ///< NaN for not specified
    Q_PROPERTY(double           missionGimbalYaw                    READ missionGimbalYaw                                                   NOTIFY missionGimbalYawChanged)                     ///< Current gimbal yaw state at this point in mission
    Q_PROPERTY(double           missionVehicleYaw                   READ missionVehicleYaw                                                  NOTIFY missionVehicleYawChanged)                    ///< Expected vehicle yaw at this point in mission
    Q_PROPERTY(bool             flyView                             READ flyView                                                            CONSTANT)
    Q_PROPERTY(bool             wizardMode                          READ wizardMode                         WRITE setWizardMode             NOTIFY wizardModeChanged)
    Q_PROPERTY(int              previousVTOLMode                    MEMBER _previousVTOLMode                                                NOTIFY previousVTOLModeChanged)                     ///< Current VTOL mode (VehicleClass_t) prior to executing this item

    Q_PROPERTY(PlanMasterController*    masterController    READ masterController                                                   CONSTANT)
    Q_PROPERTY(ReadyForSaveState        readyForSaveState   READ readyForSaveState                                                  NOTIFY readyForSaveStateChanged)
    Q_PROPERTY(VisualMissionItem*       parentItem          READ parentItem                        WRITE setParentItem              NOTIFY parentItemChanged)
    Q_PROPERTY(QmlObjectListModel*      childItems          READ childItems                                                         CONSTANT)
    Q_PROPERTY(QGCGeoBoundingCube*      boundingCube        READ boundingCube                                                       NOTIFY boundingCubeChanged)

    // The following properties are calculated/set by the MissionController recalc methods

    Q_PROPERTY(double altDifference     READ altDifference          WRITE setAltDifference      NOTIFY altDifferenceChanged)        ///< Change in altitude from previous waypoint
    Q_PROPERTY(double altPercent        READ altPercent             WRITE setAltPercent         NOTIFY altPercentChanged)           ///< Percent of total altitude change in mission altitude
    Q_PROPERTY(double terrainPercent    READ terrainPercent         WRITE setTerrainPercent     NOTIFY terrainPercentChanged)       ///< Percent of terrain altitude in mission altitude
    Q_PROPERTY(bool   terrainCollision  READ terrainCollision       WRITE setTerrainCollision   NOTIFY terrainCollisionChanged)     ///< true: Item collides with terrain
    Q_PROPERTY(double azimuth           READ azimuth                WRITE setAzimuth            NOTIFY azimuthChanged)              ///< Azimuth to previous waypoint
    Q_PROPERTY(double distance          READ distance               WRITE setDistance           NOTIFY distanceChanged)             ///< Distance to previous waypoint
    Q_PROPERTY(double distanceFromStart READ distanceFromStart      WRITE setDistanceFromStart  NOTIFY distanceFromStartChanged)    ///< Flight path cumalative horizontal distance from home point to this item

    // Property accesors
    bool    homePosition        (void) const { return _homePositionSpecialCase; }
    double  altDifference       (void) const { return _altDifference; }
    double  altPercent          (void) const { return _altPercent; }
    double  terrainPercent      (void) const { return _terrainPercent; }
    bool    terrainCollision    (void) const { return _terrainCollision; }
    double  azimuth             (void) const { return _azimuth; }
    double  distance            (void) const { return _distance; }
    double  distanceFromStart   (void) const { return _distanceFromStart; }
    bool    isCurrentItem       (void) const { return _isCurrentItem; }
    bool    hasCurrentChildItem (void) const { return _hasCurrentChildItem; }
    double  terrainAltitude     (void) const { return _terrainAltitude; }
    bool    flyView             (void) const { return _flyView; }
    bool    wizardMode          (void) const { return _wizardMode; }
    VisualMissionItem* parentItem(void) { return _parentItem; }

    QmlObjectListModel* childItems(void) { return &_childItems; }

    void setIsCurrentItem           (bool isCurrentItem);
    void setHasCurrentChildItem     (bool hasCurrentChildItem);
    void setAltDifference           (double altDifference);
    void setAltPercent              (double altPercent);
    void setTerrainPercent          (double terrainPercent);
    void setTerrainCollision        (bool terrainCollision);
    void setAzimuth                 (double azimuth);
    void setDistance                (double distance);
    void setDistanceFromStart       (double distanceFromStart);
    void setWizardMode              (bool wizardMode);
    void setParentItem              (VisualMissionItem* parentItem);

    void setHomePositionSpecialCase (bool homePositionSpecialCase) { _homePositionSpecialCase = homePositionSpecialCase; }

    FlightPathSegment* simpleFlightPathSegment(void) { return _simpleFlightPathSegment; }
    void setSimpleFlighPathSegment  (FlightPathSegment* segment) { _simpleFlightPathSegment = segment; }
    void clearSimpleFlighPathSegment(void) { _simpleFlightPathSegment = nullptr; }

    PlanMasterController* masterController(void) { return _masterController; }

    // Pure virtuals which must be provides by derived classes

    virtual bool            dirty                   (void) const = 0;
    virtual bool            isSimpleItem            (void) const = 0;
    virtual bool            isTakeoffItem           (void) const { return false; }
    virtual bool            isLandCommand           (void) const { return false; }
    virtual bool            isSurveyItem            (void) const { return false; }
    virtual bool            isStandaloneCoordinate  (void) const = 0;
    virtual bool            specifiesCoordinate     (void) const = 0;
    virtual bool            specifiesAltitudeOnly   (void) const = 0;
    virtual QString         commandDescription      (void) const = 0;
    virtual QString         commandName             (void) const = 0;
    virtual QString         abbreviation            (void) const = 0;
    virtual QGeoCoordinate  coordinate              (void) const = 0;
    virtual QGeoCoordinate  exitCoordinate          (void) const = 0;
    virtual double          amslEntryAlt            (void) const = 0;
    virtual double          amslExitAlt             (void) const = 0;
    virtual int             sequenceNumber          (void) const = 0;
    virtual double          specifiedFlightSpeed    (void) = 0;
    virtual double          specifiedGimbalYaw      (void) = 0;
    virtual double          specifiedGimbalPitch    (void) = 0;
    virtual double          specifiedVehicleYaw     (void) { return qQNaN(); }

    //-- Default implementation returns an invalid bounding cube
    virtual QGCGeoBoundingCube* boundingCube        (void) { return &_boundingCube; }

    /// Update item to mission flight status at point where this item appears in mission.
    /// IMPORTANT: Overrides must call base class implementation
    virtual void setMissionFlightStatus(MissionController::MissionFlightStatus_t& missionFlightStatus);

    virtual bool exitCoordinateSameAsEntry          (void) const = 0;

    virtual void setDirty           (bool dirty) = 0;
    virtual void setCoordinate      (const QGeoCoordinate& coordinate) = 0;
    virtual void setSequenceNumber  (int sequenceNumber) = 0;
    virtual int  lastSequenceNumber (void) const = 0;

    /// @return Returns whether the item is ready for save and if not, why
    virtual ReadyForSaveState readyForSaveState(void) const { return ReadyForSave; }

    /// Save the item(s) in Json format
    ///     @param missionItems Current set of mission items, new items should be appended to the end
    virtual void save(QJsonArray&  missionItems) = 0;

    /// @return The QML resource file which contains the control which visualizes the item on the map.
    virtual QString mapVisualQML(void) const = 0;

    /// Returns the mission items associated with the complex item. Caller is responsible for freeing.
    ///     @param items List to append to
    ///     @param missionItemParent Parent object for newly created MissionItems
    virtual void appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent) = 0;

    /// Adjust the altitude of the item if appropriate to the new altitude.
    virtual void applyNewAltitude(double newAltitude) = 0;

    /// @return Amount of additional time delay in seconds needed to fly this item
    virtual double additionalTimeDelay(void) const = 0;

    double  missionGimbalYaw    (void) const { return _missionGimbalYaw; }
    double  missionVehicleYaw   (void) const { return _missionVehicleYaw; }
    void    setMissionVehicleYaw(double vehicleYaw);

    static const char* jsonTypeKey;                 ///< Json file attribute which specifies the item type
    static const char* jsonTypeSimpleItemValue;     ///< Item type is MISSION_ITEM
    static const char* jsonTypeComplexItemValue;    ///< Item type is Complex Item

signals:
    void altDifferenceChanged           (double altDifference);
    void altPercentChanged              (double altPercent);
    void terrainPercentChanged          (double terrainPercent);
    void terrainCollisionChanged        (bool terrainCollision);
    void azimuthChanged                 (double azimuth);
    void commandDescriptionChanged      (void);
    void commandNameChanged             (void);
    void abbreviationChanged            (void);
    void coordinateChanged              (const QGeoCoordinate& coordinate);
    void exitCoordinateChanged          (const QGeoCoordinate& exitCoordinate);
    void dirtyChanged                   (bool dirty);
    void distanceChanged                (double distance);
    void distanceFromStartChanged       (double distanceFromStart);
    void isCurrentItemChanged           (bool isCurrentItem);
    void hasCurrentChildItemChanged     (bool hasCurrentChildItem);
    void sequenceNumberChanged          (int sequenceNumber);
    void isSimpleItemChanged            (bool isSimpleItem);
    void isTakeoffItemChanged           (bool isTakeoffItem);
    void isLandCommandChanged           (void);
    void specifiesCoordinateChanged     (void);
    void isStandaloneCoordinateChanged  (void);
    void specifiesAltitudeOnlyChanged   (void);
    void specifiedFlightSpeedChanged    (void);
    void specifiedGimbalYawChanged      (void);
    void specifiedGimbalPitchChanged    (void);
    void specifiedVehicleYawChanged     (void);
    void lastSequenceNumberChanged      (int sequenceNumber);
    void missionGimbalYawChanged        (double missionGimbalYaw);
    void missionVehicleYawChanged       (double missionVehicleYaw);
    void terrainAltitudeChanged         (double terrainAltitude);
    void additionalTimeDelayChanged     (void);
    void boundingCubeChanged            (void);
    void readyForSaveStateChanged       (void);
    void wizardModeChanged              (bool wizardMode);
    void parentItemChanged              (VisualMissionItem* parentItem);
    void amslEntryAltChanged            (double alt);
    void amslExitAltChanged             (double alt);
    void previousVTOLModeChanged        (void);
    void currentVTOLModeChanged         (void);                             ///< Signals that this item has changed the VTOL mode (MAV_CMD_DO_VTOL_TRANSITION)

    void exitCoordinateSameAsEntryChanged           (bool exitCoordinateSameAsEntry);

protected slots:
    void _amslEntryAltChanged(void);    // signals new amslEntryAlt value
    void _amslExitAltChanged(void);     // signals new amslExitAlt value

protected:
    bool                         _flyView                   = false;
    bool                        _isCurrentItem              = false;
    bool                        _hasCurrentChildItem        = false;
    bool                        _dirty                      = false;
    bool                        _homePositionSpecialCase    = false;                            ///< true: This item is being used as a ui home position indicator
    bool                        _wizardMode                 = false;                            ///< true: Item editor is showing wizard completion panel
    double                      _terrainAltitude            = qQNaN();                          ///< Altitude of terrain at coordinate position, NaN for not known
    double                      _altDifference              = 0;                                ///< Difference in altitude from previous waypoint
    double                      _altPercent                 = 0;                                ///< Percent of total altitude change in mission
    double                      _terrainPercent             = qQNaN();                          ///< Percent of terrain altitude for coordinate
    bool                        _terrainCollision           = false;                            ///< true: item collides with terrain
    double                      _azimuth                    = 0;                                ///< Azimuth to previous waypoint
    double                      _distance                   = 0;                                ///< Distance to previous waypoint
    double                      _distanceFromStart          = 0;                                ///< Flight path cumalative horizontal distance from home point to this item
    QString                     _editorQml;                                                     ///< Qml resource for editing item
    double                      _missionGimbalYaw           = qQNaN();
    double                      _missionVehicleYaw          = qQNaN();
    QGCMAVLink::VehicleClass_t  _previousVTOLMode           = QGCMAVLink::VehicleClassGeneric;  ///< Generic == unknown

    PlanMasterController*   _masterController           = nullptr;
    MissionController*      _missionController          = nullptr;
    Vehicle*                _controllerVehicle          = nullptr;
    FlightPathSegment *     _simpleFlightPathSegment    = nullptr;  ///< The simple item flight segment (if any) which starts with this visual item.
    VisualMissionItem*      _parentItem                 = nullptr;
    QGCGeoBoundingCube      _boundingCube;                          ///< The bounding "cube" of this element.

    /// This is used to reference any subsequent mission items which do not specify a coordinate.
    QmlObjectListModel  _childItems;

protected:
    void    _setBoundingCube                (QGCGeoBoundingCube bc);

private slots:
    void _updateTerrainAltitude (void);
    void _reallyUpdateTerrainAltitude (void);
    void _terrainDataReceived   (bool success, QList<double> heights);

private:
    void _commonInit(void);

    QTimer                      _updateTerrainTimer;
    TerrainAtCoordinateQuery*   _currentTerrainAtCoordinateQuery = nullptr;

    double _lastLatTerrainQuery = 0;
    double _lastLonTerrainQuery = 0;
};
