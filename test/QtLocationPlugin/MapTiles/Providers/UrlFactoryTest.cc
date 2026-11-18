#include "UrlFactoryTest.h"

#include <cmath>
#include <numbers>

#include <QtCore/QLocale>
#include <QtCore/QMap>
#include <QtCore/QRect>

#include "AppSettings.h"
#include "ElevationMapProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapProvider.h"
#include "QGCTileSet.h"
#include "SettingsManager.h"
#include "UrlFactory.h"

static const QString kBingRoad = QStringLiteral("Bing Road");
static const QString kBingSatellite = QStringLiteral("Bing Satellite");
static const QString kBingHybrid = QStringLiteral("Bing Hybrid");
static const QString kCopernicus = QStringLiteral("Copernicus");
static const QString kTerrarium = QStringLiteral("Terrarium");

/// What a provider is: a row for every provider there ever was
struct UrlFactoryTestProvider
{
    int id;
    const char* name;
    const char* referrer;
    const char* imageFormat;
    quint32 averageSize;
    MapProvider::MapStyle mapStyle;
    const char* attribution;
    const char* areaAttributionUrl;
    qsizetype sourceSettingCount;
    bool userSelectable;
    bool applicationUserAgent;
    bool elevation;
    /// The URLs of the tiles (873, 396) of zoom 10 and (27948, 12690) of zoom 15, with the tokens and the sources
    /// that _testProvider() sets, and {lang} for the language of the system
    const char* tileUrlZoom10;
    const char* tileUrlZoom15;
};

// clang-format off
static const UrlFactoryTestProvider kUrlFactoryTestProviders[] = {
    {1, "Google Street Map", "https://www.google.com/maps/preview", "png", 4913, MapProvider::StreetMap, "Google Maps",
     "", 0, true, false, false,
     "https://mt1.google.com/vt/lyrs=m&hl={lang}&x=873&y=396&z=10&s=Galileo&scale=1",
     "https://mt0.google.com/vt/lyrs=m&hl={lang}&x=27948&s=&y=12690&z=15&s=Galile&scale=1"},
    {2, "Google Satellite", "https://www.google.com/maps/preview", "jpg", 56887, MapProvider::SatelliteMapDay, "Google Maps",
     "", 0, true, false, false,
     "https://mt1.google.com/vt/lyrs=s&hl={lang}&x=873&y=396&z=10&s=Galileo&scale=1",
     "https://mt0.google.com/vt/lyrs=s&hl={lang}&x=27948&s=&y=12690&z=15&s=Galile&scale=1"},
    {3, "Google Terrain", "https://www.google.com/maps/preview", "png", 19391, MapProvider::TerrainMap, "Google Maps",
     "", 0, true, false, false,
     "https://mt1.google.com/vt/v=t,r&hl={lang}&x=873&y=396&z=10&s=Galileo&scale=1",
     "https://mt0.google.com/vt/v=t,r&hl={lang}&x=27948&s=&y=12690&z=15&s=Galile&scale=1"},
    {4, "Google Hybrid", "https://www.google.com/maps/preview", "png", 56887, MapProvider::HybridMap, "Google Maps",
     "", 0, true, false, false,
     "https://mt1.google.com/vt/lyrs=y&hl={lang}&x=873&y=396&z=10&s=Galileo&scale=1",
     "https://mt0.google.com/vt/lyrs=y&hl={lang}&x=27948&s=&y=12690&z=15&s=Galile&scale=1"},
    {5, "Google Labels", "https://www.google.com/maps/preview", "png", 13652, MapProvider::CustomMap, "Google Maps",
     "", 0, true, false, false,
     "https://mt1.google.com/vt/lyrs=h&hl={lang}&x=873&y=396&z=10&s=Galileo&scale=1",
     "https://mt0.google.com/vt/lyrs=h&hl={lang}&x=27948&s=&y=12690&z=15&s=Galile&scale=1"},
    {6, "Bing Road", "https://www.bing.com/maps/", "png", 1297, MapProvider::StreetMap, "© Microsoft",
     "", 0, true, false, false,
     "https://ecn.t1.tiles.virtualearth.net/tiles/r1321103201.png?g=2981&mkt={lang}",
     "https://ecn.t0.tiles.virtualearth.net/tiles/r132110320121120.png?g=2981&mkt={lang}"},
    {7, "Bing Satellite", "https://www.bing.com/maps/", "jpg", 19597, MapProvider::SatelliteMapDay, "© Microsoft",
     "", 0, true, false, false,
     "https://ecn.t1.tiles.virtualearth.net/tiles/a1321103201.jpg?g=2981&mkt={lang}",
     "https://ecn.t0.tiles.virtualearth.net/tiles/a132110320121120.jpg?g=2981&mkt={lang}"},
    {8, "Bing Hybrid", "https://www.bing.com/maps/", "jpg", 19597, MapProvider::HybridMap, "© Microsoft",
     "", 0, true, false, false,
     "https://ecn.t1.tiles.virtualearth.net/tiles/h1321103201.jpg?g=2981&mkt={lang}",
     "https://ecn.t0.tiles.virtualearth.net/tiles/h132110320121120.jpg?g=2981&mkt={lang}"},
    {9, "TianDiTu Road", "https://map.tianditu.gov.cn/", "png", 1297, MapProvider::StreetMap, "© Tianditu",
     "", 0, true, false, false,
     "https://t1.tianditu.gov.cn/DataServer?tk=tianditu-token&T=cia_w&x=873&y=396&l=10",
     "https://t0.tianditu.gov.cn/DataServer?tk=tianditu-token&T=cia_w&x=27948&y=12690&l=15"},
    {10, "TianDiTu Satellite", "https://map.tianditu.gov.cn/", "jpg", 19597, MapProvider::SatelliteMapDay, "© Tianditu",
     "", 0, true, false, false,
     "https://t1.tianditu.gov.cn/DataServer?tk=tianditu-token&T=img_w&x=873&y=396&l=10",
     "https://t0.tianditu.gov.cn/DataServer?tk=tianditu-token&T=img_w&x=27948&y=12690&l=15"},
    {11, "Statkart Topo", "https://norgeskart.no/", "png", 13652, MapProvider::StreetMap, "© Kartverket",
     "", 0, true, false, false,
     "https://cache.kartverket.no/v1/wmts/1.0.0/topo/default/webmercator/10/396/873.png",
     "https://cache.kartverket.no/v1/wmts/1.0.0/topo/default/webmercator/15/12690/27948.png"},
    {12, "Statkart Basemap", "https://norgeskart.no/", "png", 13652, MapProvider::StreetMap, "© Kartverket",
     "", 0, true, false, false,
     "https://cache.kartverket.no/v1/wmts/1.0.0/topo/default/webmercator/10/396/873.png",
     "https://cache.kartverket.no/v1/wmts/1.0.0/topo/default/webmercator/15/12690/27948.png"},
    {13, "Svalbard Topo", "https://www.npolar.no/", "png", 13652, MapProvider::StreetMap, "© Norwegian Polar Institute",
     "", 0, true, false, false,
     "https://geodata.npolar.no/arcgis/rest/services/Basisdata/NP_Basiskart_Svalbard_WMTS_3857/MapServer/WMTS/tile/1.0.0/Basisdata_NP_Basiskart_Svalbard_WMTS_3857/default/default028mm/10/396/873",
     "https://geodata.npolar.no/arcgis/rest/services/Basisdata/NP_Basiskart_Svalbard_WMTS_3857/MapServer/WMTS/tile/1.0.0/Basisdata_NP_Basiskart_Svalbard_WMTS_3857/default/default028mm/15/12690/27948"},
    {14, "Eniro Topo", "https://www.eniro.se/", "png", 13652, MapProvider::StreetMap, "© Eniro",
     "", 0, true, false, false,
     "https://map.eniro.com/geowebcache/service/tms1.0.0/map/10/873/627.png",
     "https://map.eniro.com/geowebcache/service/tms1.0.0/map/15/27948/20077.png"},
    {15, "Esri World Street", "", "", 13652, MapProvider::StreetMap, "Powered by Esri",
     "https://static.arcgis.com/attribution/World_Street_Map?f=json", 0, true, false, false,
     "https://services.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/10/396/873",
     "https://services.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/15/12690/27948"},
    {16, "Esri World Satellite", "", "", 13652, MapProvider::SatelliteMapDay, "Powered by Esri",
     "https://static.arcgis.com/attribution/World_Imagery?f=json", 0, true, false, false,
     "https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/10/396/873",
     "https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/15/12690/27948"},
    {17, "Esri Terrain", "", "", 13652, MapProvider::TerrainMap, "Powered by Esri",
     "", 0, true, false, false,
     "https://services.arcgisonline.com/ArcGIS/rest/services/World_Terrain_Base/MapServer/tile/10/396/873",
     "https://services.arcgisonline.com/ArcGIS/rest/services/World_Terrain_Base/MapServer/tile/15/12690/27948"},
    {18, "Mapbox Streets", "https://www.mapbox.com/", "jpg", 5648, MapProvider::StreetMap, "© Mapbox © OpenStreetMap",
     "", 0, true, false, false,
     "https://api.mapbox.com/styles/v1/mapbox/streets-v10/tiles/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/mapbox/streets-v10/tiles/15/27948/12690?access_token=mapbox-token"},
    {19, "Mapbox Light", "https://www.mapbox.com/", "jpg", 13652, MapProvider::CustomMap, "© Mapbox © OpenStreetMap",
     "", 0, true, false, false,
     "https://api.mapbox.com/styles/v1/mapbox/light-v9/tiles/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/mapbox/light-v9/tiles/15/27948/12690?access_token=mapbox-token"},
    {20, "Mapbox Dark", "https://www.mapbox.com/", "jpg", 13652, MapProvider::CustomMap, "© Mapbox © OpenStreetMap",
     "", 0, true, false, false,
     "https://api.mapbox.com/styles/v1/mapbox/dark-v9/tiles/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/mapbox/dark-v9/tiles/15/27948/12690?access_token=mapbox-token"},
    {21, "Mapbox Satellite", "https://www.mapbox.com/", "jpg", 15739, MapProvider::SatelliteMapDay, "© Mapbox © OpenStreetMap © Maxar",
     "", 0, true, false, false,
     "https://api.mapbox.com/styles/v1/mapbox/satellite-v9/tiles/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/mapbox/satellite-v9/tiles/15/27948/12690?access_token=mapbox-token"},
    {22, "Mapbox Hybrid", "https://www.mapbox.com/", "jpg", 15739, MapProvider::HybridMap, "© Mapbox © OpenStreetMap © Maxar",
     "", 0, true, false, false,
     "https://api.mapbox.com/styles/v1/mapbox/satellite-streets-v10/tiles/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/mapbox/satellite-streets-v10/tiles/15/27948/12690?access_token=mapbox-token"},
    {23, "Mapbox StreetsBasic", "https://www.mapbox.com/", "jpg", 13652, MapProvider::StreetMap, "© Mapbox © OpenStreetMap",
     "", 0, true, false, false,
     "https://api.mapbox.com/styles/v1/mapbox/basic-v9/tiles/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/mapbox/basic-v9/tiles/15/27948/12690?access_token=mapbox-token"},
    {24, "Mapbox Outdoors", "https://www.mapbox.com/", "jpg", 13652, MapProvider::CustomMap, "© Mapbox © OpenStreetMap",
     "", 0, true, false, false,
     "https://api.mapbox.com/styles/v1/mapbox/outdoors-v10/tiles/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/mapbox/outdoors-v10/tiles/15/27948/12690?access_token=mapbox-token"},
    {25, "Mapbox Bright", "https://www.mapbox.com/", "jpg", 13652, MapProvider::CustomMap, "© Mapbox © OpenStreetMap",
     "", 0, true, false, false,
     "https://api.mapbox.com/styles/v1/mapbox/bright-v9/tiles/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/mapbox/bright-v9/tiles/15/27948/12690?access_token=mapbox-token"},
    {26, "Mapbox Custom", "https://www.mapbox.com/", "jpg", 13652, MapProvider::CustomMap, "© Mapbox © OpenStreetMap",
     "", 2, true, false, false,
     "https://api.mapbox.com/styles/v1/account/style/tiles/256/10/873/396?access_token=mapbox-token",
     "https://api.mapbox.com/styles/v1/account/style/tiles/256/15/27948/12690?access_token=mapbox-token"},
    {27, "MapQuest Map", "https://mapquest.com", "jpg", 13652, MapProvider::StreetMap, "© MapQuest",
     "", 0, true, false, false,
     "http://otile1.mqcdn.com/tiles/1.0.0/map/10/873/396.jpg",
     "http://otile0.mqcdn.com/tiles/1.0.0/map/15/27948/12690.jpg"},
    {28, "MapQuest Sat", "https://mapquest.com", "jpg", 13652, MapProvider::SatelliteMapDay, "© MapQuest",
     "", 0, true, false, false,
     "http://otile1.mqcdn.com/tiles/1.0.0/sat/10/873/396.jpg",
     "http://otile0.mqcdn.com/tiles/1.0.0/sat/15/27948/12690.jpg"},
    {29, "VWorld Street Map", "www.vworld.kr", "png", 13652, MapProvider::StreetMap, "© VWorld",
     "", 0, true, false, false,
     "http://api.vworld.kr/req/wmts/1.0.0/vworld-token/Base/10/396/873.png",
     "http://api.vworld.kr/req/wmts/1.0.0/vworld-token/Base/15/12690/27948.png"},
    {30, "VWorld Satellite Map", "www.vworld.kr", "jpeg", 13652, MapProvider::SatelliteMapDay, "© VWorld",
     "", 0, true, false, false,
     "http://api.vworld.kr/req/wmts/1.0.0/vworld-token/Satellite/10/396/873.jpeg",
     "http://api.vworld.kr/req/wmts/1.0.0/vworld-token/Satellite/15/12690/27948.jpeg"},
    {31, "Japan-GSI Contour", "https://cyberjapandata.gsi.go.jp/xyz/std", "png", 13652, MapProvider::StreetMap, "Source: Geospatial Information Authority of Japan",
     "", 0, true, false, false,
     "https://cyberjapandata.gsi.go.jp/xyz/std/10/873/396.png",
     "https://cyberjapandata.gsi.go.jp/xyz/std/15/27948/12690.png"},
    {32, "Japan-GSI Seamless", "https://cyberjapandata.gsi.go.jp/xyz/std", "jpg", 13652, MapProvider::SatelliteMapDay, "Source: Geospatial Information Authority of Japan",
     "", 0, true, false, false,
     "https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto/10/873/396.jpg",
     "https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto/15/27948/12690.jpg"},
    {33, "Japan-GSI Anaglyph", "https://cyberjapandata.gsi.go.jp/xyz/std", "png", 13652, MapProvider::StreetMap, "Source: Geospatial Information Authority of Japan",
     "", 0, true, false, false,
     "https://cyberjapandata.gsi.go.jp/xyz/anaglyphmap_color/10/873/396.png",
     "https://cyberjapandata.gsi.go.jp/xyz/anaglyphmap_color/15/27948/12690.png"},
    {34, "Japan-GSI Slope", "https://cyberjapandata.gsi.go.jp/xyz/std", "png", 13652, MapProvider::StreetMap, "Source: Geospatial Information Authority of Japan",
     "", 0, true, false, false,
     "https://cyberjapandata.gsi.go.jp/xyz/slopemap/10/873/396.png",
     "https://cyberjapandata.gsi.go.jp/xyz/slopemap/15/27948/12690.png"},
    {35, "Japan-GSI Relief", "https://cyberjapandata.gsi.go.jp/xyz/std", "png", 13652, MapProvider::StreetMap, "Source: Geospatial Information Authority of Japan",
     "", 0, true, false, false,
     "https://cyberjapandata.gsi.go.jp/xyz/relief/10/873/396.png",
     "https://cyberjapandata.gsi.go.jp/xyz/relief/15/27948/12690.png"},
    {36, "LINZ Basemap", "https://basemaps.linz.govt.nz/v1/tiles/aerial", "png", 13652, MapProvider::SatelliteMapDay, "© LINZ CC BY 4.0 © Imagery Basemap contributors",
     "", 0, true, false, false,
     "https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/10/873/396.png?api=d01ev80nqcjxddfvc6amyvkk1ka",
     "https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/15/27948/12690.png?api=d01ev80nqcjxddfvc6amyvkk1ka"},
    {37, "Street Map", "", "png", 13652, MapProvider::StreetMap, "© OpenStreetMap contributors",
     "", 0, true, true, false,
     "https://tile.openstreetmap.org/10/873/396.png",
     "https://tile.openstreetmap.org/15/27948/12690.png"},
    {38, "OpenAIP", "https://www.openaip.net", "png", 13652, MapProvider::CustomMap, "© openAIP (CC BY-NC 4.0)",
     "", 0, true, false, false,
     "https://api.tiles.openaip.net/api/data/openaip/10/873/396.png?apiKey=openaip-token",
     "https://api.tiles.openaip.net/api/data/openaip/15/27948/12690.png?apiKey=openaip-token"},
    {39, "CustomURL Custom", "", "", 13652, MapProvider::CustomMap, "",
     "", 1, true, false, false,
     "https://example.com/10/873/396.png?zoom=10",
     "https://example.com/15/27948/12690.png?zoom=15"},
    {40, "Copernicus", "https://terrain-ce.suite.auterion.com", "bin", 2786, MapProvider::TerrainMap, "© Airbus Defence and Space GmbH",
     "", 0, true, false, true,
     "https://terrain-ce.suite.auterion.com/api/v1/carpet?points=-86.04,-171.27,-86.03,-171.26",
     "https://terrain-ce.suite.auterion.com/api/v1/carpet?points=36.9,99.48,36.91,99.49"},
    {41, "Terrarium", "https://s3.amazonaws.com/elevation-tiles-prod", "png", 45000, MapProvider::TerrainMap, "Terrain: Mapzen, USGS, NOAA and other sources",
     "", 0, false, false, true,
     "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/10/873/396.png",
     "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/15/27948/12690.png"},
};
// clang-format on

static SharedMapProvider urlFactoryTestProvider(const QString& type)
{
    return UrlFactory::getMapProviderFromProviderType(type);
}

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
    QVERIFY(!elevTypes.contains(kBingRoad));

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
    expectLogMessage("MapTiles.UrlFactory", QtWarningMsg, QRegularExpression("type not found:"));
    QCOMPARE(UrlFactory::getMapIdFromProviderType(QStringLiteral("Nonexistent")), -1);
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testProviderTypeFromInvalidId()
{
    QVERIFY(UrlFactory::getProviderTypeFromMapId(-1).isEmpty());
    expectLogMessage("MapTiles.UrlFactory", QtWarningMsg, QRegularExpression("map id not found:"));
    QVERIFY(UrlFactory::getProviderTypeFromMapId(99999).isEmpty());
    verifyExpectedLogMessage();
}

void UrlFactoryTest::_testMapIdsArePermanent()
{
    // The ids are keys of the tile cache. Add a row to kUrlFactoryTestProviders for a new provider and remove none: a
    // changed id relabels the tiles users have cached, and a reused one hands them to another provider.
    QMap<int, QString> ids;
    for (const SharedMapProvider& provider : UrlFactory::getProviders()) {
        QVERIFY2(!ids.contains(provider->getMapId()),
                 qPrintable(QStringLiteral("%1 and %2 share id %3")
                                .arg(ids.value(provider->getMapId()), provider->getMapName())
                                .arg(provider->getMapId())));
        ids.insert(provider->getMapId(), provider->getMapName());
    }

    for (const UrlFactoryTestProvider& expected : kUrlFactoryTestProviders) {
        // The build for iOS has no Google maps
        if (ids.contains(expected.id) || !QString::fromUtf8(expected.name).startsWith(QStringLiteral("Google "))) {
            QCOMPARE(ids.take(expected.id), QString::fromUtf8(expected.name));
        }
    }
    QVERIFY2(ids.isEmpty(), qPrintable(QStringLiteral("not in kUrlFactoryTestProviders: %1")
                                           .arg(QStringList(ids.values()).join(QStringLiteral(", ")))));
}

// --- What each provider is ---

void UrlFactoryTest::_testProvider_data()
{
    QTest::addColumn<int>("row");

    for (qsizetype row = 0; row < std::ssize(kUrlFactoryTestProviders); row++) {
        if (UrlFactory::findProvider(QString::fromUtf8(kUrlFactoryTestProviders[row].name))) {
            QTest::newRow(kUrlFactoryTestProviders[row].name) << static_cast<int>(row);
        }
    }
}

void UrlFactoryTest::_testProvider()
{
    QFETCH(int, row);
    const UrlFactoryTestProvider& expected = kUrlFactoryTestProviders[row];
    const SharedMapProvider provider = UrlFactory::findProvider(QString::fromUtf8(expected.name));
    QVERIFY(provider);

    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(appSettings->mapboxToken(), QStringLiteral("mapbox-token"));
    saved.setFactValue(appSettings->mapboxAccount(), QStringLiteral("account"));
    saved.setFactValue(appSettings->mapboxStyle(), QStringLiteral("style"));
    saved.setFactValue(appSettings->tiandituToken(), QStringLiteral("tianditu-token"));
    saved.setFactValue(appSettings->vworldToken(), QStringLiteral("vworld-token"));
    saved.setFactValue(appSettings->openaipToken(), QStringLiteral("openaip-token"));
    saved.setFactValue(appSettings->esriToken(), QStringLiteral("esri-token"));
    saved.setFactValue(appSettings->customURL(), QStringLiteral("https://example.com/{z}/{x}/{y}.png?zoom={zoom}"));

    QCOMPARE(provider->getMapId(), expected.id);
    QCOMPARE(provider->getReferrer(), QString::fromUtf8(expected.referrer));
    // What is no picture has the format of the provider
    QCOMPARE(provider->getImageFormat(QByteArrayView("xxx", 3)), QString::fromUtf8(expected.imageFormat));
    QCOMPARE(provider->getAverageSize(), expected.averageSize);
    QCOMPARE(provider->getMapStyle(), expected.mapStyle);
    QCOMPARE(provider->attribution(), QString::fromUtf8(expected.attribution));
    QCOMPARE(provider->areaAttributionUrl(), QUrl(QString::fromUtf8(expected.areaAttributionUrl)));
    QCOMPARE(provider->sourceSettings().size(), expected.sourceSettingCount);
    QCOMPARE(provider->isUserSelectable(), expected.userSelectable);
    QCOMPARE(provider->requiresApplicationUserAgent(), expected.applicationUserAgent);
    QCOMPARE(provider->isElevationProvider(), expected.elevation);
    // Esri takes its token in a header of the request
    const bool esri = QString::fromUtf8(expected.name).startsWith(QStringLiteral("Esri "));
    QCOMPARE(provider->getToken(), esri ? QByteArrayLiteral("esri-token") : QByteArray());

    // A tile in Korea, where VWorld has tiles, and one whose row has the 5 digits that change a Google URL
    const QString language = !QLocale::system().uiLanguages().isEmpty() ? QLocale::system().uiLanguages().constFirst()
                                                                        : QStringLiteral("en");
    const auto expectedUrl = [&language](const char* url) {
        return QString::fromUtf8(url).replace(QStringLiteral("{lang}"), language);
    };
    QCOMPARE(provider->getTileURL(873, 396, 10).toString(), expectedUrl(expected.tileUrlZoom10));
    QCOMPARE(provider->getTileURL(27948, 12690, 15).toString(), expectedUrl(expected.tileUrlZoom15));
}

// --- Tile hash ---

void UrlFactoryTest::_testGetTileHashFormat()
{
    // "%010d%08d%08d%03d" of provider id, x, y and zoom. A cached tile is found by this key only.
    QCOMPARE(urlFactoryTestProvider(kBingHybrid)->getTileHash(4270, 2917, 13),
             QStringLiteral("00000000080000427000002917013"));
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
    TestFixtures::SettingsFixture saved;

    const SharedMapProvider provider = urlFactoryTestProvider(providerType);
    QVERIFY(provider);

    saved.setFactValue(setting, sourceA);
    QCOMPARE(provider->getTileHash(4270, 2917, 13), hashA);
    // The key the cache worker computes, from the variant it is given
    QCOMPARE(provider->getTileHash(4270, 2917, 13, provider->cacheVariant()), hashA);

    saved.setFactValue(setting, sourceB);
    const QString hashB = provider->getTileHash(4270, 2917, 13);
    QVERIFY2(hashB != hashA, qPrintable(hashB));
    QCOMPARE(hashB.left(29), hashA.left(29));

    saved.setFactValue(setting, sourceA);
    QCOMPARE(provider->getTileHash(4270, 2917, 13), hashA);
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
    QCOMPARE(urlFactoryTestProvider(providerType)->getTileURL(873, 396, 10).scheme(), scheme);
}

// --- Copernicus elevation specifics ---

void UrlFactoryTest::_testCopernicusLong2tileX()
{
    const SharedMapProvider copernicus = urlFactoryTestProvider(kCopernicus);
    // floor((0+180)/0.01) = 18000
    QCOMPARE(copernicus->long2tileX(0.0, 1), 18000);
    // floor((-180+180)/0.01) = 0
    QCOMPARE(copernicus->long2tileX(-180.0, 1), 0);
}

void UrlFactoryTest::_testCopernicusLat2tileY()
{
    const SharedMapProvider copernicus = urlFactoryTestProvider(kCopernicus);
    // floor((0+90)/0.01) = 9000
    QCOMPARE(copernicus->lat2tileY(0.0, 1), 9000);
    // floor((-90+90)/0.01) = 0
    QCOMPARE(copernicus->lat2tileY(-90.0, 1), 0);
}

void UrlFactoryTest::_testCopernicusTileCount()
{
    // Small 0.05° box near origin. The rows of Copernicus count northwards: the first row of the set is that of the
    // bottom edge.
    const auto set = urlFactoryTestProvider(kCopernicus)->getTileCount(1, 0.0, 0.05, 0.05, 0.0);
    // tileX: floor((0+180)/0.01)=18000 to floor((0.05+180)/0.01)=18005 → 6 tiles
    // tileY: floor((0+90)/0.01)=9000 to floor((0.05+90)/0.01)=9005 → 6 tiles
    // total = 6*6 = 36
    QCOMPARE(set.tileCount, static_cast<quint64>(36));
    QCOMPARE(set.tileY0, 9000);
    QCOMPARE(set.tileY1, 9005);
}

// --- Terrarium elevation provider ---

void UrlFactoryTest::_testTerrariumRegistered()
{
    QVERIFY(UrlFactory::getElevationProviderTypes().contains(kTerrarium));

    auto provider = UrlFactory::getMapProviderFromProviderType(kTerrarium);
    QVERIFY(provider != nullptr);
    QVERIFY(provider->isElevationProvider());
    // Unrecognized payload falls back to the provider's declared format
    QCOMPARE(provider->getImageFormat(QByteArrayView("xxx", 3)), QStringLiteral("png"));
}

void UrlFactoryTest::_testTerrariumTileURL()
{
    const SharedMapProvider terrarium = urlFactoryTestProvider(kTerrarium);
    const QUrl url = terrarium->getTileURL(1308, 2865, 13);
    QCOMPARE(url.toString(),
             QStringLiteral("https://s3.amazonaws.com/elevation-tiles-prod/terrarium/13/1308/2865.png"));

    // Standard slippy tile math, unlike Copernicus's fixed 0.01° grid
    QCOMPARE(terrarium->long2tileX(0.0, 1), 1);
    QCOMPARE(terrarium->lat2tileY(0.0, 1), 1);
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
    // The columns run on past the last one, 2047: the region has the columns 2046, 2047, 0 and 1
    QTest::newRow("across the antimeridian")
        << QRect(QPoint(2046, 729), QPoint(2049, 730)) << QRect(QPoint(1022, 364), QPoint(1025, 365));
}

void UrlFactoryTest::_testTerrariumAnchorTiles()
{
    QFETCH(QRect, regionTiles);
    QFETCH(QRect, anchorTiles);

    constexpr int kAnchorZoom = TerrariumElevationProvider::kAnchorZoom;
    constexpr int kRegionZoom = kAnchorZoom + 1;
    constexpr int kRegionColumns = 1 << kRegionZoom;
    const double tileCount = kRegionColumns;
    const auto tileCenterLon = [tileCount](int x) {
        return (((x % kRegionColumns) + 0.5) / tileCount * 360.0) - 180.0;
    };
    const auto tileCenterLat = [tileCount](int y) {
        return std::atan(std::sinh(std::numbers::pi * (1.0 - (2.0 * (y + 0.5) / tileCount)))) * 180.0 /
               std::numbers::pi;
    };
    const double leftLon = tileCenterLon(regionTiles.left());
    const double topLat = tileCenterLat(regionTiles.top());
    const double rightLon = tileCenterLon(regionTiles.right());
    const double bottomLat = tileCenterLat(regionTiles.bottom());

    const SharedMapProvider terrarium = urlFactoryTestProvider(kTerrarium);
    const QGCTileSet region = terrarium->getTileCount(kRegionZoom, leftLon, topLat, rightLon, bottomLat);
    QCOMPARE(QRect(QPoint(region.tileX0, region.tileY0), QPoint(region.tileX1, region.tileY1)), regionTiles);

    const QGCTileSet anchors = terrarium->getTileCount(kAnchorZoom, leftLon, topLat, rightLon, bottomLat);
    QCOMPARE(QRect(QPoint(anchors.tileX0, anchors.tileY0), QPoint(anchors.tileX1, anchors.tileY1)), anchorTiles);
    const quint64 anchorCount = static_cast<quint64>(anchorTiles.width()) * static_cast<quint64>(anchorTiles.height());
    QCOMPARE(anchors.tileCount, anchorCount);
    QCOMPARE(anchors.tileSize, anchorCount * TerrariumElevationProvider::kAvgElevSize);
}

// --- getTileCount ---

void UrlFactoryTest::_testGetTileCountZoomClamped()
{
    const SharedMapProvider provider = urlFactoryTestProvider(kBingRoad);

    // zoom=0 should be clamped to 1 internally
    const auto setLow = provider->getTileCount(0, -179.9, 85.0, 179.9, -85.0);
    const auto setOne = provider->getTileCount(1, -179.9, 85.0, 179.9, -85.0);
    QCOMPARE(setOne.tileCount, static_cast<quint64>(4));
    QCOMPARE(setLow.tileCount, setOne.tileCount);

    // zoom=99 should be clamped to 23
    const auto setHigh = provider->getTileCount(99, 0.0, 0.001, 0.001, 0.0);
    const auto set23 = provider->getTileCount(23, 0.0, 0.001, 0.001, 0.0);
    QCOMPARE(setHigh.tileCount, set23.tileCount);
}

// --- Provider lookup ---

void UrlFactoryTest::_testGetMapProviderValid()
{
    const int id = UrlFactory::getMapIdFromProviderType(kBingRoad);
    auto provider = UrlFactory::getMapProviderFromMapId(id);
    QVERIFY(provider != nullptr);
    QCOMPARE(provider->getMapName(), kBingRoad);

    auto byType = UrlFactory::getMapProviderFromProviderType(kBingRoad);
    QVERIFY(byType != nullptr);
    QCOMPARE(byType->getMapName(), kBingRoad);
}

void UrlFactoryTest::_testGetMapProviderInvalid()
{
    QVERIFY(UrlFactory::getMapProviderFromMapId(-1) == nullptr);
    QVERIFY(UrlFactory::getMapProviderFromProviderType(QString()) == nullptr);
    expectLogMessage("MapTiles.UrlFactory", QtWarningMsg, QRegularExpression("type not found:"));
    QVERIFY(UrlFactory::getMapProviderFromProviderType(QStringLiteral("Nonexistent")) == nullptr);
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(UrlFactoryTest, TestLabel::Unit)
