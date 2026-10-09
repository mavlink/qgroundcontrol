#include "NTRIPSourceTableTest.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "NTRIPSourceTable.h"

void NTRIPSourceTableTest::_parseSTRLine()
{
    const QString line =
        "STR;MOUNT01;Mount Identifier;RTCM 3.2;"
        "1005(1),1074(1),1084(1),1094(1);2;GPS+GLO+GAL;NET01;USA;40.0000;-74.0000;0;1;QGC Test;none;B;N;4800;misc";
    const auto parsed = NTRIPMountpoint::fromSourceTableLine(line);
    QVERIFY(parsed);
    const NTRIPMountpoint& mp = *parsed;

    QCOMPARE(mp.mountpoint, QStringLiteral("MOUNT01"));
    QCOMPARE(mp.format, QStringLiteral("RTCM 3.2"));
    QCOMPARE(mp.navSystem, QStringLiteral("GPS+GLO+GAL"));
    QCOMPARE(mp.country, QStringLiteral("USA"));
    QVERIFY(qFuzzyCompare(mp.latitude, 40.0));
    QVERIFY(qFuzzyCompare(mp.longitude, -74.0));
    QCOMPARE(mp.bitrate, 4800);
}

void NTRIPSourceTableTest::_parseFullTable()
{
    const QString table =
        "SOURCETABLE 200 OK\r\n"
        "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
        "STR;MP2;Id2;RTCM 3.3;details;0;GLO;NET;DEU;52.0;13.0;1;0;gen;none;N;Y;9600;misc\r\n"
        "STR;SHORT;Too;Few;Fields\r\n"
        "CAS;caster.example.com;2101;desc;op;0;USA;0;0;0;0;0;0;misc\r\n"
        "ENDSOURCETABLE\r\n";

    NTRIPSourceTableModel model;
    model.parseSourceTable(table);

    // Short and non-STR lines are not mountpoints.
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.rowCount(), 2);
}

void NTRIPSourceTableTest::_updateDistancesAll()
{
    const QString table =
        "STR;NYC;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.7128;-74.0060;0;1;gen;none;B;N;4800;misc\r\n"
        "STR;LON;Id2;RTCM 3.3;details;0;GPS;NET;GBR;51.5074;-0.1278;1;0;gen;none;N;Y;9600;misc\r\n"
        "ENDSOURCETABLE\r\n";

    NTRIPSourceTableModel model;
    NTRIPSourceTableSortModel sorted(&model);
    QAbstractItemModelTester tester(&sorted, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.parseSourceTable(table);
    QCOMPARE(sorted.count(), 2);
    QSignalSpy changes(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy resets(&sorted, &QAbstractItemModel::modelReset);
    const auto mountpoints = [](const QAbstractItemModel& rows) {
        QStringList names;
        for (int row = 0; row < rows.rowCount(); ++row) {
            names.append(rows.index(row, 0).data(NTRIPSourceTableModel::MountpointRole).toString());
        }
        return names;
    };

    model.updateDistances(QGeoCoordinate(51.5074, -0.1278));
    QCOMPARE(changes.size(), 1);
    const auto change = changes.takeFirst();
    QCOMPARE(qvariant_cast<QModelIndex>(change[0]), model.index(0, 0));
    QCOMPARE(qvariant_cast<QModelIndex>(change[1]), model.index(1, 0));
    QCOMPARE(qvariant_cast<QList<int>>(change[2]),
             (QList<int>{NTRIPSourceTableModel::DistanceKmRole, NTRIPSourceTableModel::DetailsRole}));
    // The source keeps caster order; the sorted view puts the nearest mountpoint first.
    QCOMPARE(mountpoints(model), (QStringList{QStringLiteral("NYC"), QStringLiteral("LON")}));
    QCOMPARE(mountpoints(sorted), (QStringList{QStringLiteral("LON"), QStringLiteral("NYC")}));
    QVERIFY(sorted.index(0, 0).data(NTRIPSourceTableModel::DistanceKmRole).toDouble() < 1.0);
    QVERIFY(sorted.index(1, 0).data(NTRIPSourceTableModel::DistanceKmRole).toDouble() > 5000.0);

    model.updateDistances(QGeoCoordinate(40.7128, -74.0060));
    QCOMPARE(mountpoints(sorted), (QStringList{QStringLiteral("NYC"), QStringLiteral("LON")}));
    model.updateDistances(QGeoCoordinate(40.7128, -74.0060));
    QCOMPARE(changes.size(), 1);
    QVERIFY(resets.isEmpty());
}

void NTRIPSourceTableTest::_singleMountpointDistanceNotification()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable(QStringLiteral("STR;MP1;Id;RTCM 3.2;details;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\n"));
    QSignalSpy changes(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
    for (const auto& coordinate : {QGeoCoordinate(40, -74), QGeoCoordinate(52, 13), QGeoCoordinate()}) {
        model.updateDistances(coordinate);
        QCOMPARE(changes.size(), 1);
        const auto change = changes.takeFirst();
        QCOMPARE(qvariant_cast<QModelIndex>(change[0]), model.index(0, 0));
        QCOMPARE(qvariant_cast<QModelIndex>(change[1]), model.index(0, 0));
        QCOMPARE(qvariant_cast<QList<int>>(change[2]),
                 (QList<int>{NTRIPSourceTableModel::DistanceKmRole, NTRIPSourceTableModel::DetailsRole}));
        const double distance = model.data(model.index(0, 0), NTRIPSourceTableModel::DistanceKmRole).toDouble();
        QCOMPARE(distance, coordinate.isValid() ? coordinate.distanceTo(QGeoCoordinate(40, -74)) / 1000 : -1.0);
    }
    QVERIFY(resets.isEmpty());
}

void NTRIPSourceTableTest::_emptyTable()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable("");
    QCOMPARE(model.count(), 0);

    model.parseSourceTable("ENDSOURCETABLE\r\n");
    QCOMPARE(model.count(), 0);
}

void NTRIPSourceTableTest::_tableTerminator_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("complete");
    const QByteArray row = "STR;MP;Id;RTCM 3.2;;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800";
    QTest::newRow("crlf") << row + "\r\nENDSOURCETABLE\r\n" << true;
    QTest::newRow("lf") << row + "\nENDSOURCETABLE\n" << true;
    QTest::newRow("no-final-newline") << row + "\r\nENDSOURCETABLE" << true;
    QTest::newRow("empty-table") << QByteArray("ENDSOURCETABLE\r\n") << true;
    QTest::newRow("partial") << row + "\r\nENDSOURCE" << false;
    QTest::newRow("inside-row") << QByteArray("STR;ENDSOURCETABLE;Id\r\n") << false;
    QTest::newRow("prefix-of-line") << row + "\r\nENDSOURCETABLES\r\n" << false;
}

void NTRIPSourceTableTest::_tableTerminator()
{
    QFETCH(QByteArray, body);
    QFETCH(bool, complete);
    QCOMPARE(ntripSourceTableComplete(body), complete);
    // After a check of an incomplete prefix, checking only what the rest could complete gives the same answer.
    for (qsizetype split = 0; split <= body.size(); ++split) {
        if (!ntripSourceTableComplete(body.first(split))) {
            QCOMPARE(ntripSourceTableComplete(body, split), complete);
        }
    }
}

void NTRIPSourceTableTest::_rolesAreReadOnlyProperties()
{
    NTRIPSourceTableModel model;
    const auto roles = model.roleNames();

    const struct
    {
        int role;
        const char* name;
    } expected[] = {
        {NTRIPSourceTableModel::MountpointRole, "mountpoint"}, {NTRIPSourceTableModel::FormatRole, "format"},
        {NTRIPSourceTableModel::NavSystemRole, "navSystem"},   {NTRIPSourceTableModel::CountryRole, "country"},
        {NTRIPSourceTableModel::BitrateRole, "bitrate"},       {NTRIPSourceTableModel::DistanceKmRole, "distanceKm"},
        {NTRIPSourceTableModel::DetailsRole, "details"},
    };

    for (const auto& [role, name] : expected) {
        QCOMPARE(roles.value(role), QByteArray(name));
    }
    model.parseSourceTable(QStringLiteral("STR;MP1;Id;RTCM 3.2;;2;GPS;Net;DEU;52.00;13.00;1;0;Gen;none;B;N;9600;"));
    QCOMPARE(model.rowCount(), 1);
    const auto index = model.index(0, 0);
    QCOMPARE(model.data(index, NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("MP1"));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(),
             QStringLiteral("RTCM 3.2 · GPS · DEU · 9600 bps"));
    model.updateDistances(QGeoCoordinate(52, 13));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(),
             QStringLiteral("RTCM 3.2 · GPS · DEU · 9600 bps · 0.0 km"));
    QVERIFY(!(model.flags(index) & Qt::ItemIsEditable));
    QVERIFY(!model.setData(index, QStringLiteral("changed"), NTRIPSourceTableModel::MountpointRole));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("MP1"));
}

void NTRIPSourceTableTest::_coordinateValidity_data()
{
    QTest::addColumn<QString>("latitude");
    QTest::addColumn<QString>("longitude");
    QTest::addColumn<bool>("known");
    QTest::newRow("empty-latitude") << QString() << QStringLiteral("-74") << false;
    QTest::newRow("invalid-latitude") << QStringLiteral("bad") << QStringLiteral("-74") << false;
    QTest::newRow("out-of-range-latitude") << QStringLiteral("91") << QStringLiteral("-74") << false;
    QTest::newRow("nonfinite-longitude") << QStringLiteral("40") << QStringLiteral("nan") << false;
    QTest::newRow("out-of-range-longitude") << QStringLiteral("40") << QStringLiteral("181") << false;
    QTest::newRow("equator") << QStringLiteral("0") << QStringLiteral("-74") << true;
    QTest::newRow("prime-meridian") << QStringLiteral("40") << QStringLiteral("0") << true;
    QTest::newRow("unspecified-origin") << QStringLiteral("0") << QStringLiteral("0") << false;
}

void NTRIPSourceTableTest::_coordinateValidity()
{
    QFETCH(QString, latitude);
    QFETCH(QString, longitude);
    QFETCH(bool, known);
    auto mountpoint = NTRIPMountpoint::fromSourceTableLine(
        QStringLiteral("STR;TEST;Id;RTCM 3.2;;2;GPS;NET;USA;%1;%2;0;1;gen;none;B;N;4800").arg(latitude, longitude));
    QVERIFY(mountpoint);
    mountpoint->updateDistance(QGeoCoordinate(40, -74));
    QCOMPARE(mountpoint->distanceKm >= 0, known);
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPSourceTableTest, TestLabel::Unit)
