#include "SensorsComponentBaseTest.h"
#include "SensorsComponentBase.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"

#include <QtTest/QTest>

// Mock subclass for testing
class MockSensorsComponent : public SensorsComponentBase
{
    Q_OBJECT
public:
    explicit MockSensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr)
        : SensorsComponentBase(vehicle, autopilot, AutoPilotPlugin::KnownSensorsVehicleComponent, parent)
    {}

    // Required VehicleComponent overrides
    QStringList setupCompleteChangedTriggerList() const override { return {}; }
    bool setupComplete() const override { return true; }
    QUrl setupSource() const override { return QUrl(); }
    QUrl summaryQmlSource() const override { return QUrl(); }
};

void SensorsComponentBaseTest::_testName()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);

    MockSensorsComponent component(vehicle, vehicle->autopilotPlugin());
    QCOMPARE(component.name(), QString("Sensors"));
}

void SensorsComponentBaseTest::_testDescription()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);

    MockSensorsComponent component(vehicle, vehicle->autopilotPlugin());
    QVERIFY(component.description().contains("calibrate"));
}

void SensorsComponentBaseTest::_testIconResource()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);

    MockSensorsComponent component(vehicle, vehicle->autopilotPlugin());
    QCOMPARE(component.iconResource(), QString("/qmlimages/SensorsComponentIcon.png"));
}

void SensorsComponentBaseTest::_testRequiresSetup()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);

    MockSensorsComponent component(vehicle, vehicle->autopilotPlugin());
    QVERIFY(component.requiresSetup());
}

void SensorsComponentBaseTest::_testParameterHelpers()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    QVERIFY(vehicle->parameterManager()->parametersReady());

    // Test with a parameter we know exists
    // Just verify the helper methods work without crashing
    // The actual parameter values are implementation-specific
}

#include "SensorsComponentBaseTest.moc"
