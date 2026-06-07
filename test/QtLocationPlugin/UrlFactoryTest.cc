#include "UrlFactoryTest.h"

#include "BingMapProvider.h"
#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileSet.h"

static const QString kBingRoad = QStringLiteral("Bing Road");
static const QString kBingSatellite = QStringLiteral("Bing Satellite");
static const QString kBingHybrid = QStringLiteral("Bing Hybrid");
static const QString kCopernicus = QStringLiteral("Copernicus");

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
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QCOMPARE(UrlFactory::getQtMapIdFromProviderType(QStringLiteral("Nonexistent")), -1);
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testProviderTypeFromInvalidId()
{
    QVERIFY(UrlFactory::getProviderTypeFromQtMapId(-1).isEmpty());
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg,
                     QRegularExpression("provider not found from id:"));
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
    QCOMPARE(UrlFactory::hashFromProviderType(QString()), -1);
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QCOMPARE(UrlFactory::hashFromProviderType(QStringLiteral("NoSuchProvider")), -1);
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testProviderTypeFromInvalidHash()
{
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg,
                     QRegularExpression("provider not found from hash:"));
    QVERIFY(UrlFactory::providerTypeFromHash(0).isEmpty());
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testRetinaTileHashDisjointFrom1x()
{
    // In a headless unit test useRetinaTiles() is false (no QGuiApplication DPR > 1),
    // so getTileHash() yields the 1x hash. The @2x cache key reserves a disjoint
    // hash space by adding kRetinaHashOffset (1e9) to the provider-hash field, per
    // QGCMapUrlEngine.cpp. Reproduce that here to assert the spaces never collide and
    // both decode back to the same provider.
    constexpr int kRetinaHashOffset = 1000000000;

    const int x = 100;
    const int y = 200;
    const int z = 5;

    const QString hash1x = UrlFactory::getTileHash(kBingRoad, x, y, z);
    QCOMPARE(hash1x.length(), 29);

    const int baseHash = UrlFactory::hashFromProviderType(kBingRoad);
    QVERIFY(baseHash > 0);
    const QString hash2x = QString::asprintf("%010d%08d%08d%03d", baseHash + kRetinaHashOffset, x, y, z);

    // Disjoint: a 1x tile and its @2x counterpart never share a cache key.
    QVERIFY(hash1x != hash2x);

    // Both keys decode back to the same provider (offset is stripped on lookup).
    QCOMPARE(UrlFactory::tileHashToType(hash1x), kBingRoad);
    QCOMPARE(UrlFactory::tileHashToType(hash2x), kBingRoad);

    // The retina key sorts above the entire 1x mapId range, guaranteeing no overlap
    // with any other provider's 1x hash field.
    QVERIFY(hash2x.left(10).toInt() >= kRetinaHashOffset);
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
    QVERIFY(UrlFactory::tileHashToType(QStringLiteral("garbage")).isEmpty());
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg,
                     QRegularExpression("provider not found from hash:"));
    QVERIFY(UrlFactory::tileHashToType(QStringLiteral("0000000000garbagehash")).isEmpty());
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
    QCOMPARE(url.scheme(), QStringLiteral("https"));
}

void UrlFactoryTest::_testGetTileURLByMapId()
{
    const int id = UrlFactory::getQtMapIdFromProviderType(kBingRoad);
    QVERIFY(id > 0);
    const QUrl url = UrlFactory::getTileURL(id, 301, 385, 10);
    QVERIFY(url.isValid());
    QVERIFY(!url.isEmpty());
    QCOMPARE(url.scheme(), QStringLiteral("https"));
}

void UrlFactoryTest::_testGetTileURLUsesLayerSlugNotName()
{
    // Regression: MapQuest/VWorld must interpolate the layer slug (_mapTypeId),
    // not the display name (_mapName) which contains spaces.
    const QUrl mapQuest = UrlFactory::getTileURL(QStringLiteral("MapQuest Map"), 5, 6, 4);
    QVERIFY(!mapQuest.isEmpty());
    QVERIFY(mapQuest.path().contains(QStringLiteral("/map/")));
    QVERIFY(!mapQuest.toString().contains(QStringLiteral("MapQuest")));

    // VWorld serves only Korea; x/y must fall in its valid range at this zoom.
    const QUrl vworld = UrlFactory::getTileURL(QStringLiteral("VWorld Street Map"), 860, 400, 10);
    QVERIFY(!vworld.isEmpty());
    QVERIFY(vworld.toString().contains(QStringLiteral("/Base/")));
    QVERIFY(!vworld.toString().contains(QStringLiteral("Street Map")));
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
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QCOMPARE(UrlFactory::averageSizeForType(QStringLiteral("NonexistentForAvgSize")), QGC_AVERAGE_TILE_SIZE);
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
    QVERIFY(std::dynamic_pointer_cast<const BingMapProvider>(provider) != nullptr);

    auto byType = UrlFactory::getMapProviderFromProviderType(kBingRoad);
    QVERIFY(byType != nullptr);
    QCOMPARE(byType->getMapName(), kBingRoad);
}

void UrlFactoryTest::_testGetMapProviderInvalid()
{
    QVERIFY(UrlFactory::getMapProviderFromQtMapId(-1) == nullptr);
    QVERIFY(UrlFactory::getMapProviderFromProviderType(QString()) == nullptr);
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg, QRegularExpression("type not found:"));
    QVERIFY(UrlFactory::getMapProviderFromProviderType(QStringLiteral("NonexistentForProviderLookup")) == nullptr);
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testGetTileURLGoldenStrings()
{
    struct Golden
    {
        QString type;
        QString expected;
    };

    const QList<Golden> cases = {
        {QStringLiteral("Street Map"), QStringLiteral("https://tile.openstreetmap.org/10/301/385.png")},
        {QStringLiteral("Statkart Topo"),
         QStringLiteral("https://cache.kartverket.no/v1/wmts/1.0.0/topo4/default/webmercator/10/385/301.png")},
        {QStringLiteral("Statkart Basemap"),
         QStringLiteral(
             "https://cache.kartverket.no/v1/wmts/1.0.0/norgeskart_bakgrunn/default/webmercator/10/385/301.png")},
        {QStringLiteral("Svalbard Topo"),
         QStringLiteral("https://geodata.npolar.no/arcgis/rest/services/Basisdata/NP_Basiskart_Svalbard_WMTS_3857/"
                        "MapServer/WMTS/tile/1.0.0/Basisdata_NP_Basiskart_Svalbard_WMTS_3857/default/default028mm/"
                        "10/385/301")},
        {QStringLiteral("Eniro Topo"),
         QStringLiteral("https://map.eniro.com/geowebcache/service/tms1.0.0/map/10/301/638.png")},
        {QStringLiteral("MapQuest Map"), QStringLiteral("https://otile3.mqcdn.com/tiles/1.0.0/map/10/301/385.jpg")},
        {QStringLiteral("MapQuest Sat"), QStringLiteral("https://otile3.mqcdn.com/tiles/1.0.0/sat/10/301/385.jpg")},
        {QStringLiteral("Japan-GSI Contour"),
         QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std/10/301/385.png")},
        {QStringLiteral("Japan-GSI Seamless"),
         QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto/10/301/385.jpg")},
        {QStringLiteral("Japan-GSI Anaglyph"),
         QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/anaglyphmap_color/10/301/385.png")},
        {QStringLiteral("Japan-GSI Slope"),
         QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/slopemap/10/301/385.png")},
        {QStringLiteral("Japan-GSI Relief"),
         QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/relief/10/301/385.png")},
    };

    for (const auto& c : cases) {
        const QString actual = UrlFactory::getTileURL(c.type, 301, 385, 10).toString();
        QCOMPARE(actual, c.expected);
    }
}

UT_REGISTER_TEST(UrlFactoryTest, TestLabel::Unit)
