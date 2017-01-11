/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef PlanElementController_H
#define PlanElementController_H

#include <QObject>

#include "Vehicle.h"
#include "MultiVehicleManager.h"

/// This is the abstract base clas for Plan Element controllers.
/// Examples of plan elements are: missions (MissionController), geofence (GeoFenceController)
class PlanElementController : public QObject
{
    Q_OBJECT
    
public:
    PlanElementController(QObject* parent = NULL);
    ~PlanElementController();
    
    /// true: information is currently being saved/sent, false: no active save/send in progress
    Q_PROPERTY(bool syncInProgress READ syncInProgress NOTIFY syncInProgressChanged)

    /// true: unsaved/sent changes are present, false: no changes since last save/send
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

    /// Returns the file extention for plan element file type.
    Q_PROPERTY(QString fileExtension READ fileExtension CONSTANT)
    virtual QString fileExtension(void) const = 0;

    Q_PROPERTY(Vehicle* vehicle READ vehicle NOTIFY vehicleChanged)

    /// Should be called immediately upon Component.onCompleted.
    ///     @param editMode true: controller being used in Plan view, false: controller being used in Fly view
    Q_INVOKABLE virtual void start(bool editMode);

    /// Starts the controller using a single static active vehicle. Will not track global active vehicle changes.
    ///     @param editMode true: controller being used in Plan view, false: controller being used in Fly view
    Q_INVOKABLE virtual void startStaticActiveVehicle(Vehicle* vehicle);

    Q_INVOKABLE virtual void loadFromVehicle(void) = 0;
    Q_INVOKABLE virtual void sendToVehicle(void) = 0;
    Q_INVOKABLE virtual void loadFromFilePicker(void) = 0;
    Q_INVOKABLE virtual void loadFromFile(const QString& filename) = 0;
    Q_INVOKABLE virtual void saveToFilePicker(void) = 0;
    Q_INVOKABLE virtual void saveToFile(const QString& filename) = 0;
    Q_INVOKABLE virtual void removeAll(void) = 0;

    virtual bool syncInProgress (void) const = 0;
    virtual bool dirty          (void) const = 0;
    virtual void setDirty       (bool dirty) = 0;

    Vehicle* vehicle(void) { return _activeVehicle; }

signals:
    void syncInProgressChanged  (bool syncInProgress);
    void dirtyChanged           (bool dirty);
    void vehicleChanged         (Vehicle* vehicle);

protected:
    MultiVehicleManager*    _multiVehicleMgr;
    Vehicle*                _activeVehicle;     ///< Currently active vehicle, can be disconnected offline editing vehicle
    bool                    _editMode;

    /// Called when the current active vehicle is about to be removed. Derived classes should override
    /// to implement custom behavior.
    virtual void _activeVehicleBeingRemoved(void) = 0;

    /// Called when a new active vehicle has been set. Derived classes should override
    /// to implement custom behavior.
    virtual void _activeVehicleSet(void) = 0;

private slots:
    void _activeVehicleChanged(Vehicle* activeVehicle);
};

#endif
