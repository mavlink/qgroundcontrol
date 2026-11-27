#include "MapProviderTest.h"

#include <limits>

#include <QtCore/QtMath>

#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "TileRegion.h"

class TestableMapProvider : public MapProvider
{
public:
    explicit TestableMapProvider(const QString& name = QStringLiteral("TestProvider"),
                                 const QString& imageFormat = QStringLiteral("png"),
                                 quint32 avgSize = QGC_AVERAGE_TILE_SIZE)
        : MapProvider(name, QString(), imageFormat, avgSize, MapProvider::CustomMap)
    {}

    using MapProvider::_hasSha256;

private:
    QString _getURL(int, int, int) const override { return {}; }
};

// --- Image format detection ---

void MapProviderTest::_testGetImageFormat_data()
{
    QTest::addColumn<QString>("providerFormat");
    QTest::addColumn<QByteArray>("image");
    QTest::addColumn<QString>("format");

    const QString png = QStringLiteral("png");
    const QString webp = QStringLiteral("webp");
    // A picture is told by its signature, whatever the format of the provider
    QTest::newRow("PNG") << webp
                         << QByteArray(
                                "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"
                                "rest_of_data",
                                20)
                         << png;
    QTest::newRow("JPEG") << png
                          << QByteArray(
                                 "\xFF\xD8\xFF\xE0"
                                 "jpeg_data",
                                 13)
                          << QStringLiteral("jpg");
    QTest::newRow("GIF") << png << QByteArray("GIF89a_data", 11) << QStringLiteral("gif");
    QTest::newRow("WebP") << png << QByteArray("RIFF____WEBP", 12) << webp;
    QTest::newRow("no signature: the format of the provider") << webp << QByteArray("no picture") << webp;
    QTest::newRow("too small to tell") << png << QByteArray("AB") << QString();
    QTest::newRow("empty") << png << QByteArray() << QString();
}

void MapProviderTest::_testGetImageFormat()
{
    QFETCH(QString, providerFormat);
    QFETCH(QByteArray, image);
    QFETCH(QString, format);

    const TestableMapProvider p(QStringLiteral("Test"), providerFormat);
    QCOMPARE(p.getImageFormat(image), format);
}

// --- Web Mercator coordinate math ---
// Reference: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames

void MapProviderTest::_testLong2tileX_data()
{
    QTest::addColumn<double>("longitude");
    QTest::addColumn<int>("zoom");
    QTest::addColumn<int>("column");

    QTest::newRow("prime meridian, zoom 0") << 0.0 << 0 << 0;
    QTest::newRow("west edge, zoom 0") << -180.0 << 0 << 0;
    // (0+180)/360 * 2 = 1.0
    QTest::newRow("prime meridian, zoom 1") << 0.0 << 1 << 1;
    QTest::newRow("west edge, zoom 1") << -180.0 << 1 << 0;
    // New York: floor((-73.9857+180)/360 * 1024) = floor(301.48...)
    QTest::newRow("western longitude") << -73.9857 << 10 << 301;
    // 2^10 - 1
    QTest::newRow("just west of the east edge") << 179.999 << 10 << 1023;

    // A longitude that the map does not have gives a column of its edge, and none that an int cannot hold.
    // z=3: the columns 0 to 7, and 8 for the east edge.
    QTest::newRow("west edge, zoom 3") << -180.0 << 3 << 0;
    QTest::newRow("last column") << 179.9 << 3 << 7;
    QTest::newRow("east edge") << 180.0 << 3 << 8;
    QTest::newRow("west of the map") << -190.0 << 3 << 0;
    QTest::newRow("east of the map") << 190.0 << 3 << 8;
    QTest::newRow("far west of the map") << -1e300 << 3 << 0;
    QTest::newRow("far east of the map") << 1e300 << 3 << 8;
    QTest::newRow("not a number") << std::numeric_limits<double>::quiet_NaN() << 3 << 0;
}

void MapProviderTest::_testLong2tileX()
{
    QFETCH(double, longitude);
    QFETCH(int, zoom);
    QFETCH(int, column);

    QCOMPARE(TestableMapProvider().long2tileX(longitude, zoom), column);
}

void MapProviderTest::_testLat2tileY_data()
{
    QTest::addColumn<double>("latitude");
    QTest::addColumn<int>("zoom");
    QTest::addColumn<int>("row");

    // (1 - log(tan(0) + 1/cos(0))/π) / 2 * 2 = (1 - 0)/2 * 2
    QTest::newRow("equator") << 0.0 << 1 << 1;
    // New York
    QTest::newRow("northern latitude") << 40.7128 << 10 << 385;
    QTest::newRow("near the north pole") << 85.05 << 1 << 0;
    QTest::newRow("near the south pole") << -85.05 << 1 << 1;
    QTest::newRow("not a number") << std::numeric_limits<double>::quiet_NaN() << 3 << 0;
}

void MapProviderTest::_testLat2tileY()
{
    QFETCH(double, latitude);
    QFETCH(int, zoom);
    QFETCH(int, row);

    QCOMPARE(TestableMapProvider().lat2tileY(latitude, zoom), row);
}

// --- Tile count ---

void MapProviderTest::_testGetTileCount()
{
    // The size of a region is the average size of a tile of the provider for each of its tiles
    const TestableMapProvider p(QStringLiteral("Test"), QStringLiteral("png"), 5000);

    // At z=1, tile (1,1) covers lon=[0,180), lat≈[-85.05,0): a region within that tile
    const TileRegion oneTile = p.getTileCount(1, 10.0, -10.0, 20.0, -20.0);
    QCOMPARE(oneTile.tileCount, static_cast<quint64>(1));
    QCOMPARE(oneTile.tileSize, static_cast<quint64>(5000));

    // z=1: the world is 2x2 tiles
    const TileRegion world = p.getTileCount(1, -179.9, 85.0, 179.9, -85.0);
    QCOMPARE(world.tileCount, static_cast<quint64>(4));
    QCOMPARE(world.tileSize, static_cast<quint64>(20000));
}

void MapProviderTest::_testGetTileCountAcrossAntimeridian()
{
    TestableMapProvider p;
    // z=3: 8 columns. The region covers the last column and the first one, and the rows 3 and 4.
    const auto set = p.getTileCount(3, 170.0, 10.0, -170.0, -10.0);
    QCOMPARE(set.tileX0, 7);
    QCOMPARE(set.tileX1, 8);
    QCOMPARE(set.tileX1 % p.tileColumns(3), 0);
    QCOMPARE(set.tileY0, 3);
    QCOMPARE(set.tileY1, 4);
    QCOMPARE(set.tileCount, static_cast<quint64>(4));
}

void MapProviderTest::_testGetTileCountBeyondMap_data()
{
    QTest::addColumn<double>("leftLon");
    QTest::addColumn<double>("topLat");
    QTest::addColumn<double>("bottomLat");
    QTest::addColumn<quint64>("tileCount");

    constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
    // z=3: the region is in the column 4 and has every row of the map, 0 to 7, which ends near 85.05 degrees
    QTest::newRow("beyond the edges of the map") << 10.0 << 85.06 << -85.06 << quint64(8);
    QTest::newRow("poles") << 10.0 << 90.0 << -90.0 << quint64(8);
    // A map view gives a corner beyond the map as an invalid coordinate
    QTest::newRow("latitude not a number") << 10.0 << kNaN << -10.0 << quint64(0);
    QTest::newRow("longitude not a number") << kNaN << 10.0 << -10.0 << quint64(0);
}

void MapProviderTest::_testGetTileCountBeyondMap()
{
    QFETCH(double, leftLon);
    QFETCH(double, topLat);
    QFETCH(double, bottomLat);
    QFETCH(quint64, tileCount);

    TestableMapProvider p;
    const TileRegion set = p.getTileCount(3, leftLon, topLat, 20.0, bottomLat);
    QCOMPARE(set.tileCount, tileCount);
    QCOMPARE(set.tileSize, tileCount * QGC_AVERAGE_TILE_SIZE);
    if (tileCount > 0) {
        QCOMPARE(set.tileY0, 0);
        QCOMPARE(set.tileY1, 7);
    }
}

// --- Getters ---

void MapProviderTest::_testGettersReturnConstructorValues()
{
    TestableMapProvider p(QStringLiteral("MyMap"), QStringLiteral("jpg"), 7777);
    QCOMPARE(p.mapName(), QStringLiteral("MyMap"));
    QCOMPARE(p.averageSize(), static_cast<quint32>(7777));
    QCOMPARE(p.mapStyle(), MapProvider::CustomMap);
    QVERIFY(!p.isElevationProvider());
}

// --- Attribution ---

void MapProviderTest::_testFixedSourcesHaveAttribution()
{
    // Only the maps of the URL that the user sets and of the tiles that the user imported have no one to credit
    const QStringList userSources = {QStringLiteral("CustomURL Custom"), QStringLiteral("MBTiles Imported")};
    for (const SharedMapProvider& provider : MapProviderRegistry::providers()) {
        QVERIFY2(provider->attribution().isEmpty() == userSources.contains(provider->mapName()),
                 qPrintable(provider->mapName()));
    }
}

// --- Refusal images ---

// The refusal that OpenStreetMap sends is told by its content: a tile of the same size is a tile
void MapProviderTest::_testTileIsNoRefusalImage()
{
    constexpr qsizetype kRefusalSize = 6672;
    const SharedMapProvider openStreetMap =
        MapProviderRegistry::getMapProviderFromProviderType(QStringLiteral("Street Map"));
    QVERIFY(openStreetMap);
    QVERIFY(!openStreetMap->isRefusalImage(QByteArray(kRefusalSize, 'x')));
}

// The picture that Esri sends where it has no tile is told by its content, too
void MapProviderTest::_testTileOfNoTileImageSizeIsTile()
{
    constexpr qsizetype kNoDataSize = 2521;
    const SharedMapProvider esri =
        MapProviderRegistry::getMapProviderFromProviderType(QStringLiteral("Esri World Satellite"));
    QVERIFY(esri);
    QVERIFY(!esri->isNoTileImage(QByteArray(kNoDataSize, 'x')));
}

// The refusal itself is not in the sources. What tells it is: the size and the SHA-256 of an image, here those of
// the test vector "abc" of FIPS 180-2.
void MapProviderTest::_testImageToldBySizeAndSha256()
{
    constexpr QByteArrayView kAbcSha256("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    QVERIFY(TestableMapProvider::_hasSha256(QByteArrayView("abc"), 3, kAbcSha256));
    QVERIFY(!TestableMapProvider::_hasSha256(QByteArrayView("abd"), 3, kAbcSha256));
}

UT_REGISTER_TEST(MapProviderTest, TestLabel::Unit)
