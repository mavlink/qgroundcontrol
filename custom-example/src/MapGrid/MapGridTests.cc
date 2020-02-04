/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapGridMGRS.h"

#include <QtTest/QtTest>

class MapGridTests : public QObject {
    Q_OBJECT

private slots:
    void testMGRSZone();
};

bool compare(double d1, double d2)
{
    return std::abs(d1 - d2) < 0.00001;
}

bool compare(const QGeoCoordinate &c1, const QGeoCoordinate &c2)
{
    return compare(c1.latitude(), c2.latitude()) && compare(c1.longitude(), c2.longitude());
}

void MapGridTests::testMGRSZone()
{
    MGRSZone z0("3X2Y");
    QVERIFY(!z0.valid);

    MGRSZone z1("32UPU");
    QVERIFY(z1.valid);
    QVERIFY(!z1.leftOverlap);
    QVERIFY(!z1.rightOverlap);
    QVERIFY(compare(z1.bottomLeft, QGeoCoordinate(47.845565672, 10.336597771)));
    QVERIFY(compare(z1.bottomRight, QGeoCoordinate(47.822240269, 11.672048336)));
    QVERIFY(compare(z1.topLeft, QGeoCoordinate(48.744979534, 10.360285924)));
    QVERIFY(compare(z1.topRight, QGeoCoordinate(48.720910536, 11.719351462)));

    MGRSZone z2("32UQU");
    QVERIFY(z2.valid);
    QVERIFY(!z2.leftOverlap);
    QVERIFY(z2.rightOverlap);
    QVERIFY(compare(z2.bottomLeft, QGeoCoordinate(47.822239958, 11.672061682)));
    QVERIFY(compare(z2.bottomRight, QGeoCoordinate(47.814132066, 11.999990732)));
    QVERIFY(compare(z2.topLeft, QGeoCoordinate(48.720910216, 11.719365043)));
    QVERIFY(compare(z2.topRight, QGeoCoordinate(48.713939877, 11.999986648)));

    MGRSZone z3("32URU");
    QVERIFY(z3.valid);
    QVERIFY(z3.leftOverlap);
    QVERIFY(z3.rightOverlap);
    QVERIFY(compare(z3.bottomLeft, QGeoCoordinate(47.783418534, 13.005277239)));
    QVERIFY(compare(z3.bottomRight, QGeoCoordinate(47.729183703, 14.335121901)));
    QVERIFY(compare(z3.topLeft, QGeoCoordinate(48.680852741, 13.076050836)));
    QVERIFY(compare(z3.topRight, QGeoCoordinate(48.624894327, 14.429149724)));
}

QTEST_MAIN(MapGridTests)
#include "MapGridTests.moc"
