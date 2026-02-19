#include "SysStatusSensorInfoTest.h"

#include <QtTest/QSignalSpy>

#include "QGCMAVLink.h"
#include "SysStatusSensorInfo.h"

static mavlink_sys_status_t _makeSysStatus(uint32_t present, uint32_t enabled, uint32_t health)
{
    mavlink_sys_status_t status{};
    status.onboard_control_sensors_present = present;
    status.onboard_control_sensors_enabled = enabled;
    status.onboard_control_sensors_health = health;
    return status;
}

void SysStatusSensorInfoTest::_initialState_test()
{
    SysStatusSensorInfo info;
    QVERIFY(info.sensorNames().isEmpty());
    QVERIFY(info.sensorStatus().isEmpty());
}

void SysStatusSensorInfoTest::_updateSingleSensorHealthy_test()
{
    SysStatusSensorInfo info;

    // MAV_SYS_STATUS_SENSOR_3D_GYRO = 1 (bit 0)
    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    info.update(_makeSysStatus(gyro, gyro, gyro));

    QCOMPARE(info.sensorNames().count(), 1);
    QCOMPARE(info.sensorStatus().count(), 1);
    QCOMPARE(info.sensorStatus().at(0), QStringLiteral("Normal"));
}

void SysStatusSensorInfoTest::_updateSingleSensorUnhealthy_test()
{
    SysStatusSensorInfo info;

    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    // present + enabled but NOT healthy
    info.update(_makeSysStatus(gyro, gyro, 0));

    QCOMPARE(info.sensorNames().count(), 1);
    QCOMPARE(info.sensorStatus().count(), 1);
    QCOMPARE(info.sensorStatus().at(0), QStringLiteral("Error"));
}

void SysStatusSensorInfoTest::_updateSingleSensorDisabled_test()
{
    SysStatusSensorInfo info;

    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    // present but NOT enabled (health irrelevant)
    info.update(_makeSysStatus(gyro, 0, 0));

    QCOMPARE(info.sensorNames().count(), 1);
    QCOMPARE(info.sensorStatus().count(), 1);
    QCOMPARE(info.sensorStatus().at(0), QStringLiteral("Disabled"));
}

void SysStatusSensorInfoTest::_sensorOrdering_test()
{
    SysStatusSensorInfo info;

    // Set up 3 sensors in different states:
    // Gyro (bit 0): enabled + healthy → "Normal"
    // Accel (bit 1): enabled + NOT healthy → "Error"
    // Mag (bit 2): NOT enabled → "Disabled"
    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;   // 0x01
    const uint32_t accel = MAV_SYS_STATUS_SENSOR_3D_ACCEL;  // 0x02
    const uint32_t mag = MAV_SYS_STATUS_SENSOR_3D_MAG;      // 0x04

    const uint32_t present = gyro | accel | mag;
    const uint32_t enabled = gyro | accel;  // mag disabled
    const uint32_t health = gyro;           // accel unhealthy

    info.update(_makeSysStatus(present, enabled, health));

    const QStringList statuses = info.sensorStatus();
    QCOMPARE(statuses.count(), 3);

    // Ordering: unhealthy first, then healthy, then disabled
    QCOMPARE(statuses.at(0), QStringLiteral("Error"));
    QCOMPARE(statuses.at(1), QStringLiteral("Normal"));
    QCOMPARE(statuses.at(2), QStringLiteral("Disabled"));
}

void SysStatusSensorInfoTest::_sensorInfoChangedSignal_test()
{
    SysStatusSensorInfo info;
    QSignalSpy spy(&info, &SysStatusSensorInfo::sensorInfoChanged);
    QVERIFY(spy.isValid());

    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    info.update(_makeSysStatus(gyro, gyro, gyro));
    QCOMPARE(spy.count(), 1);
}

void SysStatusSensorInfoTest::_noSignalOnSameState_test()
{
    SysStatusSensorInfo info;

    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    const auto status = _makeSysStatus(gyro, gyro, gyro);
    info.update(status);

    QSignalSpy spy(&info, &SysStatusSensorInfo::sensorInfoChanged);
    QVERIFY(spy.isValid());

    // Same state again — should NOT emit
    info.update(status);
    QCOMPARE(spy.count(), 0);
}

void SysStatusSensorInfoTest::_sensorRemoval_test()
{
    SysStatusSensorInfo info;

    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    info.update(_makeSysStatus(gyro, gyro, gyro));
    QCOMPARE(info.sensorNames().count(), 1);

    QSignalSpy spy(&info, &SysStatusSensorInfo::sensorInfoChanged);

    // Sensor no longer present
    info.update(_makeSysStatus(0, 0, 0));
    QCOMPARE(spy.count(), 1);
    QVERIFY(info.sensorNames().isEmpty());
    QVERIFY(info.sensorStatus().isEmpty());
}

void SysStatusSensorInfoTest::_multipleSensors_test()
{
    SysStatusSensorInfo info;

    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    const uint32_t accel = MAV_SYS_STATUS_SENSOR_3D_ACCEL;
    const uint32_t mag = MAV_SYS_STATUS_SENSOR_3D_MAG;
    const uint32_t baro = MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE; // 0x08
    const uint32_t gps = MAV_SYS_STATUS_SENSOR_GPS;                 // 0x20

    const uint32_t present = gyro | accel | mag | baro | gps;
    const uint32_t enabled = present;
    const uint32_t health = present;

    info.update(_makeSysStatus(present, enabled, health));

    QCOMPARE(info.sensorNames().count(), 5);
    QCOMPARE(info.sensorStatus().count(), 5);

    // All healthy
    for (const QString &status : info.sensorStatus()) {
        QCOMPARE(status, QStringLiteral("Normal"));
    }
}

void SysStatusSensorInfoTest::_updateExistingSensorFlipsHealth_test()
{
    SysStatusSensorInfo info;
    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;

    info.update(_makeSysStatus(gyro, gyro, gyro));
    QSignalSpy spy(&info, &SysStatusSensorInfo::sensorInfoChanged);
    QVERIFY(spy.isValid());

    info.update(_makeSysStatus(gyro, gyro, 0));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(info.sensorStatus(), QStringList({QStringLiteral("Error")}));
}

void SysStatusSensorInfoTest::_updateExistingSensorDisables_test()
{
    SysStatusSensorInfo info;
    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;

    info.update(_makeSysStatus(gyro, gyro, gyro));
    QSignalSpy spy(&info, &SysStatusSensorInfo::sensorInfoChanged);
    QVERIFY(spy.isValid());

    info.update(_makeSysStatus(gyro, 0, 0));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(info.sensorStatus(), QStringList({QStringLiteral("Disabled")}));
}

void SysStatusSensorInfoTest::_sensorNamesOrderingMirrorsStatusOrder_test()
{
    SysStatusSensorInfo info;

    const uint32_t gyro = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    const uint32_t accel = MAV_SYS_STATUS_SENSOR_3D_ACCEL;
    const uint32_t mag = MAV_SYS_STATUS_SENSOR_3D_MAG;

    const uint32_t present = gyro | accel | mag;
    const uint32_t enabled = gyro | accel;
    const uint32_t health = gyro;

    info.update(_makeSysStatus(present, enabled, health));

    const QStringList expectedNames = {
        QGCMAVLink::mavSysStatusSensorToString(static_cast<MAV_SYS_STATUS_SENSOR>(accel)),
        QGCMAVLink::mavSysStatusSensorToString(static_cast<MAV_SYS_STATUS_SENSOR>(gyro)),
        QGCMAVLink::mavSysStatusSensorToString(static_cast<MAV_SYS_STATUS_SENSOR>(mag)),
    };
    const QStringList expectedStatus = {
        QStringLiteral("Error"),
        QStringLiteral("Normal"),
        QStringLiteral("Disabled"),
    };

    QCOMPARE(info.sensorNames(), expectedNames);
    QCOMPARE(info.sensorStatus(), expectedStatus);
}

UT_REGISTER_TEST(SysStatusSensorInfoTest, TestLabel::Unit)
