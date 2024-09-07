/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainTileTest.h"
#include "TerrainTileArduPilot.h"
#include "TerrainTile.h"

#include <QtTest/QTest>

void TerrainTileTest::_testArduPilotParseFileName()
{
    // Valid filename
    QGeoCoordinate coord = TerrainTileArduPilot::_parseFileName("S77W029.hgt");
    QCOMPARE(coord.latitude(), -77.0);
    QCOMPARE(coord.longitude(), -29.0);

    // Invalid filename
    coord = TerrainTileArduPilot::_parseFileName("InvalidName.hgt");
    QVERIFY(!coord.isValid());

    // Edge cases
    coord = TerrainTileArduPilot::_parseFileName("N00E000.hgt");
    QCOMPARE(coord.latitude(), 0.0);
    QCOMPARE(coord.longitude(), 0.0);

    coord = TerrainTileArduPilot::_parseFileName("N84E180.hgt");
    QCOMPARE(coord.latitude(), 84.0);
    QCOMPARE(coord.longitude(), 180.0);

    // Out of range
    coord = TerrainTileArduPilot::_parseFileName("N85E180.hgt");
    QVERIFY(!coord.isValid());

    coord = TerrainTileArduPilot::_parseFileName("S85E180.hgt");
    QVERIFY(!coord.isValid());
}

void TerrainTileTest::_testArduPilotParseCoordinateData()
{
    // Sample elevation data (big endian)
    QByteArray sampleData;
    QDataStream stream(&sampleData, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    // Populate with 4 elevation points for a 2x2 grid
    qint16 elevations[] = { 100, -32768, 200, 300 };
    for (int i = 0; i < 4; ++i) {
        stream << elevations[i];
    }

    QVector<QGeoCoordinate> coords = TerrainTileArduPilot::parseCoordinateData("N00E000.hgt", sampleData);
    QVERIFY(coords.size() == 4);

    QCOMPARE(coords[0].latitude(), 1.0);  // tileLat +1.0 - 0*0.00027778
    QCOMPARE(coords[0].longitude(), 0.0); // tileLon +0*0.00027778
    QCOMPARE(coords[0].altitude(), 100.0);

    QCOMPARE(coords[1].latitude(), 1.0);
    QCOMPARE(coords[1].longitude(), 0.00027778); // tileLon +1*0.00027778
    QCOMPARE(coords[1].altitude(), 0.0); // -32768 treated as 0.0

    QCOMPARE(coords[2].latitude(), 0.99972222); // tileLat +1.0 -1*0.00027778
}
