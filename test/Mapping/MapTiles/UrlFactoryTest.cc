#include "UrlFactoryTest.h"

#include <cmath>
#include <numbers>

#include <QtCore/QMap>
#include <QtCore/QRect>
#include <QtCore/QScopeGuard>

#include "AppSettings.h"
#include "ElevationMapProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileSet.h"
#include "SettingsManager.h"

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
    const int id = UrlFactory::getMapIdFromProviderType(kBingRoad);
    QVERIFY(id > 0);
}

void UrlFactoryTest::_testProviderTypeFromMapIdRoundtrip()
{
    const QStringList types = UrlFactory::getProviderTypes();
    for (const auto& type : types) {
        const int id = UrlFactory::getMapIdFromProviderType(type);
        QVERIFY2(id > 0, qPrintable(QStringLiteral("Bad id for type: %1").arg(type)));
        const QString recovered = UrlFactory::getProviderTypeFromMapId(id);
        QCOMPARE(recovered, type);
    }
}

void UrlFactoryTest::_testMapIdFromEmptyType()
{
    QCOMPARE(UrlFactory::getMapIdFromProviderType(QString()), -1);
}

void UrlFactoryTest::_testMapIdFromInvalidType()
{
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QCOMPARE(UrlFactory::getMapIdFromProviderType(QStringLiteral("Nonexistent")), -1);
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testProviderTypeFromInvalidId()
{
    QVERIFY(UrlFactory::getProviderTypeFromMapId(-1).isEmpty());
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("map id not found:"));
    QVERIFY(UrlFactory::getProviderTypeFromMapId(99999).isEmpty());
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testMapIdsArePermanent()
{
    // The ids are keys of the tile cache. Add an entry for a new provider and remove none: a changed id relabels the
    // tiles users have cached, and a reused one hands them to another provider.
    const QMap<int, QString> permanentIds = {
        {1, QStringLiteral("Google Street Map")},    {2, QStringLiteral("Google Satellite")},
        {3, QStringLiteral("Google Terrain")},       {4, QStringLiteral("Google Hybrid")},
        {5, QStringLiteral("Google Labels")},        {6, QStringLiteral("Bing Road")},
        {7, QStringLiteral("Bing Satellite")},       {8, QStringLiteral("Bing Hybrid")},
        {9, QStringLiteral("TianDiTu Road")},        {10, QStringLiteral("TianDiTu Satellite")},
        {11, QStringLiteral("Statkart Topo")},       {12, QStringLiteral("Statkart Basemap")},
        {13, QStringLiteral("Svalbard Topo")},       {14, QStringLiteral("Eniro Topo")},
        {15, QStringLiteral("Esri World Street")},   {16, QStringLiteral("Esri World Satellite")},
        {17, QStringLiteral("Esri Terrain")},        {18, QStringLiteral("Mapbox Streets")},
        {19, QStringLiteral("Mapbox Light")},        {20, QStringLiteral("Mapbox Dark")},
        {21, QStringLiteral("Mapbox Satellite")},    {22, QStringLiteral("Mapbox Hybrid")},
        {23, QStringLiteral("Mapbox StreetsBasic")}, {24, QStringLiteral("Mapbox Outdoors")},
        {25, QStringLiteral("Mapbox Bright")},       {26, QStringLiteral("Mapbox Custom")},
        {27, QStringLiteral("MapQuest Map")},        {28, QStringLiteral("MapQuest Sat")},
        {29, QStringLiteral("VWorld Street Map")},   {30, QStringLiteral("VWorld Satellite Map")},
        {31, QStringLiteral("Japan-GSI Contour")},   {32, QStringLiteral("Japan-GSI Seamless")},
        {33, QStringLiteral("Japan-GSI Anaglyph")},  {34, QStringLiteral("Japan-GSI Slope")},
        {35, QStringLiteral("Japan-GSI Relief")},    {36, QStringLiteral("LINZ Basemap")},
        {37, QStringLiteral("Street Map")},          {38, QStringLiteral("OpenAIP")},
        {39, QStringLiteral("CustomURL Custom")},    {40, QStringLiteral("Copernicus")},
        {41, QStringLiteral("Terrarium")},
    };

    QMap<int, QString> ids;
    for (const SharedMapProvider& provider : UrlFactory::getProviders()) {
        QVERIFY2(!ids.contains(provider->getMapId()),
                 qPrintable(QStringLiteral("%1 and %2 share id %3")
                                .arg(ids.value(provider->getMapId()), provider->getMapName())
                                .arg(provider->getMapId())));
        ids.insert(provider->getMapId(), provider->getMapName());
    }

    QCOMPARE(ids.size(), permanentIds.size());
    for (auto it = permanentIds.cbegin(); it != permanentIds.cend(); ++it) {
        QCOMPARE(ids.value(it.key()), it.value());
    }
}

// --- Tile hash ---

void UrlFactoryTest::_testGetTileHashFormat()
{
    // "%010d%08d%08d%03d" of provider id, x, y and zoom. A cached tile is found by this key only.
    QCOMPARE(UrlFactory::getTileHash(kBingHybrid, 4270, 2917, 13), QStringLiteral("00000000080000427000002917013"));
}

void UrlFactoryTest::_testGetTileHashWithoutProvider()
{
    // The key has the id -1. Unlike getMapIdFromProviderType(), an empty name is reported too: the key is all the
    // caller gets.
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("provider not found for type:"));
    QCOMPARE(UrlFactory::getTileHash(QString(), 4270, 2917, 13), QStringLiteral("-0000000010000427000002917013"));
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testTileHashFollowsConfiguredSource_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<QString>("settingName");
    QTest::addColumn<QString>("sourceA");
    QTest::addColumn<QString>("sourceB");
    QTest::addColumn<QString>("hashA");

    // The 8 hex digits are those of the SHA-1 of the cache variant: the URL template, or account '/' style
    QTest::newRow("Custom URL") << QStringLiteral("CustomURL Custom") << QStringLiteral("customURL")
                                << QStringLiteral("https://a.example/{z}/{x}/{y}.png")
                                << QStringLiteral("https://b.example/{z}/{x}/{y}.png")
                                << QStringLiteral("00000000390000427000002917013-e16c3bed");
    QTest::newRow("Mapbox Custom account")
        << QStringLiteral("Mapbox Custom") << QStringLiteral("mapboxAccount") << QStringLiteral("alice")
        << QStringLiteral("bob") << QStringLiteral("00000000260000427000002917013-18742c8e");
    QTest::newRow("Mapbox Custom style") << QStringLiteral("Mapbox Custom") << QStringLiteral("mapboxStyle")
                                         << QStringLiteral("streets") << QStringLiteral("night")
                                         << QStringLiteral("00000000260000427000002917013-51d5ff8f");
}

void UrlFactoryTest::_testTileHashFollowsConfiguredSource()
{
    QFETCH(QString, providerType);
    QFETCH(QString, settingName);
    QFETCH(QString, sourceA);
    QFETCH(QString, sourceB);
    QFETCH(QString, hashA);

    Fact* const setting = SettingsManager::instance()->appSettings()->property(qPrintable(settingName)).value<Fact*>();
    QVERIFY(setting);
    const QVariant saved = setting->rawValue();
    const auto restore = qScopeGuard([setting, saved] { setting->setRawValue(saved); });

    setting->setRawValue(sourceA);
    QCOMPARE(UrlFactory::getTileHash(providerType, 4270, 2917, 13), hashA);
    // The key the cache worker computes, from the variant it is given
    QCOMPARE(UrlFactory::getTileHash(providerType, 4270, 2917, 13, UrlFactory::cacheVariant(providerType)), hashA);

    setting->setRawValue(sourceB);
    const QString hashB = UrlFactory::getTileHash(providerType, 4270, 2917, 13);
    QVERIFY2(hashB != hashA, qPrintable(hashB));
    QCOMPARE(hashB.left(29), hashA.left(29));

    setting->setRawValue(sourceA);
    QCOMPARE(UrlFactory::getTileHash(providerType, 4270, 2917, 13), hashA);
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
    const int id = UrlFactory::getMapIdFromProviderType(kBingRoad);
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
    const int id = UrlFactory::getMapIdFromProviderType(kBingRoad);
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

void UrlFactoryTest::_testTileUrlScheme_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<QString>("scheme");

    // A request carries the tile a user looks at and often a token (Esri: a header, others: the URL), so it goes
    // over HTTPS. The exceptions, each with what keeps it on HTTP:
    const QStringList plainHttp = {
        // otile[0-3].mqcdn.com are in no DNS: the service is gone, whatever the scheme
        QStringLiteral("MapQuest Map"),
        QStringLiteral("MapQuest Sat"),
        // api.vworld.kr has a certificate that verifies, but a request without a valid key got an HTTP 502 or no
        // answer over either scheme: that it serves tiles over HTTPS is not verified
        QStringLiteral("VWorld Street Map"),
        QStringLiteral("VWorld Satellite Map"),
    };

    for (const QString& type : UrlFactory::getProviderTypes()) {
        // The URL of this one is the user's
        if (type == QStringLiteral("CustomURL Custom")) {
            continue;
        }
        QTest::newRow(qPrintable(type)) << type
                                        << (plainHttp.contains(type) ? QStringLiteral("http")
                                                                     : QStringLiteral("https"));
    }
}

void UrlFactoryTest::_testTileUrlScheme()
{
    QFETCH(QString, providerType);
    QFETCH(QString, scheme);

    // Mapbox and TianDiTu have no URL without a token
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(appSettings->mapboxToken(), QStringLiteral("token"));
    saved.setFactValue(appSettings->tiandituToken(), QStringLiteral("token"));

    // A tile in Korea: VWorld has a URL for no other
    QCOMPARE(UrlFactory::getTileURL(providerType, 873, 396, 10).scheme(), scheme);
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
    const int id = UrlFactory::getMapIdFromProviderType(kCopernicus);
    QVERIFY(UrlFactory::isElevation(id));
}

void UrlFactoryTest::_testIsElevationFalse()
{
    const int id = UrlFactory::getMapIdFromProviderType(kBingRoad);
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
    const int id = UrlFactory::getMapIdFromProviderType(kBingRoad);
    auto provider = UrlFactory::getMapProviderFromMapId(id);
    QVERIFY(provider != nullptr);
    QCOMPARE(provider->getMapName(), kBingRoad);
    QVERIFY(provider->isBingProvider());

    auto byType = UrlFactory::getMapProviderFromProviderType(kBingRoad);
    QVERIFY(byType != nullptr);
    QCOMPARE(byType->getMapName(), kBingRoad);
}

void UrlFactoryTest::_testGetMapProviderInvalid()
{
    QVERIFY(UrlFactory::getMapProviderFromMapId(-1) == nullptr);
    QVERIFY(UrlFactory::getMapProviderFromProviderType(QString()) == nullptr);
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QVERIFY(UrlFactory::getMapProviderFromProviderType(QStringLiteral("Nonexistent")) == nullptr);
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(UrlFactoryTest, TestLabel::Unit)
