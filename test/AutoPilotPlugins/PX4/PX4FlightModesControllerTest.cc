#include "PX4FlightModesControllerTest.h"

#include <QtTest/QSignalSpy>

#include "MockLink.h"
#include "PX4FlightModesController.h"
#include "Vehicle.h"

UT_REGISTER_TEST(PX4FlightModesControllerTest, TestLabel::Integration, TestLabel::Vehicle)

// PX4 scales the raw PWM against each channel's RC#_MIN/RC#_MAX, which can lie outside 1000-2000 us, so the page's
// live mode slot and switch positions need the unclamped values. Users that only need the mode names, such as the
// summary card, leave monitoring off and get no RC updates.
void PX4FlightModesControllerTest::_rcValuesFollowMonitoring_data()
{
    QTest::addColumn<bool>("monitorRc");
    QTest::addColumn<QVariantList>("expectedValues");

    QTest::newRow("monitoring") << true << QVariantList{900, 1500, 2100, 1000};
    QTest::newRow("notMonitoring") << false << QVariantList{};
}

void PX4FlightModesControllerTest::_rcValuesFollowMonitoring()
{
    QFETCH(bool, monitorRc);
    QFETCH(QVariantList, expectedValues);

    PX4FlightModesController controller;
    controller.setMonitorRc(monitorRc);
    QSignalSpy valuesSpy(&controller, &PX4FlightModesController::rcChannelValuesChanged);
    QVERIFY(valuesSpy.isValid());
    QSignalSpy vehicleRcSpy(vehicle(), &Vehicle::rcChannelsRawChanged);
    QVERIFY(vehicleRcSpy.isValid());

    mavlink_message_t message{};
    (void) mavlink_msg_rc_channels_pack(static_cast<uint8_t>(mockLink()->vehicleId()), MAV_COMP_ID_AUTOPILOT1, &message,
                                        0 /* time_boot_ms */, 4 /* chancount */, 900, 1500, 2100, 1000, UINT16_MAX,
                                        UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
                                        UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
                                        UINT16_MAX, 255 /* rssi */);
    mockLink()->respondWithMavlinkMessage(message);

    // The vehicle has handled the message, so the controller has seen it if it ever will
    QTRY_VERIFY_WITH_TIMEOUT(vehicleRcSpy.count() > 0, TestTimeout::mediumMs());
    QCOMPARE(valuesSpy.isEmpty(), !monitorRc);
    QCOMPARE(controller.rcChannelValues(), expectedValues);
}

// RC_CHANNELS only arrives while the vehicle has RC input, so values that stop updating are dropped rather than shown
// as live
void PX4FlightModesControllerTest::_rcValuesExpireWithoutInput()
{
    PX4FlightModesController controller;
    controller.setRcTimeoutMs(100);
    controller.setMonitorRc(true);
    QSignalSpy vehicleRcSpy(vehicle(), &Vehicle::rcChannelsRawChanged);
    QVERIFY(vehicleRcSpy.isValid());

    mavlink_message_t message{};
    (void) mavlink_msg_rc_channels_pack(static_cast<uint8_t>(mockLink()->vehicleId()), MAV_COMP_ID_AUTOPILOT1, &message,
                                        0 /* time_boot_ms */, 1 /* chancount */, 1500, UINT16_MAX, UINT16_MAX,
                                        UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
                                        UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
                                        UINT16_MAX, UINT16_MAX, UINT16_MAX, 255 /* rssi */);
    mockLink()->respondWithMavlinkMessage(message);

    QTRY_VERIFY_WITH_TIMEOUT(vehicleRcSpy.count() > 0, TestTimeout::mediumMs());
    QCOMPARE(controller.rcChannelValues(), QVariantList{1500});
    QTRY_VERIFY_WITH_TIMEOUT(controller.rcChannelValues().isEmpty(), TestTimeout::mediumMs());
}
