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
    QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("Rover")), MAV_TYPE_GROUND_ROVER);         // legacy spelling
    QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("Rover-Boat")), MAV_TYPE_GROUND_ROVER);    // canonical spelling
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

// ---------------------------------------------------------------------------
// PARAM_EXT value conversion
// ---------------------------------------------------------------------------

void QGCMAVLinkTest::_testParamExtRoundTrip_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("uint8") << static_cast<int>(MAV_PARAM_EXT_TYPE_UINT8) << QVariant(200);
    QTest::newRow("int8") << static_cast<int>(MAV_PARAM_EXT_TYPE_INT8) << QVariant(-5);
    QTest::newRow("uint16") << static_cast<int>(MAV_PARAM_EXT_TYPE_UINT16) << QVariant(60000);
    QTest::newRow("int16") << static_cast<int>(MAV_PARAM_EXT_TYPE_INT16) << QVariant(-30000);
    QTest::newRow("uint32") << static_cast<int>(MAV_PARAM_EXT_TYPE_UINT32) << QVariant(4000000000u);
    QTest::newRow("int32") << static_cast<int>(MAV_PARAM_EXT_TYPE_INT32) << QVariant(-2000000000);
    QTest::newRow("uint64") << static_cast<int>(MAV_PARAM_EXT_TYPE_UINT64)
                            << QVariant(static_cast<quint64>(9223372036854775813ull));
    QTest::newRow("int64") << static_cast<int>(MAV_PARAM_EXT_TYPE_INT64)
                           << QVariant(static_cast<qint64>(-4611686018427387904ll));
    QTest::newRow("real32") << static_cast<int>(MAV_PARAM_EXT_TYPE_REAL32) << QVariant(1.5f);
    QTest::newRow("real64") << static_cast<int>(MAV_PARAM_EXT_TYPE_REAL64) << QVariant(3.141592653589793);
    QTest::newRow("custom") << static_cast<int>(MAV_PARAM_EXT_TYPE_CUSTOM) << QVariant(QStringLiteral("MockCam"));
}

void QGCMAVLinkTest::_testParamExtRoundTrip()
{
    QFETCH(int, type);
    QFETCH(QVariant, value);

    char blob[MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN] = {};
    QVERIFY(QGCMAVLink::variantToParamExtValue(value, static_cast<uint8_t>(type), blob));

    const QVariant decoded = QGCMAVLink::paramExtValueToVariant(blob, static_cast<uint8_t>(type));
    QVERIFY(decoded.isValid());

    // Compare through the accessor matching the wire type, so a signedness or width mismatch can't pass by
    // happening to print the same. The unsigned rows use values with the top bit set for that reason.
    bool decodedOk = false;
    bool valueOk = false;
    switch (type) {
        case MAV_PARAM_EXT_TYPE_UINT8:
        case MAV_PARAM_EXT_TYPE_UINT16:
        case MAV_PARAM_EXT_TYPE_UINT32:
            QCOMPARE(decoded.toUInt(&decodedOk), value.toUInt(&valueOk));
            break;
        case MAV_PARAM_EXT_TYPE_INT8:
        case MAV_PARAM_EXT_TYPE_INT16:
        case MAV_PARAM_EXT_TYPE_INT32:
            QCOMPARE(decoded.toInt(&decodedOk), value.toInt(&valueOk));
            break;
        case MAV_PARAM_EXT_TYPE_UINT64:
            QCOMPARE(decoded.toULongLong(&decodedOk), value.toULongLong(&valueOk));
            break;
        case MAV_PARAM_EXT_TYPE_INT64:
            QCOMPARE(decoded.toLongLong(&decodedOk), value.toLongLong(&valueOk));
            break;
        case MAV_PARAM_EXT_TYPE_REAL32:
            QCOMPARE(decoded.toFloat(&decodedOk), value.toFloat(&valueOk));
            break;
        case MAV_PARAM_EXT_TYPE_REAL64:
            QCOMPARE(decoded.toDouble(&decodedOk), value.toDouble(&valueOk));
            break;
        case MAV_PARAM_EXT_TYPE_CUSTOM:
            QCOMPARE(decoded.typeId(), static_cast<int>(QMetaType::QString));
            QCOMPARE(decoded.toString().toUtf8(), value.toString().toUtf8());
            decodedOk = true;
            valueOk = true;
            break;
        default:
            QFAIL("Unexpected MAV_PARAM_EXT_TYPE in test data");
    }
    QVERIFY(decodedOk);
    QVERIFY(valueOk);
}

// A value longer than the blob must be cut to fit, not run past the end of it
void QGCMAVLinkTest::_testParamExtCustomTruncatedToBlob()
{
    const QString tooLong(MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN + 72, QLatin1Char('A'));

    char blob[MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN] = {};
    QVERIFY(QGCMAVLink::variantToParamExtValue(tooLong, MAV_PARAM_EXT_TYPE_CUSTOM, blob));

    const QVariant decoded = QGCMAVLink::paramExtValueToVariant(blob, MAV_PARAM_EXT_TYPE_CUSTOM);
    QCOMPARE(decoded.toString(), tooLong.left(MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN));
}

void QGCMAVLinkTest::_testParamExtEncodeInvalidValue()
{
    char blob[MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN] = {};

    expectLogMessage("MAVLink.QGCMAVLink", QtWarningMsg, QRegularExpression(QStringLiteral("Failed to convert value")));
    QVERIFY(!QGCMAVLink::variantToParamExtValue(QStringLiteral("not a number"), MAV_PARAM_EXT_TYPE_INT32, blob));
    verifyExpectedLogMessage();
}

void QGCMAVLinkTest::_testParamExtDecodeUnsupportedType()
{
    const char blob[MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN] = {};

    expectLogMessage("MAVLink.QGCMAVLink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unsupported MAV_PARAM_EXT_TYPE")));
    QVERIFY(!QGCMAVLink::paramExtValueToVariant(blob, 200).isValid());
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(QGCMAVLinkTest, TestLabel::Unit)
