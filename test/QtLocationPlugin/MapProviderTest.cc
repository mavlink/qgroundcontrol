#include "MapProviderTest.h"

#include "MapProvider.h"

#include <QtCore/QtMath>

class TestableMapProvider : public MapProvider
{
public:
    explicit TestableMapProvider(const QString &name = QStringLiteral("TestProvider"),
                                const QString &imageFormat = QStringLiteral("png"),
                                quint32 avgSize = QGC_AVERAGE_TILE_SIZE)
        : MapProvider(name, QString(), imageFormat, avgSize, QGeoMapType::CustomMap) {}

    using MapProvider::_tileXYToQuadKey;
    using MapProvider::_getServerNum;

private:
    QString _getURL(int, int, int) const override { return {}; }
};

// --- Image format detection ---

void MapProviderTest::_testGetImageFormatPng()
{
    TestableMapProvider p;
    const QByteArray png("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A" "rest_of_data", 20);
    QCOMPARE(p.getImageFormat(png), QStringLiteral("png"));
}

void MapProviderTest::_testGetImageFormatJpeg()
{
    TestableMapProvider p;
    const QByteArray jpeg("\xFF\xD8\xFF\xE0" "jpeg_data", 13);
    QCOMPARE(p.getImageFormat(jpeg), QStringLiteral("jpg"));
}

void MapProviderTest::_testGetImageFormatGif()
{
    TestableMapProvider p;
    const QByteArray gif("GIF89a_data", 11);
    QCOMPARE(p.getImageFormat(gif), QStringLiteral("gif"));
}

void MapProviderTest::_testGetImageFormatFallback()
{
    TestableMapProvider p(QStringLiteral("Test"), QStringLiteral("webp"));
    const QByteArray unknown("RIFF____WEBP", 12);
    QCOMPARE(p.getImageFormat(unknown), QStringLiteral("webp"));
}

void MapProviderTest::_testGetImageFormatTooSmall()
{
    TestableMapProvider p;
    QVERIFY(p.getImageFormat(QByteArray("AB")).isEmpty());
    QVERIFY(p.getImageFormat(QByteArray()).isEmpty());
}

// --- Web Mercator coordinate math ---
// Reference: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames

void MapProviderTest::_testLong2tileXZoom0()
{
    TestableMapProvider p;
    QCOMPARE(p.long2tileX(0.0, 0), 0);
    QCOMPARE(p.long2tileX(-180.0, 0), 0);
}

void MapProviderTest::_testLong2tileXZoom1()
{
    TestableMapProvider p;
    // lon=0 at z=1: (0+180)/360 * 2 = 1.0 → floor = 1
    QCOMPARE(p.long2tileX(0.0, 1), 1);
    // lon=-180 at z=1: (−180+180)/360 * 2 = 0.0 → floor = 0
    QCOMPARE(p.long2tileX(-180.0, 1), 0);
}

void MapProviderTest::_testLong2tileXNegative()
{
    TestableMapProvider p;
    // NYC longitude: floor((-73.9857+180)/360 * 1024) = floor(301.48...) = 301
    QCOMPARE(p.long2tileX(-73.9857, 10), 301);
}

void MapProviderTest::_testLong2tileXBoundary180()
{
    TestableMapProvider p;
    // lon just below 180 at z=10: should be 2^10 - 1 = 1023
    QCOMPARE(p.long2tileX(179.999, 10), 1023);
}

void MapProviderTest::_testLat2tileYEquator()
{
    TestableMapProvider p;
    // lat=0 at z=1: (1 - log(tan(0) + 1/cos(0))/π) / 2 * 2 = (1 - 0)/2 * 2 = 1
    QCOMPARE(p.lat2tileY(0.0, 1), 1);
}

void MapProviderTest::_testLat2tileYNorthern()
{
    TestableMapProvider p;
    // NYC latitude 40.7128 at z=10
    QCOMPARE(p.lat2tileY(40.7128, 10), 385);
}

void MapProviderTest::_testLat2tileYPoles()
{
    TestableMapProvider p;
    // Near north pole at z=1 → tile 0
    QCOMPARE(p.lat2tileY(85.05, 1), 0);
    // Near south pole at z=1 → tile 1
    QCOMPARE(p.lat2tileY(-85.05, 1), 1);
}

// --- Tile count ---

void MapProviderTest::_testGetTileCountSingleTile()
{
    TestableMapProvider p;
    // At z=1, tile (1,1) covers lon=[0,180), lat≈[-85.05,0).
    // Use a bbox entirely within that tile.
    const auto set = p.getTileCount(1, 10.0, -10.0, 20.0, -20.0);
    QCOMPARE(set.tileCount, static_cast<quint64>(1));
}

void MapProviderTest::_testGetTileCountMultipleTiles()
{
    TestableMapProvider p;
    // z=1: full world = 2x2 = 4 tiles
    const auto set = p.getTileCount(1, -179.9, 85.0, 179.9, -85.0);
    QCOMPARE(set.tileCount, static_cast<quint64>(4));
}

void MapProviderTest::_testGetTileCountSizeMatchesAvg()
{
    TestableMapProvider p(QStringLiteral("Test"), QStringLiteral("png"), 5000);
    const auto set = p.getTileCount(1, -179.9, 85.0, 179.9, -85.0);
    QCOMPARE(set.tileSize, set.tileCount * static_cast<quint64>(5000));
}

// --- QuadKey encoding ---

void MapProviderTest::_testQuadKeyLevel1()
{
    TestableMapProvider p;
    QCOMPARE(p._tileXYToQuadKey(0, 0, 1), QStringLiteral("0"));
    QCOMPARE(p._tileXYToQuadKey(1, 0, 1), QStringLiteral("1"));
    QCOMPARE(p._tileXYToQuadKey(0, 1, 1), QStringLiteral("2"));
    QCOMPARE(p._tileXYToQuadKey(1, 1, 1), QStringLiteral("3"));
}

void MapProviderTest::_testQuadKeyLevel3()
{
    TestableMapProvider p;
    // (3,5,3): level 3, mask=4,2,1
    // i=3: mask=4; tileX=3 (011), 3&4=0; tileY=5 (101), 5&4=4≠0 → digit='0'+2='2'
    // i=2: mask=2; 3&2=2≠0 → +1; 5&2=0 → digit='1'
    // i=1: mask=1; 3&1=1≠0 → +1; 5&1=1≠0 → +2; digit='3'
    QCOMPARE(p._tileXYToQuadKey(3, 5, 3), QStringLiteral("213"));
}

void MapProviderTest::_testQuadKeyLevel0()
{
    TestableMapProvider p;
    QCOMPARE(p._tileXYToQuadKey(0, 0, 0), QString());
}

// --- Server number ---

void MapProviderTest::_testGetServerNumBasic()
{
    TestableMapProvider p;
    // (x + 2*y) % max
    QCOMPARE(p._getServerNum(0, 0, 4), 0);
    QCOMPARE(p._getServerNum(1, 0, 4), 1);
    QCOMPARE(p._getServerNum(0, 1, 4), 2);
    QCOMPARE(p._getServerNum(1, 1, 4), 3);
}

void MapProviderTest::_testGetServerNumWrap()
{
    TestableMapProvider p;
    // (5 + 2*3) % 4 = 11 % 4 = 3
    QCOMPARE(p._getServerNum(5, 3, 4), 3);
    // max=1 always returns 0
    QCOMPARE(p._getServerNum(99, 99, 1), 0);
}

// --- Getters ---

void MapProviderTest::_testGettersReturnConstructorValues()
{
    TestableMapProvider p(QStringLiteral("MyMap"), QStringLiteral("jpg"), 7777);
    QCOMPARE(p.getMapName(), QStringLiteral("MyMap"));
    QCOMPARE(p.getAverageSize(), static_cast<quint32>(7777));
    QCOMPARE(p.getMapStyle(), QGeoMapType::CustomMap);
    QVERIFY(!p.isElevationProvider());
    QVERIFY(!p.isBingProvider());
    QVERIFY(p.getMapId() > 0);
}

UT_REGISTER_TEST(MapProviderTest, TestLabel::Unit)
