#include "TerrainTileTest.h"
#include "TerrainTileArduPilot.h"
#include "TerrainTile.h"

#include <QtTest/QTest>

QByteArray TerrainTileTest::_createValidTileData(double swLat, double swLon, double neLat, double neLon,
                                                 int16_t minElev, int16_t maxElev, double avgElev, int16_t gridSizeLat,
                                                 int16_t gridSizeLon, int16_t fillElevation)
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    const int dataSize = static_cast<int>(sizeof(int16_t)) * gridSizeLat * gridSizeLon;
    QByteArray result(headerSize + dataSize, Qt::Uninitialized);
    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(result.data());
    header->swLat = swLat;
    header->swLon = swLon;
    header->neLat = neLat;
    header->neLon = neLon;
    header->minElevation = minElev;
    header->maxElevation = maxElev;
    header->avgElevation = avgElev;
    header->gridSizeLat = gridSizeLat;
    header->gridSizeLon = gridSizeLon;
    int16_t* elevData = reinterpret_cast<int16_t*>(result.data() + headerSize);
    for (int i = 0; i < gridSizeLat * gridSizeLon; ++i) {
        elevData[i] = fillElevation;
    }
    return result;
}

void TerrainTileTest::_testValidTile()
{
    const QByteArray tileData = _createValidTileData(-48.88, -123.40, -48.87, -123.39, 10, 100, 55.0, 10, 10, 50);
    TerrainTile tile(tileData);
    QVERIFY(tile.isValid());
    QCOMPARE(tile.minElevation(), 10.0);
    QCOMPARE(tile.maxElevation(), 100.0);
    QCOMPARE(tile.avgElevation(), 55.0);
    const QGeoCoordinate centerCoord(-48.875, -123.395);
    const double elev = tile.elevation(centerCoord);
    QVERIFY(!qIsNaN(elev));
    QCOMPARE(elev, 50.0);
}

void TerrainTileTest::_testEmptyData()
{
    const QByteArray emptyData;
    TerrainTile tile(emptyData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testDataTooSmallForHeader()
{
    const QByteArray smallData(10, '\0');
    TerrainTile tile(smallData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testZeroGridDimensions()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    QByteArray tileData(headerSize, Qt::Uninitialized);
    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    header->swLat = -48.88;
    header->swLon = -123.40;
    header->neLat = -48.87;
    header->neLon = -123.39;
    header->minElevation = 10;
    header->maxElevation = 100;
    header->avgElevation = 55.0;
    header->gridSizeLat = 0;
    header->gridSizeLon = 10;
    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testNegativeGridDimensions()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    QByteArray tileData(headerSize, Qt::Uninitialized);
    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    header->swLat = -48.88;
    header->swLon = -123.40;
    header->neLat = -48.87;
    header->neLon = -123.39;
    header->minElevation = 10;
    header->maxElevation = 100;
    header->avgElevation = 55.0;
    header->gridSizeLat = -5;
    header->gridSizeLon = 10;
    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testExcessiveGridDimensions()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    QByteArray tileData(headerSize, Qt::Uninitialized);
    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    header->swLat = -48.88;
    header->swLon = -123.40;
    header->neLat = -48.87;
    header->neLon = -123.39;
    header->minElevation = 10;
    header->maxElevation = 100;
    header->avgElevation = 55.0;
    header->gridSizeLat = 20000;
    header->gridSizeLon = 20000;
    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testInfeasibleTileExtent()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    const int dataSize = static_cast<int>(sizeof(int16_t)) * 10 * 10;
    QByteArray tileData(headerSize + dataSize, Qt::Uninitialized);
    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    header->swLat = -48.87;
    header->swLon = -123.39;
    header->neLat = -48.88;
    header->neLon = -123.40;
    header->minElevation = 10;
    header->maxElevation = 100;
    header->avgElevation = 55.0;
    header->gridSizeLat = 10;
    header->gridSizeLon = 10;
    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testDataTooSmallForElevation()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    QByteArray tileData(headerSize + 10, Qt::Uninitialized);
    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    header->swLat = -48.88;
    header->swLon = -123.40;
    header->neLat = -48.87;
    header->neLon = -123.39;
    header->minElevation = 10;
    header->maxElevation = 100;
    header->avgElevation = 55.0;
    header->gridSizeLat = 100;
    header->gridSizeLon = 100;
    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testElevationOutsideBounds()
{
    const QByteArray tileData = _createValidTileData(-48.88, -123.40, -48.87, -123.39, 10, 100, 55.0, 10, 10, 50);
    TerrainTile tile(tileData);
    QVERIFY(tile.isValid());
    const QGeoCoordinate outsideCoord(-50.0, -125.0);
    const double elev = tile.elevation(outsideCoord);
    QVERIFY(qIsNaN(elev));
}

void TerrainTileTest::_testInvalidTileElevation()
{
    const QByteArray emptyData;
    TerrainTile tile(emptyData);
    QVERIFY(!tile.isValid());
    const QGeoCoordinate coord(-48.875, -123.395);
    const double elev = tile.elevation(coord);
    QVERIFY(qIsNaN(elev));
    QVERIFY(qIsNaN(tile.minElevation()));
    QVERIFY(qIsNaN(tile.maxElevation()));
    QVERIFY(qIsNaN(tile.avgElevation()));
}

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

void TerrainTileTest::_testArduPilotDetectDimension()
{
    // SRTM1: 3601 * 3601 * 2 = 25,934,402 bytes
    QCOMPARE(TerrainTileArduPilot::detectDimension(25934402), 3601);
    // SRTM3: 1201 * 1201 * 2 = 2,884,802 bytes
    QCOMPARE(TerrainTileArduPilot::detectDimension(2884802), 1201);
    // Invalid sizes
    QCOMPARE(TerrainTileArduPilot::detectDimension(0), 0);
    QCOMPARE(TerrainTileArduPilot::detectDimension(8), 0);
    QCOMPARE(TerrainTileArduPilot::detectDimension(1000000), 0);
}

void TerrainTileTest::_testArduPilotParseCoordinateData()
{
    // Create SRTM3-sized data (1201x1201 int16 values, smallest valid variant)
    constexpr int dimension = TerrainTileArduPilot::kSRTM3Dimension;
    constexpr int totalPoints = dimension * dimension;
    constexpr qsizetype dataSize = static_cast<qsizetype>(totalPoints) * 2;

    QByteArray sampleData(dataSize, '\0');

    // Write known elevations at specific positions in big-endian format
    // [row=0][col=0] = 100 (0x0064)
    sampleData[0] = 0x00;
    sampleData[1] = 0x64;
    // [row=0][col=1] = -32768 / void (0x8000)
    sampleData[2] = static_cast<char>(0x80);
    sampleData[3] = 0x00;
    // [row=1][col=0] = 200 (0x00C8)
    const int row1Offset = dimension * 2;
    sampleData[row1Offset]     = 0x00;
    sampleData[row1Offset + 1] = static_cast<char>(0xC8);
    // [row=1][col=1] = 300 (0x012C)
    sampleData[row1Offset + 2] = 0x01;
    sampleData[row1Offset + 3] = 0x2C;

    QVector<QGeoCoordinate> coords = TerrainTileArduPilot::parseCoordinateData("N00E000.hgt", sampleData);
    QCOMPARE(coords.size(), totalPoints);

    // SRTM3 spacing = 1.0 / (1201 - 1) = 1.0 / 1200
    constexpr double spacing = TerrainTileArduPilot::kSRTM3ValueSpacingDegrees;

    // [0][0]: north-west corner
    QCOMPARE(coords[0].latitude(), 1.0);
    QCOMPARE(coords[0].longitude(), 0.0);
    QCOMPARE(coords[0].altitude(), 100.0);

    // [0][1]: void value → NaN
    QCOMPARE(coords[1].latitude(), 1.0);
    QVERIFY(qFuzzyCompare(coords[1].longitude(), spacing));
    QVERIFY(qIsNaN(coords[1].altitude()));

    // [1][0]: second row
    QVERIFY(qFuzzyCompare(coords[dimension].latitude(), 1.0 - spacing));
    QCOMPARE(coords[dimension].longitude(), 0.0);
    QCOMPARE(coords[dimension].altitude(), 200.0);

    // [1][1]
    QCOMPARE(coords[dimension + 1].altitude(), 300.0);
}

void TerrainTileTest::_testArduPilotSerializeSRTM1()
{
    // Create synthetic SRTM1 data (3601x3601 int16, big-endian)
    constexpr int dimension = TerrainTileArduPilot::kTileDimension;
    constexpr int totalPoints = dimension * dimension;
    constexpr qsizetype dataSize = static_cast<qsizetype>(totalPoints) * 2;

    QByteArray hgtData(dataSize, '\0');

    // Set a known elevation at [0][0] = 500 (big-endian: 0x01F4)
    hgtData[0] = 0x01;
    hgtData[1] = static_cast<char>(0xF4);
    // Set [0][1] = 1000 (0x03E8)
    hgtData[2] = 0x03;
    hgtData[3] = static_cast<char>(0xE8);

    const QByteArray serialized = TerrainTileArduPilot::serializeFromData("N00E006.hgt", hgtData);
    QVERIFY(!serialized.isEmpty());

    // Verify the serialized tile can be loaded
    TerrainTile tile(serialized);
    QVERIFY(tile.isValid());
    QCOMPARE(tile.minElevation(), 0.0);   // most points are 0
    QCOMPARE(tile.maxElevation(), 1000.0);

    // Verify grid dimensions match SRTM1
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    const TerrainTile::TileInfo_t *header = reinterpret_cast<const TerrainTile::TileInfo_t *>(serialized.constData());
    QCOMPARE(header->gridSizeLat, static_cast<int16_t>(dimension));
    QCOMPARE(header->gridSizeLon, static_cast<int16_t>(dimension));
    QCOMPARE(header->swLat, 0.0);
    QCOMPARE(header->swLon, 6.0);
}

void TerrainTileTest::_testArduPilotSerializeSRTM3()
{
    // Create synthetic SRTM3 data (1201x1201 int16, big-endian)
    constexpr int dimension = TerrainTileArduPilot::kSRTM3Dimension;
    constexpr int totalPoints = dimension * dimension;
    constexpr qsizetype dataSize = static_cast<qsizetype>(totalPoints) * 2;

    QByteArray hgtData(dataSize, '\0');

    // Set a known elevation at [0][0] = 250 (0x00FA)
    hgtData[0] = 0x00;
    hgtData[1] = static_cast<char>(0xFA);
    // Set [600][600] (center) = 800 (0x0320) — offset = (600*1201 + 600)*2
    const qsizetype centerOffset = (static_cast<qsizetype>(600) * dimension + 600) * 2;
    hgtData[centerOffset]     = 0x03;
    hgtData[centerOffset + 1] = 0x20;

    const QByteArray serialized = TerrainTileArduPilot::serializeFromData("S10W020.hgt", hgtData);
    QVERIFY(!serialized.isEmpty());

    // Verify the serialized tile can be loaded
    TerrainTile tile(serialized);
    QVERIFY(tile.isValid());
    QCOMPARE(tile.maxElevation(), 800.0);

    // Verify grid dimensions match SRTM3
    const TerrainTile::TileInfo_t *header = reinterpret_cast<const TerrainTile::TileInfo_t *>(serialized.constData());
    QCOMPARE(header->gridSizeLat, static_cast<int16_t>(dimension));
    QCOMPARE(header->gridSizeLon, static_cast<int16_t>(dimension));
    QCOMPARE(header->swLat, -10.0);
    QCOMPARE(header->swLon, -20.0);
    QCOMPARE(header->neLat, -9.0);
    QCOMPARE(header->neLon, -19.0);

    // Verify we can query elevation at the center
    const QGeoCoordinate centerCoord(-9.5, -19.5);
    const double elev = tile.elevation(centerCoord);
    QVERIFY(!qIsNaN(elev));
}

void TerrainTileTest::_testArduPilotSerializeInvalidSize()
{
    // Data that matches neither SRTM1 nor SRTM3
    QByteArray invalidData(1000, '\0');
    const QByteArray result = TerrainTileArduPilot::serializeFromData("N00E000.hgt", invalidData);
    QVERIFY(result.isEmpty());
}

void TerrainTileTest::_testArduPilotSerializeInvalidFilename()
{
    // Valid SRTM3 data but invalid filename
    constexpr int dimension = TerrainTileArduPilot::kSRTM3Dimension;
    constexpr qsizetype dataSize = static_cast<qsizetype>(dimension) * dimension * 2;
    QByteArray hgtData(dataSize, '\0');

    const QByteArray result = TerrainTileArduPilot::serializeFromData("invalid.hgt", hgtData);
    QVERIFY(result.isEmpty());
}

UT_REGISTER_TEST(TerrainTileTest, TestLabel::Unit, TestLabel::Terrain)
