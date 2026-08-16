#include "FlyViewROIUITest.h"

#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "FactMetaData.h"
#include "MockLink.h"
#include "Vehicle.h"

UT_REGISTER_TEST(FlyViewROIUITest, TestLabel::Integration)
UT_REGISTER_TEST(FlyViewROIAPMUITest, TestLabel::Integration)

namespace {
constexpr double kSliderValue = 15.0;  // in app display units
}  // namespace

void FlyViewROIUITest::_testROIFromMapClick()
{
    _runROIFromMapClick(false /* apmFirmware */);
}

void FlyViewROIAPMUITest::init()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }
    FlyViewROIUITestBase::init();
}

void FlyViewROIAPMUITest::_testROIFromMapClick()
{
    _runROIFromMapClick(true /* apmFirmware */);
}

void FlyViewROIUITestBase::_runROIFromMapClick(bool apmFirmware)
{
    runWithMockLink(
        [apmFirmware] { return apmFirmware ? MockLink::startAPMArduCopterMockLink() : MockLink::startPX4MockLink(); },
        [this, apmFirmware](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            // Home position arrives on MockLink's first 1Hz tick and is required for the PX4 AMSL conversion
            QVERIFY_TRUE_WAIT(vehicle->homePosition().isValid() && !qIsNaN(vehicle->homePosition().altitude()),
                              TestTimeout::shortMs());

            // Put the vehicle in the air: MockLink raises its altitude above home and reports
            // MAV_LANDED_STATE_IN_AIR, which enables the ROI guided action.
            // The flying transition creates QGCPressure, which warns on hosts without a pressure backend.
            ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                             QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
            ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                             QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));
            vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */, 0.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f /* altitude */);
            QVERIFY_TRUE_WAIT(vehicle->flying(), TestTimeout::longMs());

            // Click the map to open the click-action drop panel, then choose ROI.
            // Off-center so the click can't land on the vehicle icon (map is centered on the vehicle)
            QVERIFY(clickItemFraction(QStringLiteral("flyViewMap"), 0.35, 0.65));
            QVERIFY(clickButton(QStringLiteral("mapClickROI")));

            // The relative altitude slider must appear
            QVERIFY(verifyVisibility(QStringLiteral("guidedValueSlider"), true, QStringLiteral("after choosing ROI")));

            QQuickItem* const slider = findVisibleItem(_rootItem, QStringLiteral("guidedValueSlider"));
            QVERIFY(slider);

            // ROI defaults to a point on the ground: 0 above home
            QVariant sliderOutputValue;
            QVERIFY(QMetaObject::invokeMethod(slider, "getOutputValue", Q_RETURN_ARG(QVariant, sliderOutputValue)));
            QCOMPARE(sliderOutputValue.toDouble(), 0.0);

            QVERIFY(QMetaObject::invokeMethod(slider, "setCurrentValue", Q_ARG(QVariant, kSliderValue),
                                              Q_ARG(QVariant, false /* animate */)));

            // Read back the value the slider will actually emit (pixel positioning may round it)
            QVERIFY(QMetaObject::invokeMethod(slider, "getOutputValue", Q_RETURN_ARG(QVariant, sliderOutputValue)));
            const double relativeAltitudeMeters =
                FactMetaData::appSettingsVerticalDistanceUnitsToMeters(sliderOutputValue).toDouble();

            mockLink->clearReceivedMavCommandCounts();

            // Confirm by emitting the delay button's activated signal directly instead of
            // simulating the press-and-hold gesture
            QQuickItem* const confirmButton =
                findVisibleItem(_rootItem, QStringLiteral("guidedActionConfirmButton"), 3000);
            QVERIFY2(confirmButton, "Guided action confirm button never became visible");
            QVERIFY(QMetaObject::invokeMethod(confirmButton, "activated"));

            QVERIFY_TRUE_WAIT(mockLink->receivedMavCommandCount(MAV_CMD_DO_SET_ROI_LOCATION) == 1,
                              TestTimeout::longMs());

            mavlink_message_t message{};
            QVERIFY(mockLink->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_COMMAND_INT, message));
            mavlink_command_int_t command{};
            mavlink_msg_command_int_decode(&message, &command);
            QCOMPARE(command.command, static_cast<uint16_t>(MAV_CMD_DO_SET_ROI_LOCATION));
            // The map is centered near the vehicle, so the clicked coordinate must be close to home
            QVERIFY(qAbs(command.x / 1e7 - vehicle->homePosition().latitude()) < 0.5);
            QVERIFY(qAbs(command.y / 1e7 - vehicle->homePosition().longitude()) < 0.5);
            if (apmFirmware) {
                // ArduPilot honors the frame, so the above-home slider value is sent unconverted
                QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL_RELATIVE_ALT));
                QCOMPARE(command.z, static_cast<float>(relativeAltitudeMeters));
            } else {
                // PX4 ignores the frame and treats the altitude as AMSL; QGC sends MAV_FRAME_GLOBAL
                // to match and converts the above-home slider value to AMSL
                QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL));
                QCOMPARE(command.z, static_cast<float>(vehicle->homePosition().altitude() + relativeAltitudeMeters));
            }
        });
}
