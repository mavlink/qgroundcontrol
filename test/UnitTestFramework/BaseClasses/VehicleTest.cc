#include "VehicleTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtCore/QStringList>

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
    dumpFailureContextIfTestFailed(QStringLiteral("before VehicleTest teardown"));
    _disconnectMockLink();
    UnitTest::cleanup();
}

bool VehicleTest::waitForParametersReady(int timeoutMs)
{
    if (!_vehicle || !_vehicle->parameterManager()) {
        qCWarning(VehicleTestLog) << "waitForParametersReady: no vehicle or parameter manager";
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

    return UnitTest::waitForSignal(spy, timeoutMs, QStringLiteral("ParameterManager::parametersReadyChanged"));
}

bool VehicleTest::waitForInitialConnect(int timeoutMs)
{
    if (!_vehicle) {
        qCWarning(VehicleTestLog) << "waitForInitialConnect: no vehicle";
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

    return UnitTest::waitForSignal(spy, timeoutMs, QStringLiteral("Vehicle::initialConnectComplete"));
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
    QVERIFY2(!_mockLink, "MockLink already connected");

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

    switch (autopilot) {
        case MAV_AUTOPILOT_PX4:
            _mockLink = MockLink::startPX4MockLink(false /* sendStatusText */, false /* enableCamera */, false /* enableGimbal */, failureMode);
            break;
        case MAV_AUTOPILOT_ARDUPILOTMEGA:
            _mockLink = MockLink::startAPMArduCopterMockLink(false /* sendStatusText */, false /* enableCamera */, false /* enableGimbal */, failureMode);
            break;
        case MAV_AUTOPILOT_GENERIC:
            _mockLink = MockLink::startGenericMockLink(false /* sendStatusText */, false /* enableCamera */, false /* enableGimbal */, failureMode);
            break;
        case MAV_AUTOPILOT_INVALID:
            _mockLink = MockLink::startNoInitialConnectMockLink(false /* sendStatusText */, false /* enableCamera */, false /* enableGimbal */);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Unsupported autopilot type: %1").arg(autopilot)));
            return;
    }

    // Connect to destroyed signal to prevent dangling pointer
    if (_mockLink) {
        (void)connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });
    }

    QVERIFY2(UnitTest::waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")),
             "Timeout waiting for vehicle connection");
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY2(_vehicle != nullptr, "Vehicle should not be null after connection");

    if (autopilot != MAV_AUTOPILOT_INVALID) {
        QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
        QVERIFY2(spyConnect.isValid(), "Failed to create spy for initialConnectComplete");
        QVERIFY2(UnitTest::waitForSignal(spyConnect, TestTimeout::longMs(), QStringLiteral("initialConnectComplete")),
                 "Timeout waiting for initial connect");
    }
}

void VehicleTest::_disconnectMockLink()
{
    if (_mockLink) {
        QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

        _mockLink->disconnect();
        _mockLink = nullptr;

        QVERIFY2(UnitTest::waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")),
                 "Timeout waiting for vehicle disconnection");
        _vehicle = MultiVehicleManager::instance()->activeVehicle();
        QVERIFY2(!_vehicle, "Vehicle should be null after disconnection");

        // Ensure old vehicle/MockLink cleanup fully settles before subsequent tests.
        UnitTest::settleEventLoopForCleanup();
    }
}

void VehicleTest::_linkDeleted(const LinkInterface* link)
{
    if (link == _mockLink) {
        _mockLink = nullptr;
    }
}

QString VehicleTest::failureContextSummary() const
{
    QStringList lines;

    const QString baseSummary = UnitTest::failureContextSummary();
    if (!baseSummary.isEmpty()) {
        lines.append(baseSummary.split('\n', Qt::SkipEmptyParts));
    }

    lines.append(QStringLiteral("VehicleTest: vehicle=%1 mockLink=%2")
                     .arg(_vehicle ? QStringLiteral("present") : QStringLiteral("null"))
                     .arg(_mockLink ? QStringLiteral("present") : QStringLiteral("null")));

    if (_vehicle) {
        lines.append(QStringLiteral("VehicleTest: vehicle.id=%1 initialConnectComplete=%2")
                         .arg(_vehicle->id())
                         .arg(_vehicle->isInitialConnectComplete()));
        if (_vehicle->parameterManager()) {
            lines.append(QStringLiteral("VehicleTest: parametersReady=%1")
                             .arg(_vehicle->parameterManager()->parametersReady()));
        }
    }

    if (_mockLink) {
        lines.append(QStringLiteral("VehicleTest: mockLink.connected=%1").arg(_mockLink->isConnected()));
    }

    return lines.join('\n');
}
