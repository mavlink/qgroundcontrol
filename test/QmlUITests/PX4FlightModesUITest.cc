#include "PX4FlightModesUITest.h"

#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fact.h"
#include "MockLink.h"
#include "ParameterManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(PX4FlightModesUITest, TestLabel::Integration)

void PX4FlightModesUITest::_navigateToFlightModes()
{
    navigateToConfigureView();
    if (QTest::currentTestFailed()) {
        return;
    }
    (void) clickSidebarButton(QStringLiteral("vehicleConfig_comp_FlightModes"));
}

void PX4FlightModesUITest::_setFact(Fact* fact, const QVariant& rawValue)
{
    if (fact->rawValue() == rawValue) {
        return;
    }
    QSignalSpy ackSpy(fact, &Fact::vehicleUpdated);
    QVERIFY(ackSpy.isValid());
    fact->setRawValue(rawValue);
    QVERIFY(ackSpy.wait(5000));
}

// Two switches on one channel that are on at the same position would fire together, so the page warns. Thresholds
// that split the channel, one on when low and the other when high, are a deliberate setup and are not warned about.
void PX4FlightModesUITest::_overlappingSwitchesWarned()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
                        ParameterManager* const parameterManager = vehicle->parameterManager();
                        Fact* const returnSwitch = parameterManager->getParameter(ParameterManager::defaultComponentId,
                                                                                  QStringLiteral("RC_MAP_RETURN_SW"));
                        Fact* const killSwitch = parameterManager->getParameter(ParameterManager::defaultComponentId,
                                                                                QStringLiteral("RC_MAP_KILL_SW"));
                        Fact* const killThreshold = parameterManager->getParameter(ParameterManager::defaultComponentId,
                                                                                   QStringLiteral("RC_KILLSWITCH_TH"));
                        QVERIFY(returnSwitch);
                        QVERIFY(killSwitch);
                        QVERIFY(killThreshold);
                        _setFact(returnSwitch, 15);
                        _setFact(killSwitch, 15);
                        _setFact(killThreshold, 0.75);
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        _navigateToFlightModes();
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightModes_conflict_15"), 3000));

                        // Return stays on above 0.75, Kill is now on below 0.25
                        _setFact(killThreshold, -0.25);
                        if (QTest::currentTestFailed()) {
                            return;
                        }
                        QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("flightModes_conflict_15"), 0));
                    });
}

// PX4 ignores mode buttons while a mode channel is set, so choosing buttons clears the channel and shows the buttons
void PX4FlightModesUITest::_selectingButtonsClearsModeChannel()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            ParameterManager* const parameterManager = vehicle->parameterManager();
            Fact* const modeChannel =
                parameterManager->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC_MAP_FLTMODE"));
            Fact* const buttonMask =
                parameterManager->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC_MAP_FLTM_BTN"));
            QVERIFY(modeChannel);
            QVERIFY(buttonMask);
            _setFact(modeChannel, 5);
            _setFact(buttonMask, 0);
            if (QTest::currentTestFailed()) {
                return;
            }

            _navigateToFlightModes();
            if (QTest::currentTestFailed()) {
                return;
            }

            QQuickItem* const methodBar = findVisibleItem(_rootItem, QStringLiteral("flightModes_method"), 3000);
            QVERIFY(methodBar);
            QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightModes_modeChannel"), 0));

            // Buttons is the second choice
            QVERIFY(QMetaObject::invokeMethod(methodBar, "activated", Q_ARG(int, 1)));
            QTRY_COMPARE(modeChannel->rawValue().toInt(), 0);
            QVERIFY_TRUE_WAIT(!parameterManager->pendingWrites(), TestTimeout::mediumMs());

            const QString buttonName = QStringLiteral("flightModes_button_7");
            QVERIFY(findVisibleItemScrolled(buttonName, QStringLiteral("setupPage_flickable")));
            QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("flightModes_modeChannel"), 0));

            QVERIFY(clickButton(buttonName));
            QTRY_COMPARE(buttonMask->rawValue().toInt(), 1 << 6);
            QVERIFY_TRUE_WAIT(!parameterManager->pendingWrites(), TestTimeout::mediumMs());
        });
}

void PX4FlightModesUITest::_assignedFilterHidesUnassignedSwitches()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
                        ParameterManager* const parameterManager = vehicle->parameterManager();
                        Fact* const returnSwitch = parameterManager->getParameter(ParameterManager::defaultComponentId,
                                                                                  QStringLiteral("RC_MAP_RETURN_SW"));
                        Fact* const holdSwitch = parameterManager->getParameter(ParameterManager::defaultComponentId,
                                                                                QStringLiteral("RC_MAP_LOITER_SW"));
                        QVERIFY(returnSwitch);
                        QVERIFY(holdSwitch);
                        _setFact(returnSwitch, 15);
                        _setFact(holdSwitch, 0);
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        _navigateToFlightModes();
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        QQuickItem* const filterBar =
                            findVisibleItem(_rootItem, QStringLiteral("flightModes_filter"), 3000);
                        QVERIFY(filterBar);
                        QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightModes_row_loiter"), 0));

                        QVERIFY(QMetaObject::invokeMethod(filterBar, "filterActivated", Q_ARG(int, 1)));
                        QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("flightModes_row_loiter"), 0));
                        QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightModes_row_return"), 0));
                        // Rows that are always in use stay shown
                        QVERIFY(findVisibleItem(_rootItem, QStringLiteral("flightModes_row_slots"), 0));

                        // The summary counts the rows actually shown, out of every row on the page
                        const auto [visibleRows, totalRows] = countRows(QStringLiteral("flightModes_row_"));
                        QVERIFY(visibleRows < totalRows);
                        QCOMPARE(filterBar->property("summary").toString(),
                                 QStringLiteral("Showing %1 of %2 settings.").arg(visibleRows).arg(totalRows));
                    });
}

// Releasing a momentary mode button leaves its mode selected, so the page keeps marking the slot after release
void PX4FlightModesUITest::_releasedModeButtonKeepsSlot()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
                        ParameterManager* const parameterManager = vehicle->parameterManager();
                        Fact* const modeChannel = parameterManager->getParameter(ParameterManager::defaultComponentId,
                                                                                 QStringLiteral("RC_MAP_FLTMODE"));
                        Fact* const buttonMask = parameterManager->getParameter(ParameterManager::defaultComponentId,
                                                                                QStringLiteral("RC_MAP_FLTM_BTN"));
                        QVERIFY(modeChannel);
                        QVERIFY(buttonMask);
                        _setFact(modeChannel, 0);
                        _setFact(buttonMask, 1 << 6);
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        _navigateToFlightModes();
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        QQuickItem* const slotChip = findItem(_rootItem, QStringLiteral("flightModes_slotActive_1"));
                        QVERIFY(slotChip);
                        QCOMPARE(slotChip->opacity(), 0.0);

                        _sendChannel7(mockLink, 2000);
                        QTRY_COMPARE(slotChip->opacity(), 1.0);
                        QCOMPARE(slotChip->property("text").toString(), QStringLiteral("Last pressed"));

                        QSignalSpy rcSpy(vehicle, &Vehicle::rcChannelsRawChanged);
                        QVERIFY(rcSpy.isValid());
                        _sendChannel7(mockLink, 1000);
                        QTRY_VERIFY_WITH_TIMEOUT(rcSpy.count() > 0, TestTimeout::mediumMs());
                        QCOMPARE(slotChip->opacity(), 1.0);

                        // Without RC input the page stops showing a pressed slot
                        QTRY_COMPARE_WITH_TIMEOUT(slotChip->opacity(), 0.0, TestTimeout::mediumMs());
                    });
}

void PX4FlightModesUITest::_sendChannel7(MockLink* mockLink, uint16_t pwm)
{
    mavlink_message_t message{};
    (void) mavlink_msg_rc_channels_pack(static_cast<uint8_t>(mockLink->vehicleId()), MAV_COMP_ID_AUTOPILOT1, &message,
                                        0 /* time_boot_ms */, 8 /* chancount */, 1500, 1500, 1000, 1500, 1500, 1500,
                                        pwm, 1500, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
                                        UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, 255 /* rssi */);
    mockLink->respondWithMavlinkMessage(message);
}
