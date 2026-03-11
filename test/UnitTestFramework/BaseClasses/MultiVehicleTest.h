#pragma once

#include <QtCore/QPointer>

#include "MAVLinkLib.h"
#include "MockLink.h"
#include "UnitTest.h"

class Vehicle;

/// @file
/// @brief Base class for multi-vehicle scenario tests

/// Test fixture for multi-vehicle tests. Creates multiple MockLink
/// vehicles for testing multi-vehicle management.
///
/// Example usage:
/// @code
/// class MyMultiVehicleTest : public MultiVehicleTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testMultipleVehicles() {
///         createVehicles(3);
///         QCOMPARE(vehicleCount(), 3);
///     }
/// };
/// @endcode
class MultiVehicleTest : public UnitTest
{
    Q_OBJECT

public:
    explicit MultiVehicleTest(QObject* parent = nullptr);
    ~MultiVehicleTest() override = default;

    /// Returns number of connected vehicles
    int vehicleCount() const;

    /// Returns vehicle at index
    Vehicle* vehicleAt(int index) const;

    /// Returns all connected vehicles
    QList<Vehicle*> vehicles() const;

    /// Returns the active vehicle
    Vehicle* activeVehicle() const;

protected slots:
    void cleanup() override;

protected:
    /// Creates N vehicles with unique IDs
    bool createVehicles(int count, MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_PX4);

    /// Creates a single vehicle with specific system ID
    Vehicle* createVehicle(int systemId, MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_PX4);

    /// Sets the active vehicle
    void setActiveVehicle(Vehicle* vehicle);

    /// Disconnects all vehicles
    void disconnectAllVehicles();

private:
    QList<SharedLinkInterfacePtr> _links;
    QList<QPointer<Vehicle>> _vehicles;
};
