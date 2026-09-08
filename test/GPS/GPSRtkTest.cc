#include "GPSRtkTest.h"

#include <QtCore/QFile>

#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "QGroundControlQmlGlobal.h"

void GPSRtkTest::_testCountSatellitesClampsToMax()
{
    satellite_info_s msg{};
    msg.count = 250;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(static_cast<int>(counts.inView), static_cast<int>(satellite_info_s::SAT_INFO_MAX_SATELLITES));
    QCOMPARE(counts.used, 0);
}

void GPSRtkTest::_testCountSatellitesCountsUsed()
{
    satellite_info_s msg{};
    msg.count = 6;
    msg.used[1] = 1;
    msg.used[3] = 1;
    msg.used[5] = 1;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(static_cast<int>(counts.inView), 6);
    QCOMPARE(counts.used, 3);
}

void GPSRtkTest::_testCountSatellitesIgnoresUsedBeyondCount()
{
    satellite_info_s msg{};
    msg.count = 2;
    msg.used[0] = 1;
    msg.used[5] = 1;

    const GPSRtk::SatelliteCounts counts = GPSRtk::countSatellites(msg);

    QCOMPARE(static_cast<int>(counts.inView), 2);
    QCOMPARE(counts.used, 1);
}

UT_REGISTER_TEST(GPSRtkTest, TestLabel::Unit)

void GPSRtkTest::_testCoreAvailableWithoutReceiver()
{
    GPSRtk rtk;
    QVERIFY(!rtk.connected());
    auto* facts = qobject_cast<GPSRTKFactGroup*>(rtk.gpsRtkFactGroup());
    QVERIFY(facts);
    QVERIFY(!facts->connected()->rawValue().toBool());
    QVERIFY(QFile::exists(QStringLiteral(":/json/Vehicle/GPSRTKFact.json")));
    QVERIFY(QGroundControlQmlGlobal::staticMetaObject.indexOfProperty("gpsRtk") >= 0);
}
