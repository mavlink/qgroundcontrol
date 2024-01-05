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

#include "MissionController.h"
#include "GeoFenceController.h"
#include "RallyPointController.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

class AppSettings;
class MissionControllerTest;
class PlanMasterControllerTest;

Q_DECLARE_LOGGING_CATEGORY(PlanMasterControllerLog)

/// Master controller for mission, fence, rally
class PlanMasterController : public QObject
{
    Q_OBJECT
    
public:
    PlanMasterController(QObject* parent = nullptr);
#ifdef QT_DEBUG
    // Used by test code to create master controller with specific firmware/vehicle type
    PlanMasterController(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType, QObject* parent = nullptr);
#endif

    ~PlanMasterController();

    Q_PROPERTY(QString                  planName                READ planName                               NOTIFY planNameChanged)
    Q_PROPERTY(QString                  planType                READ planType                               NOTIFY planTypeChanged)
    Q_PROPERTY(bool                     flyView                 MEMBER _flyView)
    Q_PROPERTY(Vehicle*                 controllerVehicle       READ controllerVehicle                      CONSTANT)                       ///< Offline controller vehicle
    Q_PROPERTY(Vehicle*                 managerVehicle          READ managerVehicle                         NOTIFY managerVehicleChanged)   ///< Either active vehicle or _controllerVehicle if no active vehicle
    Q_PROPERTY(MissionController*       missionController       READ missionController                      CONSTANT)
    Q_PROPERTY(GeoFenceController*      geoFenceController      READ geoFenceController                     CONSTANT)
    Q_PROPERTY(RallyPointController*    rallyPointController    READ rallyPointController                   CONSTANT)
    Q_PROPERTY(bool                     offline                 READ offline                                NOTIFY offlineChanged)          ///< true: controller is not connected to an active vehicle
    Q_PROPERTY(bool                     containsItems           READ containsItems                          NOTIFY containsItemsChanged)    ///< true: Elemement is non-empty
    Q_PROPERTY(bool                     syncInProgress          READ syncInProgress                         NOTIFY syncInProgressChanged)   ///< true: Information is currently being saved/sent, false: no active save/send in progress
    Q_PROPERTY(bool                     dirty                   READ dirty                  WRITE setDirty  NOTIFY dirtyChanged)            ///< true: Unsaved/sent changes are present, false: no changes since last save/send
    Q_PROPERTY(QString                  fileExtension           READ fileExtension                          CONSTANT)                       ///< File extension for missions
    Q_PROPERTY(QString                  kmlFileExtension        READ kmlFileExtension                       CONSTANT)
    Q_PROPERTY(QStringList              loadNameFilters         READ loadNameFilters                        CONSTANT)                       ///< File filter list loading plan files
    Q_PROPERTY(QStringList              saveNameFilters         READ saveNameFilters                        CONSTANT)                       ///< File filter list saving plan files
    Q_PROPERTY(QmlObjectListModel*      planCreators            MEMBER _planCreators                        NOTIFY planCreatorsChanged)

    /// Should be called immediately upon Component.onCompleted.
    Q_INVOKABLE void start(void);

    /// Starts the controller using a single static active vehicle. Will not track global active vehicle changes.
    ///     @param deleteWhenSendCmplete The PlanMasterController object should be deleted after the first send is completed.
    Q_INVOKABLE void startStaticActiveVehicle(Vehicle* vehicle, bool deleteWhenSendCompleted = false);

    /// Determines if the plan has all information needed to be saved or sent to the vehicle.
    /// IMPORTANT NOTE: The return value is a VisualMissionItem::ReadForSaveState value. It is an int here to work around
    /// a nightmare of circular header dependency problems.
    Q_INVOKABLE int readyForSaveState(void) const { return _missionController.readyForSaveState(); }

    /// Replaces any current plan with the plan from the manager vehicle even if offline.
    Q_INVOKABLE void showPlanFromManagerVehicle(void);

    /// Sends a plan to the specified file
    ///     @param[in] vehicle Vehicle we are sending a plan to
    ///     @param[in] filename Plan file to load
    static void sendPlanToVehicle(Vehicle* vehicle, const QString& filename);

    Q_INVOKABLE QStringList availablePlanNames(void) const;
    Q_INVOKABLE void loadFromVehicle(void);
    Q_INVOKABLE void sendToVehicle(void);
    Q_INVOKABLE void loadPlan(const QString& planName);     ///< Loads a plan from the default save location. planName is the name without the extension
    Q_INVOKABLE void import(const QString& filename);       ///< Imports a plan from a file: .plan, .waypoints, .mission, ...
    Q_INVOKABLE void deletePlan();
    Q_INVOKABLE void closePlan();
    Q_INVOKABLE void savePlan();
    Q_INVOKABLE void saveToKML(const QString& filename);
    Q_INVOKABLE void removeAll(void);                       ///< Removes all from controller only, synce required to remove from vehicle
    Q_INVOKABLE void removeAllFromVehicle(void);            ///< Removes all from vehicle and controller
    Q_INVOKABLE void renamePlan(const QString& planName);

    MissionController*      missionController(void)     { return &_missionController; }
    GeoFenceController*     geoFenceController(void)    { return &_geoFenceController; }
    RallyPointController*   rallyPointController(void)  { return &_rallyPointController; }

    bool        offline             (void) const { return _offline; }
    bool        containsItems       (void) const;
    bool        syncInProgress      (void) const;
    bool        dirty               (void) const;
    void        setDirty            (bool dirty);
    QString     fileExtension       (void) const;
    QString     kmlFileExtension    (void) const;
    QStringList loadNameFilters     (void) const;
    QStringList saveNameFilters     (void) const;
    bool        isEmpty             (void) const;
    QString     planName            (void) const { return _planName; }
    QString     planType            (void) const { return _planType; }
    QString     generateNewPlanName (const QString& prefix) const;

    void        setFlyView(bool flyView) { _flyView = flyView; }
    void        setPlanType(const QString& planType);

    QJsonDocument saveToJson    ();

    Vehicle* controllerVehicle(void) { return _controllerVehicle; }
    Vehicle* managerVehicle(void) { return _managerVehicle; }

    static const int    kPlanFileVersion;
    static const char*  kPlanFileType;
    static const char*  kJsonPlanTypeKey;
    static const char*  kJsonMissionObjectKey;
    static const char*  kJsonGeoFenceObjectKey;
    static const char*  kJsonRallyPointsObjectKey;

signals:
    void containsItemsChanged               (bool containsItems);
    void syncInProgressChanged              (void);
    void dirtyChanged                       (bool dirty);
    void offlineChanged                     (bool offlineEditing);
    void planNameChanged                    (QString planName);
    void planTypeChanged                    (QString planType);
    void planClosed                         (void);
    void planCreatorsChanged                (QmlObjectListModel* planCreators);
    void managerVehicleChanged              (Vehicle* managerVehicle);
    void promptForPlanUsageOnVehicleChange  (void);
    void importComplete                     (void);

private slots:
    void _activeVehicleChanged      (Vehicle* activeVehicle);
    void _loadMissionComplete       (void);
    void _loadGeoFenceComplete      (void);
    void _loadRallyPointsComplete   (void);
    void _sendMissionComplete       (void);
    void _sendGeoFenceComplete      (void);
    void _sendRallyPointsComplete   (void);
    void _updatePlanCreatorsList    (void);

private:
    void    _commonInit                 (void);
    void    _showPlanFromManagerVehicle (void);
    QString _fullyQualifiedPlanName     (const QString& planName) const;
    void    _loadWorker                 (const QString& filename);

    MultiVehicleManager*    _multiVehicleMgr =          nullptr;
    Vehicle*                _controllerVehicle =        nullptr;    ///< Offline controller vehicle
    Vehicle*                _managerVehicle =           nullptr;    ///< Either active vehicle or _controllerVehicle if none
    bool                    _flyView =                  true;
    bool                    _offline =                  true;
    MissionController       _missionController;
    GeoFenceController      _geoFenceController;
    RallyPointController    _rallyPointController;
    bool                    _loadGeoFence =             false;
    bool                    _loadRallyPoints =          false;
    bool                    _sendGeoFence =             false;
    bool                    _sendRallyPoints =          false;
    bool                    _deleteWhenSendCompleted =  false;
    QmlObjectListModel*     _planCreators =             nullptr;
    QString                 _planName;
    QString                 _planType;
    AppSettings*            _appSettings =              nullptr;
};
