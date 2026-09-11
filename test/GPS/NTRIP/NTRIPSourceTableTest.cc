#include "NTRIPSourceTableTest.h"

#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "NTRIPSourceTable.h"

void NTRIPSourceTableTest::_testParseSTRLine()
{
    const QString line =
        "STR;MOUNT01;Mount Identifier;RTCM 3.2;"
        "1005(1),1074(1),1084(1),1094(1);2;GPS+GLO+GAL;NET01;USA;40.0000;-74.0000;0;1;QGC Test;none;B;N;4800;misc";
    NTRIPMountpoint mp;
    QVERIFY(NTRIPMountpoint::fromSourceTableLine(line, mp));

    QCOMPARE(mp.mountpoint, QStringLiteral("MOUNT01"));
    QCOMPARE(mp.identifier, QStringLiteral("Mount Identifier"));
    QCOMPARE(mp.format, QStringLiteral("RTCM 3.2"));
    QCOMPARE(mp.carrier, 2);
    QCOMPARE(mp.navSystem, QStringLiteral("GPS+GLO+GAL"));
    QCOMPARE(mp.network, QStringLiteral("NET01"));
    QCOMPARE(mp.country, QStringLiteral("USA"));
    QVERIFY(qFuzzyCompare(mp.coordinate.latitude(), 40.0));
    QVERIFY(qFuzzyCompare(mp.coordinate.longitude(), -74.0));
    QVERIFY(!mp.nmea);
    QVERIFY(mp.solution);
    QCOMPARE(mp.generator, QStringLiteral("QGC Test"));
    QCOMPARE(mp.authentication, QStringLiteral("B"));
    QVERIFY(!mp.fee);
    QCOMPARE(mp.bitrate, 4800);
}

void NTRIPSourceTableTest::_testParseShortLine()
{
    const QString line = "STR;SHORT;Too;Few;Fields";
    NTRIPMountpoint mp;
    QVERIFY(!NTRIPMountpoint::fromSourceTableLine(line, mp));
}

void NTRIPSourceTableTest::_testParseNonSTRLine()
{
    const QString line = "CAS;caster.example.com;2101;NTRIP Caster;Operator;0;USA;0.0;0.0;0.0;0.0;0;0;misc";
    NTRIPMountpoint mp;
    QVERIFY(!NTRIPMountpoint::fromSourceTableLine(line, mp));
}

void NTRIPSourceTableTest::_testParseFullTable()
{
    const QString table =
        "SOURCETABLE 200 OK\r\n"
        "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
        "STR;MP2;Id2;RTCM 3.3;details;0;GLO;NET;DEU;52.0;13.0;1;0;gen;none;N;Y;9600;misc\r\n"
        "CAS;caster.example.com;2101;desc;op;0;USA;0;0;0;0;0;0;misc\r\n"
        "ENDSOURCETABLE\r\n";

    NTRIPSourceTableModel model;
    model.parseSourceTable(table);

    QCOMPARE(model.count(), 2);
    QCOMPARE(model.rowCount(), 2);
}

void NTRIPSourceTableTest::_testDistanceCalculation()
{
    const QString line = "STR;NEAR;Id;RTCM 3.2;details;2;GPS;NET;USA;40.7128;-74.0060;0;1;gen;none;B;N;4800;misc";
    NTRIPMountpoint mp;
    QVERIFY(NTRIPMountpoint::fromSourceTableLine(line, mp));

    QCOMPARE(mp.distanceFrom({}), -1.0);

    QGeoCoordinate userPos(40.7128, -74.0060);
    QVERIFY(mp.distanceFrom(userPos) >= 0.0);
    QVERIFY(mp.distanceFrom(userPos) < 1.0);

    QGeoCoordinate farPos(51.5074, -0.1278);
    QVERIFY(mp.distanceFrom(farPos) > 5000.0);
}

void NTRIPSourceTableTest::_testUpdateDistancesAll()
{
    const QString table =
        "STR;NYC;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.7128;-74.0060;0;1;gen;none;B;N;4800;misc\r\n"
        "STR;LON;Id2;RTCM 3.3;details;0;GPS;NET;GBR;51.5074;-0.1278;1;0;gen;none;N;Y;9600;misc\r\n"
        "ENDSOURCETABLE\r\n";

    NTRIPSourceTableModel model;
    model.parseSourceTable(table);
    QCOMPARE(model.count(), 2);

    const auto distanceAt = [&model](int row) {
        return model.data(model.index(row), NTRIPSourceTableModel::DistanceKmRole).toDouble();
    };

    QVERIFY(distanceAt(0) < 0.0);
    QVERIFY(distanceAt(1) < 0.0);

    QGeoCoordinate nycPos(40.7128, -74.0060);
    model.updateDistances(nycPos);

    // After sortByDistance, the NYC mount is first (closest), London last.
    QVERIFY(distanceAt(0) >= 0.0);
    QVERIFY(distanceAt(0) < 1.0);
    QVERIFY(distanceAt(1) > 5000.0);
}

void NTRIPSourceTableTest::_testEmptyTable()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable("");
    QCOMPARE(model.count(), 0);

    model.parseSourceTable("ENDSOURCETABLE\r\n");
    QCOMPARE(model.count(), 0);
}

UT_REGISTER_TEST(NTRIPSourceTableTest, TestLabel::Unit)

void NTRIPSourceTableTest::_testCoordinates_data()
{
    QTest::addColumn<QString>("latitude");
    QTest::addColumn<QString>("longitude");
    QTest::addColumn<bool>("valid");
    QTest::newRow("zero") << QStringLiteral("0") << QStringLiteral("0") << true;
    QTest::newRow("equator") << QStringLiteral("0") << QStringLiteral("13") << true;
    QTest::newRow("prime-meridian") << QStringLiteral("51") << QStringLiteral("0") << true;
    QTest::newRow("missing-lat") << QString() << QStringLiteral("13") << false;
    QTest::newRow("missing-lon") << QStringLiteral("51") << QString() << false;
    QTest::newRow("nan") << QStringLiteral("nan") << QStringLiteral("13") << false;
    QTest::newRow("infinity") << QStringLiteral("51") << QStringLiteral("inf") << false;
    QTest::newRow("latitude-range") << QStringLiteral("91") << QStringLiteral("13") << false;
    QTest::newRow("longitude-range") << QStringLiteral("51") << QStringLiteral("181") << false;
    QTest::newRow("garbage") << QStringLiteral("wrong") << QStringLiteral("13") << false;
}

void NTRIPSourceTableTest::_testCoordinates()
{
    QFETCH(QString, latitude);
    QFETCH(QString, longitude);
    QFETCH(bool, valid);
    const QString row =
        QStringLiteral("STR;MP;Id;RTCM;details;2;GPS;NET;USA;%1;%2;0;1;gen;none;B;N;4800").arg(latitude, longitude);
    NTRIPSourceTableModel model;
    model.parseSourceTable(row);
    const auto index = model.index(0);
    QCOMPARE(model.data(index, NTRIPSourceTableModel::LatitudeRole).isValid(), valid);
    QCOMPARE(model.data(index, NTRIPSourceTableModel::LongitudeRole).isValid(), valid);
    model.updateDistances(QGeoCoordinate(0, 0));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DistanceKmRole).toDouble() >= 0, valid);
    model.updateDistances({});
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DistanceKmRole).toDouble(), -1.0);
}

void NTRIPSourceTableTest::_testProjectionNotifications()
{
    NTRIPSourceTableModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    const QString first = QStringLiteral("STR;A;Id;RTCM;details;2;GPS;NET;USA;0;0;0;1;gen;none;B;N;4800\n");
    const QString second = QStringLiteral("STR;B;Id;RTCM;details;2;GPS;NET;USA;0;10;0;1;gen;none;B;N;4800\n");
    model.parseSourceTable(first);
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
    QSignalSpy count(&model, &NTRIPSourceTableModel::countChanged);
    model.updateDistances(QGeoCoordinate(0, 0));
    QCOMPARE(changed.size(), 1);
    QCOMPARE(qvariant_cast<QList<int>>(changed.first().at(2)), QList<int>{NTRIPSourceTableModel::DistanceKmRole});
    QCOMPARE(reset.size(), 0);
    QCOMPARE(count.size(), 0);
    model.updateDistances(QGeoCoordinate(0, 0));
    QCOMPARE(changed.size(), 1);
    model.updateDistances({});
    QCOMPARE(changed.size(), 2);
    model.parseSourceTable(first + second);
    reset.clear();
    count.clear();
    model.updateDistances(QGeoCoordinate(0, 10));
    QCOMPARE(reset.size(), 1);
    QCOMPARE(count.size(), 0);
    QCOMPARE(model.data(model.index(0), NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("B"));
    model.updateDistances({});
    QCOMPARE(reset.size(), 2);
    QCOMPARE(model.data(model.index(0), NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("A"));
    QCOMPARE(model.data(model.index(0), NTRIPSourceTableModel::DistanceKmRole).toDouble(), -1.0);
}

void NTRIPSourceTableTest::_testReentrantPublication_data()
{
    QTest::addColumn<bool>("replace");
    QTest::addColumn<bool>("distanceReset");
    QTest::newRow("clear-during-parse") << false << false;
    QTest::newRow("replace-during-parse") << true << false;
    QTest::newRow("clear-during-distance") << false << true;
    QTest::newRow("replace-during-distance") << true << true;
}

void NTRIPSourceTableTest::_testReentrantPublication()
{
    QFETCH(bool, replace);
    QFETCH(bool, distanceReset);
    NTRIPSourceTableModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    const QString first = QStringLiteral("STR;A;Id;RTCM;details;2;GPS;NET;USA;0;0;0;1;gen;none;B;N;4800\n");
    const QString second = QStringLiteral("STR;B;Id;RTCM;details;2;GPS;NET;USA;0;10;0;1;gen;none;B;N;4800\n");
    if (distanceReset) {
        model.parseSourceTable(first + second);
    }
    bool triggered = false;
    bool resetting = false;
    connect(&model, &QAbstractItemModel::modelAboutToBeReset, &model, [&]() {
        QVERIFY(!resetting);
        resetting = true;
        if (triggered) {
            return;
        }
        triggered = true;
        model.clear();
        if (replace) {
            model.parseSourceTable(second);
            model.updateDistances(QGeoCoordinate(0, 10));
        }
    });
    connect(&model, &QAbstractItemModel::modelReset, &model, [&]() { resetting = false; });
    if (distanceReset) {
        model.updateDistances(QGeoCoordinate(0, 10));
    } else {
        model.parseSourceTable(first + second);
    }
    QVERIFY(triggered);
    QTRY_COMPARE_WITH_TIMEOUT(model.count(), replace ? 1 : 0, TestTimeout::shortMs());
    QVERIFY(!resetting);
    if (replace) {
        QCOMPARE(model.data(model.index(0), NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("B"));
        QCOMPARE(model.data(model.index(0), NTRIPSourceTableModel::DistanceKmRole).toDouble(), 0.0);
    }
}

void NTRIPSourceTableTest::_testDestructionDuringReset_data()
{
    QTest::addColumn<bool>("distanceReset");
    QTest::addColumn<bool>("beforeReset");
    QTest::newRow("parse-about-to-reset") << false << true;
    QTest::newRow("parse-reset") << false << false;
    QTest::newRow("distance-about-to-reset") << true << true;
    QTest::newRow("distance-reset") << true << false;
}

void NTRIPSourceTableTest::_testDestructionDuringReset()
{
    QFETCH(bool, distanceReset);
    QFETCH(bool, beforeReset);
    QPointer<NTRIPSourceTableModel> model = new NTRIPSourceTableModel;
    new QAbstractItemModelTester(model, QAbstractItemModelTester::FailureReportingMode::QtTest, model);
    const QString table = QStringLiteral(
        "STR;A;Id;RTCM;details;2;GPS;NET;USA;0;0;0;1;gen;none;B;N;4800\n"
        "STR;B;Id;RTCM;details;2;GPS;NET;USA;0;10;0;1;gen;none;B;N;4800\n");
    if (distanceReset) {
        model->parseSourceTable(table);
    }
    const auto signal = beforeReset ? &QAbstractItemModel::modelAboutToBeReset : &QAbstractItemModel::modelReset;
    connect(model, signal, this, [&]() { delete model.data(); });
    if (distanceReset) {
        model->updateDistances(QGeoCoordinate(0, 10));
    } else {
        model->parseSourceTable(table);
    }
    QVERIFY(model.isNull());
}
