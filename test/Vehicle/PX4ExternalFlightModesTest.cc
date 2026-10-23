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

// The Flight Modes page labels PX4's "External Mode N" choices with the name a registered mode reports in
// AVAILABLE_MODES, read from that vehicle's own mode list rather than the firmware plugin shared by all vehicles
void PX4ExternalFlightModesTest::_externalModeNameFromAvailableModes()
{
    px4_custom_mode ext2{};
    ext2.main_mode = PX4_CUSTOM_MAIN_MODE_AUTO;
    ext2.sub_mode = PX4_CUSTOM_SUB_MODE_EXTERNAL2;
    px4_custom_mode ext3 = ext2;
    ext3.sub_mode = PX4_CUSTOM_SUB_MODE_EXTERNAL3;
    px4_custom_mode ext4 = ext2;
    ext4.sub_mode = PX4_CUSTOM_SUB_MODE_EXTERNAL4;

    // PX4 names each external slot with no registered mode "(Mode not available)", which QGC then numbers to keep the
    // names unique
    FlightModeList modeList = {
        FirmwareFlightMode{QStringLiteral("Position"), 1, PX4CustomMode::POSCTL_POSCTL, true, false, true, true},
        FirmwareFlightMode{QStringLiteral("Survey Assist"), 0, ext2.data, true, false, true, true},
        FirmwareFlightMode{QStringLiteral("(Mode not available)"), 0, ext3.data, true, false, true, true},
        FirmwareFlightMode{QStringLiteral("(Mode not available) (1)"), 0, ext4.data, true, false, true, true},
    };
    const FlightModeList otherVehicleModeList = {
        FirmwareFlightMode{QStringLiteral("Inspection"), 0, ext2.data, true, false, true, true},
    };

    QCOMPARE(PX4FirmwarePlugin::externalFlightModeName(modeList, 2), QStringLiteral("Survey Assist"));
    QCOMPARE(PX4FirmwarePlugin::externalFlightModeName(otherVehicleModeList, 2), QStringLiteral("Inspection"));
    QVERIFY(PX4FirmwarePlugin::externalFlightModeName(modeList, 1).isEmpty());
    QVERIFY(PX4FirmwarePlugin::externalFlightModeName(modeList, 3).isEmpty());
    QVERIFY(PX4FirmwarePlugin::externalFlightModeName(modeList, 4).isEmpty());
    QVERIFY(PX4FirmwarePlugin::externalFlightModeName(modeList, 0).isEmpty());
    QVERIFY(PX4FirmwarePlugin::externalFlightModeName(modeList, 9).isEmpty());
}

UT_REGISTER_TEST(PX4ExternalFlightModesTest, TestLabel::Unit, TestLabel::Vehicle)
