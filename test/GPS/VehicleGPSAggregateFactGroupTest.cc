#include "VehicleGPSAggregateFactGroupTest.h"
#include "UnitTest.h"
#include "VehicleGPSAggregateFactGroup.h"
#include "VehicleGPSFactGroup.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void VehicleGPSAggregateFactGroupTest::testInitialValues()
{
    VehicleGPSAggregateFactGroup agg;

    QCOMPARE(agg.spoofingState()->rawValue().toInt(), 255);
    QCOMPARE(agg.jammingState()->rawValue().toInt(), 255);
    QCOMPARE(agg.authenticationState()->rawValue().toInt(), 255);
    QCOMPARE(agg.isStale()->rawValue().toBool(), true);
}

void VehicleGPSAggregateFactGroupTest::testDualGpsMerge()
{
    VehicleGPSAggregateFactGroup agg;
    VehicleGPSFactGroup gps1;
    VehicleGPSFactGroup gps2;

    // GPS1: low threat, GPS2: high threat — merged should be worst-case
    gps1.spoofingState()->setRawValue(1);  // OK
    gps1.jammingState()->setRawValue(1);   // OK
    gps2.spoofingState()->setRawValue(3);  // Detected
    gps2.jammingState()->setRawValue(2);   // Mitigated

    agg.updateFromGps(&gps1, &gps2);

    QCOMPARE(agg.spoofingState()->rawValue().toInt(), 3);  // worst
    QCOMPARE(agg.jammingState()->rawValue().toInt(), 2);   // worst
}

void VehicleGPSAggregateFactGroupTest::testSingleGps()
{
    VehicleGPSAggregateFactGroup agg;
    VehicleGPSFactGroup gps1;

    gps1.spoofingState()->setRawValue(2);
    gps1.jammingState()->setRawValue(1);

    agg.updateFromGps(&gps1, nullptr);

    QCOMPARE(agg.spoofingState()->rawValue().toInt(), 2);
    QCOMPARE(agg.jammingState()->rawValue().toInt(), 1);
}

void VehicleGPSAggregateFactGroupTest::testAuthPriority()
{
    VehicleGPSAggregateFactGroup agg;
    VehicleGPSFactGroup gps1;
    VehicleGPSFactGroup gps2;

    // AUTH_OK=3 vs AUTH_ERROR=2 — error has higher weight, should win
    gps1.authenticationState()->setRawValue(3); // OK
    gps2.authenticationState()->setRawValue(2); // Error

    agg.updateFromGps(&gps1, &gps2);

    // Error (weight 4) > OK (weight 3), so error wins
    QCOMPARE(agg.authenticationState()->rawValue().toInt(), 2);
}

void VehicleGPSAggregateFactGroupTest::testStaleTimeout()
{
    VehicleGPSAggregateFactGroup agg;
    VehicleGPSFactGroup gps1;

    agg.bindToGps(&gps1, nullptr);

    // Simulate integrity update
    gps1.spoofingState()->setRawValue(1);
    emit gps1.gnssIntegrityReceived();

    QCOMPARE(agg.isStale()->rawValue().toBool(), false);
    QCOMPARE(agg.spoofingState()->rawValue().toInt(), 1);

    // Wait for stale timeout (5s + margin)
    QTest::qWait(5500);

    QCOMPARE(agg.isStale()->rawValue().toBool(), true);
    QCOMPARE(agg.spoofingState()->rawValue().toInt(), 255);
}

void VehicleGPSAggregateFactGroupTest::testRebind()
{
    VehicleGPSAggregateFactGroup agg;
    VehicleGPSFactGroup gps1;
    VehicleGPSFactGroup gps2;

    agg.bindToGps(&gps1, nullptr);

    gps1.spoofingState()->setRawValue(2);
    emit gps1.gnssIntegrityReceived();
    QCOMPARE(agg.spoofingState()->rawValue().toInt(), 2);

    // Rebind to different GPS
    agg.bindToGps(&gps2, nullptr);

    gps2.spoofingState()->setRawValue(1);
    emit gps2.gnssIntegrityReceived();
    QCOMPARE(agg.spoofingState()->rawValue().toInt(), 1);
}

UT_REGISTER_TEST(VehicleGPSAggregateFactGroupTest, TestLabel::Unit)
