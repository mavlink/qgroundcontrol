#include "GPSSatelliteModelTest.h"

#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "GPSSatelliteModel.h"

void GPSSatelliteModelTest::_rolesAndUnknownValues()
{
    GPSSatelliteModel model;
    QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.beginSession(QStringLiteral("nativeReceiver"), 7);
    GPSSatellite satellite;
    satellite.id = 4;
    GPSSatelliteObservation report{GPSObservation::monotonicNowUs(), 7, {satellite}};
    model.updateObservation(report);
    QCOMPARE(model.count(), 1);
    QVERIFY(model.fresh());
    const QModelIndex row = model.index(0);
    QVERIFY(!model.data(row, GPSSatelliteModel::UsedRole).isValid());
    QVERIFY(!model.data(row, GPSSatelliteModel::ElevationRole).isValid());
    QVERIFY(!model.data(row, GPSSatelliteModel::SignalStrengthRole).isValid());
    QVERIFY(!model.data(row, GPSSatelliteModel::AzimuthRole).isValid());
    QVERIFY(!model.data(QModelIndex(), GPSSatelliteModel::SatelliteIdRole).isValid());
    QCOMPARE(model.rowCount(row), 0);
    QCOMPARE(model.data(row, GPSSatelliteModel::SourceIdRole).toString(), QStringLiteral("nativeReceiver"));
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy changeSpy(&model, &QAbstractItemModel::dataChanged);
    report.satellites[0].used = false;
    report.satellites[0].elevationDegrees = 0;
    report.satellites[0].signalStrength = 0;
    report.satellites[0].rawAzimuth = 255;
    report.satellites[0].azimuthEncoding = GPSSatellite::AzimuthEncoding::ScaledFullCircleByte;
    model.updateObservation(report);
    QCOMPARE(resetSpy.size(), 0);
    QCOMPARE(changeSpy.size(), 1);
    QCOMPARE(model.data(row, GPSSatelliteModel::UsedRole), QVariant(false));
    QCOMPARE(model.data(row, GPSSatelliteModel::ElevationRole), QVariant(0.0));
    QCOMPARE(model.data(row, GPSSatelliteModel::SignalStrengthRole), QVariant(0));
    QCOMPARE(model.data(row, GPSSatelliteModel::AzimuthRole), QVariant(0.0));
    report.satellites[0].azimuthEncoding = GPSSatellite::AzimuthEncoding::DegreesModulo256;
    model.updateObservation(report);
    QVERIFY(!model.data(row, GPSSatelliteModel::AzimuthRole).isValid());
}

void GPSSatelliteModelTest::_nmeaConstellationAndUsedIdentity()
{
    GPSSatelliteModel model;
    QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.beginSession(QStringLiteral("nmeaReceiver"), 12);
    QGeoSatelliteInfo gps;
    gps.setSatelliteIdentifier(3);
    gps.setSatelliteSystem(QGeoSatelliteInfo::GPS);
    gps.setAttribute(QGeoSatelliteInfo::Azimuth, 275.0);
    QGeoSatelliteInfo galileo = gps;
    galileo.setSatelliteSystem(QGeoSatelliteInfo::GALILEO);
    const quint64 receipt = GPSObservation::monotonicNowUs();
    model.updateNmeaSatellites({galileo, gps}, {gps}, true, receipt, 12);
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.data(model.index(0), GPSSatelliteModel::ConstellationRole).toString(), QStringLiteral("GPS"));
    QCOMPARE(model.data(model.index(0), GPSSatelliteModel::UsedRole), QVariant(true));
    QCOMPARE(model.data(model.index(1), GPSSatelliteModel::UsedRole), QVariant(false));
    QCOMPARE(model.data(model.index(1), GPSSatelliteModel::AzimuthRole), QVariant(275.0));
    model.updateNmeaSatellites({galileo, gps}, {gps}, true, receipt, 12, QSet<int>{QGeoSatelliteInfo::GPS});
    QCOMPARE(model.data(model.index(0), GPSSatelliteModel::UsedRole), QVariant(true));
    QVERIFY(!model.data(model.index(1), GPSSatelliteModel::UsedRole).isValid());
    model.updateNmeaSatellites({galileo, gps}, {}, true, receipt, 12, QSet<int>{QGeoSatelliteInfo::GALILEO});
    QVERIFY(!model.data(model.index(0), GPSSatelliteModel::UsedRole).isValid());
    QCOMPARE(model.data(model.index(1), GPSSatelliteModel::UsedRole), QVariant(false));
    model.updateNmeaSatellites({galileo, gps}, {gps}, false, receipt, 12);
    QVERIFY(!model.data(model.index(0), GPSSatelliteModel::UsedRole).isValid());
    QVERIFY(!model.data(model.index(1), GPSSatelliteModel::UsedRole).isValid());
    model.updateNmeaSatellites({}, {}, false, 0, 11);
    QVERIFY(model.fresh());
    model.updateNmeaSatellites({}, {}, false, 0, 12);
    QVERIFY(!model.fresh());
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.sessionId(), 12ULL);
    QCOMPARE(model.sourceId(), QStringLiteral("nmeaReceiver"));
}

void GPSSatelliteModelTest::_freshnessAndSessionIsolation()
{
    GPSSatelliteModel model(nullptr, 100);
    QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.beginSession(QStringLiteral("receiver"), 1);
    GPSSatellite satellite;
    satellite.id = 4;
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    GPSSatelliteObservation report{nowUs - 20000, 1, {satellite}};
    model.updateObservation(report);
    QVERIFY(model.fresh());
    report.satellites[0].id = 99;
    report.monotonicTimestampUs = nowUs - 30000;
    model.updateObservation(report);
    QCOMPARE(model.data(model.index(0), GPSSatelliteModel::SatelliteIdRole).toInt(), 4);
    report.monotonicTimestampUs = nowUs + 1000000;
    model.updateObservation(report);
    QCOMPARE(model.data(model.index(0), GPSSatelliteModel::SatelliteIdRole).toInt(), 4);
    QTRY_VERIFY_WITH_TIMEOUT(!model.fresh(), 1000);
    QCOMPARE(model.count(), 0);
    report.monotonicTimestampUs = nowUs - 200000;
    model.updateObservation(report);
    QVERIFY(!model.fresh());
    model.beginSession(QStringLiteral("receiver"), 2);
    report.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    model.updateObservation(report);
    QCOMPARE(model.count(), 0);
    report.sessionId = 2;
    model.updateObservation(report);
    QCOMPARE(model.count(), 1);
    model.reset();
    QVERIFY(model.sourceId().isEmpty());
    QCOMPARE(model.sessionId(), 0ULL);
    QCOMPARE(model.count(), 0);
}

void GPSSatelliteModelTest::_reentrantReset()
{
    GPSSatelliteModel model;
    QAbstractItemModelTester contract(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.beginSession(QStringLiteral("receiver"), 1);
    bool replaced = false;
    connect(&model, &QAbstractItemModel::modelAboutToBeReset, &model, [&]() {
        if (!replaced) {
            replaced = true;
            model.beginSession(QStringLiteral("replacement"), 2);
            GPSSatellite next;
            next.id = 8;
            model.updateObservation({GPSObservation::monotonicNowUs(), 2, {next}});
        }
    });
    GPSSatellite old;
    old.id = 3;
    model.updateObservation({GPSObservation::monotonicNowUs(), 1, {old}});
    QTRY_COMPARE_WITH_TIMEOUT(model.sessionId(), 2ULL, 1000);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0), GPSSatelliteModel::SatelliteIdRole).toInt(), 8);
}

UT_REGISTER_TEST(GPSSatelliteModelTest, TestLabel::Unit)
