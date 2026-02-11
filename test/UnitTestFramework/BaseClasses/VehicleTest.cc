#include "VehicleTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MissionItem.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGC.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(VehicleTestLog, "Test.VehicleTest")

VehicleTest::VehicleTest(QObject* parent) : UnitTest(parent)
{
}

void VehicleTest::init()
{
    UnitTest::init();

    // Initialize vehicle management systems
    MultiVehicleManager::instance()->init();
    LinkManager::instance()->setConnectionsAllowed();
    MAVLinkProtocol::deleteTempLogFiles();

    _connectMockLink(_autopilotType, _failureMode);

    if (!_vehicle) {
        QFAIL("VehicleTest::init - Vehicle connection failed");
        return;
    }

    if (_waitForInitialConnect) {
        if (!waitForInitialConnect()) {
            QFAIL("VehicleTest::init - Timeout waiting for initial connect");
            return;
        }
    }

    if (_waitForParameters) {
        if (!waitForParametersReady()) {
            QFAIL("VehicleTest::init - Timeout waiting for parameters");
            return;
        }
    }
}

void VehicleTest::cleanup()
{
    _disconnectMockLink();
    UnitTest::cleanup();
}

bool VehicleTest::waitForParametersReady(int timeoutMs)
{
    if (!_vehicle || !_vehicle->parameterManager()) {
        return false;
    }

    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::longMs();
    }

    if (_vehicle->parameterManager()->parametersReady()) {
        return true;
    }

    QSignalSpy spy(_vehicle->parameterManager(), &ParameterManager::parametersReadyChanged);
    if (!spy.isValid()) {
        qCWarning(VehicleTestLog) << "waitForParametersReady: Failed to create signal spy";
        return false;
    }

    if (_vehicle->parameterManager()->parametersReady()) {
        return true;
    }

    return spy.wait(timeoutMs);
}

bool VehicleTest::waitForInitialConnect(int timeoutMs)
{
    if (!_vehicle) {
        return false;
    }

    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::longMs();
    }

    if (_vehicle->isInitialConnectComplete()) {
        return true;
    }

    QSignalSpy spy(_vehicle, &Vehicle::initialConnectComplete);
    if (!spy.isValid()) {
        qCWarning(VehicleTestLog) << "waitForInitialConnect: Failed to create signal spy";
        return false;
    }

    if (_vehicle->isInitialConnectComplete()) {
        return true;
    }

    return spy.wait(timeoutMs);
}

void VehicleTest::simulateCommLoss(bool lost)
{
    if (_mockLink) {
        _mockLink->setCommLost(lost);
    }
}

void VehicleTest::simulateConnectionRemoved()
{
    if (_mockLink) {
        _mockLink->simulateConnectionRemoved();
    }
}

void VehicleTest::_connectMockLink(MAV_AUTOPILOT autopilot, MockConfiguration::FailureMode_t failureMode)
{
    Q_ASSERT(!_mockLink);

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

    switch (autopilot) {
        case MAV_AUTOPILOT_PX4:
            _mockLink = MockLink::startPX4MockLink(false /* sendStatusText */, false /* enableCamera */, failureMode);
            break;
        case MAV_AUTOPILOT_ARDUPILOTMEGA:
            _mockLink = MockLink::startAPMArduCopterMockLink(false /* sendStatusText */, false /* enableCamera */, failureMode);
            break;
        case MAV_AUTOPILOT_GENERIC:
            _mockLink = MockLink::startGenericMockLink(false /* sendStatusText */, false /* enableCamera */, failureMode);
            break;
        case MAV_AUTOPILOT_INVALID:
            _mockLink = MockLink::startNoInitialConnectMockLink(false /* sendStatusText */, false /* enableCamera */);
            break;
        default:
            qCWarning(VehicleTestLog) << "Unsupported autopilot type:" << autopilot;
            return;
    }

    // Connect to destroyed signal to prevent dangling pointer
    if (_mockLink) {
        (void)connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });
    }

    QVERIFY2(spyVehicle.wait(TestTimeout::longMs()), "Timeout waiting for vehicle connection");
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY2(_vehicle != nullptr, "Vehicle should not be null after connection");

    if (autopilot != MAV_AUTOPILOT_INVALID) {
        QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
        QVERIFY2(spyConnect.isValid(), "Failed to create spy for initialConnectComplete");
        QVERIFY2(spyConnect.wait(TestTimeout::longMs()), "Timeout waiting for initial connect");
    }
}

void VehicleTest::_disconnectMockLink()
{
    if (_mockLink) {
        QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

        _mockLink->disconnect();
        _mockLink = nullptr;

        QVERIFY2(spyVehicle.wait(TestTimeout::longMs()), "Timeout waiting for vehicle disconnection");
        _vehicle = MultiVehicleManager::instance()->activeVehicle();
        QVERIFY2(!_vehicle, "Vehicle should be null after disconnection");

        // Process ALL pending events including deleteLater() calls to ensure the old vehicle
        // and MockLink are fully destroyed before any subsequent test case creates new ones.
        // This prevents use-after-free issues when callbacks from old objects fire during
        // new object creation.
        //
        // We use a more aggressive approach on CI where system load can cause timer callbacks
        // to be delayed: process events, wait a bit for any pending timers to fire, then
        // process events again. This ensures cascading deletions complete fully.
        //
        // The DeferredDelete event type is specifically important for deleteLater() cleanup.
        static const bool isCi = qEnvironmentVariableIsSet("CI") || qEnvironmentVariableIsSet("GITHUB_ACTIONS");
        const int iterations = isCi ? 10 : 5;
        const int waitMs = isCi ? 20 : 10;

        for (int i = 0; i < iterations; ++i) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            if (i < iterations - 1) {
                QTest::qWait(waitMs);  // Allow pending timer callbacks to fire
            }
        }
    }
}

void VehicleTest::_linkDeleted(const LinkInterface* link)
{
    if (link == _mockLink) {
        _mockLink = nullptr;
    }
}

void VehicleTest::_missionItemsEqual(const MissionItem& actual, const MissionItem& expected)
{
    QCOMPARE(static_cast<int>(actual.command()), static_cast<int>(expected.command()));
    QCOMPARE(static_cast<int>(actual.frame()), static_cast<int>(expected.frame()));
    QCOMPARE(actual.autoContinue(), expected.autoContinue());

    QVERIFY(QGC::fuzzyCompare(actual.param1(), expected.param1()));
    QVERIFY(QGC::fuzzyCompare(actual.param2(), expected.param2()));
    QVERIFY(QGC::fuzzyCompare(actual.param3(), expected.param3()));
    QVERIFY(QGC::fuzzyCompare(actual.param4(), expected.param4()));
    QVERIFY(QGC::fuzzyCompare(actual.param5(), expected.param5()));
    QVERIFY(QGC::fuzzyCompare(actual.param6(), expected.param6()));
    QVERIFY(QGC::fuzzyCompare(actual.param7(), expected.param7()));
}
