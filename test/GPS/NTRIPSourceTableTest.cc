#include "NTRIPSourceTableTest.h"
#include "NTRIPSourceTable.h"
#include "QmlObjectListModel.h"

#include <QtPositioning/QGeoCoordinate>

void NTRIPSourceTableTest::_testParseSTRLine()
{
    const QString line = "STR;MOUNT01;Mount Identifier;RTCM 3.2;1005(1),1074(1),1084(1),1094(1);2;GPS+GLO+GAL;NET01;USA;40.0000;-74.0000;0;1;QGC Test;none;B;N;4800;misc";
    auto* mp = NTRIPMountpointModel::fromSourceTableLine(line);
    QVERIFY(mp != nullptr);

    QCOMPARE(mp->mountpoint(), QStringLiteral("MOUNT01"));
    QCOMPARE(mp->identifier(), QStringLiteral("Mount Identifier"));
    QCOMPARE(mp->format(), QStringLiteral("RTCM 3.2"));
    QCOMPARE(mp->carrier(), 2);
    QCOMPARE(mp->navSystem(), QStringLiteral("GPS+GLO+GAL"));
    QCOMPARE(mp->network(), QStringLiteral("NET01"));
    QCOMPARE(mp->country(), QStringLiteral("USA"));
    QVERIFY(qFuzzyCompare(mp->latitude(), 40.0));
    QVERIFY(qFuzzyCompare(mp->longitude(), -74.0));
    QVERIFY(!mp->nmea());
    QVERIFY(mp->solution());
    QCOMPARE(mp->generator(), QStringLiteral("QGC Test"));
    QCOMPARE(mp->authentication(), QStringLiteral("B"));
    QVERIFY(!mp->fee());
    QCOMPARE(mp->bitrate(), 4800);

    delete mp;
}

void NTRIPSourceTableTest::_testParseShortLine()
{
    const QString line = "STR;SHORT;Too;Few;Fields";
    auto* mp = NTRIPMountpointModel::fromSourceTableLine(line);
    QVERIFY(mp == nullptr);
}

void NTRIPSourceTableTest::_testParseNonSTRLine()
{
    const QString line = "CAS;caster.example.com;2101;NTRIP Caster;Operator;0;USA;0.0;0.0;0.0;0.0;0;0;misc";
    auto* mp = NTRIPMountpointModel::fromSourceTableLine(line);
    QVERIFY(mp == nullptr);
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
}

void NTRIPSourceTableTest::_testDistanceCalculation()
{
    const QString line = "STR;NEAR;Id;RTCM 3.2;details;2;GPS;NET;USA;40.7128;-74.0060;0;1;gen;none;B;N;4800;misc";
    auto* mp = NTRIPMountpointModel::fromSourceTableLine(line);
    QVERIFY(mp != nullptr);

    QVERIFY(mp->distanceKm() < 0.0);

    QGeoCoordinate userPos(40.7128, -74.0060);
    mp->updateDistance(userPos);
    QVERIFY(mp->distanceKm() >= 0.0);
    QVERIFY(mp->distanceKm() < 1.0);

    QGeoCoordinate farPos(51.5074, -0.1278);
    mp->updateDistance(farPos);
    QVERIFY(mp->distanceKm() > 5000.0);

    delete mp;
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

    // Before update, distances should be negative (unset)
    auto* mp0 = qobject_cast<NTRIPMountpointModel*>(model.mountpoints()->get(0));
    auto* mp1 = qobject_cast<NTRIPMountpointModel*>(model.mountpoints()->get(1));
    QVERIFY(mp0->distanceKm() < 0.0);
    QVERIFY(mp1->distanceKm() < 0.0);

    // Update from NYC position
    QGeoCoordinate nycPos(40.7128, -74.0060);
    model.updateDistances(nycPos);

    // NYC mount should be very close, London should be far
    QVERIFY(mp0->distanceKm() >= 0.0);
    QVERIFY(mp0->distanceKm() < 1.0);
    QVERIFY(mp1->distanceKm() > 5000.0);
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
