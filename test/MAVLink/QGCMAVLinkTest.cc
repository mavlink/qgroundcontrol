#include "QGCMAVLinkTest.h"

#include "QGCMAVLink.h"
#include "MAVLinkMessageType.h"

// ---------------------------------------------------------------------------
// Firmware class
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testFirmwareClassClassification()
{
    QVERIFY(QGCMAVLink::isPX4FirmwareClass(MAV_AUTOPILOT_PX4));
    QVERIFY(!QGCMAVLink::isArduPilotFirmwareClass(MAV_AUTOPILOT_PX4));
    QVERIFY(!QGCMAVLink::isGenericFirmwareClass(MAV_AUTOPILOT_PX4));

    QVERIFY(QGCMAVLink::isArduPilotFirmwareClass(MAV_AUTOPILOT_ARDUPILOTMEGA));
    QVERIFY(!QGCMAVLink::isPX4FirmwareClass(MAV_AUTOPILOT_ARDUPILOTMEGA));

    QVERIFY(QGCMAVLink::isGenericFirmwareClass(MAV_AUTOPILOT_GENERIC));
    QVERIFY(QGCMAVLink::isGenericFirmwareClass(MAV_AUTOPILOT_INVALID));
}

void QGCMAVLinkTest::_testFirmwareClassRoundTrip()
{
    QCOMPARE(QGCMAVLink::firmwareClass(MAV_AUTOPILOT_PX4), QGCMAVLink::FirmwareClassPX4);
    QCOMPARE(QGCMAVLink::firmwareClass(MAV_AUTOPILOT_ARDUPILOTMEGA), QGCMAVLink::FirmwareClassArduPilot);
    QCOMPARE(QGCMAVLink::firmwareClass(MAV_AUTOPILOT_GENERIC), QGCMAVLink::FirmwareClassGeneric);

    // Round trip through string form
    const QString px4Str = QGCMAVLink::firmwareClassToString(QGCMAVLink::FirmwareClassPX4);
    const QString apmStr = QGCMAVLink::firmwareClassToString(QGCMAVLink::FirmwareClassArduPilot);
    QCOMPARE(QGCMAVLink::firmwareTypeFromString(px4Str), MAV_AUTOPILOT_PX4);
    QCOMPARE(QGCMAVLink::firmwareTypeFromString(apmStr), MAV_AUTOPILOT_ARDUPILOTMEGA);
    QCOMPARE(QGCMAVLink::firmwareTypeFromString(QStringLiteral("not-a-firmware")), MAV_AUTOPILOT_GENERIC);
}

void QGCMAVLinkTest::_testFirmwareClassToStringUnknown()
{
    // Cast an out-of-enum value to exercise the default arm.
    const auto bogus = static_cast<QGCMAVLink::FirmwareClass_t>(9999);
    QCOMPARE(QGCMAVLink::firmwareClassToString(bogus), QStringLiteral("Unknown"));
}

void QGCMAVLinkTest::_testFirmwareTypeFromStringWhitespace()
{
    // Whitespace around the name must be tolerated.
    QCOMPARE(QGCMAVLink::firmwareTypeFromString(QStringLiteral("  ArduPilot  ")), MAV_AUTOPILOT_ARDUPILOTMEGA);
    QCOMPARE(QGCMAVLink::firmwareTypeFromString(QStringLiteral("\tPX4 Pro\n")), MAV_AUTOPILOT_PX4);
}

// ---------------------------------------------------------------------------
// Vehicle class
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testVehicleClassClassification()
{
    QVERIFY(QGCMAVLink::isFixedWing(MAV_TYPE_FIXED_WING));
    QVERIFY(!QGCMAVLink::isFixedWing(MAV_TYPE_QUADROTOR));

    QVERIFY(QGCMAVLink::isMultiRotor(MAV_TYPE_QUADROTOR));
    QVERIFY(QGCMAVLink::isMultiRotor(MAV_TYPE_HEXAROTOR));
    QVERIFY(QGCMAVLink::isMultiRotor(MAV_TYPE_OCTOROTOR));
    QVERIFY(QGCMAVLink::isMultiRotor(MAV_TYPE_TRICOPTER));
    QVERIFY(QGCMAVLink::isMultiRotor(MAV_TYPE_HELICOPTER));
    QVERIFY(QGCMAVLink::isMultiRotor(MAV_TYPE_COAXIAL));

    QVERIFY(QGCMAVLink::isSub(MAV_TYPE_SUBMARINE));
    QVERIFY(QGCMAVLink::isAirship(MAV_TYPE_AIRSHIP));
    QVERIFY(QGCMAVLink::isSpacecraft(MAV_TYPE_SPACECRAFT_ORBITER));
}

void QGCMAVLinkTest::_testVehicleClassVTOL()
{
    // Every VTOL MAV_TYPE variant should classify as VTOL.
    const MAV_TYPE vtolTypes[] = {
        MAV_TYPE_VTOL_TAILSITTER_DUOROTOR,
        MAV_TYPE_VTOL_TAILSITTER_QUADROTOR,
        MAV_TYPE_VTOL_TILTROTOR,
        MAV_TYPE_VTOL_FIXEDROTOR,
        MAV_TYPE_VTOL_TAILSITTER,
        MAV_TYPE_VTOL_TILTWING,
        MAV_TYPE_VTOL_RESERVED5,
    };
    for (MAV_TYPE t : vtolTypes) {
        QVERIFY2(QGCMAVLink::isVTOL(t), qPrintable(QStringLiteral("type %1").arg(t)));
    }
}

void QGCMAVLinkTest::_testVehicleClassRoverBoatCovers()
{
    // Both rover and boat MAV_TYPE values collapse to RoverBoat class.
    QCOMPARE(QGCMAVLink::vehicleClass(MAV_TYPE_GROUND_ROVER), QGCMAVLink::VehicleClassRoverBoat);
    QCOMPARE(QGCMAVLink::vehicleClass(MAV_TYPE_SURFACE_BOAT), QGCMAVLink::VehicleClassRoverBoat);
}

void QGCMAVLinkTest::_testVehicleClassGenericFallback()
{
    // Types with no explicit mapping (e.g. ANTENNA_TRACKER, GCS, KITE) fall
    // through to Generic.
    QCOMPARE(QGCMAVLink::vehicleClass(MAV_TYPE_ANTENNA_TRACKER), QGCMAVLink::VehicleClassGeneric);
    QCOMPARE(QGCMAVLink::vehicleClass(MAV_TYPE_GCS), QGCMAVLink::VehicleClassGeneric);
    QCOMPARE(QGCMAVLink::vehicleClass(MAV_TYPE_KITE), QGCMAVLink::VehicleClassGeneric);
}

void QGCMAVLinkTest::_testVehicleClassToString()
{
    QCOMPARE(QGCMAVLink::vehicleClassToUserVisibleString(QGCMAVLink::VehicleClassFixedWing), QStringLiteral("Fixed Wing"));
    QCOMPARE(QGCMAVLink::vehicleClassToUserVisibleString(QGCMAVLink::VehicleClassMultiRotor), QStringLiteral("Multi-Rotor"));
    QCOMPARE(QGCMAVLink::vehicleClassToUserVisibleString(QGCMAVLink::VehicleClassVTOL), QStringLiteral("VTOL"));
    QCOMPARE(QGCMAVLink::vehicleClassToUserVisibleString(QGCMAVLink::VehicleClassSub), QStringLiteral("Sub"));
    QCOMPARE(QGCMAVLink::vehicleClassToUserVisibleString(QGCMAVLink::VehicleClassAirship), QStringLiteral("Airship"));
    QCOMPARE(QGCMAVLink::vehicleClassToUserVisibleString(QGCMAVLink::VehicleClassRoverBoat), QStringLiteral("Rover-Boat"));
    QCOMPARE(QGCMAVLink::vehicleClassToUserVisibleString(QGCMAVLink::VehicleClassGeneric), QStringLiteral("Generic"));

    const auto bogus = static_cast<QGCMAVLink::VehicleClass_t>(9999);
    QCOMPARE(QGCMAVLink::vehicleClassToUserVisibleString(bogus), QStringLiteral("Unknown"));
}

void QGCMAVLinkTest::_testVehicleClassInternalString()
{
    // Internal strings are non-translated identifiers (no dash/space).
    QCOMPARE(QGCMAVLink::vehicleClassToInternalString(QGCMAVLink::VehicleClassFixedWing), QStringLiteral("FixedWing"));
    QCOMPARE(QGCMAVLink::vehicleClassToInternalString(QGCMAVLink::VehicleClassMultiRotor), QStringLiteral("MultiRotor"));
    QCOMPARE(QGCMAVLink::vehicleClassToInternalString(QGCMAVLink::VehicleClassRoverBoat), QStringLiteral("RoverBoat"));
    QCOMPARE(QGCMAVLink::vehicleClassToInternalString(QGCMAVLink::VehicleClassGeneric), QStringLiteral("Generic"));

    const auto bogus = static_cast<QGCMAVLink::VehicleClass_t>(9999);
    QCOMPARE(QGCMAVLink::vehicleClassToInternalString(bogus), QStringLiteral("Unknown"));
}

void QGCMAVLinkTest::_testVehicleTypeFromString()
{
    QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("Multi-Rotor")), MAV_TYPE_QUADROTOR);
    QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("Fixed Wing")), MAV_TYPE_FIXED_WING);
    QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("Rover")), MAV_TYPE_GROUND_ROVER);
    QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("Sub")), MAV_TYPE_SUBMARINE);

    // Whitespace tolerance.
    QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("  Multi-Rotor ")), MAV_TYPE_QUADROTOR);

    // Unknown falls back to generic.
    QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("UnknownStuff")), MAV_TYPE_GENERIC);
}

// ---------------------------------------------------------------------------
// motorCount
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testMotorCountStandardTypes()
{
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_HELICOPTER),                      1);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_VTOL_TAILSITTER_DUOROTOR),        2);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_TRICOPTER),                       3);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_QUADROTOR),                       4);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_VTOL_TAILSITTER_QUADROTOR),       4);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_HEXAROTOR),                       6);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_OCTOROTOR),                       8);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SPACECRAFT_ORBITER),              8);
}

void QGCMAVLinkTest::_testMotorCountSubmarineFrames()
{
    // Frame type constants are internal to motorCount (sub_frame_t), use raw
    // indices that match the enum declaration order in QGCMAVLink.cc.
    constexpr uint8_t kBluerov1     = 0;
    constexpr uint8_t kVectored     = 1;
    constexpr uint8_t kVectored6DOF = 2;
    constexpr uint8_t kSimplerov3   = 4;
    constexpr uint8_t kSimplerov4   = 5;
    constexpr uint8_t kSimplerov5   = 6;
    constexpr uint8_t kCustom       = 7;

    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SUBMARINE, kBluerov1),     6);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SUBMARINE, kVectored),     6);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SUBMARINE, kVectored6DOF), 8);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SUBMARINE, kSimplerov3),   3);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SUBMARINE, kSimplerov4),   4);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SUBMARINE, kSimplerov5),   5);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SUBMARINE, kCustom),       8);

    // Unknown submarine frame returns -1.
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_SUBMARINE, 99),           -1);
}

void QGCMAVLinkTest::_testMotorCountUnknownReturnsNegative()
{
    // Fixed wing / rover / generic have no motor count in this model.
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_FIXED_WING),    -1);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_GROUND_ROVER),  -1);
    QCOMPARE(QGCMAVLink::motorCount(MAV_TYPE_GENERIC),       -1);
}

// ---------------------------------------------------------------------------
// mavResultToString
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testMavResultToString()
{
    QCOMPARE(QGCMAVLink::mavResultToString(MAV_RESULT_ACCEPTED),                QStringLiteral("MAV_RESULT_ACCEPTED"));
    QCOMPARE(QGCMAVLink::mavResultToString(MAV_RESULT_TEMPORARILY_REJECTED),    QStringLiteral("MAV_RESULT_TEMPORARILY_REJECTED"));
    QCOMPARE(QGCMAVLink::mavResultToString(MAV_RESULT_DENIED),                  QStringLiteral("MAV_RESULT_DENIED"));
    QCOMPARE(QGCMAVLink::mavResultToString(MAV_RESULT_UNSUPPORTED),             QStringLiteral("MAV_RESULT_UNSUPPORTED"));
    QCOMPARE(QGCMAVLink::mavResultToString(MAV_RESULT_FAILED),                  QStringLiteral("MAV_RESULT_FAILED"));
    QCOMPARE(QGCMAVLink::mavResultToString(MAV_RESULT_IN_PROGRESS),             QStringLiteral("MAV_RESULT_IN_PROGRESS"));
}

void QGCMAVLinkTest::_testMavResultToStringUnknown()
{
    // Unknown result codes are formatted with the numeric suffix.
    const QString result = QGCMAVLink::mavResultToString(static_cast<uint8_t>(250));
    QVERIFY2(result.contains(QStringLiteral("unknown")), qPrintable(result));
    QVERIFY2(result.contains(QStringLiteral("250")),     qPrintable(result));
}

// ---------------------------------------------------------------------------
// firmwareVersionType
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testFirmwareVersionTypeToString()
{
    QCOMPARE(QGCMAVLink::firmwareVersionTypeToString(FIRMWARE_VERSION_TYPE_DEV),      QStringLiteral("dev"));
    QCOMPARE(QGCMAVLink::firmwareVersionTypeToString(FIRMWARE_VERSION_TYPE_ALPHA),    QStringLiteral("alpha"));
    QCOMPARE(QGCMAVLink::firmwareVersionTypeToString(FIRMWARE_VERSION_TYPE_BETA),     QStringLiteral("beta"));
    QCOMPARE(QGCMAVLink::firmwareVersionTypeToString(FIRMWARE_VERSION_TYPE_RC),       QStringLiteral("rc"));
    // OFFICIAL / unknown both map to empty string.
    QCOMPARE(QGCMAVLink::firmwareVersionTypeToString(FIRMWARE_VERSION_TYPE_OFFICIAL), QString());
}

void QGCMAVLinkTest::_testFirmwareVersionTypeRoundTrip()
{
    const FIRMWARE_VERSION_TYPE types[] = {
        FIRMWARE_VERSION_TYPE_DEV,
        FIRMWARE_VERSION_TYPE_ALPHA,
        FIRMWARE_VERSION_TYPE_BETA,
        FIRMWARE_VERSION_TYPE_RC,
    };
    for (FIRMWARE_VERSION_TYPE t : types) {
        const QString s = QGCMAVLink::firmwareVersionTypeToString(t);
        QCOMPARE(QGCMAVLink::firmwareVersionTypeFromString(s), t);
    }

    // Empty/unknown tokens fall back to OFFICIAL.
    QCOMPARE(QGCMAVLink::firmwareVersionTypeFromString(QString()),                      FIRMWARE_VERSION_TYPE_OFFICIAL);
    QCOMPARE(QGCMAVLink::firmwareVersionTypeFromString(QStringLiteral("garbage")),      FIRMWARE_VERSION_TYPE_OFFICIAL);

    // Whitespace tolerance in parse.
    QCOMPARE(QGCMAVLink::firmwareVersionTypeFromString(QStringLiteral("  beta ")),      FIRMWARE_VERSION_TYPE_BETA);
}

// ---------------------------------------------------------------------------
// compIdToString
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testCompIdToStringKnown()
{
    const QString autopilot = QGCMAVLink::compIdToString(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY2(autopilot.contains(QStringLiteral("Autopilot")), qPrintable(autopilot));
    QVERIFY2(autopilot.contains(QString::number(static_cast<int>(MAV_COMP_ID_AUTOPILOT1))),
             qPrintable(autopilot));

    const QString camera1 = QGCMAVLink::compIdToString(MAV_COMP_ID_CAMERA);
    QVERIFY2(camera1.contains(QStringLiteral("Camera1")), qPrintable(camera1));
}

void QGCMAVLinkTest::_testCompIdToStringUnknown()
{
    // An arbitrary compId not present in the hash falls back to "Unknown (N)".
    const uint8_t unmappedCompId = 250;
    const QString formatted = QGCMAVLink::compIdToString(unmappedCompId);
    QVERIFY2(formatted.contains(QStringLiteral("Unknown")), qPrintable(formatted));
    QVERIFY2(formatted.contains(QString::number(static_cast<int>(unmappedCompId))),
             qPrintable(formatted));
}

// ---------------------------------------------------------------------------
// Channel validation
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testIsValidChannel()
{
    QVERIFY(QGCMAVLink::isValidChannel(uint8_t{0}));
    QVERIFY(QGCMAVLink::isValidChannel(static_cast<uint8_t>(MAVLINK_COMM_NUM_BUFFERS - 1)));
    QVERIFY(!QGCMAVLink::isValidChannel(static_cast<uint8_t>(MAVLINK_COMM_NUM_BUFFERS)));
    QVERIFY(!QGCMAVLink::isValidChannel(uint8_t{255}));
}

// ---------------------------------------------------------------------------
// Static lists
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testAllFirmwareClasses()
{
    const auto classes = QGCMAVLink::allFirmwareClasses();
    QCOMPARE(classes.size(), 3);
    QVERIFY(classes.contains(QGCMAVLink::FirmwareClassPX4));
    QVERIFY(classes.contains(QGCMAVLink::FirmwareClassArduPilot));
    QVERIFY(classes.contains(QGCMAVLink::FirmwareClassGeneric));
}

void QGCMAVLinkTest::_testAllVehicleClasses()
{
    const auto classes = QGCMAVLink::allVehicleClasses();
    QVERIFY(classes.contains(QGCMAVLink::VehicleClassFixedWing));
    QVERIFY(classes.contains(QGCMAVLink::VehicleClassMultiRotor));
    QVERIFY(classes.contains(QGCMAVLink::VehicleClassVTOL));
    QVERIFY(classes.contains(QGCMAVLink::VehicleClassRoverBoat));
    QVERIFY(classes.contains(QGCMAVLink::VehicleClassSub));
    QVERIFY(classes.contains(QGCMAVLink::VehicleClassGeneric));

    // No duplicates — use a plain linear scan since QList<enum> has no toSet().
    for (int i = 0; i < classes.size(); ++i) {
        for (int j = i + 1; j < classes.size(); ++j) {
            QVERIFY2(classes.at(i) != classes.at(j),
                     qPrintable(QStringLiteral("duplicate vehicle class at %1/%2").arg(i).arg(j)));
        }
    }
}

UT_REGISTER_TEST(QGCMAVLinkTest, TestLabel::Unit)
