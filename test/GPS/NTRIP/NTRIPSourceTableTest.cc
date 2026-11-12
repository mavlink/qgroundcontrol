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
        "CAS;caster.example.com;2101;desc;op;0;USA;0;0;0;0;0;0;misc\r\n"
        "NET;NET;operator;B;N;https://example.com;https://example.com/streams;contact@example.com;none\r\n"
        "ENDSOURCETABLE\r\n";

    NTRIPSourceTableModel model;
    model.parseSourceTable(table);

    // Only STR records are mountpoints.
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.rowCount(), 2);
}

void NTRIPSourceTableTest::_shortRow_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<bool>("listed");
    QTest::addColumn<QString>("details");
    // RTKLIB's caster sends "STR;<mountpoint>;<text>", where the text is whatever its operator gave it.
    QTest::newRow("rtklib-text") << QStringLiteral("STR;BASE;Rooftop antenna") << true
                                 << QStringLiteral("Rooftop antenna");
    QTest::newRow("rtklib-no-text") << QStringLiteral("STR;BASE;") << true << QString();
    QTest::newRow("name-only") << QStringLiteral("STR;BASE") << true << QString();
    QTest::newRow("through-format") << QStringLiteral("STR;BASE;Rooftop antenna;RTCM 3.2;1005(10),1077(1)") << true
                                    << QStringLiteral("Rooftop antenna · RTCM 3.2");
    QTest::newRow("through-country") << QStringLiteral("STR;BASE;;RTCM 3.2;;2;GPS+GLO;NET;DEU") << true
                                     << QStringLiteral("RTCM 3.2 · GPS+GLO · DEU");
    // A description that only repeats the name adds nothing.
    QTest::newRow("named-twice") << QStringLiteral("STR;BASE;base;RTCM 3.2") << true << QStringLiteral("RTCM 3.2");
    // Without a name there is nothing to connect to.
    QTest::newRow("no-name") << QStringLiteral("STR;;Rooftop antenna;RTCM 3.2") << false << QString();
    QTest::newRow("blank-name") << QStringLiteral("STR; ;Rooftop antenna") << false << QString();
    QTest::newRow("type-only") << QStringLiteral("STR") << false << QString();
    // A control character does not show in the list, where such a name would look like one that can be requested.
    QTest::newRow("carriage-return-in-name") << QStringLiteral("STR;BA\rSE;Rooftop antenna") << false << QString();
    QTest::newRow("tab-in-name") << QStringLiteral("STR;BA\tSE;Rooftop antenna") << false << QString();
    QTest::newRow("delete-in-name") << QStringLiteral("STR;BA\x7fSE;Rooftop antenna") << false << QString();
}

void NTRIPSourceTableTest::_shortRow()
{
    QFETCH(QString, line);
    QFETCH(bool, listed);
    QFETCH(QString, details);
    NTRIPSourceTableModel model;
    model.parseSourceTable(line + QStringLiteral("\r\nENDSOURCETABLE\r\n"), QGeoCoordinate(40, -74));
    QCOMPARE(model.count(), listed ? 1 : 0);
    if (listed) {
        const auto index = model.index(0, 0);
        QCOMPARE(model.data(index, NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("BASE"));
        QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(), details);
        // A row without coordinates has no distance.
        QCOMPARE(model.data(index, NTRIPSourceTableModel::DistanceKmRole).toDouble(), -1.0);
    }
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
    // The bytes after which the table is complete while more of the body may follow; -1 when it never is.
    QTest::addColumn<int>("completeAfter");
    // Whether the table is complete once the body has ended.
    QTest::addColumn<bool>("completeAtEnd");
    const QByteArray row = "STR;MP;Id;RTCM 3.2;;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800";
    const int rowSize = static_cast<int>(row.size());
    // The first byte of its line ending shows that the terminator line is whole.
    QTest::newRow("crlf") << row + "\r\nENDSOURCETABLE\r\n" << rowSize + 17 << true;
    QTest::newRow("lf") << row + "\nENDSOURCETABLE\n" << rowSize + 16 << true;
    QTest::newRow("empty-table") << QByteArray("ENDSOURCETABLE\r\n") << 15 << true;
    // Without a line ending only the end of the body shows it.
    QTest::newRow("no-final-newline") << row + "\r\nENDSOURCETABLE" << -1 << true;
    QTest::newRow("partial") << row + "\r\nENDSOURCE" << -1 << false;
    QTest::newRow("inside-row") << QByteArray("STR;ENDSOURCETABLE;Id\r\n") << -1 << false;
    QTest::newRow("prefix-of-line") << row + "\r\nENDSOURCETABLES\r\n" << -1 << false;
    const QByteArray longerLine = row + "\r\nENDSOURCETABLES\r\n" + row;
    QTest::newRow("terminator-after-prefix-of-line") << longerLine + "\r\nENDSOURCETABLE\r\n"
                                                     << static_cast<int>(longerLine.size()) + 17 << true;
}

void NTRIPSourceTableTest::_tableTerminator()
{
    QFETCH(QByteArray, body);
    QFETCH(int, completeAfter);
    QFETCH(bool, completeAtEnd);
    // However the body is split, a piece completes the table only once it holds the bytes that complete the whole.
    for (qsizetype split = 0; split <= body.size(); ++split) {
        const bool complete = ntripSourceTableComplete(body.first(split));
        QCOMPARE(complete, completeAfter >= 0 && split >= completeAfter);
        // After a check of an incomplete piece, checking only what the rest could complete gives the same answer.
        if (!complete) {
            QCOMPARE(ntripSourceTableComplete(body, split), completeAfter >= 0);
        }
    }
    QCOMPARE(ntripSourceTableComplete(body, 0, true), completeAtEnd);
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
             QStringLiteral("Id · RTCM 3.2 · GPS · DEU · 9600 bps"));
    model.updateDistances(QGeoCoordinate(52, 13));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(),
             QStringLiteral("Id · RTCM 3.2 · GPS · DEU · 9600 bps · 0.0 km"));
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
