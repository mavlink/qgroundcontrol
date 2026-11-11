#include "UrlFactoryTest.h"

#include <cmath>
#include <numbers>

#include <QtCore/QRect>

#include "ElevationMapProvider.h"
#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileSet.h"

static const QString kBingRoad = QStringLiteral("Bing Road");
static const QString kBingSatellite = QStringLiteral("Bing Satellite");
static const QString kBingHybrid = QStringLiteral("Bing Hybrid");
static const QString kCopernicus = QStringLiteral("Copernicus");
static const QString kTerrarium = QStringLiteral("Terrarium");

// --- Provider registry ---

void UrlFactoryTest::_testGetProviderTypesNotEmpty()
{
    const QStringList types = UrlFactory::getProviderTypes();
    QVERIFY(!types.isEmpty());
}

void UrlFactoryTest::_testGetProviderTypesContainsBing()
{
    const QStringList types = UrlFactory::getProviderTypes();
    QVERIFY(types.contains(kBingRoad));
    QVERIFY(types.contains(kBingSatellite));
    QVERIFY(types.contains(kBingHybrid));
}

void UrlFactoryTest::_testGetElevationProviderTypes()
{
    const QStringList elevTypes = UrlFactory::getElevationProviderTypes();
    QVERIFY(!elevTypes.isEmpty());
    QVERIFY(elevTypes.contains(kCopernicus));

    const QStringList allTypes = UrlFactory::getProviderTypes();
    for (const auto& t : elevTypes) {
        QVERIFY2(allTypes.contains(t), qPrintable(QStringLiteral("Elevation type '%1' not in allTypes").arg(t)));
    }
}

// --- Type <-> MapId roundtrip ---

void UrlFactoryTest::_testMapIdFromProviderTypeValid()
{
    const int id = UrlFactory::getQtMapIdFromProviderType(kBingRoad);
    QVERIFY(id > 0);
}

void UrlFactoryTest::_testProviderTypeFromMapIdRoundtrip()
{
    const QStringList types = UrlFactory::getProviderTypes();
    for (const auto& type : types) {
        const int id = UrlFactory::getQtMapIdFromProviderType(type);
        QVERIFY2(id > 0, qPrintable(QStringLiteral("Bad id for type: %1").arg(type)));
        const QString recovered = UrlFactory::getProviderTypeFromQtMapId(id);
        QCOMPARE(recovered, type);
    }
}

void UrlFactoryTest::_testMapIdFromEmptyType()
{
    QCOMPARE(UrlFactory::getQtMapIdFromProviderType(QString()), -1);
}

void UrlFactoryTest::_testMapIdFromInvalidType()
{
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QCOMPARE(UrlFactory::getQtMapIdFromProviderType(QStringLiteral("Nonexistent")), -1);
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testProviderTypeFromInvalidId()
{
    QVERIFY(UrlFactory::getProviderTypeFromQtMapId(-1).isEmpty());
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("map id not found:"));
    QVERIFY(UrlFactory::getProviderTypeFromQtMapId(99999).isEmpty());
    verifyExpectedLogMessage();
}

// --- Hash <-> ProviderType roundtrip ---

void UrlFactoryTest::_testHashFromProviderTypeValid()
{
    const int hash = UrlFactory::hashFromProviderType(kBingRoad);
    const int id = UrlFactory::getQtMapIdFromProviderType(kBingRoad);
    QCOMPARE(hash, id);
}

void UrlFactoryTest::_testProviderTypeFromHashRoundtrip()
{
    const QStringList types = UrlFactory::getProviderTypes();
    for (const auto& type : types) {
        const int hash = UrlFactory::hashFromProviderType(type);
        QVERIFY2(hash > 0, qPrintable(QStringLiteral("Bad hash for: %1").arg(type)));
        const QString recovered = UrlFactory::providerTypeFromHash(hash);
        QCOMPARE(recovered, type);
    }
}

void UrlFactoryTest::_testHashFromInvalidType()
{
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("provider not found for type:"));
    QCOMPARE(UrlFactory::hashFromProviderType(QString()), -1);
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testProviderTypeFromInvalidHash()
{
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("provider not found from hash:"));
    QVERIFY(UrlFactory::providerTypeFromHash(0).isEmpty());
    verifyExpectedLogMessage();
}

// --- Tile hash encode/decode ---

void UrlFactoryTest::_testGetTileHashFormat()
{
    const QString hash = UrlFactory::getTileHash(kBingRoad, 100, 200, 5);
    // Format: "%010d%08d%08d%03d" = 10+8+8+3 = 29 chars
    QCOMPARE(hash.length(), 29);
}

void UrlFactoryTest::_testTileHashToTypeRoundtrip()
{
    const QStringList types = UrlFactory::getProviderTypes();
    for (const auto& type : types) {
        const QString hash = UrlFactory::getTileHash(type, 42, 99, 7);
        const QString recovered = UrlFactory::tileHashToType(hash);
        QCOMPARE(recovered, type);
    }
}

void UrlFactoryTest::_testTileHashToTypeInvalid()
{
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("provider not found from hash:"));
    QVERIFY(UrlFactory::tileHashToType(QStringLiteral("garbage")).isEmpty());
    verifyExpectedLogMessage();
}

// --- Image format via facade ---

void UrlFactoryTest::_testGetImageFormatByType()
{
    const QByteArray png(
        "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"
        "data",
        12);
    QCOMPARE(UrlFactory::getImageFormat(kBingRoad, png), QStringLiteral("png"));
}

void UrlFactoryTest::_testGetImageFormatByMapId()
{
    const int id = UrlFactory::getQtMapIdFromProviderType(kBingRoad);
    const QByteArray jpeg(
        "\xFF\xD8\xFF\xE0"
        "data",
        8);
    QCOMPARE(UrlFactory::getImageFormat(id, jpeg), QStringLiteral("jpg"));
}

void UrlFactoryTest::_testGetImageFormatInvalidInputs()
{
    const QByteArray png("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8);
    QVERIFY(UrlFactory::getImageFormat(QString(), png).isEmpty());
    QVERIFY(UrlFactory::getImageFormat(-1, png).isEmpty());
}

void UrlFactoryTest::_testGetTileURLByType()
{
    const QUrl url = UrlFactory::getTileURL(kBingRoad, 301, 385, 10);
    QVERIFY(url.isValid());
    QVERIFY(!url.isEmpty());
    QVERIFY(url.scheme().startsWith(QStringLiteral("http")));
}

void UrlFactoryTest::_testGetTileURLByMapId()
{
    const int id = UrlFactory::getQtMapIdFromProviderType(kBingRoad);
    QVERIFY(id > 0);
    const QUrl url = UrlFactory::getTileURL(id, 301, 385, 10);
    QVERIFY(url.isValid());
    QVERIFY(!url.isEmpty());
    QVERIFY(url.scheme().startsWith(QStringLiteral("http")));
}

void UrlFactoryTest::_testGetTileURLInvalidInputs()
{
    QVERIFY(UrlFactory::getTileURL(QString(), 0, 0, 1).isEmpty());
    QVERIFY(UrlFactory::getTileURL(-1, 0, 0, 1).isEmpty());
}

// --- averageSizeForType ---

void UrlFactoryTest::_testAverageSizeForKnownProviders()
{
    QCOMPARE(UrlFactory::averageSizeForType(kBingRoad), static_cast<quint32>(1297));
    QCOMPARE(UrlFactory::averageSizeForType(kBingSatellite), static_cast<quint32>(19597));
    QCOMPARE(UrlFactory::averageSizeForType(kCopernicus), static_cast<quint32>(2786));
}

void UrlFactoryTest::_testAverageSizeForInvalidType()
{
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QCOMPARE(UrlFactory::averageSizeForType(QStringLiteral("Nonexistent")), QGC_AVERAGE_TILE_SIZE);
    verifyExpectedLogMessage();
}

// --- isElevation ---

void UrlFactoryTest::_testIsElevationTrue()
{
    const int id = UrlFactory::getQtMapIdFromProviderType(kCopernicus);
    QVERIFY(UrlFactory::isElevation(id));
}

void UrlFactoryTest::_testIsElevationFalse()
{
    const int id = UrlFactory::getQtMapIdFromProviderType(kBingRoad);
    QVERIFY(!UrlFactory::isElevation(id));
    QVERIFY(!UrlFactory::isElevation(-1));
}

// --- Coordinate conversion via facade ---

void UrlFactoryTest::_testLong2tileXDelegates()
{
    // NYC at z=10
    QCOMPARE(UrlFactory::long2tileX(kBingRoad, -73.9857, 10), 301);
}

void UrlFactoryTest::_testLat2tileYDelegates()
{
    // NYC at z=10
    QCOMPARE(UrlFactory::lat2tileY(kBingRoad, 40.7128, 10), 385);
}

void UrlFactoryTest::_testCoordConversionInvalidType()
{
    QCOMPARE(UrlFactory::long2tileX(QString(), -73.9857, 10), 0);
    QCOMPARE(UrlFactory::lat2tileY(QString(), 40.7128, 10), 0);
}

// --- Copernicus elevation specifics ---

void UrlFactoryTest::_testCopernicusLong2tileX()
{
    // floor((0+180)/0.01) = 18000
    QCOMPARE(UrlFactory::long2tileX(kCopernicus, 0.0, 1), 18000);
    // floor((-180+180)/0.01) = 0
    QCOMPARE(UrlFactory::long2tileX(kCopernicus, -180.0, 1), 0);
}

void UrlFactoryTest::_testCopernicusLat2tileY()
{
    // floor((0+90)/0.01) = 9000
    QCOMPARE(UrlFactory::lat2tileY(kCopernicus, 0.0, 1), 9000);
    // floor((-90+90)/0.01) = 0
    QCOMPARE(UrlFactory::lat2tileY(kCopernicus, -90.0, 1), 0);
}

void UrlFactoryTest::_testCopernicusTileCount()
{
    // Small 0.05° box near origin
    // Copernicus getTileCount swaps lat semantics: tileY0 = lat2tileY(bottomRightLat), tileY1 = lat2tileY(topleftLat)
    const auto set = UrlFactory::getTileCount(1, 0.0, 0.05, 0.05, 0.0, kCopernicus);
    // tileX: floor((0+180)/0.01)=18000 to floor((0.05+180)/0.01)=18005 → 6 tiles
    // tileY: floor((0+90)/0.01)=9000 to floor((0.05+90)/0.01)=9005 → 6 tiles
    // total = 6*6 = 36
    QCOMPARE(set.tileCount, static_cast<quint64>(36));
}

// --- Terrarium elevation provider ---

void UrlFactoryTest::_testTerrariumRegistered()
{
    QVERIFY(UrlFactory::getElevationProviderTypes().contains(kTerrarium));

    auto provider = UrlFactory::getMapProviderFromProviderType(kTerrarium);
    QVERIFY(provider != nullptr);
    QVERIFY(provider->isElevationProvider());
    // Unrecognized payload falls back to the provider's declared format
    QCOMPARE(UrlFactory::getImageFormat(kTerrarium, QByteArrayView("xxx", 3)), QStringLiteral("png"));
}

void UrlFactoryTest::_testTerrariumTileURL()
{
    const QUrl url = UrlFactory::getTileURL(kTerrarium, 1308, 2865, 13);
    QCOMPARE(url.toString(),
             QStringLiteral("https://s3.amazonaws.com/elevation-tiles-prod/terrarium/13/1308/2865.png"));

    // Standard slippy tile math, unlike Copernicus's fixed 0.01° grid
    QCOMPARE(UrlFactory::long2tileX(kTerrarium, 0.0, 1), 1);
    QCOMPARE(UrlFactory::lat2tileY(kTerrarium, 0.0, 1), 1);
}

void UrlFactoryTest::_testTerrariumNotUserSelectable()
{
    // GeoMap-only provider: hidden from the elevation dropdown and the
    // imagery provider list (which subtracts elevation types)
    auto provider = UrlFactory::getMapProviderFromProviderType(kTerrarium);
    QVERIFY(provider != nullptr);
    QVERIFY(!provider->isUserSelectable());

    auto copernicus = UrlFactory::getMapProviderFromProviderType(kCopernicus);
    QVERIFY(copernicus != nullptr);
    QVERIFY(copernicus->isUserSelectable());
}

void UrlFactoryTest::_testTerrariumAnchorTiles_data()
{
    // A region given by its tiles of the zoom below the anchor zoom, where a tile is half an anchor tile wide
    QTest::addColumn<QRect>("regionTiles");
    QTest::addColumn<QRect>("anchorTiles");

    // GeoMap fetches the anchor of a tile and of its neighbors: an anchor beside the region is needed where the
    // region ends on an anchor boundary
    QTest::newRow("no edge on an anchor boundary")
        << QRect(QPoint(1067, 729), QPoint(1068, 730)) << QRect(QPoint(533, 364), QPoint(534, 365));
    QTest::newRow("every edge on an anchor boundary")
        << QRect(QPoint(1066, 728), QPoint(1069, 731)) << QRect(QPoint(532, 363), QPoint(535, 366));
    QTest::newRow("west and south edge on an anchor boundary")
        << QRect(QPoint(1072, 717), QPoint(1072, 717)) << QRect(QPoint(535, 358), QPoint(536, 359));
    QTest::newRow("north west corner of the world")
        << QRect(QPoint(0, 0), QPoint(0, 0)) << QRect(QPoint(0, 0), QPoint(0, 0));
    QTest::newRow("south east corner of the world")
        << QRect(QPoint(2047, 2047), QPoint(2047, 2047)) << QRect(QPoint(1023, 1023), QPoint(1023, 1023));
}

void UrlFactoryTest::_testTerrariumAnchorTiles()
{
    QFETCH(QRect, regionTiles);
    QFETCH(QRect, anchorTiles);

    constexpr int kAnchorZoom = TerrariumElevationProvider::kAnchorZoom;
    constexpr int kRegionZoom = kAnchorZoom + 1;
    const double tileCount = std::pow(2.0, kRegionZoom);
    const auto tileCenterLon = [tileCount](int x) { return ((x + 0.5) / tileCount * 360.0) - 180.0; };
    const auto tileCenterLat = [tileCount](int y) {
        return std::atan(std::sinh(std::numbers::pi * (1.0 - (2.0 * (y + 0.5) / tileCount)))) * 180.0 /
               std::numbers::pi;
    };
    const double leftLon = tileCenterLon(regionTiles.left());
    const double topLat = tileCenterLat(regionTiles.top());
    const double rightLon = tileCenterLon(regionTiles.right());
    const double bottomLat = tileCenterLat(regionTiles.bottom());

    const QGCTileSet region = UrlFactory::getTileCount(kRegionZoom, leftLon, topLat, rightLon, bottomLat, kTerrarium);
    QCOMPARE(QRect(QPoint(region.tileX0, region.tileY0), QPoint(region.tileX1, region.tileY1)), regionTiles);

    const QGCTileSet anchors = UrlFactory::getTileCount(kAnchorZoom, leftLon, topLat, rightLon, bottomLat, kTerrarium);
    QCOMPARE(QRect(QPoint(anchors.tileX0, anchors.tileY0), QPoint(anchors.tileX1, anchors.tileY1)), anchorTiles);
    const quint64 anchorCount = static_cast<quint64>(anchorTiles.width()) * static_cast<quint64>(anchorTiles.height());
    QCOMPARE(anchors.tileCount, anchorCount);
    QCOMPARE(anchors.tileSize, anchorCount * TerrariumElevationProvider::kAvgElevSize);
}

// --- getTileCount ---

void UrlFactoryTest::_testGetTileCountValid()
{
    // z=1 full world via Bing Road
    const auto set = UrlFactory::getTileCount(1, -179.9, 85.0, 179.9, -85.0, kBingRoad);
    QVERIFY(set.tileCount > 0);
    QCOMPARE(set.tileCount, static_cast<quint64>(4));
}

void UrlFactoryTest::_testGetTileCountInvalidType()
{
    const auto set = UrlFactory::getTileCount(10, -73.0, 41.0, -72.0, 40.0, QString());
    QCOMPARE(set.tileCount, static_cast<quint64>(0));
    QCOMPARE(set.tileSize, static_cast<quint64>(0));
}

void UrlFactoryTest::_testGetTileCountZoomClamped()
{
    // zoom=0 should be clamped to 1 internally
    const auto setLow = UrlFactory::getTileCount(0, -179.9, 85.0, 179.9, -85.0, kBingRoad);
    const auto setOne = UrlFactory::getTileCount(1, -179.9, 85.0, 179.9, -85.0, kBingRoad);
    QCOMPARE(setLow.tileCount, setOne.tileCount);

    // zoom=99 should be clamped to 23
    const auto setHigh = UrlFactory::getTileCount(99, 0.0, 0.001, 0.001, 0.0, kBingRoad);
    const auto set23 = UrlFactory::getTileCount(23, 0.0, 0.001, 0.001, 0.0, kBingRoad);
    QCOMPARE(setHigh.tileCount, set23.tileCount);
}

// --- Provider lookup ---

void UrlFactoryTest::_testGetMapProviderValid()
{
    const int id = UrlFactory::getQtMapIdFromProviderType(kBingRoad);
    auto provider = UrlFactory::getMapProviderFromQtMapId(id);
    QVERIFY(provider != nullptr);
    QCOMPARE(provider->getMapName(), kBingRoad);
    QVERIFY(provider->isBingProvider());

    auto byType = UrlFactory::getMapProviderFromProviderType(kBingRoad);
    QVERIFY(byType != nullptr);
    QCOMPARE(byType->getMapName(), kBingRoad);
}

void UrlFactoryTest::_testGetMapProviderInvalid()
{
    QVERIFY(UrlFactory::getMapProviderFromQtMapId(-1) == nullptr);
    QVERIFY(UrlFactory::getMapProviderFromProviderType(QString()) == nullptr);
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QVERIFY(UrlFactory::getMapProviderFromProviderType(QStringLiteral("Nonexistent")) == nullptr);
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(UrlFactoryTest, TestLabel::Unit)
