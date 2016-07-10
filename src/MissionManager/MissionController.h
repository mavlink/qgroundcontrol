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

#include <QObject>
#include <QHash>

#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"
#include "MavlinkQmlSingleton.h"
#include "VisualMissionItem.h"

class CoordinateVector;

Q_DECLARE_LOGGING_CATEGORY(MissionControllerLog)

typedef QPair<VisualMissionItem*,VisualMissionItem*> VisualItemPair;
typedef QHash<VisualItemPair, CoordinateVector*> CoordVectHashTable;
class MissionController : public QObject
{
    Q_OBJECT

public:
    MissionController(QObject* parent = NULL);
    ~MissionController();

    Q_PROPERTY(QmlObjectListModel*  visualItems         READ visualItems                                NOTIFY visualItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  complexVisualItems  READ complexVisualItems                         NOTIFY complexVisualItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  waypointLines       READ waypointLines                              NOTIFY waypointLinesChanged)
    Q_PROPERTY(bool                 autoSync            READ autoSync               WRITE setAutoSync   NOTIFY autoSyncChanged)
    Q_PROPERTY(bool                 syncInProgress      READ syncInProgress                             NOTIFY syncInProgressChanged)

    Q_INVOKABLE void start(bool editMode);
    Q_INVOKABLE void getMissionItems(void);
    Q_INVOKABLE void sendMissionItems(void);
    Q_INVOKABLE void loadMissionFromFilePicker(void);
    Q_INVOKABLE void loadMissionFromFile(const QString& filename);
    Q_INVOKABLE void saveMissionToFilePicker(void);
    Q_INVOKABLE void saveMissionToFile(const QString& filename);
    Q_INVOKABLE void removeMissionItem(int index);
    Q_INVOKABLE void removeAllMissionItems(void);

    /// Add a new simple mission item to the list
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertSimpleMissionItem(QGeoCoordinate coordinate, int i);

    /// Add a new complex mission item to the list
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertComplexMissionItem(QGeoCoordinate coordinate, int i);

    // Property accessors

    QmlObjectListModel* visualItems         (void) { return _visualItems; }
    QmlObjectListModel* complexVisualItems  (void) { return _complexItems; }
    QmlObjectListModel* waypointLines       (void) { return &_waypointLines; }

    bool autoSync(void) { return _autoSync; }
    void setAutoSync(bool autoSync);
    bool syncInProgress(void);

    static const char* jsonSimpleItemsKey;  ///< Key for simple items in a json file

signals:
    void visualItemsChanged(void);
    void complexVisualItemsChanged(void);
    void waypointLinesChanged(void);
    void autoSyncChanged(bool autoSync);
    void newItemsFromVehicle(void);
    void syncInProgressChanged(bool syncInProgress);

private slots:
    void _newMissionItemsAvailableFromVehicle();
    void _itemCommandChanged(void);
    void _activeVehicleChanged(Vehicle* activeVehicle);
    void _activeVehicleHomePositionAvailableChanged(bool homePositionAvailable);
    void _activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition);
    void _dirtyChanged(bool dirty);
    void _inProgressChanged(bool inProgress);
    void _currentMissionItemChanged(int sequenceNumber);
    void _recalcWaypointLines(void);
    void _recalcAltitudeRangeBearing();

private:
    void _recalcSequence(void);
    void _recalcChildItems(void);
    void _recalcAll(void);
    void _initAllVisualItems(void);
    void _deinitAllVisualItems(void);
    void _initVisualItem(VisualMissionItem* item);
    void _deinitVisualItem(VisualMissionItem* item);
    void _autoSyncSend(void);
    void _setupActiveVehicle(Vehicle* activeVehicle, bool forceLoadFromVehicle);
    static void _calcPrevWaypointValues(double homeAlt, VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference);
    bool _findLastAltitude(double* lastAltitude, MAV_FRAME* frame);
    bool _findLastAcceptanceRadius(double* lastAcceptanceRadius);
    void _addPlannedHomePosition(QmlObjectListModel* visualItems, bool addToCenter);
    double _normalizeLat(double lat);
    double _normalizeLon(double lon);
    bool _loadJsonMissionFile(const QByteArray& bytes, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString);
    bool _loadTextMissionFile(QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString);
    int _nextSequenceNumber(void);

private:
    bool                _editMode;
    QmlObjectListModel* _visualItems;
    QmlObjectListModel* _complexItems;
    QmlObjectListModel  _waypointLines;
    CoordVectHashTable  _linesTable;
    Vehicle*            _activeVehicle;
    bool                _autoSync;
    bool                _firstItemsFromVehicle;
    bool                _missionItemsRequested;
    bool                _queuedSend;

    static const char*  _settingsGroup;
    static const char*  _jsonVersionKey;
    static const char*  _jsonGroundStationKey;
    static const char*  _jsonMavAutopilotKey;
    static const char*  _jsonComplexItemsKey;
    static const char*  _jsonPlannedHomePositionKey;
};

#endif
