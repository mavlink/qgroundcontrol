#include "FailureInjectionTest.h"

#include <QtTest/QSignalSpy>

#include "FailureInjection.h"
#include "MAVLinkLib.h"

UT_REGISTER_TEST_LIGHTWEIGHT(FailureInjectionTest, TestLabel::Unit)

void FailureInjectionTest::_catalogPopulatedFromMavlinkEnums()
{
    FailureInjection failureInjection;

    const QVariantList units = failureInjection.units();
    const QVariantList types = failureInjection.types();
    QVERIFY2(!units.isEmpty(), "FAILURE_UNIT catalog failed to build from the MAVLink dialect");
    QVERIFY2(!types.isEmpty(), "FAILURE_TYPE catalog failed to build from the MAVLink dialect");

    bool foundGps = false;
    for (const QVariant& entry : units) {
        const QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("unit")).toInt() == static_cast<int>(FAILURE_UNIT_SENSOR_GPS)) {
            QCOMPARE(map.value(QStringLiteral("name")).toString(), QStringLiteral("GPS"));
            foundGps = true;
        }
    }
    QVERIFY2(foundGps, "FAILURE_UNIT_SENSOR_GPS missing from the units catalog, or its prefix was not stripped");

    bool foundOk = false;
    for (const QVariant& entry : types) {
        const QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("type")).toInt() == static_cast<int>(FAILURE_TYPE_OK)) {
            QCOMPARE(map.value(QStringLiteral("name")).toString(), QStringLiteral("OK"));
            foundOk = true;
        }
    }
    QVERIFY2(foundOk, "FAILURE_TYPE_OK missing from the types catalog, or its prefix was not stripped");
}

void FailureInjectionTest::_logRowAddsPendingEntryWithoutTracking()
{
    FailureInjection failureInjection;
    QSignalSpy activityChangedSpy(&failureInjection, &FailureInjection::activityChanged);

    failureInjection.logRow(QStringLiteral("GPS"), QStringLiteral("Off"), QStringLiteral("1"),
                            QStringLiteral("12:00:00"));

    QCOMPARE(activityChangedSpy.count(), 1);
    QCOMPARE(failureInjection.activity().count(), 1);
    QCOMPARE(failureInjection.activity().first().toMap().value(QStringLiteral("result")).toString(),
             QStringLiteral("pending"));
    QVERIFY2(failureInjection.injectedUnits().isEmpty(), "logRow() must not track the unit for Reset");
}

void FailureInjectionTest::_logInjectionTracksUnitOnce()
{
    FailureInjection failureInjection;

    failureInjection.logInjection(QStringLiteral("GPS"), QStringLiteral("Off"), FAILURE_UNIT_SENSOR_GPS,
                                  QStringLiteral("1"), QStringLiteral("12:00:00"));
    QCOMPARE(failureInjection.injectedUnits().count(), 1);
    QCOMPARE(failureInjection.injectedUnits().first().toInt(), static_cast<int>(FAILURE_UNIT_SENSOR_GPS));

    // Injecting the same unit again must not duplicate the tracked entry.
    failureInjection.logInjection(QStringLiteral("GPS"), QStringLiteral("Stuck"), FAILURE_UNIT_SENSOR_GPS,
                                  QStringLiteral("2"), QStringLiteral("12:00:01"));
    QCOMPARE(failureInjection.injectedUnits().count(), 1);
    QCOMPARE(failureInjection.activity().count(), 2);
}

void FailureInjectionTest::_resolveResultResolvesOldestPendingRow()
{
    FailureInjection failureInjection;

    // Newest is prepended: index 0 holds the second row logged, index 1 the first.
    failureInjection.logRow(QStringLiteral("GPS"), QStringLiteral("Off"), QStringLiteral("1"),
                            QStringLiteral("12:00:00"));
    failureInjection.logRow(QStringLiteral("GYRO"), QStringLiteral("Stuck"), QStringLiteral("2"),
                            QStringLiteral("12:00:01"));

    failureInjection.resolveResult(MAV_RESULT_ACCEPTED);

    const QVariantList activity = failureInjection.activity();
    QCOMPARE(activity.at(1).toMap().value(QStringLiteral("result")).toString(),
             QStringLiteral("accepted"));  // oldest (GPS) resolved first
    QCOMPARE(activity.at(0).toMap().value(QStringLiteral("result")).toString(),
             QStringLiteral("pending"));   // newest (GYRO) still pending
}

void FailureInjectionTest::_resolveResultIgnoresInProgress()
{
    FailureInjection failureInjection;
    failureInjection.logRow(QStringLiteral("GPS"), QStringLiteral("Off"), QStringLiteral("1"),
                            QStringLiteral("12:00:00"));

    failureInjection.resolveResult(MAV_RESULT_IN_PROGRESS);

    QCOMPARE(failureInjection.activity().first().toMap().value(QStringLiteral("result")).toString(),
             QStringLiteral("pending"));
}

void FailureInjectionTest::_resolveResultUnknownCodeFallsBackToMavResultString()
{
    FailureInjection failureInjection;
    failureInjection.logRow(QStringLiteral("GPS"), QStringLiteral("Off"), QStringLiteral("1"),
                            QStringLiteral("12:00:00"));

    // Not a real MAV_RESULT value on the wire today, but resolveResult() must still produce
    // *some* readable text for a future/unrecognized code instead of silently leaving it pending.
    failureInjection.resolveResult(99);

    QCOMPARE(failureInjection.activity().first().toMap().value(QStringLiteral("result")).toString(),
             QStringLiteral("MAV_RESULT unknown 99"));
}

void FailureInjectionTest::_clearInjectedUnitsForgetsTrackedUnits()
{
    FailureInjection failureInjection;
    failureInjection.logInjection(QStringLiteral("GPS"), QStringLiteral("Off"), FAILURE_UNIT_SENSOR_GPS,
                                  QStringLiteral("1"), QStringLiteral("12:00:00"));
    QVERIFY(!failureInjection.injectedUnits().isEmpty());

    failureInjection.clearInjectedUnits();

    QVERIFY(failureInjection.injectedUnits().isEmpty());
    QCOMPARE(failureInjection.activity().count(), 1);  // the activity log itself is left intact
}

void FailureInjectionTest::_detailParamsMapCombos()
{
    FailureInjection failureInjection;

    // BATTERY + WRONG exposes the SYS_FAIL_BAT_LVL detail parameter.
    const QVariantList batteryWrong = failureInjection.detailParams(FAILURE_UNIT_SYSTEM_BATTERY, FAILURE_TYPE_WRONG);
    QCOMPARE(batteryWrong.count(), 1);
    QCOMPARE(batteryWrong.first().toMap().value(QStringLiteral("param")).toString(),
             QStringLiteral("SYS_FAIL_BAT_LVL"));
    QVERIFY2(!batteryWrong.first().toMap().value(QStringLiteral("label")).toString().isEmpty(),
             "detail param must have a display label");

    // Combos without detail parameters return an empty list.
    QVERIFY(failureInjection.detailParams(FAILURE_UNIT_SYSTEM_BATTERY, FAILURE_TYPE_OFF).isEmpty());
    QVERIFY(failureInjection.detailParams(FAILURE_UNIT_SENSOR_GPS, FAILURE_TYPE_WRONG).isEmpty());
}
