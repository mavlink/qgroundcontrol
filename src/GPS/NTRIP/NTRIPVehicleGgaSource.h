#pragma once

#include <optional>

#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/qnumeric.h>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>

#include "GPSObservation.h"

class NTRIPManager;
class RuntimeScheduler;
class Vehicle;

/// The active vehicle's GPS and global position estimate as NTRIP's Vehicle GPS and Vehicle EKF GGA sources.
/// Reports only record their receipt; GGA reports, seconds apart, build observations from the latest.
class NTRIPVehicleGgaSource : public QObject
{
    Q_OBJECT

public:
    /// @a scheduler stamps receipt times and must outlive this source.
    explicit NTRIPVehicleGgaSource(RuntimeScheduler* scheduler, QObject* parent = nullptr);

    /// Supplies both sources to @a ntrip, gated on a connected active vehicle, and follows MultiVehicleManager's active
    /// vehicle. Must precede NTRIPManager::init().
    void attach(NTRIPManager* ntrip);
    /// Tracks @a vehicle's reports; reports from before this call are not used.
    void setVehicle(Vehicle* vehicle);

    /// The fresh observation from the vehicle's GPS with a fix. It has the receiver's height above the ellipsoid
    /// beside its altitude when the vehicle reports one (GPS_RAW_INT.alt_ellipsoid), so that a GGA carries the
    /// receiver's own geoid separation.
    std::optional<GPSObservation> gpsObservation() const;
    /// The fresh observation from the vehicle's GLOBAL_POSITION_INT estimate.
    std::optional<GPSObservation> estimateObservation() const;

private:
    struct GpsReport
    {
        QGeoPositionInfo position;
        int fixType = 0;
        double horizontalDop = qQNaN();
        /// GPS_RAW_INT.alt_ellipsoid, which follows the report it belongs to.
        std::optional<double> altitudeEllipsoidMeters;
        quint64 receivedAtUs = 0;
    };

    struct EstimateReport
    {
        QGeoCoordinate coordinate;
        quint64 receivedAtUs = 0;
    };

    bool _fresh(quint64 receivedAtUs) const;

    RuntimeScheduler* const _scheduler;
    std::optional<GpsReport> _gps;
    std::optional<EstimateReport> _estimate;
    QMetaObject::Connection _gpsConnection;
    QMetaObject::Connection _messageConnection;
};
