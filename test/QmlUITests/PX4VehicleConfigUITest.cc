#include "PX4VehicleConfigUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MockLink.h"

#include <QtCore/QPointer>
#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(PX4VehicleConfigUITest, TestLabel::Integration)

void PX4VehicleConfigUITest::_testNavigateVehicleConfig()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Click Overview
    // -------------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("vehicleConfig_summary")), "Failed to click Overview button");
    QTest::qWait(_viewDelay);

    // The Overview page lists what the vehicle still needs and shows the values of the components that have values to show
    for (const QString& itemName :
         {QStringLiteral("vehicleSummary_tile_Airframe"), QStringLiteral("vehicleSummary_tile_Sensors"),
          QStringLiteral("vehicleSummary_card_Airframe"), QStringLiteral("vehicleSummary_card_Flight Modes")}) {
        QQuickItem* const item = findItem(_rootItem, itemName);
        QVERIFY2(item, qPrintable(itemName));
        QVERIFY2(item->isVisible(), qPrintable(itemName));
    }

    // Each control on the Overview page opens its component's setup page
    const auto openedComponentName = [this]() {
        QQuickItem* const panelLoader = findItem(_rootItem, QStringLiteral("vehicleConfig_panelLoader"));
        const auto* const component =
            panelLoader ? panelLoader->property("vehicleComponent").value<VehicleComponent*>() : nullptr;
        return component ? component->name() : QString();
    };
    const auto clickFromOverview = [&](const QString& controlName, const QString& expectedComponent) {
        QVERIFY2(clickButton(QStringLiteral("vehicleConfig_summary")), "Failed to click Overview button");
        QVERIFY2(clickButton(controlName), qPrintable(controlName));
        QTRY_COMPARE_WITH_TIMEOUT(openedComponentName(), expectedComponent, TestTimeout::shortMs());
    };
    clickFromOverview(QStringLiteral("vehicleSummary_tile_Sensors"), QStringLiteral("Sensors"));
    if (QTest::currentTestFailed()) return;
    clickFromOverview(QStringLiteral("vehicleSummary_card_Airframe"), QStringLiteral("Airframe"));
    if (QTest::currentTestFailed()) return;

    // An unset battery makes Battery the next component to set up
    Fact* const cellCount = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId,
                                                                      QStringLiteral("BAT1_N_CELLS"));
    QSignalSpy ackSpy(cellCount, &Fact::vehicleUpdated);
    QVERIFY(ackSpy.isValid());
    cellCount->setRawValue(0);
    QVERIFY(ackSpy.wait(TestTimeout::mediumMs()));
    clickFromOverview(QStringLiteral("vehicleSummary_continueSetup"), QStringLiteral("Battery"));
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Click through each vehicle component, in English and Chinese
    // -------------------------------------------------------------------------
    clickThroughAllComponentsAllLocales(vehicle);

    });
}

// With a full-parameter vehicle both dividers show; searching flattens the sidebar and hides them.
void PX4VehicleConfigUITest::_testSidebarDividers()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle * /*vehicle*/) {
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    QQuickItem *const summaryDivider = findItem(_rootItem, QStringLiteral("vehicleConfig_summaryDivider"));
    QQuickItem *const componentsDivider = findItem(_rootItem, QStringLiteral("vehicleConfig_componentsDivider"));
    QVERIFY(summaryDivider);
    QVERIFY(componentsDivider);
    QTRY_VERIFY(summaryDivider->isVisible());
    QTRY_VERIFY(componentsDivider->isVisible());

    QQuickItem *const searchField = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_searchField"), 3000);
    QVERIFY(searchField);
    searchField->setProperty("text", QStringLiteral("sensors"));
    QTRY_VERIFY(!summaryDivider->isVisible());
    QTRY_VERIFY(!componentsDivider->isVisible());

    searchField->setProperty("text", QString());
    QTRY_VERIFY(summaryDivider->isVisible());
    QTRY_VERIFY(componentsDivider->isVisible());

    });
}

// Row-based pages have no vehicleConfigJson() sections, so search relies on their component-level keywords.
void PX4VehicleConfigUITest::_testSearchMatchesComponentKeywords()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* /*vehicle*/) {
                        navigateToConfigureView();
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        QQuickItem* const searchField =
                            findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_searchField"), 3000);
                        QVERIFY(searchField);

                        const QList<QPair<QString, QString>> queryToComponent = {
                            {QStringLiteral("loiter"), QStringLiteral("vehicleConfig_comp_Return&Land")},
                            {QStringLiteral("obstacle"), QStringLiteral("vehicleConfig_comp_Collision")},
                            {QStringLiteral("dshot"), QStringLiteral("vehicleConfig_comp_ESC")},
                            {QStringLiteral("voltage"), QStringLiteral("vehicleConfig_comp_Battery")},
                            {QStringLiteral("geofence"), QStringLiteral("vehicleConfig_comp_Failsafes")},
                        };
                        for (const auto& [query, componentName] : queryToComponent) {
                            searchField->setProperty("text", query);
                            QQuickItem* const componentButton = findItem(_rootItem, componentName);
                            QVERIFY2(componentButton, qPrintable(componentName));
                            QTRY_VERIFY2(componentButton->isVisible(), qPrintable(query));
                            QQuickItem* const tuningButton =
                                findItem(_rootItem, QStringLiteral("vehicleConfig_comp_PIDTuning"));
                            QVERIFY(tuningButton);
                            QTRY_VERIFY2(!tuningButton->isVisible(), qPrintable(query));
                        }

                        searchField->setProperty("text", QString());
                    });
}

// Cycle through the axis buttons on a PID tuning tab. Each click exercises
// the LineSeries remove/re-add path on GraphsView; qWait lets the polish pass run.
void PX4VehicleConfigUITest::_cycleAxisButtons(const QStringList &axisNames)
{
    for (const QString &name : axisNames) {
        const QString objectName = QStringLiteral("pidTuning_axisButton_") + name;
        QVERIFY2(findVisibleItem(_rootItem, objectName, 2000),
                 qPrintable(QStringLiteral("Axis button not found: %1").arg(objectName)));
        QVERIFY2(clickButton(objectName),
                 qPrintable(QStringLiteral("Failed to click axis button: %1").arg(objectName)));
        QTest::qWait(_pageDelay > 0 ? _pageDelay : 200); // let the polish pass run
    }
}

void PX4VehicleConfigUITest::_testDisconnectWithPIDTuningOpen()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle * /*vehicle*/) {
    // -------------------------------------------------------------------------
    // Navigate to Configure view and open the PID Tuning sidebar page
    // -------------------------------------------------------------------------
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    // PX4TuningComponent::name() returns "PID Tuning" -> objectName "vehicleConfig_comp_PIDTuning"
    clickSidebarButton(QStringLiteral("vehicleConfig_comp_PIDTuning"));
    if (QTest::currentTestFailed()) return;

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_panelLoader"), 2000),
             "vehicleConfig_panelLoader not found after opening PID Tuning");

    // -------------------------------------------------------------------------
    // Rate Controller: Roll -> Pitch -> Yaw -> Roll
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("pidTuning_tab_RateController"), 2000),
             "Rate Controller tab not found");
    QVERIFY2(clickButton(QStringLiteral("pidTuning_tab_RateController")), "Failed to click Rate Controller tab");
    QTest::qWait(_pageDelay);
    _cycleAxisButtons({QStringLiteral("Roll"), QStringLiteral("Pitch"), QStringLiteral("Yaw"), QStringLiteral("Roll")});
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Attitude Controller: Roll -> Pitch -> Yaw -> Roll
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("pidTuning_tab_AttitudeController"), 2000),
             "Attitude Controller tab not found");
    QVERIFY2(clickButton(QStringLiteral("pidTuning_tab_AttitudeController")), "Failed to click Attitude Controller tab");
    QTest::qWait(_pageDelay);
    _cycleAxisButtons({QStringLiteral("Roll"), QStringLiteral("Pitch"), QStringLiteral("Yaw"), QStringLiteral("Roll")});
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Velocity Controller: Horizontal -> Vertical -> Horizontal
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("pidTuning_tab_VelocityController"), 2000),
             "Velocity Controller tab not found");
    QVERIFY2(clickButton(QStringLiteral("pidTuning_tab_VelocityController")), "Failed to click Velocity Controller tab");
    QTest::qWait(_pageDelay);
    _cycleAxisButtons({QStringLiteral("Horizontal"), QStringLiteral("Vertical"), QStringLiteral("Horizontal")});
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Position Controller: Horizontal -> Vertical -> Horizontal
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("pidTuning_tab_PositionController"), 2000),
             "Position Controller tab not found");
    QVERIFY2(clickButton(QStringLiteral("pidTuning_tab_PositionController")), "Failed to click Position Controller tab");
    QTest::qWait(_pageDelay);
    _cycleAxisButtons({QStringLiteral("Horizontal"), QStringLiteral("Vertical"), QStringLiteral("Horizontal")});
    if (QTest::currentTestFailed()) return;

    });
}
