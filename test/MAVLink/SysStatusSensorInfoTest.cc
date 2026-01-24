#include "SysStatusSensorInfoTest.h"
#include "SysStatusSensorInfo.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void SysStatusSensorInfoTest::_initialStateTest()
{
    SysStatusSensorInfo sensorInfo;

    QVERIFY(sensorInfo.sensorNames().isEmpty());
    QVERIFY(sensorInfo.sensorStatus().isEmpty());
}

void SysStatusSensorInfoTest::_singleSensorUpdateTest()
{
    SysStatusSensorInfo sensorInfo;
    QSignalSpy spy(&sensorInfo, &SysStatusSensorInfo::sensorInfoChanged);

    mavlink_sys_status_t sysStatus{};
    sysStatus.onboard_control_sensors_present = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    sysStatus.onboard_control_sensors_enabled = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    sysStatus.onboard_control_sensors_health = MAV_SYS_STATUS_SENSOR_3D_GYRO;

    sensorInfo.update(sysStatus);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(sensorInfo.sensorNames().size(), 1);
    QCOMPARE(sensorInfo.sensorStatus().size(), 1);
}

void SysStatusSensorInfoTest::_multipleSensorUpdateTest()
{
    SysStatusSensorInfo sensorInfo;
    QSignalSpy spy(&sensorInfo, &SysStatusSensorInfo::sensorInfoChanged);

    mavlink_sys_status_t sysStatus{};
    // Present: gyro, accel, mag
    sysStatus.onboard_control_sensors_present =
        MAV_SYS_STATUS_SENSOR_3D_GYRO |
        MAV_SYS_STATUS_SENSOR_3D_ACCEL |
        MAV_SYS_STATUS_SENSOR_3D_MAG;
    // All enabled
    sysStatus.onboard_control_sensors_enabled =
        MAV_SYS_STATUS_SENSOR_3D_GYRO |
        MAV_SYS_STATUS_SENSOR_3D_ACCEL |
        MAV_SYS_STATUS_SENSOR_3D_MAG;
    // All healthy
    sysStatus.onboard_control_sensors_health =
        MAV_SYS_STATUS_SENSOR_3D_GYRO |
        MAV_SYS_STATUS_SENSOR_3D_ACCEL |
        MAV_SYS_STATUS_SENSOR_3D_MAG;

    sensorInfo.update(sysStatus);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(sensorInfo.sensorNames().size(), 3);
    QCOMPARE(sensorInfo.sensorStatus().size(), 3);
}

void SysStatusSensorInfoTest::_sensorEnabledHealthyTest()
{
    SysStatusSensorInfo sensorInfo;

    mavlink_sys_status_t sysStatus{};
    sysStatus.onboard_control_sensors_present = MAV_SYS_STATUS_SENSOR_GPS;
    sysStatus.onboard_control_sensors_enabled = MAV_SYS_STATUS_SENSOR_GPS;
    sysStatus.onboard_control_sensors_health = MAV_SYS_STATUS_SENSOR_GPS;

    sensorInfo.update(sysStatus);

    QCOMPARE(sensorInfo.sensorStatus().size(), 1);
    QCOMPARE(sensorInfo.sensorStatus().at(0), QStringLiteral("Normal"));
}

void SysStatusSensorInfoTest::_sensorEnabledUnhealthyTest()
{
    SysStatusSensorInfo sensorInfo;

    mavlink_sys_status_t sysStatus{};
    sysStatus.onboard_control_sensors_present = MAV_SYS_STATUS_SENSOR_GPS;
    sysStatus.onboard_control_sensors_enabled = MAV_SYS_STATUS_SENSOR_GPS;
    sysStatus.onboard_control_sensors_health = 0;  // Not healthy

    sensorInfo.update(sysStatus);

    QCOMPARE(sensorInfo.sensorStatus().size(), 1);
    QCOMPARE(sensorInfo.sensorStatus().at(0), QStringLiteral("Error"));
}

void SysStatusSensorInfoTest::_sensorDisabledTest()
{
    SysStatusSensorInfo sensorInfo;

    mavlink_sys_status_t sysStatus{};
    sysStatus.onboard_control_sensors_present = MAV_SYS_STATUS_SENSOR_GPS;
    sysStatus.onboard_control_sensors_enabled = 0;  // Not enabled
    sysStatus.onboard_control_sensors_health = 0;

    sensorInfo.update(sysStatus);

    QCOMPARE(sensorInfo.sensorStatus().size(), 1);
    QCOMPARE(sensorInfo.sensorStatus().at(0), QStringLiteral("Disabled"));
}

void SysStatusSensorInfoTest::_sensorAddedRemovedTest()
{
    SysStatusSensorInfo sensorInfo;
    QSignalSpy spy(&sensorInfo, &SysStatusSensorInfo::sensorInfoChanged);

    // Add sensor
    mavlink_sys_status_t sysStatus{};
    sysStatus.onboard_control_sensors_present = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    sysStatus.onboard_control_sensors_enabled = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    sysStatus.onboard_control_sensors_health = MAV_SYS_STATUS_SENSOR_3D_GYRO;

    sensorInfo.update(sysStatus);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(sensorInfo.sensorNames().size(), 1);

    // Remove sensor (not present anymore)
    sysStatus.onboard_control_sensors_present = 0;
    sysStatus.onboard_control_sensors_enabled = 0;
    sysStatus.onboard_control_sensors_health = 0;

    sensorInfo.update(sysStatus);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(sensorInfo.sensorNames().size(), 0);
}

void SysStatusSensorInfoTest::_sensorStatusChangeTest()
{
    SysStatusSensorInfo sensorInfo;
    QSignalSpy spy(&sensorInfo, &SysStatusSensorInfo::sensorInfoChanged);

    // Start healthy
    mavlink_sys_status_t sysStatus{};
    sysStatus.onboard_control_sensors_present = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    sysStatus.onboard_control_sensors_enabled = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    sysStatus.onboard_control_sensors_health = MAV_SYS_STATUS_SENSOR_3D_GYRO;

    sensorInfo.update(sysStatus);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(sensorInfo.sensorStatus().at(0), QStringLiteral("Normal"));

    // Become unhealthy
    sysStatus.onboard_control_sensors_health = 0;
    sensorInfo.update(sysStatus);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(sensorInfo.sensorStatus().at(0), QStringLiteral("Error"));
}

void SysStatusSensorInfoTest::_noChangeNoSignalTest()
{
    SysStatusSensorInfo sensorInfo;
    QSignalSpy spy(&sensorInfo, &SysStatusSensorInfo::sensorInfoChanged);

    mavlink_sys_status_t sysStatus{};
    sysStatus.onboard_control_sensors_present = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    sysStatus.onboard_control_sensors_enabled = MAV_SYS_STATUS_SENSOR_3D_GYRO;
    sysStatus.onboard_control_sensors_health = MAV_SYS_STATUS_SENSOR_3D_GYRO;

    sensorInfo.update(sysStatus);
    QCOMPARE(spy.count(), 1);

    // Same update - should not emit signal
    sensorInfo.update(sysStatus);
    QCOMPARE(spy.count(), 1);
}

void SysStatusSensorInfoTest::_sensorOrderingTest()
{
    SysStatusSensorInfo sensorInfo;

    mavlink_sys_status_t sysStatus{};
    // Set up three sensors with different states
    sysStatus.onboard_control_sensors_present =
        MAV_SYS_STATUS_SENSOR_3D_GYRO |   // Will be unhealthy
        MAV_SYS_STATUS_SENSOR_3D_ACCEL |  // Will be healthy
        MAV_SYS_STATUS_SENSOR_3D_MAG;     // Will be disabled

    sysStatus.onboard_control_sensors_enabled =
        MAV_SYS_STATUS_SENSOR_3D_GYRO |
        MAV_SYS_STATUS_SENSOR_3D_ACCEL;
    // MAG not enabled (disabled)

    sysStatus.onboard_control_sensors_health =
        MAV_SYS_STATUS_SENSOR_3D_ACCEL;
    // GYRO not healthy (error), MAG not healthy but also disabled

    sensorInfo.update(sysStatus);

    // Ordering should be: unhealthy first, then healthy, then disabled
    const QStringList statuses = sensorInfo.sensorStatus();
    QCOMPARE(statuses.size(), 3);
    QCOMPARE(statuses.at(0), QStringLiteral("Error"));    // Gyro - enabled but unhealthy
    QCOMPARE(statuses.at(1), QStringLiteral("Normal"));   // Accel - enabled and healthy
    QCOMPARE(statuses.at(2), QStringLiteral("Disabled")); // Mag - not enabled
}
