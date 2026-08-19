#include "PX4ExternalFlightModesTest.h"

#include "PX4FirmwarePlugin.h"
#include "Vehicle.h"
#include "px4_custom_mode.h"

void PX4ExternalFlightModesTest::_externalModeSelectableOnMultiRotor()
{
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);

    // Local plugin instance so the shared firmware plugin singleton is not mutated
    PX4FirmwarePlugin plugin;

    // External mode 1 (PX4 AUTO sub_mode 11) as reported via AVAILABLE_MODES.
    // Must not collide with any built-in PX4CustomMode value (see issue #14886).
    px4_custom_mode ext1{};
    ext1.main_mode = PX4_CUSTOM_MAIN_MODE_AUTO;
    ext1.sub_mode = PX4_CUSTOM_SUB_MODE_EXTERNAL1;

    FlightModeList modeList = {
        FirmwareFlightMode{QStringLiteral("Position"), 1, PX4CustomMode::POSCTL_POSCTL, true, false, true, true},
        FirmwareFlightMode{QStringLiteral("Cage (Autonomous)"), 0, ext1.data, true, false, true, true},
    };
    plugin.updateAvailableFlightModes(modeList);

    QVERIFY(plugin.flightModes(&vehicle).contains(QStringLiteral("Cage (Autonomous)")));
}

UT_REGISTER_TEST(PX4ExternalFlightModesTest, TestLabel::Unit, TestLabel::Vehicle)
