#include "TerrainTileTest.h"

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

UT_REGISTER_TEST(TerrainTileTest, TestLabel::Unit, TestLabel::Terrain)
