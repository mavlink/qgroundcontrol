/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "PlanElementController.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"
#include "KMLPlanDomDocument.h"
#include "QGCGeoBoundingCube.h"
#include "QGroundControlQmlGlobal.h"

#include <QHash>

class FlightPathSegment;
class VisualMissionItem;
class MissionItem;
class AppSettings;
class MissionManager;
class SimpleMissionItem;
class ComplexMissionItem;
class MissionSettingsItem;
class TakeoffMissionItem;
class QDomDocument;
class PlanViewSettings;

Q_DECLARE_LOGGING_CATEGORY(MissionControllerLog)

typedef QPair<VisualMissionItem*,VisualMissionItem*> VisualItemPair;
typedef QHash<VisualItemPair, FlightPathSegment*> FlightPathSegmentHashTable;

class MissionController : public PlanElementController
{
    Q_OBJECT

public:
    MissionController(PlanMasterController* masterController, QObject* parent = nullptr);
    ~MissionController();

    typedef struct {
        double                      maxTelemetryDistance;
        double                      totalDistance;
        double                      totalTime;
        double                      hoverDistance;
        double                      hoverTime;
        double                      cruiseDistance;
        double                      cruiseTime;
        int                         mAhBattery;             ///< 0 for not available
        double                      hoverAmps;              ///< Amp consumption during hover
        double                      cruiseAmps;             ///< Amp consumption during cruise
        double                      ampMinutesAvailable;    ///< Amp minutes available from single battery
        double                      hoverAmpsTotal;         ///< Total hover amps used
        double                      cruiseAmpsTotal;        ///< Total cruise amps used
        int                         batteryChangePoint;     ///< -1 for not supported, 0 for not needed
        int                         batteriesRequired;      ///< -1 for not supported
        double                      vehicleYaw;
        double                      gimbalYaw;              ///< NaN signals yaw was never changed
        double                      gimbalPitch;            ///< NaN signals pitch was never changed
        // The following values are the state prior to executing this item
        QGCMAVLink::VehicleClass_t  vtolMode;               ///< Either VehicleClassFixedWing, VehicleClassMultiRotor, VehicleClassGeneric (mode unknown)
        double                      cruiseSpeed;
        double                      hoverSpeed;
        double                      vehicleSpeed;           ///< Either cruise or hover speed based on vehicle type and vtol state
    } MissionFlightStatus_t;

    Q_PROPERTY(QmlObjectListModel*  visualItems                     READ visualItems                    NOTIFY visualItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  simpleFlightPathSegments        READ simpleFlightPathSegments       CONSTANT)                               ///< Used by Plan view only for interactive editing
    Q_PROPERTY(QVariantList         waypointPath                    READ waypointPath                   NOTIFY waypointPathChanged)             ///< Used by Fly view only for static display
    Q_PROPERTY(QmlObjectListModel*  directionArrows                 READ directionArrows                CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  incompleteComplexItemLines      READ incompleteComplexItemLines     CONSTANT)                               ///< Segments which are not yet completed.
    Q_PROPERTY(QStringList          complexMissionItemNames         READ complexMissionItemNames        NOTIFY complexMissionItemNamesChanged)
    Q_PROPERTY(QGeoCoordinate       plannedHomePosition             READ plannedHomePosition            NOTIFY plannedHomePositionChanged)      ///< Includes AMSL altitude
    Q_PROPERTY(QGeoCoordinate       previousCoordinate              MEMBER _previousCoordinate          NOTIFY previousCoordinateChanged)
    Q_PROPERTY(FlightPathSegment*   splitSegment                    MEMBER _splitSegment                NOTIFY splitSegmentChanged)             ///< Segment which show show + split ui element
    Q_PROPERTY(double               progressPct                     READ progressPct                    NOTIFY progressPctChanged)
    Q_PROPERTY(int                  missionItemCount                READ missionItemCount               NOTIFY missionItemCountChanged)         ///< True mission item command count (only valid in Fly View)
    Q_PROPERTY(int                  currentMissionIndex             READ currentMissionIndex            NOTIFY currentMissionIndexChanged)
    Q_PROPERTY(int                  resumeMissionIndex              READ resumeMissionIndex             NOTIFY resumeMissionIndexChanged)       ///< Returns the item index two which a mission should be resumed. -1 indicates resume mission not available.
    Q_PROPERTY(int                  currentPlanViewSeqNum           READ currentPlanViewSeqNum          NOTIFY currentPlanViewSeqNumChanged)
    Q_PROPERTY(int                  currentPlanViewVIIndex          READ currentPlanViewVIIndex         NOTIFY currentPlanViewVIIndexChanged)
    Q_PROPERTY(VisualMissionItem*   currentPlanViewItem             READ currentPlanViewItem            NOTIFY currentPlanViewItemChanged)
    Q_PROPERTY(TakeoffMissionItem*  takeoffMissionItem              READ takeoffMissionItem             NOTIFY takeoffMissionItemChanged)
    Q_PROPERTY(double               missionDistance                 READ missionDistance                NOTIFY missionDistanceChanged)
    Q_PROPERTY(double               missionTime                     READ missionTime                    NOTIFY missionTimeChanged)
    Q_PROPERTY(double               missionHoverDistance            READ missionHoverDistance           NOTIFY missionHoverDistanceChanged)
    Q_PROPERTY(double               missionCruiseDistance           READ missionCruiseDistance          NOTIFY missionCruiseDistanceChanged)
    Q_PROPERTY(double               missionHoverTime                READ missionHoverTime               NOTIFY missionHoverTimeChanged)
    Q_PROPERTY(double               missionCruiseTime               READ missionCruiseTime              NOTIFY missionCruiseTimeChanged)
    Q_PROPERTY(double               missionMaxTelemetry             READ missionMaxTelemetry            NOTIFY missionMaxTelemetryChanged)
    Q_PROPERTY(int                  batteryChangePoint              READ batteryChangePoint             NOTIFY batteryChangePointChanged)
    Q_PROPERTY(int                  batteriesRequired               READ batteriesRequired              NOTIFY batteriesRequiredChanged)
    Q_PROPERTY(QGCGeoBoundingCube*  travelBoundingCube              READ travelBoundingCube             NOTIFY missionBoundingCubeChanged)
    Q_PROPERTY(QString              surveyComplexItemName           READ surveyComplexItemName          CONSTANT)
    Q_PROPERTY(QString              corridorScanComplexItemName     READ corridorScanComplexItemName    CONSTANT)
    Q_PROPERTY(QString              structureScanComplexItemName    READ structureScanComplexItemName   CONSTANT)
    Q_PROPERTY(bool                 onlyInsertTakeoffValid          MEMBER _onlyInsertTakeoffValid      NOTIFY onlyInsertTakeoffValidChanged)
    Q_PROPERTY(bool                 isInsertTakeoffValid            MEMBER _isInsertTakeoffValid        NOTIFY isInsertTakeoffValidChanged)
    Q_PROPERTY(bool                 isInsertLandValid               MEMBER _isInsertLandValid           NOTIFY isInsertLandValidChanged)
    Q_PROPERTY(bool                 isROIActive                     MEMBER _isROIActive                 NOTIFY isROIActiveChanged)
    Q_PROPERTY(bool                 isROIBeginCurrentItem           MEMBER _isROIBeginCurrentItem       NOTIFY isROIBeginCurrentItemChanged)
    Q_PROPERTY(bool                 flyThroughCommandsAllowed       MEMBER _flyThroughCommandsAllowed   NOTIFY flyThroughCommandsAllowedChanged)
    Q_PROPERTY(double               minAMSLAltitude                 MEMBER _minAMSLAltitude             NOTIFY minAMSLAltitudeChanged)          ///< Minimum altitude associated with this mission. Used to calculate percentages for terrain status.
    Q_PROPERTY(double               maxAMSLAltitude                 MEMBER _maxAMSLAltitude             NOTIFY maxAMSLAltitudeChanged)          ///< Maximum altitude associated with this mission. Used to calculate percentages for terrain status.

    Q_PROPERTY(QGroundControlQmlGlobal::AltMode globalAltitudeMode         READ globalAltitudeMode         WRITE setGlobalAltitudeMode NOTIFY globalAltitudeModeChanged)
    Q_PROPERTY(QGroundControlQmlGlobal::AltMode globalAltitudeModeDefault  READ globalAltitudeModeDefault  NOTIFY globalAltitudeModeChanged)                               ///< Default to use for newly created items

    Q_INVOKABLE void removeVisualItem(int viIndex);

    /// Add a new simple mission item to the list
    ///     @param coordinate: Coordinate for item
    ///     @param visualItemIndex: index to insert at, -1 for end of list
    ///     @param makeCurrentItem: true: Make this item the current item
    /// @return Newly created item
    Q_INVOKABLE VisualMissionItem* insertSimpleMissionItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem = false);

    /// Add a new takeoff item to the list
    ///     @param coordinate: Coordinate for item
    ///     @param visualItemIndex: index to insert at, -1 for end of list
    ///     @param makeCurrentItem: true: Make this item the current item
    /// @return Newly created item
    Q_INVOKABLE VisualMissionItem* insertTakeoffItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem = false);

    /// Add a new land item to the list
    ///     @param coordinate: Coordinate for item
    ///     @param visualItemIndex: index to insert at, -1 for end of list
    ///     @param makeCurrentItem: true: Make this item the current item
    /// @return Newly created item
    Q_INVOKABLE VisualMissionItem* insertLandItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem = false);

    /// Add a new ROI mission item to the list
    ///     @param coordinate: Coordinate for item
    ///     @param visualItemIndex: index to insert at, -1 for end of list
    ///     @param makeCurrentItem: true: Make this item the current item
    /// @return Newly created item
    Q_INVOKABLE VisualMissionItem*  insertROIMissionItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem = false);

    /// Add a new Cancel ROI mission item to the list
    ///     @param visualItemIndex: index to insert at, -1 for end of list
    ///     @param makeCurrentItem: true: Make this item the current item
    /// @return Newly created item
    Q_INVOKABLE VisualMissionItem*  insertCancelROIMissionItem(int visualItemIndex, bool makeCurrentItem = false);

    /// Add a new complex mission item to the list
    ///     @param itemName: Name of complex item to create (from complexMissionItemNames)
    ///     @param mapCenterCoordinate: coordinate for current center of map
    ///     @param visualItemIndex: index to insert at, -1 for end of list
    ///     @param makeCurrentItem: true: Make this item the current item
    /// @return Newly created item
    Q_INVOKABLE VisualMissionItem*  insertComplexMissionItem(QString itemName, QGeoCoordinate mapCenterCoordinate, int visualItemIndex, bool makeCurrentItem = false);

    /// Add a new complex mission item to the list
    ///     @param itemName: Name of complex item to create (from complexMissionItemNames)
    ///     @param file: kml or shp file to load from shape from
    ///     @param coordinate: Coordinate for item
    ///     @param visualItemIndex: index to insert at, -1 for end of list
    ///     @param makeCurrentItem: true: Make this item the current item
    /// @return Newly created item
    Q_INVOKABLE VisualMissionItem*  insertComplexMissionItemFromKMLOrSHP(QString itemName, QString file, int visualItemIndex, bool makeCurrentItem = false);

    Q_INVOKABLE void resumeMission(int resumeIndex);

    /// Updates the altitudes of the items in the current mission to the new default altitude
    Q_INVOKABLE void applyDefaultMissionAltitude(void);

    /// Sets a new current mission item (PlanView).
    ///     @param sequenceNumber - index for new item, -1 to clear current item
    Q_INVOKABLE void setCurrentPlanViewSeqNum(int sequenceNumber, bool force);

    enum SendToVehiclePreCheckState {
        SendToVehiclePreCheckStateOk,                       // Ok to send plan to vehicle
        SendToVehiclePreCheckStateNoActiveVehicle,          // There is no active vehicle
        SendToVehiclePreCheckStateFirwmareVehicleMismatch,  // Firmware/Vehicle type for plan mismatch with actual vehicle
        SendToVehiclePreCheckStateActiveMission,            // Vehicle is currently flying a mission
    };
    Q_ENUM(SendToVehiclePreCheckState)

    Q_INVOKABLE SendToVehiclePreCheckState sendToVehiclePreCheck(void);

    /// Determines if the mission has all data needed to be saved or sent to the vehicle.
    /// IMPORTANT NOTE: The return value is a VisualMissionItem::ReadForSaveState value. It is an int here to work around
    /// a nightmare of circular header dependency problems.
    int readyForSaveState(void) const;

    /// Sends the mission items to the specified vehicle
    static void sendItemsToVehicle(Vehicle* vehicle, QmlObjectListModel* visualMissionItems);

    bool loadJsonFile(QFile& file, QString& errorString);
    bool loadTextFile(QFile& file, QString& errorString);

    QGCGeoBoundingCube* travelBoundingCube  () { return &_travelBoundingCube; }
    QGeoCoordinate      takeoffCoordinate   () { return _takeoffCoordinate; }

    // Overrides from PlanElementController
    bool supported                  (void) const final { return true; }
    void start                      (bool flyView) final;
    void save                       (QJsonObject& json) final;
    bool load                       (const QJsonObject& json, QString& errorString) final;
    void loadFromVehicle            (void) final;
    void sendToVehicle              (void) final;
    void removeAll                  (void) final;
    void removeAllFromVehicle       (void) final;
    bool syncInProgress             (void) const final;
    bool dirty                      (void) const final;
    void setDirty                   (bool dirty) final;
    bool containsItems              (void) const final;
    bool showPlanFromManagerVehicle (void) final;

    // Create KML file
    void addMissionToKML(KMLPlanDomDocument& planKML);

    // Property accessors

    QmlObjectListModel* visualItems                 (void) { return _visualItems; }
    QmlObjectListModel* simpleFlightPathSegments    (void) { return &_simpleFlightPathSegments; }
    QmlObjectListModel* directionArrows             (void) { return &_directionArrows; }
    QmlObjectListModel* incompleteComplexItemLines  (void) { return &_incompleteComplexItemLines; }
    QVariantList        waypointPath                (void) { return _waypointPath; }
    QStringList         complexMissionItemNames     (void) const;
    QGeoCoordinate      plannedHomePosition         (void) const;
    VisualMissionItem*  currentPlanViewItem         (void) const { return _currentPlanViewItem; }
    TakeoffMissionItem* takeoffMissionItem          (void) const { return _takeoffMissionItem; }
    double              progressPct                 (void) const { return _progressPct; }
    QString             surveyComplexItemName       (void) const;
    QString             corridorScanComplexItemName (void) const;
    QString             structureScanComplexItemName(void) const;
    bool                isInsertTakeoffValid        (void) const;
    double              minAMSLAltitude             (void) const { return _minAMSLAltitude; }
    double              maxAMSLAltitude             (void) const { return _maxAMSLAltitude; }

    int missionItemCount            (void) const { return _missionItemCount; }
    int currentMissionIndex         (void) const;
    int resumeMissionIndex          (void) const;
    int currentPlanViewSeqNum       (void) const { return _currentPlanViewSeqNum; }
    int currentPlanViewVIIndex      (void) const { return _currentPlanViewVIIndex; }

    double  missionDistance         (void) const { return _missionFlightStatus.totalDistance; }
    double  missionTime             (void) const { return _missionFlightStatus.totalTime; }
    double  missionHoverDistance    (void) const { return _missionFlightStatus.hoverDistance; }
    double  missionHoverTime        (void) const { return _missionFlightStatus.hoverTime; }
    double  missionCruiseDistance   (void) const { return _missionFlightStatus.cruiseDistance; }
    double  missionCruiseTime       (void) const { return _missionFlightStatus.cruiseTime; }
    double  missionMaxTelemetry     (void) const { return _missionFlightStatus.maxTelemetryDistance; }

    int  batteryChangePoint         (void) const { return _missionFlightStatus.batteryChangePoint; }    ///< -1 for not supported, 0 for not needed
    int  batteriesRequired          (void) const { return _missionFlightStatus.batteriesRequired; }     ///< -1 for not supported

    bool isEmpty                    (void) const;

    QGroundControlQmlGlobal::AltMode globalAltitudeMode(void);
    QGroundControlQmlGlobal::AltMode globalAltitudeModeDefault(void);
    void setGlobalAltitudeMode(QGroundControlQmlGlobal::AltMode altMode);

signals:
    void visualItemsChanged                 (void);
    void waypointPathChanged                (void);
    void splitSegmentChanged                (void);
    void newItemsFromVehicle                (void);
    void missionDistanceChanged             (double missionDistance);
    void missionTimeChanged                 (void);
    void missionHoverDistanceChanged        (double missionHoverDistance);
    void missionHoverTimeChanged            (void);
    void missionCruiseDistanceChanged       (double missionCruiseDistance);
    void missionCruiseTimeChanged           (void);
    void missionMaxTelemetryChanged         (double missionMaxTelemetry);
    void complexMissionItemNamesChanged     (void);
    void resumeMissionIndexChanged          (void);
    void resumeMissionReady                 (void);
    void resumeMissionUploadFail            (void);
    void batteryChangePointChanged          (int batteryChangePoint);
    void batteriesRequiredChanged           (int batteriesRequired);
    void plannedHomePositionChanged         (QGeoCoordinate plannedHomePosition);
    void progressPctChanged                 (double progressPct);
    void currentMissionIndexChanged         (int currentMissionIndex);
    void currentPlanViewSeqNumChanged       (void);
    void currentPlanViewVIIndexChanged      (void);
    void currentPlanViewItemChanged         (void);
    void takeoffMissionItemChanged          (void);
    void missionBoundingCubeChanged         (void);
    void missionItemCountChanged            (int missionItemCount);
    void onlyInsertTakeoffValidChanged      (void);
    void isInsertTakeoffValidChanged        (void);
    void isInsertLandValidChanged           (void);
    void isROIActiveChanged                 (void);
    void isROIBeginCurrentItemChanged       (void);
    void flyThroughCommandsAllowedChanged   (void);
    void previousCoordinateChanged          (void);
    void minAMSLAltitudeChanged             (double minAMSLAltitude);
    void maxAMSLAltitudeChanged             (double maxAMSLAltitude);
    void recalcTerrainProfile               (void);
    void _recalcMissionFlightStatusSignal   (void);
    void _recalcFlightPathSegmentsSignal    (void);
    void globalAltitudeModeChanged          (void);

private slots:
    void _newMissionItemsAvailableFromVehicle   (bool removeAllRequested);
    void _itemCommandChanged                    (void);
    void _inProgressChanged                     (bool inProgress);
    void _currentMissionIndexChanged            (int sequenceNumber);
    void _recalcFlightPathSegments              (void);
    void _recalcMissionFlightStatus             (void);
    void _updateContainsItems                   (void);
    void _progressPctChanged                    (double progressPct);
    void _visualItemsDirtyChanged               (bool dirty);
    void _managerSendComplete                   (bool error);
    void _managerRemoveAllComplete              (bool error);
    void _updateTimeout                         (void);
    void _complexBoundingBoxChanged             (void);
    void _recalcAll                             (void);
    void _managerVehicleChanged                 (Vehicle* managerVehicle);
    void _takeoffItemNotRequiredChanged         (void);

private:
    void                    _init                               (void);
    void                    _recalcSequence                     (void);
    void                    _recalcChildItems                   (void);
    void                    _recalcAllWithCoordinate            (const QGeoCoordinate& coordinate);
    void                    _recalcROISpecialVisuals            (void);
    void                    _initAllVisualItems                 (void);
    void                    _deinitAllVisualItems               (void);
    void                    _initVisualItem                     (VisualMissionItem* item);
    void                    _deinitVisualItem                   (VisualMissionItem* item);
    void                    _setupActiveVehicle                 (Vehicle* activeVehicle, bool forceLoadFromVehicle);
    void                    _calcPrevWaypointValues             (VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference);
    bool                    _findPreviousAltitude               (int newIndex, double* prevAltitude, QGroundControlQmlGlobal::AltMode* prevAltMode);
    MissionSettingsItem*    _addMissionSettings                 (QmlObjectListModel* visualItems);
    void                    _centerHomePositionOnMissionItems   (QmlObjectListModel* visualItems);
    bool                    _loadJsonMissionFile                (const QByteArray& bytes, QmlObjectListModel* visualItems, QString& errorString);
    bool                    _loadJsonMissionFileV1              (const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    bool                    _loadJsonMissionFileV2              (const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    bool                    _loadTextMissionFile                (QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString);
    int                     _nextSequenceNumber                 (void);
    void                    _scanForAdditionalSettings          (QmlObjectListModel* visualItems, PlanMasterController* masterController);
    void                    _setPlannedHomePositionFromFirstCoordinate(const QGeoCoordinate& clickCoordinate);
    void                    _resetMissionFlightStatus           (void);
    void                    _addHoverTime                       (double hoverTime, double hoverDistance, int waypointIndex);
    void                    _addCruiseTime                      (double cruiseTime, double cruiseDistance, int wayPointIndex);
    void                    _updateBatteryInfo                  (int waypointIndex);
    bool                    _loadItemsFromJson                  (const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    void                    _initLoadedVisualItems              (QmlObjectListModel* loadedVisualItems);
    FlightPathSegment*      _addFlightPathSegment               (FlightPathSegmentHashTable& prevItemPairHashTable, VisualItemPair& pair, bool mavlinkTerrainFrame);
    void                    _addTimeDistance                    (bool vtolInHover, double hoverTime, double cruiseTime, double extraTime, double distance, int seqNum);
    VisualMissionItem*      _insertSimpleMissionItemWorker      (QGeoCoordinate coordinate, MAV_CMD command, int visualItemIndex, bool makeCurrentItem);
    void                    _insertComplexMissionItemWorker     (const QGeoCoordinate& mapCenterCoordinate, ComplexMissionItem* complexItem, int visualItemIndex, bool makeCurrentItem);
    bool                    _isROIBeginItem                     (SimpleMissionItem* simpleItem);
    bool                    _isROICancelItem                    (SimpleMissionItem* simpleItem);
    FlightPathSegment*      _createFlightPathSegmentWorker      (VisualItemPair& pair, bool mavlinkTerrainFrame);
    void                    _allItemsRemoved                    (void);
    void                    _firstItemAdded                     (void);

    static double           _calcDistanceToHome                 (VisualMissionItem* currentItem, VisualMissionItem* homeItem);
    static double           _normalizeLat                       (double lat);
    static double           _normalizeLon                       (double lon);
    static bool             _convertToMissionItems              (QmlObjectListModel* visualMissionItems, QList<MissionItem*>& rgMissionItems, QObject* missionItemParent);

private:
    Vehicle*                    _controllerVehicle =            nullptr;
    Vehicle*                    _managerVehicle =               nullptr;
    MissionManager*             _missionManager =               nullptr;
    int                         _missionItemCount =             0;
    QmlObjectListModel*         _visualItems =                  nullptr;
    MissionSettingsItem*        _settingsItem =                 nullptr;
    PlanViewSettings*           _planViewSettings =             nullptr;
    QmlObjectListModel          _simpleFlightPathSegments;
    QVariantList                _waypointPath;
    QmlObjectListModel          _directionArrows;
    QmlObjectListModel          _incompleteComplexItemLines;
    FlightPathSegmentHashTable  _flightPathSegmentHashTable;
    bool                        _firstItemsFromVehicle =        false;
    bool                        _itemsRequested =               false;
    bool                        _inRecalcSequence =             false;
    MissionFlightStatus_t       _missionFlightStatus;
    AppSettings*                _appSettings =                  nullptr;
    double                      _progressPct =                  0;
    int                         _currentPlanViewSeqNum =        -1;
    int                         _currentPlanViewVIIndex =       -1;
    VisualMissionItem*          _currentPlanViewItem =          nullptr;
    TakeoffMissionItem*         _takeoffMissionItem =           nullptr;
    QTimer                      _updateTimer;
    QGCGeoBoundingCube          _travelBoundingCube;
    QGeoCoordinate              _takeoffCoordinate;
    QGeoCoordinate              _previousCoordinate;
    FlightPathSegment*          _splitSegment =                 nullptr;
    bool                        _delayedSplitSegmentUpdate =    false;
    bool                        _onlyInsertTakeoffValid =       true;
    bool                        _isInsertTakeoffValid =         true;
    bool                        _isInsertLandValid =            false;
    bool                        _isROIActive =                  false;
    bool                        _flyThroughCommandsAllowed =    false;
    bool                        _isROIBeginCurrentItem =        false;
    double                      _minAMSLAltitude =              0;
    double                      _maxAMSLAltitude =              0;
    bool                        _missionContainsVTOLTakeoff =   false;

    QGroundControlQmlGlobal::AltMode _globalAltMode = QGroundControlQmlGlobal::AltitudeModeRelative;

    static const char*  _settingsGroup;

    // Json file keys for persistence
    static const char*  _jsonFileTypeValue;
    static const char*  _jsonFirmwareTypeKey;
    static const char*  _jsonVehicleTypeKey;
    static const char*  _jsonCruiseSpeedKey;
    static const char*  _jsonHoverSpeedKey;
    static const char*  _jsonItemsKey;
    static const char*  _jsonPlannedHomePositionKey;
    static const char*  _jsonParamsKey;
    static const char*  _jsonGlobalPlanAltitudeModeKey;

    // Deprecated V1 format keys
    static const char*  _jsonMavAutopilotKey;
    static const char*  _jsonComplexItemsKey;

    static const int    _missionFileVersion;
};
