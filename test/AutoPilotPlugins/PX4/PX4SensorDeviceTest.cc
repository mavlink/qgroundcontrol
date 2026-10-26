#include "PX4SensorDeviceTest.h"

#include "PX4SensorDevice.h"

UT_REGISTER_TEST_LIGHTWEIGHT(PX4SensorDeviceTest, TestLabel::Unit)

void PX4SensorDeviceTest::_description_data()
{
    QTest::addColumn<quint32>("deviceId");
    QTest::addColumn<QString>("expected");

    QTest::newRow("unset") << 0u << QString();
    QTest::newRow("imuOnSpi") << 0x26000Au << QStringLiteral("ICM42688P · SPI");
    QTest::newRow("magOnI2c") << 0x060E09u << QStringLiteral("IST8310 · I2C");
    QTest::newRow("highDeviceType") << 0xB7000Au << QStringLiteral("ICP101XX · SPI");
    QTest::newRow("droneCanSensor") << 0x880003u << QStringLiteral("DroneCAN");
    QTest::newRow("unknownTypeKeepsBus") << 0x30000Au << QStringLiteral("SPI");
    QTest::newRow("unknownTypeAndBus") << 0x100u << QStringLiteral("0x00000100");
}

void PX4SensorDeviceTest::_description()
{
    QFETCH(quint32, deviceId);
    QFETCH(QString, expected);

    QCOMPARE(PX4SensorDevice::description(deviceId), expected);
}
