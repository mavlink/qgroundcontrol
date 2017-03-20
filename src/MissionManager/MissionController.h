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
#include "MavlinkQmlSingleton.h"
#include "VisualMissionItem.h"

#include <QHash>

class CoordinateVector;

Q_DECLARE_LOGGING_CATEGORY(MissionControllerLog)

typedef QPair<VisualMissionItem*,VisualMissionItem*> VisualItemPair;
typedef QHash<VisualItemPair, CoordinateVector*> CoordVectHashTable;
class MissionController : public PlanElementController
{
    Q_OBJECT

public:
    MissionController(QObject* parent = NULL);
    ~MissionController();

    // Mission settings
    Q_PROPERTY(QGeoCoordinate       plannedHomePosition     READ plannedHomePosition    NOTIFY plannedHomePositionChanged)
    Q_PROPERTY(QmlObjectListModel*  visualItems             READ visualItems            NOTIFY visualItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  waypointLines           READ waypointLines          NOTIFY waypointLinesChanged)
    Q_PROPERTY(QStringList          complexMissionItemNames MEMBER _complexMissionItemNames CONSTANT)

    Q_PROPERTY(double               missionDistance         READ missionDistance        NOTIFY missionDistanceChanged)
    Q_PROPERTY(double               missionTime             READ missionTime            NOTIFY missionTimeChanged)
    Q_PROPERTY(double               missionHoverDistance    READ missionHoverDistance   NOTIFY missionHoverDistanceChanged)
    Q_PROPERTY(double               missionCruiseDistance   READ missionCruiseDistance  NOTIFY missionCruiseDistanceChanged)
    Q_PROPERTY(double               missionHoverTime        READ missionHoverTime       NOTIFY missionHoverTimeChanged)
    Q_PROPERTY(double               missionCruiseTime       READ missionCruiseTime      NOTIFY missionCruiseTimeChanged)
    Q_PROPERTY(double               missionMaxTelemetry     READ missionMaxTelemetry    NOTIFY missionMaxTelemetryChanged)

    Q_INVOKABLE void removeMissionItem(int index);

    /// Add a new simple mission item to the list
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertSimpleMissionItem(QGeoCoordinate coordinate, int i);

    /// Add a new complex mission item to the list
    ///     @param itemName: Name of complex item to create (from complexMissionItemNames)
    ///     @param mapCenterCoordinate: coordinate for current center of map
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertComplexMissionItem(QString itemName, QGeoCoordinate mapCenterCoordinate, int i);

    /// Loads the mission items from the specified file
    ///     @param[in] vehicle Vehicle we are loading items for
    ///     @param[in] filename File to load from
    ///     @param[out] visualItems Visual items loaded, returns NULL if error
    /// @return success/fail
    static bool loadItemsFromFile(Vehicle* vehicle, const QString& filename, QmlObjectListModel** visualItems);

    /// Sends the mission items to the specified vehicle
    static void sendItemsToVehicle(Vehicle* vehicle, QmlObjectListModel* visualMissionItems);

    // Overrides from PlanElementController
    void start                      (bool editMode) final;
    void startStaticActiveVehicle   (Vehicle* vehicle) final;
    void loadFromVehicle            (void) final;
    void sendToVehicle              (void) final;
    void loadFromFilePicker         (void) final;
    void loadFromFile               (const QString& filename) final;
    void saveToFilePicker           (void) final;
    void saveToFile                 (const QString& filename) final;
    void removeAll                  (void) final;
    bool syncInProgress             (void) const final;
    bool dirty                      (void) const final;
    void setDirty                   (bool dirty) final;

    QString fileExtension(void) const final;

    // Property accessors

    QGeoCoordinate      plannedHomePosition (void);
    QmlObjectListModel* visualItems         (void) { return _visualItems; }
    QmlObjectListModel* waypointLines       (void) { return &_waypointLines; }

    double  missionDistance         (void) const { return _missionDistance; }
    double  missionTime             (void) const { return _missionTime; }
    double  missionHoverDistance    (void) const { return _missionHoverDistance; }
    double  missionHoverTime        (void) const { return _missionHoverTime; }
    double  missionCruiseDistance   (void) const { return _missionCruiseDistance; }
    double  missionCruiseTime       (void) const { return _missionCruiseTime; }
    double  missionMaxTelemetry     (void) const { return _missionMaxTelemetry; }
    double  cruiseSpeed             (void) const;
    double  hoverSpeed              (void) const;

signals:
    void plannedHomePositionChanged(QGeoCoordinate plannedHomePosition);
    void visualItemsChanged(void);
    void waypointLinesChanged(void);
    void newItemsFromVehicle(void);
    void missionDistanceChanged(double missionDistance);
    void missionTimeChanged(void);
    void missionHoverDistanceChanged(double missionHoverDistance);
    void missionHoverTimeChanged(void);
    void missionCruiseDistanceChanged(double missionCruiseDistance);
    void missionCruiseTimeChanged(void);
    void missionMaxTelemetryChanged(double missionMaxTelemetry);
    void cruiseDistanceChanged(double cruiseDistance);
    void hoverDistanceChanged(double hoverDistance);
    void cruiseSpeedChanged(double cruiseSpeed);
    void hoverSpeedChanged(double hoverSpeed);

private slots:
    void _newMissionItemsAvailableFromVehicle();
    void _itemCommandChanged(void);
    void _activeVehicleHomePositionAvailableChanged(bool homePositionAvailable);
    void _activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition);
    void _inProgressChanged(bool inProgress);
    void _currentMissionItemChanged(int sequenceNumber);
    void _recalcWaypointLines(void);
    void _recalcAltitudeRangeBearing(void);
    void _homeCoordinateChanged(void);

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
    static void _calcPrevWaypointValues(double homeAlt, VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference);
    static double _calcDistanceToHome(VisualMissionItem* currentItem, VisualMissionItem* homeItem);
    bool _findPreviousAltitude(int newIndex, double* prevAltitude, MAV_FRAME* prevFrame);
    static double _normalizeLat(double lat);
    static double _normalizeLon(double lon);
    static void _addMissionSettings(Vehicle* vehicle, QmlObjectListModel* visualItems, bool addToCenter);
    static bool _loadJsonMissionFile(Vehicle* vehicle, const QByteArray& bytes, QmlObjectListModel* visualItems, QString& errorString);
    static bool _loadJsonMissionFileV1(Vehicle* vehicle, const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    static bool _loadJsonMissionFileV2(Vehicle* vehicle, const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString);
    static bool _loadTextMissionFile(Vehicle* vehicle, QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString);
    int _nextSequenceNumber(void);
    void _setMissionDistance(double missionDistance);
    void _setMissionTime(double missionTime);
    void _setMissionHoverDistance(double missionHoverDistance);
    void _setMissionHoverTime(double missionHoverTime);
    void _setMissionCruiseDistance(double missionCruiseDistance);
    void _setMissionCruiseTime(double missionCruiseTime);
    void _setMissionMaxTelemetry(double missionMaxTelemetry);

    // Overrides from PlanElementController
    void _activeVehicleBeingRemoved(void) final;
    void _activeVehicleSet(void) final;

private:
    QmlObjectListModel* _visualItems;
    QmlObjectListModel  _waypointLines;
    CoordVectHashTable  _linesTable;
    bool                _firstItemsFromVehicle;
    bool                _missionItemsRequested;
    bool                _queuedSend;
    double              _missionDistance;
    double              _missionTime;
    double              _missionHoverDistance;
    double              _missionHoverTime;
    double              _missionCruiseDistance;
    double              _missionCruiseTime;
    double              _missionMaxTelemetry;    
    QString             _surveyMissionItemName;
    QString             _fwLandingMissionItemName;
    QStringList         _complexMissionItemNames;

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
