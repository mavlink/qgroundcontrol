#include "APMParamValueTest.h"

#include <bit>

#include <QtCore/QRegularExpression>

#include "FirmwarePlugin.h"
#include "MAVLinkLib.h"
#include "Vehicle.h"

namespace {

mavlink_param_value_t adjustedParamValue(Vehicle* vehicle, float value, uint8_t paramType)
{
    // The packer copies the full fixed-size param_id field, so it needs a full-size buffer.
    const char paramId[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN] = "QGC_TEST";
    mavlink_status_t status{};
    mavlink_message_t message{};
    (void) mavlink_msg_param_value_pack_status(vehicle->id(), MAV_COMP_ID_AUTOPILOT1, &status, &message, paramId, value,
                                               paramType, 1, 0);
    (void) vehicle->firmwarePlugin()->adjustIncomingMavlinkMessage(vehicle, &message);

    mavlink_param_value_t paramValue{};
    mavlink_msg_param_value_decode(&message, &paramValue);
    return paramValue;
}

}  // namespace

void APMParamValueTest::_supportedTypeReencodedBytewise_data()
{
    QTest::addColumn<int>("paramType");
    QTest::addColumn<float>("ardupilotValue");
    QTest::addColumn<quint32>("expectedBits");

    QTest::newRow("uint8") << int{MAV_PARAM_TYPE_UINT8} << 200.0f << quint32{0x000000C8};
    QTest::newRow("int8") << int{MAV_PARAM_TYPE_INT8} << -5.0f << quint32{0x000000FB};
    QTest::newRow("uint16") << int{MAV_PARAM_TYPE_UINT16} << 60000.0f << quint32{0x0000EA60};
    QTest::newRow("int16") << int{MAV_PARAM_TYPE_INT16} << -1234.0f << quint32{0x0000FB2E};
    QTest::newRow("uint32") << int{MAV_PARAM_TYPE_UINT32} << 100000.0f << quint32{0x000186A0};
    QTest::newRow("int32") << int{MAV_PARAM_TYPE_INT32} << -100000.0f << quint32{0xFFFE7960};
    QTest::newRow("real32") << int{MAV_PARAM_TYPE_REAL32} << 1.5f << quint32{0x3FC00000};
}

void APMParamValueTest::_supportedTypeReencodedBytewise()
{
    QFETCH(int, paramType);
    QFETCH(float, ardupilotValue);
    QFETCH(quint32, expectedBits);
    QVERIFY(vehicle());

    const mavlink_param_value_t paramValue =
        adjustedParamValue(vehicle(), ardupilotValue, static_cast<uint8_t>(paramType));

    QCOMPARE(paramValue.param_type, static_cast<uint8_t>(paramType));
    QCOMPARE(std::bit_cast<quint32>(paramValue.param_value), expectedBits);
}

void APMParamValueTest::_unsupportedTypeReencodedAsZero()
{
    QVERIFY(vehicle());

    expectLogMessage("FirmwarePlugin.APMFirmwarePlugin", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("Invalid/Unsupported data type used in parameter")));
    const mavlink_param_value_t paramValue = adjustedParamValue(vehicle(), 3.0f, MAV_PARAM_TYPE_REAL64);
    verifyExpectedLogMessage();

    QCOMPARE(paramValue.param_type, static_cast<uint8_t>(MAV_PARAM_TYPE_REAL64));
    QCOMPARE(std::bit_cast<quint32>(paramValue.param_value), quint32{0});
}

UT_REGISTER_TEST(APMParamValueTest, TestLabel::Integration, TestLabel::Vehicle)
