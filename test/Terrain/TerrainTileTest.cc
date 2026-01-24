#include "TerrainTileTest.h"

#include <QtTest/QTest>
#include <QtPositioning/QGeoCoordinate>

// Test tile coordinates (Pacific Ocean region - avoids most land features)
namespace {
    constexpr double kTestTileSwLat = -48.88;
    constexpr double kTestTileSwLon = -123.40;
    constexpr double kTestTileNeLat = -48.87;
    constexpr double kTestTileNeLon = -123.39;
    constexpr double kTestTileCenterLat = -48.875;
    constexpr double kTestTileCenterLon = -123.395;

    constexpr int16_t kTestMinElevation = 10;
    constexpr int16_t kTestMaxElevation = 100;
    constexpr double kTestAvgElevation = 55.0;
    constexpr int16_t kTestFillElevation = 50;
    constexpr int16_t kTestGridSize = 10;

    // Coordinate outside test tile bounds
    constexpr double kOutsideLat = -50.0;
    constexpr double kOutsideLon = -125.0;
}

QByteArray TerrainTileTest::_createValidTileData(
    double swLat, double swLon, double neLat, double neLon,
    int16_t minElev, int16_t maxElev, double avgElev,
    int16_t gridSizeLat, int16_t gridSizeLon,
    int16_t fillElevation)
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
    const QByteArray tileData = _createValidTileData(
        kTestTileSwLat, kTestTileSwLon, kTestTileNeLat, kTestTileNeLon,
        kTestMinElevation, kTestMaxElevation, kTestAvgElevation,
        kTestGridSize, kTestGridSize,
        kTestFillElevation
    );

    TerrainTile tile(tileData);

    QVERIFY(tile.isValid());
    QCOMPARE_EQ(tile.minElevation(), static_cast<double>(kTestMinElevation));
    QCOMPARE_EQ(tile.maxElevation(), static_cast<double>(kTestMaxElevation));
    QCOMPARE_EQ(tile.avgElevation(), kTestAvgElevation);

    const QGeoCoordinate centerCoord(kTestTileCenterLat, kTestTileCenterLon);
    QGC_VERIFY_VALID_COORDINATE(centerCoord);
    const double elev = tile.elevation(centerCoord);
    QVERIFY(!qIsNaN(elev));
    QCOMPARE_EQ(elev, static_cast<double>(kTestFillElevation));
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
    header->swLat = kTestTileSwLat;
    header->swLon = kTestTileSwLon;
    header->neLat = kTestTileNeLat;
    header->neLon = kTestTileNeLon;
    header->minElevation = kTestMinElevation;
    header->maxElevation = kTestMaxElevation;
    header->avgElevation = kTestAvgElevation;
    header->gridSizeLat = 0;  // Invalid: zero grid size
    header->gridSizeLon = kTestGridSize;

    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testNegativeGridDimensions()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    QByteArray tileData(headerSize, Qt::Uninitialized);

    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    header->swLat = kTestTileSwLat;
    header->swLon = kTestTileSwLon;
    header->neLat = kTestTileNeLat;
    header->neLon = kTestTileNeLon;
    header->minElevation = kTestMinElevation;
    header->maxElevation = kTestMaxElevation;
    header->avgElevation = kTestAvgElevation;
    header->gridSizeLat = -5;  // Invalid: negative grid size
    header->gridSizeLon = kTestGridSize;

    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testExcessiveGridDimensions()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    QByteArray tileData(headerSize, Qt::Uninitialized);

    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    header->swLat = kTestTileSwLat;
    header->swLon = kTestTileSwLon;
    header->neLat = kTestTileNeLat;
    header->neLon = kTestTileNeLon;
    header->minElevation = kTestMinElevation;
    header->maxElevation = kTestMaxElevation;
    header->avgElevation = kTestAvgElevation;
    header->gridSizeLat = 20000;  // Invalid: excessively large grid
    header->gridSizeLon = 20000;

    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testInfeasibleTileExtent()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    const int dataSize = static_cast<int>(sizeof(int16_t)) * kTestGridSize * kTestGridSize;
    QByteArray tileData(headerSize + dataSize, Qt::Uninitialized);

    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    // Swap SW and NE coordinates to create invalid extent (SW > NE)
    header->swLat = kTestTileNeLat;  // Invalid: SW lat > NE lat
    header->swLon = kTestTileNeLon;
    header->neLat = kTestTileSwLat;
    header->neLon = kTestTileSwLon;
    header->minElevation = kTestMinElevation;
    header->maxElevation = kTestMaxElevation;
    header->avgElevation = kTestAvgElevation;
    header->gridSizeLat = kTestGridSize;
    header->gridSizeLon = kTestGridSize;

    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testDataTooSmallForElevation()
{
    constexpr int headerSize = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    // Only add 10 bytes after header - not enough for 100x100 grid
    QByteArray tileData(headerSize + 10, Qt::Uninitialized);

    TerrainTile::TileInfo_t* header = reinterpret_cast<TerrainTile::TileInfo_t*>(tileData.data());
    header->swLat = kTestTileSwLat;
    header->swLon = kTestTileSwLon;
    header->neLat = kTestTileNeLat;
    header->neLon = kTestTileNeLon;
    header->minElevation = kTestMinElevation;
    header->maxElevation = kTestMaxElevation;
    header->avgElevation = kTestAvgElevation;
    header->gridSizeLat = 100;  // Claims 100x100 grid but data is too small
    header->gridSizeLon = 100;

    TerrainTile tile(tileData);
    QVERIFY(!tile.isValid());
}

void TerrainTileTest::_testElevationOutsideBounds()
{
    const QByteArray tileData = _createValidTileData(
        kTestTileSwLat, kTestTileSwLon, kTestTileNeLat, kTestTileNeLon,
        kTestMinElevation, kTestMaxElevation, kTestAvgElevation,
        kTestGridSize, kTestGridSize,
        kTestFillElevation
    );

    TerrainTile tile(tileData);
    QVERIFY(tile.isValid());

    // Request elevation for coordinate outside tile bounds
    const QGeoCoordinate outsideCoord(kOutsideLat, kOutsideLon);
    QGC_VERIFY_VALID_COORDINATE(outsideCoord);
    const double elev = tile.elevation(outsideCoord);
    QVERIFY(qIsNaN(elev));
}

void TerrainTileTest::_testInvalidTileElevation()
{
    const QByteArray emptyData;
    TerrainTile tile(emptyData);
    QVERIFY(!tile.isValid());

    // Requesting elevation from invalid tile should return NaN
    const QGeoCoordinate coord(kTestTileCenterLat, kTestTileCenterLon);
    const double elev = tile.elevation(coord);
    QVERIFY(qIsNaN(elev));

    // All elevation accessors should return NaN for invalid tile
    QVERIFY(qIsNaN(tile.minElevation()));
    QVERIFY(qIsNaN(tile.maxElevation()));
    QVERIFY(qIsNaN(tile.avgElevation()));
}
