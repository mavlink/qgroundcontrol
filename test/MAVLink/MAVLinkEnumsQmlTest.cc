#include "MAVLinkEnumsQmlTest.h"

#include <QtCore/QMetaEnum>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QTest>

#include "MAVLinkEnumsQml.h"
#include "MAVLinkLib.h"

void MAVLinkEnumsQmlTest::_metaObjectExposesEveryEnumerator()
{
    const QMetaObject& metaObject = MAVLinkEnums::staticMetaObject;

    // The generated namespace must carry real enum declarations; `using` aliases leave moc
    // with registered names but empty key tables.
    QVERIFY(metaObject.enumeratorCount() > 100);
    for (int i = 0; i < metaObject.enumeratorCount(); ++i) {
        const QMetaEnum metaEnum = metaObject.enumerator(i);
        QVERIFY2(metaEnum.keyCount() > 0, metaEnum.name());
    }

    const int index = metaObject.indexOfEnumerator("MAV_BATTERY_CHARGE_STATE");
    QVERIFY(index >= 0);
    const QMetaEnum chargeState = metaObject.enumerator(index);
    QCOMPARE(chargeState.keyToValue("MAV_BATTERY_CHARGE_STATE_OK"), static_cast<int>(MAV_BATTERY_CHARGE_STATE_OK));
    QCOMPARE(chargeState.keyToValue("MAV_BATTERY_CHARGE_STATE_CRITICAL"),
             static_cast<int>(MAV_BATTERY_CHARGE_STATE_CRITICAL));
}

void MAVLinkEnumsQmlTest::_qmlReadsMavlinkValues()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QGroundControl

        QtObject {
            property var chargeStateOk:       MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_OK
            property var chargeStateCritical: MAVLinkEnums.MAV_BATTERY_CHARGE_STATE_CRITICAL
            property var gpsSensor:           MAVLinkEnums.MAV_SYS_STATUS_SENSOR_GPS
            property var gripperGrab:         MAVLinkEnums.GRIPPER_ACTION_GRAB
            property var sensorMask:          MAVLinkEnums.MAV_SYS_STATUS_SENSOR_3D_GYRO | MAVLinkEnums.MAV_SYS_STATUS_SENSOR_GPS
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    const QScopedPointer<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));

    // With the broken generator every property below is undefined (invalid QVariant).
    const auto valueOf = [&object](const char* name) {
        const QVariant value = object->property(name);
        return value.isValid() && !value.isNull() ? value.toLongLong() : -1LL;
    };
    QCOMPARE(valueOf("chargeStateOk"), static_cast<qlonglong>(MAV_BATTERY_CHARGE_STATE_OK));
    QCOMPARE(valueOf("chargeStateCritical"), static_cast<qlonglong>(MAV_BATTERY_CHARGE_STATE_CRITICAL));
    QCOMPARE(valueOf("gpsSensor"), static_cast<qlonglong>(MAV_SYS_STATUS_SENSOR_GPS));
    QCOMPARE(valueOf("gripperGrab"), static_cast<qlonglong>(GRIPPER_ACTION_GRAB));
    QCOMPARE(valueOf("sensorMask"), static_cast<qlonglong>(MAV_SYS_STATUS_SENSOR_3D_GYRO | MAV_SYS_STATUS_SENSOR_GPS));
}

UT_REGISTER_TEST(MAVLinkEnumsQmlTest, TestLabel::Unit)
