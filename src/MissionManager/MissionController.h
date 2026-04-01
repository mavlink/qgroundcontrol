#pragma once

#include <QtCore/QHash>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QPersistentModelIndex>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "PlanElementController.h"
#include "QmlObjectListModel.h"
#include "QmlObjectTreeModel.h"
#include "QGCGeoBoundingCube.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCMAVLink.h"
#include "MissionFlightStatus.h"
#include "MissionFlightStatusCalculator.h"

Q_DECLARE_LOGGING_CATEGORY(MissionControllerLog)

class FlightPathSegment;
class VisualMissionItem;
class MissionItem;
class AppSettings;
class MissionManager;
class SimpleMissionItem;
class ComplexMissionItem;
class LandingComplexItem;
class MissionSettingsItem;
class TakeoffMissionItem;
class PlanViewSettings;
class KMLPlanDomDocument;
class Vehicle;

typedef QPair<VisualMissionItem*,VisualMissionItem*> VisualItemPair;
typedef QHash<VisualItemPair, FlightPathSegment*> FlightPathSegmentHashTable;

class MissionController : public PlanElementController
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("FlightPathSegment.h")
    Q_MOC_INCLUDE("VisualMissionItem.h")
    Q_MOC_INCLUDE("TakeoffMissionItem.h")

public:
    MissionController(PlanMasterController* masterController, QObject* parent = nullptr);
    ~MissionController();

    // Legacy alias kept for source compatibility with external code
    using MissionFlightStatus_t = ::MissionFlightStatus_t;

    Q_PROPERTY(QmlObjectListModel*  visualItems                     READ visualItems                    NOTIFY visualItemsReset)
    Q_PROPERTY(QmlObjectTreeModel*  visualItemsTree                 READ visualItemsTree                CONSTANT)                               ///< Tree-structured view of visualItems for TreeView
    Q_PROPERTY(QPersistentModelIndex planFileGroupIndex              READ planFileGroupIndex              CONSTANT)
    Q_PROPERTY(QPersistentModelIndex defaultsGroupIndex              READ defaultsGroupIndex              CONSTANT)
    Q_PROPERTY(QPersistentModelIndex missionGroupIndex               READ missionGroupIndex               CONSTANT)
    Q_PROPERTY(QPersistentModelIndex fenceGroupIndex                 READ fenceGroupIndex                 CONSTANT)
    Q_PROPERTY(QPersistentModelIndex rallyGroupIndex                 READ rallyGroupIndex                 CONSTANT)
    Q_PROPERTY(QPersistentModelIndex transformGroupIndex              READ transformGroupIndex              CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  simpleFlightPathSegments        READ simpleFlightPathSegments       CONSTANT)                               ///< Used by Plan view only for interactive editing
    Q_PROPERTY(QmlObjectListModel*  directionArrows                 READ directionArrows                CONSTANT)
    Q_PROPERTY(QStringList          complexMissionItemNames         READ complexMissionItemNames        NOTIFY complexMissionItemNamesChanged)
    Q_PROPERTY(QGeoCoordinate       plannedHomePosition             READ plannedHomePosition            NOTIFY plannedHomePositionChanged)      ///< Includes AMSL altitude
    Q_PROPERTY(bool                 homePositionSet                 READ homePositionSet                NOTIFY homePositionSetChanged)          ///< true: Home position has been set by the user
    Q_PROPERTY(QGeoCoordinate       previousCoordinate              MEMBER _previousCoordinate          NOTIFY planViewStateChanged)
    Q_PROPERTY(FlightPathSegment*   splitSegment                    MEMBER _splitSegment                NOTIFY splitSegmentChanged)             ///< Segment which show show + split ui element
    Q_PROPERTY(double               progressPct                     READ progressPct                    NOTIFY progressPctChanged)
    Q_PROPERTY(int                  currentMissionIndex             READ currentMissionIndex            NOTIFY currentMissionIndexChanged)
    Q_PROPERTY(int                  resumeMissionIndex              READ resumeMissionIndex             NOTIFY resumeMissionIndexChanged)       ///< Returns the item index two which a mission should be resumed. -1 indicates resume mission not available.
    Q_PROPERTY(int                  currentPlanViewSeqNum           READ currentPlanViewSeqNum          NOTIFY planViewStateChanged)
    Q_PROPERTY(int                  currentPlanViewVIIndex          READ currentPlanViewVIIndex         NOTIFY planViewStateChanged)
    Q_PROPERTY(VisualMissionItem*   currentPlanViewItem             READ currentPlanViewItem            NOTIFY planViewStateChanged)
    Q_PROPERTY(TakeoffMissionItem*  takeoffMissionItem              READ takeoffMissionItem             NOTIFY takeoffMissionItemChanged)
    Q_PROPERTY(double               missionTotalDistance            READ missionTotalDistance           NOTIFY missionTotalDistanceChanged)
    Q_PROPERTY(double               missionPlannedDistance          READ missionPlannedDistance         NOTIFY missionPlannedDistanceChanged)
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
    Q_PROPERTY(bool                 onlyInsertTakeoffValid          MEMBER _onlyInsertTakeoffValid      NOTIFY planViewStateChanged)
    Q_PROPERTY(bool                 isInsertTakeoffValid            MEMBER _isInsertTakeoffValid        NOTIFY planViewStateChanged)
    Q_PROPERTY(bool                 isInsertLandValid               MEMBER _isInsertLandValid           NOTIFY planViewStateChanged)
    Q_PROPERTY(bool                 hasLandItem                     MEMBER _hasLandItem                 NOTIFY hasLandItemChanged)
    Q_PROPERTY(bool                 multipleLandPatternsAllowed     READ multipleLandPatternsAllowed    NOTIFY multipleLandPatternsAllowedChanged)
    Q_PROPERTY(bool                 isROIActive                     MEMBER _isROIActive                 NOTIFY planViewStateChanged)
    Q_PROPERTY(bool                 isROIBeginCurrentItem           MEMBER _isROIBeginCurrentItem       NOTIFY planViewStateChanged)
    Q_PROPERTY(bool                 flyThroughCommandsAllowed       MEMBER _flyThroughCommandsAllowed   NOTIFY planViewStateChanged)
    Q_PROPERTY(double               minAMSLAltitude                 MEMBER _minAMSLAltitude             NOTIFY minAMSLAltitudeChanged)          ///< Minimum altitude associated with this mission. Used to calculate percentages for terrain status.
    Q_PROPERTY(double               maxAMSLAltitude                 MEMBER _maxAMSLAltitude             NOTIFY maxAMSLAltitudeChanged)          ///< Maximum altitude associated with this mission. Used to calculate percentages for terrain status.

    Q_PROPERTY(QGroundControlQmlGlobal::AltitudeFrame globalAltitudeFrame         READ globalAltitudeFrame         WRITE setGlobalAltitudeFrame NOTIFY globalAltitudeFrameChanged)
    Q_PROPERTY(QGroundControlQmlGlobal::AltitudeFrame globalAltitudeFrameDefault  READ globalAltitudeFrameDefault  NOTIFY globalAltitudeFrameChanged)                               ///< Default to use for newly created items

    Q_INVOKABLE void removeVisualItem(int viIndex);

    /// Returns the visual item index for the given VisualMissionItem object, or -1 if not found
    Q_INVOKABLE int visualItemIndexForObject(QObject* object) const;

    /// Set the planned home position from a map click
    Q_INVOKABLE void setHomePosition(QGeoCoordinate coordinate);

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
    ///     @param force - true: reset internals even if specified item is already selected
    Q_INVOKABLE void setCurrentPlanViewSeqNum(int sequenceNumber, bool force);

    /// Repositions all mission items which specify a coordinate around a new
    /// home coordinate. Requires a valid planned home position; otherwise the
    /// mission is not modified and a warning is logged.
    /// @param newHome New coordinate for the home item
    /// @param repositionTakeoffItems If true, items identified as takeoff items
    ///                               (isTakeoffItem) will be repositioned
    /// @param repositionLandingItems If true, items identified as landing items
    ///                               (isLandCommand) will be repositioned
    Q_INVOKABLE void repositionMission(const QGeoCoordinate& newHome,
                                       bool repositionTakeoffItems = true,
                                       bool repositionLandingItems = true);

    /// Offsets all mission items which specify a coordinate by the specified
    /// ENU amounts in meters. Home altitude remains unchanged.
    /// @param eastMeters Distance to offset items to the east, in meters
    /// @param northMeters Distance to offset items to the north, in meters
    /// @param upMeters Distance to offset items upwards, in meters
    /// @param offsetTakeoffItems If true, items identified as takeoff items
    ///                           (isTakeoffItem) will be offset
    /// @param offsetLandingItems If true, items identified as landing items
    ///                           (isLandCommand) will be offset
    Q_INVOKABLE void offsetMission(double eastMeters,
                                   double northMeters,
                                   double upMeters = 0.0,
                                   bool offsetTakeoffItems = false,
                                   bool offsetLandingItems = false);

    /// Rotates all mission items which specify a coordinate around the up axis
    /// of the home position. Complex items are rotated by moving their
    /// reference coordinate: their geometry and orientation are not modified.
    /// Requires a valid planned home position; otherwise the mission is not
    /// modified and a warning is logged.
    /// @param degreesCW Angle to rotate items by, in degrees clockwise
    /// @param rotateTakeoffItems If true, items identified as takeoff items
    ///                           (isTakeoffItem) will be rotated
    /// @param rotateLandingItems If true, items identified as landing items
    ///                           (isLandCommand) will be rotated
    Q_INVOKABLE void rotateMission(double degreesCW,
                                   bool rotateTakeoffItems = false,
                                   bool rotateLandingItems = false);

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
    QmlObjectTreeModel* visualItemsTree             (void) { return &_visualItemsTree; }
    QPersistentModelIndex planFileGroupIndex         (void) const { return _planFileGroupIndex; }
    QPersistentModelIndex defaultsGroupIndex         (void) const { return _defaultsGroupIndex; }
    QPersistentModelIndex missionGroupIndex          (void) const { return _missionGroupIndex; }
    QPersistentModelIndex fenceGroupIndex            (void) const { return _fenceGroupIndex; }
    QPersistentModelIndex rallyGroupIndex            (void) const { return _rallyGroupIndex; }
    QPersistentModelIndex transformGroupIndex         (void) const { return _transformGroupIndex; }
    QmlObjectListModel* simpleFlightPathSegments    (void) { return &_simpleFlightPathSegments; }
    QmlObjectListModel* directionArrows             (void) { return &_directionArrows; }
    QStringList         complexMissionItemNames     (void) const;
    QGeoCoordinate      plannedHomePosition         (void) const;
    bool                homePositionSet             (void) const;
    VisualMissionItem*  currentPlanViewItem         (void) const { return _currentPlanViewItem; }
    TakeoffMissionItem* takeoffMissionItem          (void) const { return _takeoffMissionItem; }
    double              progressPct                 (void) const { return _progressPct; }
    QString             surveyComplexItemName       (void) const;
    QString             corridorScanComplexItemName (void) const;
    QString             structureScanComplexItemName(void) const;
    bool                isInsertTakeoffValid        (void) const;
    bool                multipleLandPatternsAllowed (void) const;
    double              minAMSLAltitude             (void) const { return _minAMSLAltitude; }
    double              maxAMSLAltitude             (void) const { return _maxAMSLAltitude; }

    int currentMissionIndex         (void) const;
    int resumeMissionIndex          (void) const;
    int currentPlanViewSeqNum       (void) const { return _currentPlanViewSeqNum; }
    int currentPlanViewVIIndex      (void) const { return _currentPlanViewVIIndex; }

    double  missionTotalDistance    (void) const { return _missionFlightStatus.totalDistance; }
    double  missionPlannedDistance  (void) const { return _missionFlightStatus.plannedDistance; }
    double  missionTime             (void) const { return _missionFlightStatus.totalTime; }
    double  missionHoverDistance    (void) const { return _missionFlightStatus.hoverDistance; }
    double  missionHoverTime        (void) const { return _missionFlightStatus.hoverTime; }
    double  missionCruiseDistance   (void) const { return _missionFlightStatus.cruiseDistance; }
    double  missionCruiseTime       (void) const { return _missionFlightStatus.cruiseTime; }
    double  missionMaxTelemetry     (void) const { return _missionFlightStatus.maxTelemetryDistance; }

    int  batteryChangePoint         (void) const { return _missionFlightStatus.batteryChangePoint; }    ///< -1 for not supported, 0 for not needed
    int  batteriesRequired          (void) const { return _missionFlightStatus.batteriesRequired; }     ///< -1 for not supported

    bool isFirstLandingComplexItem  (const LandingComplexItem* item) const;
    bool isEmpty                    (void) const;

    QGroundControlQmlGlobal::AltitudeFrame globalAltitudeFrame(void);
    QGroundControlQmlGlobal::AltitudeFrame globalAltitudeFrameDefault(void);
    void setGlobalAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrame altFrame);

    // Top-level group row indices in _visualItemsTree (must match _setupTreeModel order)
    static constexpr int kPlanFileGroupRow = 0;
    static constexpr int kDefaultsGroupRow = 1;
    static constexpr int kMissionGroupRow  = 2;
    static constexpr int kFenceGroupRow    = 3;
    static constexpr int kRallyGroupRow    = 4;
    static constexpr int kTransformGroupRow = 5;
    static constexpr int kGroupCount       = 6;

signals:
    void visualItemsReset                   (void);
    void splitSegmentChanged                (void);
    void newItemsFromVehicle                (void);
    void missionTotalDistanceChanged        (double missionTotalDistance);
    void missionPlannedDistanceChanged      (double missionPlannedDistance);
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
    void homePositionSetChanged              (void);
    void progressPctChanged                 (double progressPct);
    void currentMissionIndexChanged         (int currentMissionIndex);
    void planViewStateChanged               (void);  ///< All plan-view properties are recomputed together in setCurrentPlanViewSeqNum, so one signal covers them all
    void takeoffMissionItemChanged          (void);
    void missionBoundingCubeChanged         (void);
    void hasLandItemChanged                 (void);
    void multipleLandPatternsAllowedChanged (void);
    void minAMSLAltitudeChanged             (double minAMSLAltitude);
    void maxAMSLAltitudeChanged             (double maxAMSLAltitude);
    void recalcTerrainProfile               (void);
    void _recalcMissionFlightStatusSignal   (void);
    void _recalcFlightPathSegmentsSignal    (void);
    void globalAltitudeFrameChanged          (void);

private slots:
    void _newMissionItemsAvailableFromVehicle   (bool removeAllRequested);
    void _itemCommandChanged                    (void);
    void _inProgressChanged                     (bool inProgress);
    void _currentMissionIndexChanged            (int sequenceNumber);
    void _recalcFlightPathSegments              (void);
    void _recalcMissionFlightStatus             (void);
    void _progressPctChanged                    (double progressPct);
    void _visualItemsDirtyChanged               (bool dirty);
    void _managerSendComplete                   (bool error);
    void _managerRemoveAllComplete              (bool error);
    void _updateTimeout                         (void);
    void _complexBoundingBoxChanged             (void);
    void _recalcAll                             (void);
    void _managerVehicleChanged                 (Vehicle* managerVehicle);
    void _forceRecalcOfAllowedBits              (void);
    // Incremental tree model sync slots
    void _syncTreeMissionItemsInserted                (const QModelIndex& parent, int first, int last);
    void _syncTreeMissionItemsAboutToBeRemoved         (const QModelIndex& parent, int first, int last);
    void _syncTreeMissionItemsReset                   (void);
    void _syncTreeRallyPointsInserted                 (const QModelIndex& parent, int first, int last);
    void _syncTreeRallyPointsAboutToBeRemoved          (const QModelIndex& parent, int first, int last);
    void _syncTreeRallyPointsRemoved                   (const QModelIndex& parent, int first, int last);

    void _syncTreeRallyPointsReset                    (void);
private:
    void                    _init                               (void);
    void                    _setupTreeModel                     (void);
    void                    _recalcSequence                     (void);
    void                    _recalcChildItems                   (void);
    void                    _recalcAllWithCoordinate            (const QGeoCoordinate& coordinate);
    void                    _setupNewVisualItems                (QmlObjectListModel* newItems = nullptr);
    void                    _initAllVisualItems                 (void);
    void                    _deinitAllVisualItems               (void);
    void                    _initVisualItem                     (VisualMissionItem* item);
    void                    _deinitVisualItem                   (VisualMissionItem* item);
    void                    _setupActiveVehicle                 (Vehicle* activeVehicle, bool forceLoadFromVehicle);
    bool                    _findPreviousAltitude               (int newIndex, double* prevAltitude, QGroundControlQmlGlobal::AltitudeFrame* prevAltFrame);
    MissionSettingsItem*    _addMissionSettings                 (QmlObjectListModel* visualItems);
    bool                    _loadJsonMissionFileV2              (const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    bool                    _loadTextMissionFile                (QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString);
    int                     _nextSequenceNumber                 (void);
    void                    _scanForAdditionalSettings          (QmlObjectListModel* visualItems, PlanMasterController* masterController);
    void                    _setPlannedHomePositionFromFirstCoordinate(const QGeoCoordinate& clickCoordinate);
    void                    _resetMissionFlightStatus           (void);
    void                    _initLoadedVisualItems              (QmlObjectListModel* loadedVisualItems);
    FlightPathSegment*      _addFlightPathSegment               (FlightPathSegmentHashTable& prevItemPairHashTable, VisualItemPair& pair, bool mavlinkTerrainFrame);
    VisualMissionItem*      _insertSimpleMissionItemWorker      (QGeoCoordinate coordinate, MAV_CMD command, int visualItemIndex, bool makeCurrentItem);
    void                    _insertComplexMissionItemWorker     (const QGeoCoordinate& mapCenterCoordinate, ComplexMissionItem* complexItem, int visualItemIndex, bool makeCurrentItem);
    bool                    _isROIBeginItem                     (SimpleMissionItem* simpleItem);
    bool                    _isROICancelItem                    (SimpleMissionItem* simpleItem);
    FlightPathSegment*      _createFlightPathSegmentWorker      (VisualItemPair& pair, bool mavlinkTerrainFrame);
    void                    _allItemsRemoved                    (void);
    void                    _firstItemAdded                     (void);

    static double           _normalizeLat                       (double lat);
    static double           _normalizeLon                       (double lon);
    static bool             _convertToMissionItems              (QmlObjectListModel* visualMissionItems, QList<MissionItem*>& rgMissionItems, QObject* missionItemParent);

private:
    Vehicle*                    _controllerVehicle =            nullptr;
    Vehicle*                    _managerVehicle =               nullptr;
    MissionManager*             _missionManager =               nullptr;
    QmlObjectListModel*         _visualItems =                  nullptr;
    QPersistentModelIndex       _planFileGroupIndex;            ///< Persistent index for "Plan File" group in tree
    QPersistentModelIndex       _defaultsGroupIndex;            ///< Persistent index for "Defaults" group in tree
    QPersistentModelIndex       _missionGroupIndex;             ///< Persistent index for "Mission Items" group in tree
    QPersistentModelIndex       _fenceGroupIndex;               ///< Persistent index for "GeoFence" group in tree
    QPersistentModelIndex       _rallyGroupIndex;               ///< Persistent index for "Rally Points" group in tree
    QPersistentModelIndex       _transformGroupIndex;            ///< Persistent index for "Transform" group in tree
    QObject                     _planFileGroupNode;             ///< Group node for "Plan File" in tree view
    QObject                     _planFileInfoMarker;            ///< Marker child for plan file info delegate
    QObject                     _defaultsGroupNode;             ///< Group node for "Defaults" in tree view
    QObject                     _defaultsInfoMarker;            ///< Marker child for defaults editor delegate
    QObject                     _missionItemsGroupNode;         ///< Group node for "Mission Items" in tree view
    QObject                     _fenceGroupNode;                ///< Group node for "GeoFence" in tree view
    QObject                     _rallyGroupNode;                ///< Group node for "Rally Points" in tree view
    QObject                     _transformGroupNode;             ///< Group node for "Transform" in tree view
    QObject                     _fenceEditorMarker;             ///< Marker child for GeoFenceEditor delegate
    QObject                     _rallyHeaderMarker;             ///< Marker child for RallyPointEditorHeader delegate
    QObject                     _transformEditorMarker;          ///< Marker child for TransformEditor delegate
    QmlObjectTreeModel          _visualItemsTree;               // Must be declared after group nodes so it's destroyed first
    MissionSettingsItem*        _settingsItem =                 nullptr;
    PlanViewSettings*           _planViewSettings =             nullptr;
    QmlObjectListModel          _simpleFlightPathSegments;
    QmlObjectListModel          _directionArrows;
    FlightPathSegmentHashTable  _flightPathSegmentHashTable;
    bool                        _firstItemsFromVehicle =        false;
    bool                        _itemsRequested =               false;
    bool                        _inRecalcSequence =             false;
    MissionFlightStatusCalculator _flightStatusCalc;
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
    bool                        _hasLandItem =                  false;
    bool                        _isROIActive =                  false;
    bool                        _flyThroughCommandsAllowed =    false;
    bool                        _isROIBeginCurrentItem =        false;
    double                      _minAMSLAltitude =              0;
    double                      _maxAMSLAltitude =              0;
    bool                        _missionContainsVTOLTakeoff =   false;

    QGroundControlQmlGlobal::AltitudeFrame _globalAltFrame = QGroundControlQmlGlobal::AltitudeFrameRelative;

    static constexpr const char* _settingsGroup =                 "MissionController";
    static constexpr const char* _jsonItemsKey =                  "items";
    static constexpr const char* _jsonPlannedHomePositionKey =    "plannedHomePosition";
    static constexpr const char* _jsonFirmwareTypeKey =           "firmwareType";
    static constexpr const char* _jsonVehicleTypeKey =            "vehicleType";
    static constexpr const char* _jsonCruiseSpeedKey =            "cruiseSpeed";
    static constexpr const char* _jsonHoverSpeedKey =             "hoverSpeed";
    static constexpr const char* _jsonParamsKey =                 "params";
    static constexpr const char* _jsonGlobalPlanAltitudeModeKey = "globalPlanAltitudeMode";

    static constexpr int   _missionFileVersion =            2;
};
