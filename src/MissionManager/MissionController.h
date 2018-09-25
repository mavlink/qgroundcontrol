/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MissionController_H
#define MissionController_H

#include "PlanElementController.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include "QGCGeoBoundingCube.h"

#include <QHash>

class CoordinateVector;
class VisualMissionItem;
class MissionItem;
class MissionSettingsItem;
class AppSettings;
class MissionManager;
class SimpleMissionItem;
class ComplexMissionItem;
class QDomDocument;

Q_DECLARE_LOGGING_CATEGORY(MissionControllerLog)

typedef QPair<VisualMissionItem*,VisualMissionItem*> VisualItemPair;
typedef QHash<VisualItemPair, CoordinateVector*> CoordVectHashTable;

class MissionController : public PlanElementController
{
    Q_OBJECT

public:
    MissionController(PlanMasterController* masterController, QObject* parent = NULL);
    ~MissionController();

    typedef struct _MissionFlightStatus_t {
        double  maxTelemetryDistance;
        double  totalDistance;
        double  totalTime;
        double  hoverDistance;
        double  hoverTime;
        double  cruiseDistance;
        double  cruiseTime;
        double  cruiseSpeed;
        double  hoverSpeed;
        double  vehicleSpeed;           ///< Either cruise or hover speed based on vehicle type and vtol state
        double  vehicleYaw;
        double  gimbalYaw;              ///< NaN signals yaw was never changed
        double  gimbalPitch;            ///< NaN signals pitch was never changed
        int     mAhBattery;             ///< 0 for not available
        double  hoverAmps;              ///< Amp consumption during hover
        double  cruiseAmps;             ///< Amp consumption during cruise
        double  ampMinutesAvailable;    ///< Amp minutes available from single battery
        double  hoverAmpsTotal;         ///< Total hover amps used
        double  cruiseAmpsTotal;        ///< Total cruise amps used
        int     batteryChangePoint;     ///< -1 for not supported, 0 for not needed
        int     batteriesRequired;      ///< -1 for not supported
    } MissionFlightStatus_t;

    Q_PROPERTY(QmlObjectListModel*  visualItems             READ visualItems                NOTIFY visualItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  waypointLines           READ waypointLines              NOTIFY waypointLinesChanged)        ///< Used by Plan view only for interactive editing
    Q_PROPERTY(QVariantList         waypointPath            READ waypointPath               NOTIFY waypointPathChanged)         ///< Used by Fly view only for static display
    Q_PROPERTY(QStringList          complexMissionItemNames READ complexMissionItemNames    NOTIFY complexMissionItemNamesChanged)
    Q_PROPERTY(QGeoCoordinate       plannedHomePosition     READ plannedHomePosition        NOTIFY plannedHomePositionChanged)

    Q_PROPERTY(double               progressPct             READ progressPct                NOTIFY progressPctChanged)

    Q_PROPERTY(int                  missionItemCount        READ missionItemCount           NOTIFY missionItemCountChanged)     ///< True mission item command count (only valid in Fly View)
    Q_PROPERTY(int                  currentMissionIndex     READ currentMissionIndex        NOTIFY currentMissionIndexChanged)
    Q_PROPERTY(int                  resumeMissionIndex      READ resumeMissionIndex         NOTIFY resumeMissionIndexChanged)   ///< Returns the item index two which a mission should be resumed. -1 indicates resume mission not available.

    Q_PROPERTY(int                  currentPlanViewIndex    READ currentPlanViewIndex       NOTIFY currentPlanViewIndexChanged)
    Q_PROPERTY(VisualMissionItem*   currentPlanViewItem     READ currentPlanViewItem        NOTIFY currentPlanViewItemChanged)

    Q_PROPERTY(double               missionDistance         READ missionDistance            NOTIFY missionDistanceChanged)
    Q_PROPERTY(double               missionTime             READ missionTime                NOTIFY missionTimeChanged)
    Q_PROPERTY(double               missionHoverDistance    READ missionHoverDistance       NOTIFY missionHoverDistanceChanged)
    Q_PROPERTY(double               missionCruiseDistance   READ missionCruiseDistance      NOTIFY missionCruiseDistanceChanged)
    Q_PROPERTY(double               missionHoverTime        READ missionHoverTime           NOTIFY missionHoverTimeChanged)
    Q_PROPERTY(double               missionCruiseTime       READ missionCruiseTime          NOTIFY missionCruiseTimeChanged)
    Q_PROPERTY(double               missionMaxTelemetry     READ missionMaxTelemetry        NOTIFY missionMaxTelemetryChanged)

    Q_PROPERTY(int                  batteryChangePoint      READ batteryChangePoint         NOTIFY batteryChangePointChanged)
    Q_PROPERTY(int                  batteriesRequired       READ batteriesRequired          NOTIFY batteriesRequiredChanged)
    Q_PROPERTY(QGCGeoBoundingCube*  travelBoundingCube      READ travelBoundingCube         NOTIFY missionBoundingCubeChanged)

    Q_PROPERTY(QString              surveyComplexItemName           READ surveyComplexItemName          CONSTANT)
    Q_PROPERTY(QString              corridorScanComplexItemName     READ corridorScanComplexItemName    CONSTANT)
    Q_PROPERTY(QString              structureScanComplexItemName    READ structureScanComplexItemName   CONSTANT)

    Q_INVOKABLE void removeMissionItem(int index);

    /// Add a new simple mission item to the list
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertSimpleMissionItem(QGeoCoordinate coordinate, int i);

    /// Add a new ROI mission item to the list
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertROIMissionItem(QGeoCoordinate coordinate, int i);

    /// Add a new complex mission item to the list
    ///     @param itemName: Name of complex item to create (from complexMissionItemNames)
    ///     @param mapCenterCoordinate: coordinate for current center of map
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertComplexMissionItem(QString itemName, QGeoCoordinate mapCenterCoordinate, int i);

    /// Add a new complex mission item to the list
    ///     @param itemName: Name of complex item to create (from complexMissionItemNames)
    ///     @param i: index to insert at, -1 for end
    /// @return Sequence number for new item
    Q_INVOKABLE int insertComplexMissionItemFromKML(QString itemName, QString kmlFile, int i);

    Q_INVOKABLE void resumeMission(int resumeIndex);

    /// Updates the altitudes of the items in the current mission to the new default altitude
    Q_INVOKABLE void applyDefaultMissionAltitude(void);

    /// Sets a new current mission item (PlanView).
    ///     @param sequenceNumber - index for new item, -1 to clear current item
    Q_INVOKABLE void setCurrentPlanViewIndex(int sequenceNumber, bool force);

    /// Determines if the mission has all data needed to be saved or sent to the vehicle. Currently the only case where this
    /// would return false is when it is still waiting on terrain data to determine correct altitudes.
    bool readyForSaveSend(void) const;

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
    void managerVehicleChanged      (Vehicle* managerVehicle) final;
    bool showPlanFromManagerVehicle (void) final;

    // Create KML file
    void convertToKMLDocument(QDomDocument& document);

    // Property accessors

    QmlObjectListModel* visualItems                 (void) { return _visualItems; }
    QmlObjectListModel* waypointLines               (void) { return &_waypointLines; }
    QVariantList        waypointPath                (void) { return _waypointPath; }
    QStringList         complexMissionItemNames     (void) const;
    QGeoCoordinate      plannedHomePosition         (void) const;
    VisualMissionItem*  currentPlanViewItem         (void) const;
    double              progressPct                 (void) const { return _progressPct; }
    QString             surveyComplexItemName       (void) const { return _surveyMissionItemName; }
    QString             corridorScanComplexItemName (void) const { return _corridorScanMissionItemName; }
    QString             structureScanComplexItemName(void) const { return _structureScanMissionItemName; }

    int missionItemCount            (void) const { return _missionItemCount; }
    int currentMissionIndex         (void) const;
    int resumeMissionIndex          (void) const;
    int currentPlanViewIndex        (void) const;

    double  missionDistance         (void) const { return _missionFlightStatus.totalDistance; }
    double  missionTime             (void) const { return _missionFlightStatus.totalTime; }
    double  missionHoverDistance    (void) const { return _missionFlightStatus.hoverDistance; }
    double  missionHoverTime        (void) const { return _missionFlightStatus.hoverTime; }
    double  missionCruiseDistance   (void) const { return _missionFlightStatus.cruiseDistance; }
    double  missionCruiseTime       (void) const { return _missionFlightStatus.cruiseTime; }
    double  missionMaxTelemetry     (void) const { return _missionFlightStatus.maxTelemetryDistance; }

    int  batteryChangePoint         (void) const { return _missionFlightStatus.batteryChangePoint; }    ///< -1 for not supported, 0 for not needed
    int  batteriesRequired          (void) const { return _missionFlightStatus.batteriesRequired; }     ///< -1 for not supported

signals:
    void visualItemsChanged             (void);
    void waypointLinesChanged           (void);
    void waypointPathChanged            (void);
    void newItemsFromVehicle            (void);
    void missionDistanceChanged         (double missionDistance);
    void missionTimeChanged             (void);
    void missionHoverDistanceChanged    (double missionHoverDistance);
    void missionHoverTimeChanged        (void);
    void missionCruiseDistanceChanged   (double missionCruiseDistance);
    void missionCruiseTimeChanged       (void);
    void missionMaxTelemetryChanged     (double missionMaxTelemetry);
    void complexMissionItemNamesChanged (void);
    void resumeMissionIndexChanged      (void);
    void resumeMissionReady             (void);
    void resumeMissionUploadFail        (void);
    void batteryChangePointChanged      (int batteryChangePoint);
    void batteriesRequiredChanged       (int batteriesRequired);
    void plannedHomePositionChanged     (QGeoCoordinate plannedHomePosition);
    void progressPctChanged             (double progressPct);
    void currentMissionIndexChanged     (int currentMissionIndex);
    void currentPlanViewIndexChanged    (void);
    void currentPlanViewItemChanged     (void);
    void missionBoundingCubeChanged     (void);
    void missionItemCountChanged        (int missionItemCount);

private slots:
    void _newMissionItemsAvailableFromVehicle(bool removeAllRequested);
    void _itemCommandChanged(void);
    void _managerVehicleHomePositionChanged(const QGeoCoordinate& homePosition);
    void _inProgressChanged(bool inProgress);
    void _currentMissionIndexChanged(int sequenceNumber);
    void _recalcWaypointLines(void);
    void _recalcMissionFlightStatus(void);
    void _updateContainsItems(void);
    void _progressPctChanged(double progressPct);
    void _visualItemsDirtyChanged(bool dirty);
    void _managerSendComplete(bool error);
    void _managerRemoveAllComplete(bool error);
    void _updateTimeout();
    void _complexBoundingBoxChanged();

private:
    void _init(void);
    void _recalcSequence(void);
    void _recalcChildItems(void);
    void _recalcAll(void);
    void _initAllVisualItems(void);
    void _deinitAllVisualItems(void);
    void _initVisualItem(VisualMissionItem* item);
    void _deinitVisualItem(VisualMissionItem* item);
    void _setupActiveVehicle(Vehicle* activeVehicle, bool forceLoadFromVehicle);
    void _calcPrevWaypointValues(double homeAlt, VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference);
    static double _calcDistanceToHome(VisualMissionItem* currentItem, VisualMissionItem* homeItem);
    bool _findPreviousAltitude(int newIndex, double* prevAltitude, int* prevAltitudeMode);
    static double _normalizeLat(double lat);
    static double _normalizeLon(double lon);
    void _addMissionSettings(QmlObjectListModel* visualItems, bool addToCenter);
    bool _loadJsonMissionFile(const QByteArray& bytes, QmlObjectListModel* visualItems, QString& errorString);
    bool _loadJsonMissionFileV1(const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    bool _loadJsonMissionFileV2(const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    bool _loadTextMissionFile(QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString);
    int _nextSequenceNumber(void);
    void _scanForAdditionalSettings(QmlObjectListModel* visualItems, Vehicle* vehicle);
    static bool _convertToMissionItems(QmlObjectListModel* visualMissionItems, QList<MissionItem*>& rgMissionItems, QObject* missionItemParent);
    void _setPlannedHomePositionFromFirstCoordinate(void);
    void _resetMissionFlightStatus(void);
    void _addHoverTime(double hoverTime, double hoverDistance, int waypointIndex);
    void _addCruiseTime(double cruiseTime, double cruiseDistance, int wayPointIndex);
    void _updateBatteryInfo(int waypointIndex);
    bool _loadItemsFromJson(const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    void _initLoadedVisualItems(QmlObjectListModel* loadedVisualItems);
    void _addWaypointLineSegment(CoordVectHashTable& prevItemPairHashTable, VisualItemPair& pair);
    void _addTimeDistance(bool vtolInHover, double hoverTime, double cruiseTime, double extraTime, double distance, int seqNum);
    int _insertComplexMissionItemWorker(ComplexMissionItem* complexItem, int i);
    void _warnIfTerrainFrameUsed(void);

private:
    MissionManager*         _missionManager;
    int                     _missionItemCount;
    QmlObjectListModel*     _visualItems;
    MissionSettingsItem*    _settingsItem;
    QmlObjectListModel      _waypointLines;
    QVariantList            _waypointPath;
    CoordVectHashTable      _linesTable;
    bool                    _firstItemsFromVehicle;
    bool                    _itemsRequested;
    bool                    _inRecalcSequence;
    MissionFlightStatus_t   _missionFlightStatus;
    QString                 _surveyMissionItemName;
    QString                 _fwLandingMissionItemName;
    QString                 _structureScanMissionItemName;
    QString                 _corridorScanMissionItemName;
    AppSettings*            _appSettings;
    double                  _progressPct;
    int                     _currentPlanViewIndex;
    VisualMissionItem*      _currentPlanViewItem;
    QTimer                  _updateTimer;
    QGCGeoBoundingCube      _travelBoundingCube;
    QGeoCoordinate          _takeoffCoordinate;

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

    // Deprecated V1 format keys
    static const char*  _jsonMavAutopilotKey;
    static const char*  _jsonComplexItemsKey;

    static const int    _missionFileVersion;
};

#endif
