#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

class GPSPositionService;
class GPSRtk;
class GPSSourceHealth;
class NTRIPManager;
class Vehicle;

/// Supplies NTRIP's GGA and caster-sort positions from the active vehicle, the RTK receiver and the ground station.
class GPSGgaSources : public QObject
{
    Q_OBJECT

public:
    GPSGgaSources(NTRIPManager* ntrip, GPSRtk* rtk, GPSPositionService* groundStation, QObject* parent = nullptr);

    /// Installs the NTRIP position providers, which must precede NTRIPManager::init(), and follows
    /// MultiVehicleManager's active vehicle.
    void init();

    /// Tracks @a vehicle's position reports as the Vehicle EKF source. Reports from before this call are not used.
    void setActiveVehicle(Vehicle* vehicle);

    GPSSourceHealth* vehicleEstimateHealth() const { return _vehicleEstimateHealth; }

private:
    QPointer<NTRIPManager> _ntrip;
    QPointer<GPSRtk> _rtk;
    QPointer<GPSPositionService> _groundStation;
    GPSSourceHealth* const _vehicleEstimateHealth;
    QMetaObject::Connection _vehiclePositionConnection;
};
