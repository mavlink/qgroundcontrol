#include "PX4SummaryItemsTest.h"

#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "UnitsSettings.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(PX4SummaryItemsTest, TestLabel::Integration, TestLabel::Vehicle)

namespace {

QString describe(const VehicleSummaryItem& item)
{
    static const QStringList stateNames{QStringLiteral("Normal"), QStringLiteral("Attention"), QStringLiteral("Muted")};
    return QStringLiteral("%1=%2:%3").arg(item.caption, item.value, stateNames.value(item.state));
}

}  // namespace

VehicleComponent* PX4SummaryItemsTest::_component(const QString& name) const
{
    for (const QVariant& variant : vehicle()->autopilotPlugin()->vehicleComponents()) {
        auto* const component = variant.value<VehicleComponent*>();
        if (component && component->name() == name) {
            return component;
        }
    }

    return nullptr;
}

// Only the captions in expectedItems are compared, so a row does not depend on rows it is not about, such as the
// firmware version MockLink reports. The vehicle starts with the PX4MockLink parameters: calibrated sensors, no RC
// mapping and RC input optional (COM_RC_IN_MODE 1), a 4S battery, and mode selection, boot mode and collision
// prevention off.
void PX4SummaryItemsTest::_summaryItems_data()
{
    QTest::addColumn<QString>("component");
    QTest::addColumn<QVariantMap>("params");
    QTest::addColumn<QStringList>("expectedItems");

    QTest::newRow("collision/off") << "Collision" << QVariantMap{} << QStringList{"Collision Prevention=Disabled:Muted"};
    QTest::newRow("collision/on") << "Collision" << QVariantMap{{"CP_DIST", 5}}
                                  << QStringList{"Collision Prevention=5.000 m:Normal"};

    QTest::newRow("returnLand/landImmediately")
        << "Return & Land" << QVariantMap{}
        << QStringList{"RTL Climb To=30.0 m:Normal", "RTL, Then=Land immediately:Normal"};
    QTest::newRow("returnLand/loiterThenLand")
        << "Return & Land" << QVariantMap{{"RTL_LAND_DELAY", 20}}
        << QStringList{"RTL Climb To=30.0 m:Normal", "RTL, Then=Loiter and land after specified time:Normal",
                       "Loiter Alt=10.0 m:Normal", "Land Delay=20.0 s:Normal"};
    QTest::newRow("returnLand/loiterForever")
        << "Return & Land" << QVariantMap{{"RTL_LAND_DELAY", -1}}
        << QStringList{"RTL, Then=Loiter and do not land:Normal", "Loiter Alt=10.0 m:Normal"};

    QTest::newRow("sensors/calibrated")
        << "Sensors" << QVariantMap{}
        << QStringList{"Compass 0=Ready:Normal", "Compass 1=Ready:Normal", "Gyro=Ready:Normal",
                       "Accelerometer=Ready:Normal"};
    QTest::newRow("sensors/uncalibrated")
        << "Sensors" << QVariantMap{{"CAL_MAG0_ID", 0}, {"CAL_MAG1_ID", 0}, {"CAL_GYRO0_ID", 0}}
        << QStringList{"Compass 0=Setup required:Attention", "Gyro=Setup required:Attention",
                       "Accelerometer=Ready:Normal"};

    QTest::newRow("radio/rcOptional")
        << "Radio" << QVariantMap{}
        << QStringList{"Roll=Setup required:Muted", "Aux1=Disabled:Muted"};
    QTest::newRow("radio/rcRequired")
        << "Radio" << QVariantMap{{"COM_RC_IN_MODE", 0}, {"RC_MAP_PITCH", 2}}
        << QStringList{"Roll=Setup required:Attention", "Pitch=2:Normal", "Aux1=Disabled:Muted"};

    QTest::newRow("airframe/notSet")
        << "Airframe" << QVariantMap{{"SYS_AUTOSTART", 0}}
        << QStringList{"System ID=1:Normal", "Airframe type=Setup required:Attention",
                       "Vehicle=Setup required:Attention"};

    QTest::newRow("battery/estimated")
        << "Battery" << QVariantMap{}
        << QStringList{"Battery Full=4.05 V:Normal", "Battery Empty=3.60 V:Normal", "Number of Cells=4:Normal"};
    QTest::newRow("battery/noCells")
        << "Battery" << QVariantMap{{"BAT1_N_CELLS", 0}}
        << QStringList{"Number of Cells=0:Attention"};
    QTest::newRow("battery/disabledSecond")
        << "Battery" << QVariantMap{{"BAT2_SOURCE", -1}}
        << QStringList{"Battery Full=4.05 V:Normal", "Battery Empty=3.60 V:Normal", "Number of Cells=4:Normal"};
    QTest::newRow("battery/disabledSecondCanFiltered")
        << "Battery" << QVariantMap{{"BAT2_SOURCE", -1}, {"UAVCAN_ENABLE", 2}, {"UAVCAN_SUB_BAT", 2}}
        << QStringList{"Full=4.05 V:Normal", "Empty=3.60 V:Normal", "Number of Cells=4:Normal", "Full=N/A:Muted",
                       "Empty=N/A:Muted", "Number of Cells=N/A:Muted"};

    QTest::newRow("flightModes/noSelection")
        << "Flight Modes" << QVariantMap{}
        << QStringList{"Mode selection=Not assigned:Muted"};
    QTest::newRow("flightModes/buttons")
        << "Flight Modes" << QVariantMap{{"RC_MAP_FLTM_BTN", 0b110000}}
        << QStringList{"Mode selection=Buttons on Ch 5, Ch 6:Normal"};
    QTest::newRow("flightModes/externalModeNotRunning")
        << "Flight Modes" << QVariantMap{{"RC_MAP_FLTM_BTN", 1}, {"COM_FLTMODE2", 100}}
        << QStringList{"Flight Mode 2=External Mode 1 (not running):Normal"};
}

void PX4SummaryItemsTest::_summaryItems()
{
    QFETCH(QString, component);
    QFETCH(QVariantMap, params);
    QFETCH(QStringList, expectedItems);

    UnitsSettings* const unitsSettings = SettingsManager::instance()->unitsSettings();
    Fact* const horizontalUnits = unitsSettings->horizontalDistanceUnits();
    Fact* const verticalUnits = unitsSettings->verticalDistanceUnits();
    const QVariant savedHorizontalUnits = horizontalUnits->rawValue();
    const QVariant savedVerticalUnits = verticalUnits->rawValue();
    const auto restoreUnits = qScopeGuard([&]() {
        horizontalUnits->setRawValue(savedHorizontalUnits);
        verticalUnits->setRawValue(savedVerticalUnits);
    });
    horizontalUnits->setRawValue(UnitsSettings::HorizontalDistanceUnitsMeters);
    verticalUnits->setRawValue(UnitsSettings::VerticalDistanceUnitsMeters);

    ParameterManager* const parameterManager = vehicle()->parameterManager();
    for (auto it = params.cbegin(); it != params.cend(); ++it) {
        // PX4 only streams the parameters it uses, such as the second battery's, so ask for the others
        if (!parameterManager->parameterExists(ParameterManager::defaultComponentId, it.key())) {
            parameterManager->refreshParameter(ParameterManager::defaultComponentId, it.key(), false);
            QTRY_VERIFY_WITH_TIMEOUT(
                parameterManager->parameterExists(ParameterManager::defaultComponentId, it.key()),
                TestTimeout::mediumMs());
        }
        Fact* const fact = parameterManager->getParameter(ParameterManager::defaultComponentId, it.key());
        if (fact->rawValue() == it.value()) {
            continue;
        }
        // The test ends with no write in flight, which would fail once the link is gone
        QSignalSpy ackSpy(fact, &Fact::vehicleUpdated);
        QVERIFY(ackSpy.isValid());
        const bool rebootNotice = fact->vehicleRebootRequired();
        if (rebootNotice) {
            qgcApp()->resetRebootMessageDebounce();
            expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required for changes to take effect")));
        }
        fact->setRawValue(it.value());
        QVERIFY(ackSpy.wait(TestTimeout::mediumMs()));
        if (rebootNotice) {
            verifyExpectedLogMessage();
        }
    }

    VehicleComponent* const vehicleComponent = _component(component);
    QVERIFY(vehicleComponent);

    QStringList expectedCaptions;
    for (const QString& item : expectedItems) {
        expectedCaptions.append(item.section(QLatin1Char('='), 0, 0));
    }
    QStringList actualItems;
    for (const VehicleSummaryItem& item : vehicleComponent->summaryItems()) {
        if (expectedCaptions.contains(item.caption)) {
            actualItems.append(describe(item));
        }
    }

    QCOMPARE(actualItems, expectedItems);
}

// The Summary page binds to summaryItems, so a component has to announce each change to the values it shows
void PX4SummaryItemsTest::_summaryItemsChangedWithParameter()
{
    VehicleComponent* const collision = _component(QStringLiteral("Collision"));
    QVERIFY(collision);
    QSignalSpy changedSpy(collision, &VehicleComponent::summaryItemsChanged);
    QVERIFY(changedSpy.isValid());

    Fact* const distance =
        vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("CP_DIST"));
    QSignalSpy ackSpy(distance, &Fact::vehicleUpdated);
    QVERIFY(ackSpy.isValid());
    distance->setRawValue(7);

    QCOMPARE(changedSpy.count(), 1);
    QVERIFY(ackSpy.wait(TestTimeout::mediumMs()));
}

// PX4 only streams the parameters of the batteries it uses, so battery parameters can arrive after the first download
void PX4SummaryItemsTest::_summaryItemsChangedWhenBatteryFactAdded()
{
    VehicleComponent* const battery = _component(QStringLiteral("Battery"));
    QVERIFY(battery);
    QSignalSpy changedSpy(battery, &VehicleComponent::summaryItemsChanged);
    QSignalSpy setupSpy(battery, &VehicleComponent::setupCompleteChanged);
    QVERIFY(changedSpy.isValid());
    QVERIFY(setupSpy.isValid());

    ParameterManager* const parameterManager = vehicle()->parameterManager();
    Fact* const source =
        parameterManager->getParameter(ParameterManager::defaultComponentId, QStringLiteral("BAT1_SOURCE"));
    emit parameterManager->factAdded(vehicle()->defaultComponentId(), source);

    QVERIFY(changedSpy.count() >= 1);
    QVERIFY(setupSpy.count() >= 1);
}

// The Summary page lists the components that require setup, and Radio only does when RC input is not optional
void PX4SummaryItemsTest::_requiresSetupChangedWithParameter()
{
    VehicleComponent* const radio = _component(QStringLiteral("Radio"));
    QVERIFY(radio);
    QVERIFY(!radio->requiresSetup());
    QSignalSpy changedSpy(radio, &VehicleComponent::requiresSetupChanged);
    QVERIFY(changedSpy.isValid());

    ParameterManager* const parameterManager = vehicle()->parameterManager();
    Fact* const rcInputMode =
        parameterManager->getParameter(ParameterManager::defaultComponentId, QStringLiteral("COM_RC_IN_MODE"));
    QSignalSpy ackSpy(rcInputMode, &Fact::vehicleUpdated);
    QVERIFY(ackSpy.isValid());
    const bool rebootNotice = rcInputMode->vehicleRebootRequired();
    if (rebootNotice) {
        qgcApp()->resetRebootMessageDebounce();
        expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required for changes to take effect")));
    }
    rcInputMode->setRawValue(0);

    QCOMPARE(changedSpy.count(), 1);
    QVERIFY(radio->requiresSetup());
    QVERIFY(ackSpy.wait(TestTimeout::mediumMs()));
    if (rebootNotice) {
        verifyExpectedLogMessage();
    }
}
