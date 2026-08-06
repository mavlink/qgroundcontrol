#include "APMFreshFlashParamsTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(APMFreshFlashParamsTest, TestLabel::Integration, TestLabel::Vehicle)

void APMFreshFlashParamsTest::_connectAPMMockLink(const QString &vehicleKey, MockConfiguration::Options options)
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

    if (vehicleKey == QLatin1String("Copter")) {
        _mockLink = MockLink::startAPMArduCopterMockLink(options);
    } else if (vehicleKey == QLatin1String("Plane")) {
        _mockLink = MockLink::startAPMArduPlaneMockLink(options);
    } else if (vehicleKey == QLatin1String("Rover")) {
        _mockLink = MockLink::startAPMArduRoverMockLink(options);
    } else if (vehicleKey == QLatin1String("Sub")) {
        _mockLink = MockLink::startAPMArduSubMockLink(options);
    } else {
        QFAIL(qPrintable(QStringLiteral("Unknown vehicle key: %1").arg(vehicleKey)));
    }
    QVERIFY2(_mockLink, "Failed to start MockLink");
    (void) connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });

    QVERIFY2(UnitTest::waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")),
             "Timeout waiting for vehicle connection");
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY2(_vehicle, "Vehicle should not be null after connection");

    QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
    QVERIFY2(spyConnect.isValid(), "Failed to create spy for initialConnectComplete");
    QVERIFY2(UnitTest::waitForSignal(spyConnect, TestTimeout::longMs(), QStringLiteral("initialConnectComplete")),
             "Timeout waiting for initial connect");

    QVERIFY2(waitForParametersReady(), "Timeout waiting for parameters ready");
}

void APMFreshFlashParamsTest::_testFreshFlashParams_data()
{
    QTest::addColumn<QString>("vehicleKey");
    QTest::addColumn<bool>("hasFrameClass");
    QTest::addColumn<bool>("hasRadio");

    QTest::newRow("Copter") << QStringLiteral("Copter") << true << true;
    QTest::newRow("Plane") << QStringLiteral("Plane") << false << true;
    QTest::newRow("Rover") << QStringLiteral("Rover") << true << true;
    QTest::newRow("Sub") << QStringLiteral("Sub") << false << false;
}

void APMFreshFlashParamsTest::_testFreshFlashParams()
{
    QFETCH(QString, vehicleKey);
    QFETCH(bool, hasFrameClass);
    QFETCH(bool, hasRadio);

    // Fresh-flash vehicles must show the setup-incomplete app message on connect
    expectAppMessage(QRegularExpression(QStringLiteral("Configuration tasks remain before this vehicle is ready to fly")));

    _connectAPMMockLink(vehicleKey, MockConfiguration::OptionAPMStartFreshParams);
    if (QTest::currentTestFailed()) return;
    verifyExpectedLogMessage();

    ParameterManager *mgr = _vehicle->parameterManager();

    // -------------------------------------------------------------------
    // Sensors uncalibrated: compass and accel offsets zeroed
    // -------------------------------------------------------------------
    QVERIFY2(qFuzzyIsNull(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_OFS_X"))->rawValue().toFloat()),
             "COMPASS_OFS_X not 0 on fresh-flash MockLink");
    QVERIFY2(qFuzzyIsNull(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("INS_ACCOFFS_X"))->rawValue().toFloat()),
             "INS_ACCOFFS_X not 0 on fresh-flash MockLink");

    // -------------------------------------------------------------------
    // No airframe selected — only for vehicle types that have FRAME_CLASS
    // -------------------------------------------------------------------
    QCOMPARE(mgr->parameterExists(ParameterManager::defaultComponentId, QStringLiteral("FRAME_CLASS")), hasFrameClass);
    if (hasFrameClass) {
        Fact *frameClassFact = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("FRAME_CLASS"));
        QCOMPARE(frameClassFact->rawValue().toInt(), 0);
        // FRAME_CLASS is INT8 in the param file: the fresh-flash rewrite must preserve
        // the wire type through the PARAM_VALUE round-trip (Fact type comes from it)
        QCOMPARE(frameClassFact->type(), FactMetaData::valueTypeInt8);
        QCOMPARE(_mockLink->paramValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("FRAME_CLASS")).toInt(), 0);
    }

    // -------------------------------------------------------------------
    // Radio uncalibrated: RC min/max/trim at firmware defaults for all
    // RCMAP'd attitude channels
    // -------------------------------------------------------------------
    const QStringList rcMapParams = {
        QStringLiteral("RCMAP_ROLL"),
        QStringLiteral("RCMAP_PITCH"),
        QStringLiteral("RCMAP_YAW"),
        QStringLiteral("RCMAP_THROTTLE"),
    };
    for (const QString &mapParam : rcMapParams) {
        QVERIFY2(mgr->parameterExists(ParameterManager::defaultComponentId, mapParam),
                 qPrintable(QStringLiteral("%1 missing").arg(mapParam)));
        const int channel = mgr->getParameter(ParameterManager::defaultComponentId, mapParam)->rawValue().toInt();

        Fact *minFact = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_MIN").arg(channel));
        QCOMPARE(minFact->rawValue().toInt(), 1100);
        QCOMPARE(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_MAX").arg(channel))->rawValue().toInt(), 1900);
        QCOMPARE(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_TRIM").arg(channel))->rawValue().toInt(), 1500);

        // RC params are INT16 in the param file: the wire type must be preserved
        // through the PARAM_VALUE round-trip (Fact type comes from it)
        QCOMPARE(minFact->type(), FactMetaData::valueTypeInt16);
        QCOMPARE(_mockLink->paramValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("RC%1_MIN").arg(channel)).toInt(), 1100);
    }

    // -------------------------------------------------------------------
    // Vehicle components report setup required
    // -------------------------------------------------------------------
    VehicleComponent *sensorsComp = findVehicleComponent(_vehicle, QStringLiteral("Sensors"));
    QVERIFY2(sensorsComp, "Sensors component not found");
    QVERIFY2(!sensorsComp->setupComplete(), "Sensors setupComplete despite zero cal offsets");

    VehicleComponent *radioComp = findVehicleComponent(_vehicle, QStringLiteral("Radio"));
    if (hasRadio) {
        QVERIFY2(radioComp, "Radio component not found");
        QVERIFY2(!radioComp->setupComplete(), "Radio setupComplete despite default RC min/max/trim");
    } else {
        QVERIFY2(!radioComp, "Radio component unexpectedly present");
    }

    VehicleComponent *frameComp = findVehicleComponent(_vehicle, QStringLiteral("Frame"));
    QVERIFY2(frameComp, "Frame component not found");
    QCOMPARE(frameComp->setupComplete(), !hasFrameClass);
}

void APMFreshFlashParamsTest::_testNormalConnectNoSetupRequiredMessage()
{
    // ArduPilot MockLink does not serve COMP_METADATA_TYPE_GENERAL.
    ignoreLogMessage(
        "ComponentInformation.RequestMetaDataTypeStateMachine",
        QtWarningMsg,
        QRegularExpression(QStringLiteral("\"COMP_METADATA_TYPE_GENERAL\" : failed to load metadata \\(primary and fallback\\) \"\"")));

    // A normal (calibrated) connect must NOT pop the setup-required app message.
    // Strict log capture fails this test if any unexpected app message fires.
    _connectAPMMockLink(QStringLiteral("Copter"), MockConfiguration::OptionNone);
    if (QTest::currentTestFailed()) return;

    VehicleComponent *sensorsComp = findVehicleComponent(_vehicle, QStringLiteral("Sensors"));
    QVERIFY2(sensorsComp, "Sensors component not found");
    QVERIFY2(sensorsComp->setupComplete(), "Sensors setup incomplete on normally provisioned MockLink");
    QVERIFY2(_vehicle->autopilotPlugin()->setupComplete(), "Vehicle setup incomplete on normally provisioned MockLink");
}
