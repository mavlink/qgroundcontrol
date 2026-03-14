#include "RTKSatelliteModelTest.h"
#include "RTKSatelliteModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void RTKSatelliteModelTest::testEmptyModel()
{
    RTKSatelliteModel model;
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.usedCount(), 0);
    QVERIFY(model.constellationSummary().isEmpty());
}

void RTKSatelliteModelTest::testUpdatePopulatesModel()
{
    RTKSatelliteModel model;
    QSignalSpy spy(&model, &RTKSatelliteModel::satelliteDataChanged);

    satellite_info_s info{};
    info.count = 3;
    info.svid[0] = 1;  info.used[0] = 1; info.elevation[0] = 45; info.azimuth[0] = 90;  info.snr[0] = 30;
    info.svid[1] = 5;  info.used[1] = 0; info.elevation[1] = 20; info.azimuth[1] = 180; info.snr[1] = 15;
    info.svid[2] = 10; info.used[2] = 1; info.elevation[2] = 60; info.azimuth[2] = 270; info.snr[2] = 40;

    model.update(info);

    QCOMPARE(model.count(), 3);
    QVERIFY(spy.count() > 0);

    QModelIndex idx0 = model.index(0);
    QCOMPARE(model.data(idx0, RTKSatelliteModel::SvidRole).toInt(), 1);
    QCOMPARE(model.data(idx0, RTKSatelliteModel::UsedRole).toBool(), true);
    QCOMPARE(model.data(idx0, RTKSatelliteModel::ElevationRole).toInt(), 45);
    QCOMPARE(model.data(idx0, RTKSatelliteModel::AzimuthRole).toInt(), 90);
    QCOMPARE(model.data(idx0, RTKSatelliteModel::SnrRole).toInt(), 30);

    QModelIndex idx1 = model.index(1);
    QCOMPARE(model.data(idx1, RTKSatelliteModel::UsedRole).toBool(), false);
}

void RTKSatelliteModelTest::testClear()
{
    RTKSatelliteModel model;

    satellite_info_s info{};
    info.count = 2;
    info.svid[0] = 1; info.used[0] = 1; info.elevation[0] = 30; info.azimuth[0] = 45; info.snr[0] = 25;
    info.svid[1] = 2; info.used[1] = 1; info.elevation[1] = 50; info.azimuth[1] = 90; info.snr[1] = 35;
    model.update(info);
    QCOMPARE(model.count(), 2);

    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.usedCount(), 0);
}

void RTKSatelliteModelTest::testRoleNames()
{
    RTKSatelliteModel model;
    auto roles = model.roleNames();

    QVERIFY(roles.contains(RTKSatelliteModel::SvidRole));
    QVERIFY(roles.contains(RTKSatelliteModel::UsedRole));
    QVERIFY(roles.contains(RTKSatelliteModel::ElevationRole));
    QVERIFY(roles.contains(RTKSatelliteModel::AzimuthRole));
    QVERIFY(roles.contains(RTKSatelliteModel::SnrRole));
    QVERIFY(roles.contains(RTKSatelliteModel::ConstellationRole));
}

void RTKSatelliteModelTest::testUsedCount()
{
    RTKSatelliteModel model;

    satellite_info_s info{};
    info.count = 5;
    for (int i = 0; i < 5; ++i) {
        info.svid[i] = static_cast<uint8_t>(i + 1);
        info.used[i] = (i % 2 == 0) ? 1 : 0;
        info.elevation[i] = 30;
        info.azimuth[i] = static_cast<uint16_t>(60 * i);
        info.snr[i] = 25;
    }

    model.update(info);
    QCOMPARE(model.usedCount(), 3);
}

UT_REGISTER_TEST(RTKSatelliteModelTest, TestLabel::Unit)
