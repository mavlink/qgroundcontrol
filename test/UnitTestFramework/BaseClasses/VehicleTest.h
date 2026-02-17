#pragma once

#include "MAVLinkLib.h"
#include "MockLink.h"
#include "UnitTest.h"

class Vehicle;
class LinkInterface;
class MissionItem;

// ============================================================================
// Parameterized Test Data Providers (Vehicle-related)
// ============================================================================

namespace TestData {

/// Adds autopilot type rows for data-driven tests
/// Creates rows: "PX4" and "ArduPilot" with corresponding MAV_AUTOPILOT values
/// Usage in _data() method: TestData::addAutopilotRows();
inline void addAutopilotRows()
{
    QTest::addColumn<MAV_AUTOPILOT>("autopilot");
    QTest::addColumn<QString>("autopilotName");

    QTest::newRow("PX4") << MAV_AUTOPILOT_PX4 << QStringLiteral("PX4");
    QTest::newRow("ArduPilot") << MAV_AUTOPILOT_ARDUPILOTMEGA << QStringLiteral("ArduPilot");
}

}  // namespace TestData

/// Fetch autopilot data in parameterized test
/// Must be used in test function after UT_PARAMETERIZED_TEST with TestData::addAutopilotRows()
#define UT_FETCH_AUTOPILOT()          \
    QFETCH(MAV_AUTOPILOT, autopilot); \
    QFETCH(QString, autopilotName)

// ============================================================================
// VehicleTest Base Class
// ============================================================================

/// @file
/// @brief Base class for tests requiring a connected vehicle

/// Test fixture that automatically connects a MockLink vehicle before each test
/// and disconnects after. Use this when your tests need an active vehicle.
///
/// Example usage:
/// @code
/// class MyVehicleTest : public VehicleTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testSomething() {
///         // vehicle() is already connected and ready
///         QVERIFY(vehicle());
///         QCOMPARE(vehicle()->armed(), false);
///     }
/// };
/// @endcode
class VehicleTest : public UnitTest
{
    Q_OBJECT

public:
    explicit VehicleTest(QObject* parent = nullptr);
    ~VehicleTest() override = default;

    /// Returns the connected vehicle (only valid during test execution)
    Vehicle* vehicle() const
    {
        return _vehicle;
    }

    /// Returns the MockLink (only valid during test execution)
    MockLink* mockLink() const
    {
        return _mockLink;
    }

    /// Returns the autopilot type used for this test
    MAV_AUTOPILOT autopilotType() const
    {
        return _autopilotType;
    }

protected slots:
    void init() override;
    void cleanup() override;

protected:
    /// Waits for vehicle parameters to be fully loaded
    bool waitForParametersReady(int timeoutMs = 0);

    /// Waits for initial connect sequence to complete
    bool waitForInitialConnect(int timeoutMs = 0);

    /// Set the autopilot type before init() is called
    void setAutopilotType(MAV_AUTOPILOT type)
    {
        _autopilotType = type;
    }

    /// Set failure mode for MockLink
    void setFailureMode(MockConfiguration::FailureMode_t mode)
    {
        _failureMode = mode;
    }

    /// Set whether to wait for parameters during init()
    void setWaitForParameters(bool wait)
    {
        _waitForParameters = wait;
    }

    /// Set whether to wait for initial connect during init()
    void setWaitForInitialConnect(bool wait)
    {
        _waitForInitialConnect = wait;
    }

    /// Simulate communication loss
    void simulateCommLoss(bool lost);

    /// Simulate connection removed
    void simulateConnectionRemoved();

    // ========================================================================
    // MockLink Vehicle Helpers (for tests that need custom connection logic)
    // ========================================================================

    /// Connects a MockLink vehicle and waits for initial connect sequence
    /// @param autopilot Autopilot type (PX4, ArduPilot, Generic)
    /// @param failureMode Optional failure mode for testing error handling
    void _connectMockLink(MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_PX4,
                          MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);

    /// Connects MockLink without waiting for initial connect sequence
    void _connectMockLinkNoInitialConnectSequence()
    {
        _connectMockLink(MAV_AUTOPILOT_INVALID);
    }

    /// Disconnects the current MockLink and waits for vehicle to be removed
    void _disconnectMockLink();

    /// Compares two MissionItems for equality using QCOMPARE/QVERIFY
    static void _missionItemsEqual(const MissionItem& actual, const MissionItem& expected);

    MockLink* _mockLink = nullptr;
    Vehicle* _vehicle = nullptr;

private slots:
    void _linkDeleted(const LinkInterface* link);

private:
    MAV_AUTOPILOT _autopilotType = MAV_AUTOPILOT_PX4;
    MockConfiguration::FailureMode_t _failureMode = MockConfiguration::FailNone;
    bool _waitForParameters = false;
    bool _waitForInitialConnect = true;
};

/// Convenience class for ArduPilot-specific vehicle tests
class VehicleTestAPM : public VehicleTest
{
    Q_OBJECT

public:
    explicit VehicleTestAPM(QObject* parent = nullptr) : VehicleTest(parent)
    {
        setAutopilotType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    }
};

/// Test fixture that connects MockLink but skips the initial connect sequence.
class VehicleTestNoInitialConnect : public VehicleTest
{
    Q_OBJECT

public:
    explicit VehicleTestNoInitialConnect(QObject* parent = nullptr) : VehicleTest(parent)
    {
        setWaitForInitialConnect(false);
    }
};
