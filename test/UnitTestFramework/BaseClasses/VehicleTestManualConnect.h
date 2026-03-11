#pragma once

#include "MultiVehicleManager.h"
#include "VehicleTest.h"

/// Test fixture for tests that manage their own MockLink connection.
///
/// Unlike VehicleTest which auto-connects in init(), this class skips
/// auto-connect and lets tests call _connectMockLink() manually.
/// Use this when tests need to:
/// - Create MockLinks with specific failure modes
/// - Create MockLinks directly via MockLink::startPX4MockLink()
/// - Control exactly when connection happens
///
/// Example:
/// @code
/// class MyTest : public VehicleTestManualConnect
/// {
///     Q_OBJECT
/// private slots:
///     void _myTest() {
///         _connectMockLink(MAV_AUTOPILOT_PX4);
///         // ... test with vehicle ...
///         _disconnectMockLink();
///     }
/// };
/// @endcode
class VehicleTestManualConnect : public VehicleTest
{
    Q_OBJECT

public:
    explicit VehicleTestManualConnect(QObject* parent = nullptr) : VehicleTest(parent)
    {
    }

protected slots:

    void init() override
    {
        UnitTest::init();
        MultiVehicleManager::instance()->init();
    }

    void cleanup() override
    {
        _disconnectMockLink();
        UnitTest::cleanup();
    }
};
