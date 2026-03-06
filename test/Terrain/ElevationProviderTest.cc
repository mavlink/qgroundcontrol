#include "ElevationProviderTest.h"
#include "ElevationMapProvider.h"
#include "TerrainTile.h"

#include <QtCore/QFile>
#include <QtTest/QTest>

void ElevationProviderTest::_testSRTM1ProviderConstants()
{
    ArduPilotTerrainElevationProvider provider;
    QCOMPARE(provider.tileValueSpacingDegrees(), 1.0 / 3600);
    QCOMPARE(provider.tileValueSpacingMeters(), 30.0);
    QVERIFY(provider.isElevationProvider());
    QCOMPARE(provider.getAverageSize(), ArduPilotTerrainElevationProvider::kAvgElevSize);
}

void ElevationProviderTest::_testSRTM3ProviderConstants()
{
    ArduPilotTerrainSRTM3ElevationProvider provider;
    QCOMPARE(provider.tileValueSpacingDegrees(), 1.0 / 1200);
    QCOMPARE(provider.tileValueSpacingMeters(), 90.0);
    QVERIFY(provider.isElevationProvider());
    QCOMPARE(provider.getAverageSize(), ArduPilotTerrainSRTM3ElevationProvider::kAvgElevSize);
}

void ElevationProviderTest::_testOpenElevationProviderConstants()
{
    OpenElevationProvider provider;
    QCOMPARE(provider.tileValueSpacingDegrees(), 1.0 / 3600);
    QCOMPARE(provider.tileValueSpacingMeters(), 30.0);
    QVERIFY(provider.isElevationProvider());
    QCOMPARE(QString(OpenElevationProvider::kProviderKey), QStringLiteral("Open-Elevation"));
}

void ElevationProviderTest::_testSRTM1TileCoordinates()
{
    ArduPilotTerrainElevationProvider provider;

    // 1-degree tiles: lon 0 → tile (0+180)/1 = 180, lat 0 → tile (0+90)/1 = 90
    QCOMPARE(provider.long2tileX(0.0, 1), 180);
    QCOMPARE(provider.lat2tileY(0.0, 1), 90);

    // lon -180 → tile 0, lat -90 → tile 0
    QCOMPARE(provider.long2tileX(-180.0, 1), 0);
    QCOMPARE(provider.lat2tileY(-90.0, 1), 0);

    // lon 179.5 → tile 359, lat 89.5 → tile 179
    QCOMPARE(provider.long2tileX(179.5, 1), 359);
    QCOMPARE(provider.lat2tileY(89.5, 1), 179);
}

void ElevationProviderTest::_testSRTM3TileCoordinates()
{
    ArduPilotTerrainSRTM3ElevationProvider provider;

    // Same tile grid as SRTM1 (both 1-degree tiles)
    QCOMPARE(provider.long2tileX(0.0, 1), 180);
    QCOMPARE(provider.lat2tileY(0.0, 1), 90);
    QCOMPARE(provider.long2tileX(-180.0, 1), 0);
    QCOMPARE(provider.lat2tileY(-90.0, 1), 0);
    QCOMPARE(provider.long2tileX(6.5, 1), 186);
    QCOMPARE(provider.lat2tileY(45.5, 1), 135);
}

void ElevationProviderTest::_testOpenElevationTileNoOp()
{
    OpenElevationProvider provider;

    // All tile methods return 0/empty for online-only provider
    QCOMPARE(provider.long2tileX(10.0, 1), 0);
    QCOMPARE(provider.lat2tileY(45.0, 1), 0);

    const QByteArray serialized = provider.serialize(QByteArray("dummy"));
    QVERIFY(serialized.isEmpty());
}

void ElevationProviderTest::_testSRTM1TileCount()
{
    ArduPilotTerrainElevationProvider provider;

    // Single point → 1 tile
    const QGCTileSet singleSet = provider.getTileCount(1, 6.5, 6.5, 6.5, 6.5);
    QCOMPARE(singleSet.tileCount, static_cast<quint64>(1));

    // 1-degree range within a single tile → 1×1 = 1 tile
    const QGCTileSet oneTile = provider.getTileCount(1, 6.0, 6.9, 6.9, 6.0);
    QCOMPARE(oneTile.tileCount, static_cast<quint64>(1));

    // Spanning two tile boundaries in each axis → 2×2 = 4 tiles
    // lon [5.5, 6.5] spans tiles 185,186; lat [0.5, 1.5] spans tiles 90,91
    const QGCTileSet fourTiles = provider.getTileCount(1, 5.5, 1.5, 6.5, 0.5);
    QCOMPARE(fourTiles.tileCount, static_cast<quint64>(4));
    QCOMPARE(fourTiles.tileSize, static_cast<quint64>(4) * ArduPilotTerrainElevationProvider::kAvgElevSize);
}

void ElevationProviderTest::_testSRTM3TileCount()
{
    ArduPilotTerrainSRTM3ElevationProvider provider;

    // Same tile grid as SRTM1
    const QGCTileSet singleSet = provider.getTileCount(1, 6.5, 6.5, 6.5, 6.5);
    QCOMPARE(singleSet.tileCount, static_cast<quint64>(1));

    const QGCTileSet fourTiles = provider.getTileCount(1, 5.5, 1.5, 6.5, 0.5);
    QCOMPARE(fourTiles.tileCount, static_cast<quint64>(4));
    QCOMPARE(fourTiles.tileSize, static_cast<quint64>(4) * ArduPilotTerrainSRTM3ElevationProvider::kAvgElevSize);
}

void ElevationProviderTest::_testOpenElevationTileCountEmpty()
{
    OpenElevationProvider provider;

    const QGCTileSet set = provider.getTileCount(1, 0.0, 10.0, 10.0, 0.0);
    QCOMPARE(set.tileCount, static_cast<quint64>(0));
    QCOMPARE(set.tileSize, static_cast<quint64>(0));
}

void ElevationProviderTest::_testSRTM1SerializeFromResource()
{
    // Load the test SRTM1 zip resource
    QFile zipFile(":/N00E006.hgt.zip");
    QVERIFY(zipFile.open(QIODevice::ReadOnly));
    const QByteArray zipData = zipFile.readAll();
    zipFile.close();
    QVERIFY(!zipData.isEmpty());

    ArduPilotTerrainElevationProvider provider;
    const QByteArray serialized = provider.serialize(zipData);
    QVERIFY(!serialized.isEmpty());

    // Verify the serialized tile is valid
    TerrainTile tile(serialized);
    QVERIFY(tile.isValid());

    // Elevation at coast of Gulf of Guinea should be reasonable
    QVERIFY(tile.minElevation() >= -500.0);
    QVERIFY(tile.maxElevation() <= 5000.0);
    QVERIFY(tile.maxElevation() > tile.minElevation());

    // Verify we can query a point inside the tile (N00E006 → lat [0,1], lon [6,7])
    const QGeoCoordinate centerCoord(0.5, 6.5);
    const double elev = tile.elevation(centerCoord);
    QVERIFY(!qIsNaN(elev));
}

UT_REGISTER_TEST(ElevationProviderTest, TestLabel::Unit, TestLabel::Terrain)
