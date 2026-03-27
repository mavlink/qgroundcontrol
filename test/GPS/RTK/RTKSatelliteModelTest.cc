#include "RTKSatelliteModelTest.h"
#include "SatelliteModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void RTKSatelliteModelTest::testEmptyModel()
{
    SatelliteModel model;
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.usedCount(), 0);
    QVERIFY(model.constellationSummary().isEmpty());
}

void RTKSatelliteModelTest::testUpdatePopulatesModel()
{
    SatelliteModel model;
    QSignalSpy spy(&model, &SatelliteModel::dataChanged);

    satellite_info_s info{};
    info.count = 3;
    info.svid[0] = 1;  info.used[0] = 1; info.elevation[0] = 45; info.azimuth[0] = 90;  info.snr[0] = 30;
    info.svid[1] = 5;  info.used[1] = 0; info.elevation[1] = 20; info.azimuth[1] = 180; info.snr[1] = 15;
    info.svid[2] = 10; info.used[2] = 1; info.elevation[2] = 60; info.azimuth[2] = 270; info.snr[2] = 40;

    model.updateFromDriverInfo(info);

    QCOMPARE(model.count(), 3);
    QVERIFY(spy.count() > 0);

    QModelIndex idx0 = model.index(0);
    QCOMPARE(model.data(idx0, SatelliteModel::SvidRole).toInt(), 1);
    QCOMPARE(model.data(idx0, SatelliteModel::UsedRole).toBool(), true);
    QCOMPARE(model.data(idx0, SatelliteModel::ElevationRole).toInt(), 45);
    QCOMPARE(model.data(idx0, SatelliteModel::AzimuthRole).toInt(), 90);
    QCOMPARE(model.data(idx0, SatelliteModel::SnrRole).toInt(), 30);

    QModelIndex idx1 = model.index(1);
    QCOMPARE(model.data(idx1, SatelliteModel::UsedRole).toBool(), false);
}

void RTKSatelliteModelTest::testClear()
{
    SatelliteModel model;

    satellite_info_s info{};
    info.count = 2;
    info.svid[0] = 1; info.used[0] = 1; info.elevation[0] = 30; info.azimuth[0] = 45; info.snr[0] = 25;
    info.svid[1] = 2; info.used[1] = 1; info.elevation[1] = 50; info.azimuth[1] = 90; info.snr[1] = 35;
    model.updateFromDriverInfo(info);
    QCOMPARE(model.count(), 2);

    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.usedCount(), 0);
}

void RTKSatelliteModelTest::testRoleNames()
{
    SatelliteModel model;
    auto roles = model.roleNames();

    QVERIFY(roles.contains(SatelliteModel::SvidRole));
    QVERIFY(roles.contains(SatelliteModel::UsedRole));
    QVERIFY(roles.contains(SatelliteModel::ElevationRole));
    QVERIFY(roles.contains(SatelliteModel::AzimuthRole));
    QVERIFY(roles.contains(SatelliteModel::SnrRole));
    QVERIFY(roles.contains(SatelliteModel::ConstellationRole));
}

void RTKSatelliteModelTest::testUsedCount()
{
    SatelliteModel model;

    satellite_info_s info{};
    info.count = 5;
    for (int i = 0; i < 5; ++i) {
        info.svid[i] = static_cast<uint8_t>(i + 1);
        info.used[i] = (i % 2 == 0) ? 1 : 0;
        info.elevation[i] = 30;
        info.azimuth[i] = static_cast<uint16_t>(60 * i);
        info.snr[i] = 25;
    }

    model.updateFromDriverInfo(info);
    QCOMPARE(model.usedCount(), 3);
}

void RTKSatelliteModelTest::testGrowModel()
{
    SatelliteModel model;

    satellite_info_s info{};
    info.count = 2;
    info.svid[0] = 1; info.used[0] = 1; info.snr[0] = 30;
    info.svid[1] = 2; info.used[1] = 1; info.snr[1] = 25;
    model.updateFromDriverInfo(info);
    QCOMPARE(model.count(), 2);

    // Grow from 2 to 4
    satellite_info_s info2{};
    info2.count = 4;
    for (int i = 0; i < 4; ++i) {
        info2.svid[i] = static_cast<uint8_t>(i + 1);
        info2.used[i] = 1;
        info2.snr[i] = 20;
    }
    model.updateFromDriverInfo(info2);
    QCOMPARE(model.count(), 4);
    QCOMPARE(model.usedCount(), 4);
}

void RTKSatelliteModelTest::testShrinkModel()
{
    SatelliteModel model;

    satellite_info_s info{};
    info.count = 5;
    for (int i = 0; i < 5; ++i) {
        info.svid[i] = static_cast<uint8_t>(i + 1);
        info.used[i] = 1;
        info.snr[i] = 30;
    }
    model.updateFromDriverInfo(info);
    QCOMPARE(model.count(), 5);

    // Shrink from 5 to 2
    satellite_info_s info2{};
    info2.count = 2;
    info2.svid[0] = 1; info2.used[0] = 1; info2.snr[0] = 30;
    info2.svid[1] = 2; info2.used[1] = 0; info2.snr[1] = 10;
    model.updateFromDriverInfo(info2);
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.usedCount(), 1);
}

void RTKSatelliteModelTest::testSameSizeUpdate()
{
    SatelliteModel model;
    QSignalSpy spy(&model, &SatelliteModel::dataChanged);

    satellite_info_s info{};
    info.count = 3;
    for (int i = 0; i < 3; ++i) {
        info.svid[i] = static_cast<uint8_t>(i + 1);
        info.used[i] = 1;
        info.snr[i] = 20;
    }
    model.updateFromDriverInfo(info);
    QCOMPARE(spy.count(), 1);

    // Same count, different data
    info.snr[0] = 40;
    info.used[1] = 0;
    model.updateFromDriverInfo(info);
    QCOMPARE(model.count(), 3);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(model.data(model.index(0), SatelliteModel::SnrRole).toInt(), 40);
    QCOMPARE(model.data(model.index(1), SatelliteModel::UsedRole).toBool(), false);
}

void RTKSatelliteModelTest::testConstellationSummaryContent()
{
    SatelliteModel model;

    satellite_info_s info{};
    info.count = 3;
    // GPS SVIDs: 1-32
    info.svid[0] = 1;  info.used[0] = 1; info.snr[0] = 30;
    info.svid[1] = 10; info.used[1] = 1; info.snr[1] = 25;
    // GLONASS SVIDs: 65-96
    info.svid[2] = 65; info.used[2] = 1; info.snr[2] = 20;
    model.updateFromDriverInfo(info);

    QVERIFY(model.constellationSummary().contains("GPS:2"));
    QVERIFY(model.constellationSummary().contains("GLO:1"));
}

UT_REGISTER_TEST(RTKSatelliteModelTest, TestLabel::Unit)
