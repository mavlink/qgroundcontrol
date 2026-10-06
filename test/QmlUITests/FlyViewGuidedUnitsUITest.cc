#include "FlyViewGuidedUnitsUITest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "Fact.h"
#include "FlyViewSettings.h"
#include "MockLink.h"
#include "SettingsManager.h"
#include "UnitsSettings.h"
#include "Vehicle.h"

UT_REGISTER_TEST(FlyViewGuidedUnitsUITest, TestLabel::Integration)

namespace {
constexpr double kFeetToMeters = 0.3048;
}  // namespace

void FlyViewGuidedUnitsUITest::_testSliderFollowsUnitsChange()
{
    Fact* const verticalUnitsFact = SettingsManager::instance()->unitsSettings()->verticalDistanceUnits();
    const QVariant savedUnits = verticalUnitsFact->rawValue();
    const auto restoreUnits =
        qScopeGuard([verticalUnitsFact, savedUnits] { verticalUnitsFact->setRawValue(savedUnits); });
    verticalUnitsFact->setRawValue(UnitsSettings::VerticalDistanceUnitsMeters);

    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this, verticalUnitsFact](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            // Home position arrives on MockLink's first 1Hz tick and is required for the PX4 AMSL conversion
            QVERIFY_TRUE_WAIT(vehicle->homePosition().isValid() && !qIsNaN(vehicle->homePosition().altitude()),
                              TestTimeout::shortMs());

            // The airborne transition creates QGCPressure, which warns on hosts without a pressure backend
            ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                             QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
            ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                             QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));
            vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */, 0.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f /* altitude */);
            QVERIFY_TRUE_WAIT(vehicle->airborne(), TestTimeout::longMs());

            // Off-center so the click can't land on the vehicle icon (map is centered on the vehicle)
            QVERIFY(clickItemFraction(QStringLiteral("flyViewMap"), 0.35, 0.65));
            QVERIFY(clickButton(QStringLiteral("mapClickROI")));
            QVERIFY(verifyVisibility(QStringLiteral("guidedValueSlider"), true, QStringLiteral("after choosing ROI")));
            QQuickItem* const slider = findVisibleItem(_rootItem, QStringLiteral("guidedValueSlider"));
            QVERIFY(slider);

            Fact* const maxAltitudeFact = SettingsManager::instance()->flyViewSettings()->guidedMaximumAltitude();
            QCOMPARE_FUZZY(slider->property("_sliderMaxVal").toDouble(), maxAltitudeFact->cookedValue().toDouble(),
                           1e-6);
            QVERIFY(QMetaObject::invokeMethod(slider, "setCurrentValue", Q_ARG(QVariant, 15.0),
                                              Q_ARG(QVariant, false /* animate */)));

            verticalUnitsFact->setRawValue(UnitsSettings::VerticalDistanceUnitsFeet);

            // Range is re-captured in feet and the value resets to the ROI default of 0 above home
            QCOMPARE_FUZZY(slider->property("_sliderMaxVal").toDouble(), maxAltitudeFact->cookedValue().toDouble(),
                           1e-6);
            QVariant sliderOutputValue;
            QVERIFY(QMetaObject::invokeMethod(slider, "getOutputValue", Q_RETURN_ARG(QVariant, sliderOutputValue)));
            QCOMPARE(sliderOutputValue.toDouble(), 0.0);

            // From the second change on, unitsChanged arrives before the Facts' own conversions update
            for (const auto units :
                 {UnitsSettings::VerticalDistanceUnitsMeters, UnitsSettings::VerticalDistanceUnitsFeet}) {
                verticalUnitsFact->setRawValue(units);
                QCOMPARE_FUZZY(slider->property("_sliderMaxVal").toDouble(), maxAltitudeFact->cookedValue().toDouble(),
                               1e-6);
            }

            QVERIFY(QMetaObject::invokeMethod(slider, "setCurrentValue", Q_ARG(QVariant, 50.0),
                                              Q_ARG(QVariant, false /* animate */)));
            QVERIFY(QMetaObject::invokeMethod(slider, "getOutputValue", Q_RETURN_ARG(QVariant, sliderOutputValue)));
            QVERIFY(sliderOutputValue.toDouble() > 0.0);
            const double relativeAltitudeMeters = sliderOutputValue.toDouble() * kFeetToMeters;

            mockLink->clearReceivedMavCommandCounts();

            // Confirm by emitting the delay button's activated signal instead of simulating press-and-hold
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
            // PX4 treats the altitude as AMSL, so QGC sends home + the above-home slider value converted from feet
            QCOMPARE_FUZZY(command.z, vehicle->homePosition().altitude() + relativeAltitudeMeters, 1e-3);
        });
}
