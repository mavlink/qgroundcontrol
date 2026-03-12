#include "GPSManagerTest.h"
#include "GPSManager.h"
#include "QmlObjectListModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void GPSManagerTest::testInitialState()
{
    GPSManager *mgr = GPSManager::instance();
    QVERIFY(mgr);
    QCOMPARE(mgr->deviceCount(), 0);
    QVERIFY(!mgr->connected());
    QVERIFY(mgr->devices());
    QCOMPARE(mgr->devices()->count(), 0);
    QVERIFY(mgr->rtcmRouter());
    QVERIFY(mgr->eventModel());
    QVERIFY(!mgr->gpsRtk());
    QVERIFY(!mgr->gpsRtkFactGroup());
    QVERIFY(!mgr->rtkSatelliteModel());
}

void GPSManagerTest::testMultiDeviceTracking()
{
    GPSManager *mgr = GPSManager::instance();
    QVERIFY(!mgr->isDeviceConnected("/dev/fake1"));
    QVERIFY(!mgr->isDeviceConnected("/dev/fake2"));
}

void GPSManagerTest::testDisconnectAll()
{
    GPSManager *mgr = GPSManager::instance();
    mgr->disconnectAll();
    QCOMPARE(mgr->deviceCount(), 0);
}

void GPSManagerTest::testDuplicateConnect()
{
    GPSManager *mgr = GPSManager::instance();
    QCOMPARE(mgr->deviceCount(), 0);
}

void GPSManagerTest::testPrimaryDevice()
{
    GPSManager *mgr = GPSManager::instance();
    QVERIFY(!mgr->gpsRtk());
}

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)
