#include "GPSManagerTest.h"
#include "GPSManager.h"
#include "GPSEvent.h"
#include "GPSEventModel.h"
#include "QmlObjectListModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void GPSManagerTest::cleanup()
{
    GPSManager::instance()->disconnectAll();
    UnitTest::cleanup();
}

void GPSManagerTest::testInitialState()
{
    GPSManager *mgr = GPSManager::instance();
    QVERIFY(mgr);
    QCOMPARE(mgr->deviceCount(), 0);
    QVERIFY(!mgr->connected());
    QVERIFY(mgr->devices());
    QCOMPARE(mgr->devices()->count(), 0);
    QVERIFY(mgr->rtcmMavlink());
    QVERIFY(mgr->eventModel());
    QVERIFY(!mgr->gpsRtk());
    QVERIFY(mgr->gpsRtkFactGroup());
    QVERIFY(!mgr->rtkSatelliteModel());
}

void GPSManagerTest::testMultiDeviceTracking()
{
    GPSManager *mgr = GPSManager::instance();
    QVERIFY(!mgr->isDeviceRegistered("/dev/fake1"));
    QVERIFY(!mgr->isDeviceRegistered("/dev/fake2"));
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

void GPSManagerTest::testConnectGPSRegistersDevice()
{
    GPSManager *mgr = GPSManager::instance();
    QSignalSpy connSpy(mgr, &GPSManager::deviceConnected);
    QSignalSpy countSpy(mgr, &GPSManager::deviceCountChanged);

    mgr->connectGPS(QStringLiteral("/dev/fakegps_reg"), QStringLiteral("u-blox"));

    QVERIFY(mgr->isDeviceRegistered(QStringLiteral("/dev/fakegps_reg")));
    QCOMPARE(mgr->deviceCount(), 1);
    QCOMPARE(mgr->devices()->count(), 1);
    QCOMPARE(connSpy.count(), 1);
    QCOMPARE(connSpy.first().first().toString(), QStringLiteral("/dev/fakegps_reg"));
    QCOMPARE(countSpy.count(), 1);
    QVERIFY(mgr->gpsRtk(QStringLiteral("/dev/fakegps_reg")));
}

void GPSManagerTest::testDisconnectGPSRemovesDevice()
{
    GPSManager *mgr = GPSManager::instance();
    mgr->connectGPS(QStringLiteral("/dev/fakegps_disc"), QStringLiteral("u-blox"));
    QCOMPARE(mgr->deviceCount(), 1);

    QSignalSpy discSpy(mgr, &GPSManager::deviceDisconnected);
    QSignalSpy countSpy(mgr, &GPSManager::deviceCountChanged);

    mgr->disconnectGPS(QStringLiteral("/dev/fakegps_disc"));

    QVERIFY(!mgr->isDeviceRegistered(QStringLiteral("/dev/fakegps_disc")));
    QCOMPARE(mgr->deviceCount(), 0);
    QCOMPARE(mgr->devices()->count(), 0);
    QCOMPARE(discSpy.count(), 1);
    QCOMPARE(discSpy.first().first().toString(), QStringLiteral("/dev/fakegps_disc"));
    QCOMPARE(countSpy.count(), 1);
    QVERIFY(!mgr->gpsRtk(QStringLiteral("/dev/fakegps_disc")));
}

void GPSManagerTest::testDisconnectAllMultiple()
{
    GPSManager *mgr = GPSManager::instance();
    mgr->connectGPS(QStringLiteral("/dev/fakegps_a"), QStringLiteral("u-blox"));
    mgr->connectGPS(QStringLiteral("/dev/fakegps_b"), QStringLiteral("u-blox"));
    mgr->connectGPS(QStringLiteral("/dev/fakegps_c"), QStringLiteral("u-blox"));
    QCOMPARE(mgr->deviceCount(), 3);

    QSignalSpy discSpy(mgr, &GPSManager::deviceDisconnected);

    mgr->disconnectAll();

    QCOMPARE(mgr->deviceCount(), 0);
    QCOMPARE(mgr->devices()->count(), 0);
    QVERIFY(!mgr->isDeviceRegistered(QStringLiteral("/dev/fakegps_a")));
    QVERIFY(!mgr->isDeviceRegistered(QStringLiteral("/dev/fakegps_b")));
    QVERIFY(!mgr->isDeviceRegistered(QStringLiteral("/dev/fakegps_c")));
    QCOMPARE(discSpy.count(), 3);
}

void GPSManagerTest::testGpsRtkFactGroupReturnsStaticDefault()
{
    GPSManager *mgr = GPSManager::instance();
    QCOMPARE(mgr->deviceCount(), 0);

    FactGroup *fg = mgr->gpsRtkFactGroup();
    QVERIFY(fg);

    FactGroup *fg2 = mgr->gpsRtkFactGroup();
    QCOMPARE(fg, fg2);
}

void GPSManagerTest::testLogEvent()
{
    GPSManager *mgr = GPSManager::instance();
    const int before = mgr->eventModel()->count();

    QSignalSpy logSpy(mgr, &GPSManager::eventLogged);
    QVERIFY(logSpy.isValid());

    mgr->logEvent(GPSEvent::info(GPSEvent::Source::GPS, QStringLiteral("unit test event")));

    QCOMPARE(mgr->eventModel()->count(), before + 1);
    QCOMPARE(logSpy.count(), 1);

    mgr->logEvent(GPSEvent::warning(GPSEvent::Source::RTKBase, QStringLiteral("second event")));
    mgr->logEvent(GPSEvent::error(GPSEvent::Source::NTRIP, QStringLiteral("third event")));

    QCOMPARE(mgr->eventModel()->count(), before + 3);
    QCOMPARE(logSpy.count(), 3);
}

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)
