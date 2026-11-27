#include "MapProviderRegistryTest.h"

#include <cmath>
#include <limits>
#include <numbers>

#include <QtCore/QLocale>
#include <QtCore/QMap>
#include <QtCore/QRect>
#include <QtCore/QRegularExpression>

#include "AppSettings.h"
#include "ElevationMapProvider.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "SettingsManager.h"
#include "TileRegion.h"

static const QString kBingRoad = QStringLiteral("Bing Road");
static const QString kBingHybrid = QStringLiteral("Bing Hybrid");
static const QString kCopernicus = QStringLiteral("Copernicus");
static const QString kTerrarium = QStringLiteral("Terrarium");

/// What a provider is: a row for every provider there ever was
struct MapProviderRegistryTestProvider
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
static const MapProviderRegistryTestProvider kMapProviderRegistryTestProviders[] = {
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
     "", 0, false, false, false,
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
    // For GeoMap only: not offered where the user selects the elevation provider or a map
    {41, "Terrarium", "https://s3.amazonaws.com/elevation-tiles-prod", "png", 45000, MapProvider::TerrainMap, "Terrain: Mapzen, USGS, NOAA and other sources",
     "", 0, false, false, true,
     "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/10/873/396.png",
     "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/15/27948/12690.png"},
    // The tiles that the user imported, which no server has
    {42, "MBTiles Imported", "", "", 13652, MapProvider::CustomMap, "",
     "", 0, true, false, false,
     "",
     ""},
};
// clang-format on

static SharedMapProvider mapProviderRegistryTestProvider(const QString& type)
{
    return MapProviderRegistry::getMapProviderFromProviderType(type);
}

/// The language of the user interface of the system, as a request for a tile names it
static QString mapProviderRegistryTestSystemLanguage()
{
    const QStringList languages = QLocale::system().uiLanguages();
    const QLocale locale = languages.isEmpty() ? QLocale(QLocale::English) : QLocale(languages.constFirst());
    return locale.name().replace(QLatin1Char('_'), QLatin1Char('-'));
}

/// A provider whose tiles have labels in the language of a setting of the test
class MapProviderRegistryTestLabelProvider final : public MapProvider
{
public:
    explicit MapProviderRegistryTestLabelProvider(const Fact* languageSetting)
        : MapProvider(QStringLiteral("Labels"), QString(), QStringLiteral("png"))
        , _languageSetting(languageSetting)
    {}

    const Fact* labelLanguageSetting() const final { return _languageSetting; }

private:
    QString _getURL(int x, int y, int zoom) const final
    {
        return QStringLiteral("https://example.com/%1/%2/%3?language=%4").arg(zoom).arg(x).arg(y).arg(_labelLanguage());
    }

    const Fact* const _languageSetting;
};

// --- Provider registry ---

void MapProviderRegistryTest::_testGetElevationProviderTypes()
{
    const QStringList elevTypes = MapProviderRegistry::elevationProviderTypes();
    QVERIFY(!elevTypes.isEmpty());
    QVERIFY(elevTypes.contains(kCopernicus));
    QVERIFY(elevTypes.contains(kTerrarium));
    QVERIFY(!elevTypes.contains(kBingRoad));

    const QStringList allTypes = MapProviderRegistry::providerTypes();
    for (const auto& t : elevTypes) {
        QVERIFY2(allTypes.contains(t), qPrintable(QStringLiteral("Elevation type '%1' not in allTypes").arg(t)));
    }
}

// --- Type <-> MapId roundtrip ---

void MapProviderRegistryTest::_testProviderTypeFromMapIdRoundtrip()
{
    const QStringList types = MapProviderRegistry::providerTypes();
    for (const auto& type : types) {
        const int id = MapProviderRegistry::getMapIdFromProviderType(type);
        QVERIFY2(id > 0, qPrintable(QStringLiteral("Bad id for type: %1").arg(type)));
        const QString recovered = MapProviderRegistry::getProviderTypeFromMapId(id);
        QCOMPARE(recovered, type);
    }
}

// No name and no id are none to look up. A name or an id that no provider has is worth a warning.
void MapProviderRegistryTest::_testLookupOfUnknownProvider()
{
    QCOMPARE(MapProviderRegistry::getMapIdFromProviderType(QString()), -1);
    QVERIFY(MapProviderRegistry::getProviderTypeFromMapId(-1).isEmpty());
    QVERIFY(MapProviderRegistry::getMapProviderFromProviderType(QString()) == nullptr);
    QVERIFY(MapProviderRegistry::getMapProviderFromMapId(-1) == nullptr);

    expectLogMessage("MapTiles.MapProviderRegistry", QtWarningMsg, QRegularExpression("type not found:"));
    QCOMPARE(MapProviderRegistry::getMapIdFromProviderType(QStringLiteral("Nonexistent")), -1);
    verifyExpectedLogMessage();

    expectLogMessage("MapTiles.MapProviderRegistry", QtWarningMsg, QRegularExpression("type not found:"));
    QVERIFY(MapProviderRegistry::getMapProviderFromProviderType(QStringLiteral("Nonexistent")) == nullptr);
    verifyExpectedLogMessage();

    expectLogMessage("MapTiles.MapProviderRegistry", QtWarningMsg, QRegularExpression("map id not found:"));
    QVERIFY(MapProviderRegistry::getProviderTypeFromMapId(99999).isEmpty());
    verifyExpectedLogMessage();
}

void MapProviderRegistryTest::_testMapIdsArePermanent()
{
    // The ids are keys of the tile cache. Add a row to kMapProviderRegistryTestProviders for a new provider and remove
    // none: a changed id relabels the tiles users have cached, and a reused one hands them to another provider.
    QMap<int, QString> ids;
    for (const SharedMapProvider& provider : MapProviderRegistry::providers()) {
        QVERIFY2(!ids.contains(provider->mapId()),
                 qPrintable(QStringLiteral("%1 and %2 share id %3")
                                .arg(ids.value(provider->mapId()), provider->mapName())
                                .arg(provider->mapId())));
        ids.insert(provider->mapId(), provider->mapName());
    }

    for (const MapProviderRegistryTestProvider& expected : kMapProviderRegistryTestProviders) {
        // The build for iOS has no Google maps
        if (ids.contains(expected.id) || !QString::fromUtf8(expected.name).startsWith(QStringLiteral("Google "))) {
            QCOMPARE(ids.take(expected.id), QString::fromUtf8(expected.name));
        }
    }
    QVERIFY2(ids.isEmpty(), qPrintable(QStringLiteral("not in kMapProviderRegistryTestProviders: %1")
                                           .arg(QStringList(ids.values()).join(QStringLiteral(", ")))));
}

// --- What each provider is ---

void MapProviderRegistryTest::_testProvider_data()
{
    QTest::addColumn<int>("row");

    for (qsizetype row = 0; row < std::ssize(kMapProviderRegistryTestProviders); row++) {
        if (MapProviderRegistry::findProvider(QString::fromUtf8(kMapProviderRegistryTestProviders[row].name))) {
            QTest::newRow(kMapProviderRegistryTestProviders[row].name) << static_cast<int>(row);
        }
    }
}

void MapProviderRegistryTest::_testProvider()
{
    QFETCH(int, row);
    const MapProviderRegistryTestProvider& expected = kMapProviderRegistryTestProviders[row];
    const SharedMapProvider provider = MapProviderRegistry::findProvider(QString::fromUtf8(expected.name));
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

    QCOMPARE(provider->mapId(), expected.id);
    QCOMPARE(provider->referrer(), QString::fromUtf8(expected.referrer));
    // What is no picture has the format of the provider
    QCOMPARE(provider->getImageFormat(QByteArrayView("xxx", 3)), QString::fromUtf8(expected.imageFormat));
    QCOMPARE(provider->averageSize(), expected.averageSize);
    QCOMPARE(provider->mapStyle(), expected.mapStyle);
    QCOMPARE(provider->attribution(), QString::fromUtf8(expected.attribution));
    QCOMPARE(provider->areaAttributionUrl(), QUrl(QString::fromUtf8(expected.areaAttributionUrl)));
    QCOMPARE(provider->sourceSettings().size(), expected.sourceSettingCount);
    QCOMPARE(provider->isUserSelectable(), expected.userSelectable);
    QCOMPARE(provider->requiresApplicationUserAgent(), expected.applicationUserAgent);
    QCOMPARE(provider->isElevationProvider(), expected.elevation);
    // Esri takes its token in a header of the request
    const bool esri = QString::fromUtf8(expected.name).startsWith(QStringLiteral("Esri "));
    QCOMPARE(provider->token(), esri ? QByteArrayLiteral("esri-token") : QByteArray());

    // A tile in Korea, where VWorld has tiles, and one whose row has the 5 digits that change a Google URL
    const QString language = mapProviderRegistryTestSystemLanguage();
    const auto expectedUrl = [&language](const char* url) {
        return QString::fromUtf8(url).replace(QStringLiteral("{lang}"), language);
    };
    QCOMPARE(provider->getTileURL(873, 396, 10).toString(), expectedUrl(expected.tileUrlZoom10));
    QCOMPARE(provider->getTileURL(27948, 12690, 15).toString(), expectedUrl(expected.tileUrlZoom15));
}

// --- Tile hash ---

void MapProviderRegistryTest::_testGetTileHashFormat()
{
    // "%010d%08d%08d%03d" of provider id, x, y and zoom. A cached tile is found by this key only.
    QCOMPARE(mapProviderRegistryTestProvider(kBingHybrid)->getTileHash(4270, 2917, 13),
             QStringLiteral("00000000080000427000002917013"));
}

void MapProviderRegistryTest::_testTileHashFollowsConfiguredSource_data()
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

void MapProviderRegistryTest::_testTileHashFollowsConfiguredSource()
{
    QFETCH(QString, providerType);
    QFETCH(QString, settingName);
    QFETCH(QString, sourceA);
    QFETCH(QString, sourceB);
    QFETCH(QString, hashA);

    Fact* const setting = SettingsManager::instance()->appSettings()->property(qPrintable(settingName)).value<Fact*>();
    QVERIFY(setting);
    TestFixtures::SettingsFixture saved;

    const SharedMapProvider provider = mapProviderRegistryTestProvider(providerType);
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

void MapProviderRegistryTest::_testLabelLanguageProviders()
{
    // The maps of Google and Bing with labels. Their satellite maps have none.
    const QStringList labelled = {QStringLiteral("Google Street Map"), QStringLiteral("Google Terrain"),
                                  QStringLiteral("Google Hybrid"),     QStringLiteral("Google Labels"),
                                  QStringLiteral("Bing Road"),         QStringLiteral("Bing Hybrid")};
    Fact* const languageSetting = SettingsManager::instance()->appSettings()->qLocaleLanguage();

    for (const SharedMapProvider& provider : MapProviderRegistry::providers()) {
        const bool hasLabelLanguage = labelled.contains(provider->mapName());
        QVERIFY2(provider->labelLanguageSetting() == (hasLabelLanguage ? languageSetting : nullptr),
                 qPrintable(provider->mapName()));
        // The maps get other tiles when a setting of this list changes
        QVERIFY2(provider->cacheVariantSettings().contains(languageSetting) == hasLabelLanguage,
                 qPrintable(provider->mapName()));
    }
}

void MapProviderRegistryTest::_testLabelLanguage()
{
    const QString systemLanguage = mapProviderRegistryTestSystemLanguage();
    const QLocale::Language systemLanguageId = QLocale(systemLanguage).language();
    const bool systemIsGerman = systemLanguageId == QLocale::German;
    const QLocale::Language otherLanguageId = systemIsGerman ? QLocale::French : QLocale::German;
    const QString otherLanguage = systemIsGerman ? QStringLiteral("fr-FR") : QStringLiteral("de-DE");

    Fact languageSetting(0, QStringLiteral("language"), FactMetaData::valueTypeUint32);
    const MapProviderRegistryTestLabelProvider provider(&languageSetting);
    const QString plainHash = provider.getTileHash(4270, 2917, 13, QString());

    // A language and a territory, as "de-DE", or "C". Not with the script that QLocale::uiLanguages() names first, as
    // "de-Latn-DE": Bing answers that with labels in English.
    static const QRegularExpression languageTag(QStringLiteral("^(?:[a-z]{2,3}-(?:[A-Z]{2}|[0-9]{3})|C)$"));
    QVERIFY2(languageTag.match(systemLanguage).hasMatch(), qPrintable(systemLanguage));

    // QGC in the language of the system: the keys that the tiles always had, so the tiles in the cache stay in use
    for (const QLocale::Language language : {QLocale::AnyLanguage, systemLanguageId}) {
        languageSetting.setRawValue(language);
        QCOMPARE(provider.cacheVariant(), QString());
        QCOMPARE(provider.getTileHash(4270, 2917, 13), plainHash);
        QCOMPARE(provider.getTileURL(4270, 2917, 13).toString(),
                 QStringLiteral("https://example.com/13/4270/2917?language=%1").arg(systemLanguage));
    }

    // QGC in another language: tiles with labels in it, which have keys of their own
    languageSetting.setRawValue(otherLanguageId);
    QCOMPARE(provider.cacheVariant(), otherLanguage);
    QCOMPARE(provider.getTileHash(4270, 2917, 13), plainHash + MapProvider::cacheVariantSuffix(otherLanguage));
    QCOMPARE(provider.getTileURL(4270, 2917, 13).toString(),
             QStringLiteral("https://example.com/13/4270/2917?language=%1").arg(otherLanguage));
}

void MapProviderRegistryTest::_testTileUrlScheme_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<QString>("scheme");

    // A request carries the tile a user looks at and often a token (Esri: a header, others: the URL), so it goes
    // over HTTPS. The exceptions, each with what keeps it on HTTP:
    const QStringList plainHttp = {
        // api.vworld.kr has a certificate that verifies, but a request without a valid key got an HTTP 502 or no
        // answer over either scheme: that it serves tiles over HTTPS is not verified
        QStringLiteral("VWorld Street Map"),
        QStringLiteral("VWorld Satellite Map"),
    };

    for (const QString& type : MapProviderRegistry::providerTypes()) {
        // The URL of the first is the user's, and the second has no server
        if ((type == QStringLiteral("CustomURL Custom")) || (type == QStringLiteral("MBTiles Imported"))) {
            continue;
        }
        QTest::newRow(qPrintable(type)) << type
                                        << (plainHttp.contains(type) ? QStringLiteral("http")
                                                                     : QStringLiteral("https"));
    }
}

void MapProviderRegistryTest::_testTileUrlScheme()
{
    QFETCH(QString, providerType);
    QFETCH(QString, scheme);

    // Mapbox, TianDiTu and VWorld have no URL without a token, and Mapbox Custom none without what names its map
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(appSettings->mapboxToken(), QStringLiteral("token"));
    saved.setFactValue(appSettings->mapboxAccount(), QStringLiteral("account"));
    saved.setFactValue(appSettings->mapboxStyle(), QStringLiteral("style"));
    saved.setFactValue(appSettings->tiandituToken(), QStringLiteral("token"));
    saved.setFactValue(appSettings->vworldToken(), QStringLiteral("token"));

    // A tile in Korea: VWorld has a URL for no other
    QCOMPARE(mapProviderRegistryTestProvider(providerType)->getTileURL(873, 396, 10).scheme(), scheme);
}

void MapProviderRegistryTest::_testBingTileUrl_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::addColumn<int>("zoom");
    QTest::addColumn<int>("server");
    QTest::addColumn<QString>("quadKey");

    // The quarters of the world, each from a server of its own
    QTest::newRow("northwest quarter") << 0 << 0 << 1 << 0 << QStringLiteral("0");
    QTest::newRow("northeast quarter") << 1 << 0 << 1 << 1 << QStringLiteral("1");
    QTest::newRow("southwest quarter") << 0 << 1 << 1 << 2 << QStringLiteral("2");
    QTest::newRow("southeast quarter") << 1 << 1 << 1 << 3 << QStringLiteral("3");
    // Column 011 and row 101: the quarters 2, 1 and 3 from the top down. The servers are counted around: 13 is 1.
    QTest::newRow("tile of the third level") << 3 << 5 << 3 << 1 << QStringLiteral("213");
}

// Bing names a tile by its place in a quadtree, and has four servers that the tiles are spread over
void MapProviderRegistryTest::_testBingTileUrl()
{
    QFETCH(int, x);
    QFETCH(int, y);
    QFETCH(int, zoom);
    QFETCH(int, server);
    QFETCH(QString, quadKey);

    QCOMPARE(mapProviderRegistryTestProvider(QStringLiteral("Bing Road"))->getTileURL(x, y, zoom).toString(),
             QStringLiteral("https://ecn.t%1.tiles.virtualearth.net/tiles/r%2.png?g=2981&mkt=%3")
                 .arg(server)
                 .arg(quadKey, mapProviderRegistryTestSystemLanguage()));
}

void MapProviderRegistryTest::_testMapsOfServiceNameIt_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<QString>("otherProviderType");
    QTest::addColumn<bool>("sameService");

    // Maps whose credits differ, which the same servers serve all the same
    QTest::newRow("Bing road and satellite") << QStringLiteral("Bing Road") << QStringLiteral("Bing Satellite") << true;
    QTest::newRow("Mapbox streets and satellite")
        << QStringLiteral("Mapbox Streets") << QStringLiteral("Mapbox Satellite") << true;
    QTest::newRow("Mapbox streets and custom")
        << QStringLiteral("Mapbox Streets") << QStringLiteral("Mapbox Custom") << true;
    QTest::newRow("Esri street and terrain")
        << QStringLiteral("Esri World Street") << QStringLiteral("Esri Terrain") << true;
    QTest::newRow("Bing and OpenStreetMap") << QStringLiteral("Bing Road") << QStringLiteral("Street Map") << false;
    QTest::newRow("the elevation providers") << kCopernicus << kTerrarium << false;
}

// The pauses that a server asks for hold the maps that name its service (TileServerPauses)
void MapProviderRegistryTest::_testMapsOfServiceNameIt()
{
    QFETCH(QString, providerType);
    QFETCH(QString, otherProviderType);
    QFETCH(bool, sameService);

    const QString service = mapProviderRegistryTestProvider(providerType)->service();
    QVERIFY(!service.isEmpty());
    QCOMPARE(mapProviderRegistryTestProvider(otherProviderType)->service() == service, sameService);
}

void MapProviderRegistryTest::_testNoTileUrlWithoutSetting_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<QString>("settingName");

    QTest::newRow("VWorld token") << QStringLiteral("VWorld Street Map") << QStringLiteral("vworldToken");
    QTest::newRow("Mapbox Custom account") << QStringLiteral("Mapbox Custom") << QStringLiteral("mapboxAccount");
    QTest::newRow("Mapbox Custom style") << QStringLiteral("Mapbox Custom") << QStringLiteral("mapboxStyle");
}

// The server refuses a request that lacks what the setting gives: none is sent, as for a map that lacks its token
void MapProviderRegistryTest::_testNoTileUrlWithoutSetting()
{
    QFETCH(QString, providerType);
    QFETCH(QString, settingName);

    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const setting = appSettings->property(qPrintable(settingName)).value<Fact*>();
    QVERIFY(setting);
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(appSettings->mapboxToken(), QStringLiteral("token"));
    saved.setFactValue(appSettings->mapboxAccount(), QStringLiteral("account"));
    saved.setFactValue(appSettings->mapboxStyle(), QStringLiteral("style"));
    saved.setFactValue(appSettings->vworldToken(), QStringLiteral("token"));

    // A tile in Korea: VWorld has a URL for no other
    const SharedMapProvider provider = mapProviderRegistryTestProvider(providerType);
    QVERIFY(!provider->getTileURL(873, 396, 10).isEmpty());

    saved.setFactValue(setting, QString());
    QVERIFY(provider->getTileURL(873, 396, 10).isEmpty());
}

// VWorld has the tiles of Korea, of the zoom levels 6 to 19. An offline set and a map that is zoomed in further
// stop at the deepest zoom level that the map tells of, so that must be the last one with a URL.
void MapProviderRegistryTest::_testVWorldTileUrls()
{
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->appSettings()->vworldToken(), QStringLiteral("token"));
    const SharedMapProvider provider = mapProviderRegistryTestProvider(QStringLiteral("VWorld Satellite Map"));

    constexpr int kFirstZoom = 6;
    constexpr int kDeepestZoom = 19;
    QCOMPARE(provider->maxZoom(), kDeepestZoom);
    constexpr double kSeoulLon = 127.0;
    constexpr double kSeoulLat = 37.5;
    for (int zoom = 0; zoom <= QGC_MAX_MAP_ZOOM; zoom++) {
        const QUrl url =
            provider->getTileURL(provider->long2tileX(kSeoulLon, zoom), provider->lat2tileY(kSeoulLat, zoom), zoom);
        QVERIFY2(url.isEmpty() == ((zoom < kFirstZoom) || (zoom > kDeepestZoom)), qPrintable(QString::number(zoom)));
    }

    // The columns 53 and 54 and the rows 22 to 25 of the first zoom level
    for (int x = 52; x <= 55; x++) {
        for (int y = 21; y <= 26; y++) {
            const bool inKorea = (x >= 53) && (x <= 54) && (y >= 22) && (y <= 25);
            QVERIFY2(provider->getTileURL(x, y, kFirstZoom).isEmpty() != inKorea,
                     qPrintable(QStringLiteral("%1, %2").arg(x).arg(y)));
        }
    }
    // A zoom level below, where the area has a margin of a tile to the east and to the south
    QVERIFY(provider->getTileURL(105, 44, kFirstZoom + 1).isEmpty());
    QVERIFY(!provider->getTileURL(106, 44, kFirstZoom + 1).isEmpty());
    QVERIFY(!provider->getTileURL(111, 53, kFirstZoom + 1).isEmpty());
    QVERIFY(provider->getTileURL(112, 53, kFirstZoom + 1).isEmpty());
    QVERIFY(provider->getTileURL(111, 54, kFirstZoom + 1).isEmpty());
}

// --- Copernicus elevation specifics ---

void MapProviderRegistryTest::_testCopernicusTileOfCoordinate()
{
    const SharedMapProvider copernicus = mapProviderRegistryTestProvider(kCopernicus);
    // floor((0+180)/0.01) = 18000
    QCOMPARE(copernicus->long2tileX(0.0, 1), 18000);
    // floor((-180+180)/0.01) = 0
    QCOMPARE(copernicus->long2tileX(-180.0, 1), 0);
    // floor((0+90)/0.01) = 9000
    QCOMPARE(copernicus->lat2tileY(0.0, 1), 9000);
    // floor((-90+90)/0.01) = 0
    QCOMPARE(copernicus->lat2tileY(-90.0, 1), 0);

    // The edges of the grid for what lies beyond them
    QCOMPARE(copernicus->long2tileX(-1e300, 1), 0);
    QCOMPARE(copernicus->long2tileX(1e300, 1), 36000);
    QCOMPARE(copernicus->lat2tileY(-1e300, 1), 0);
    QCOMPARE(copernicus->lat2tileY(1e300, 1), 18000);
}

void MapProviderRegistryTest::_testCopernicusTileCount()
{
    // Small 0.05° box near origin. The rows of Copernicus count northwards: the first row of the set is that of the
    // bottom edge.
    const auto set = mapProviderRegistryTestProvider(kCopernicus)->getTileCount(1, 0.0, 0.05, 0.05, 0.0);
    // tileX: floor((0+180)/0.01)=18000 to floor((0.05+180)/0.01)=18005 → 6 tiles
    // tileY: floor((0+90)/0.01)=9000 to floor((0.05+90)/0.01)=9005 → 6 tiles
    // total = 6*6 = 36
    QCOMPARE(set.tileCount, static_cast<quint64>(36));
    QCOMPARE(set.tileY0, 9000);
    QCOMPARE(set.tileY1, 9005);
}

// --- Terrarium elevation provider ---

void MapProviderRegistryTest::_testTerrariumAnchorTiles_data()
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

void MapProviderRegistryTest::_testTerrariumAnchorTiles()
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

    const SharedMapProvider terrarium = mapProviderRegistryTestProvider(kTerrarium);
    const TileRegion region = terrarium->getTileCount(kRegionZoom, leftLon, topLat, rightLon, bottomLat);
    QCOMPARE(QRect(QPoint(region.tileX0, region.tileY0), QPoint(region.tileX1, region.tileY1)), regionTiles);

    const TileRegion anchors = terrarium->getTileCount(kAnchorZoom, leftLon, topLat, rightLon, bottomLat);
    QCOMPARE(QRect(QPoint(anchors.tileX0, anchors.tileY0), QPoint(anchors.tileX1, anchors.tileY1)), anchorTiles);
    const quint64 anchorCount = static_cast<quint64>(anchorTiles.width()) * static_cast<quint64>(anchorTiles.height());
    QCOMPARE(anchors.tileCount, anchorCount);
    QCOMPARE(anchors.tileSize, anchorCount * TerrariumElevationProvider::kAvgElevSize);
}

// A map view gives a corner beyond the map as an invalid coordinate: the region has no tiles, and so no anchors
void MapProviderRegistryTest::_testTerrariumNoAnchorsWithoutTiles()
{
    constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
    const TileRegion anchors = mapProviderRegistryTestProvider(kTerrarium)
                                   ->getTileCount(TerrariumElevationProvider::kAnchorZoom, kNaN, 10.0, 20.0, -10.0);
    QCOMPARE(anchors.tileCount, quint64(0));
    QCOMPARE(anchors.tileSize, quint64(0));
}

// --- getTileCount ---

void MapProviderRegistryTest::_testGetTileCountZoomClamped()
{
    const SharedMapProvider provider = mapProviderRegistryTestProvider(kBingRoad);

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

void MapProviderRegistryTest::_testGetMapProviderValid()
{
    const int id = MapProviderRegistry::getMapIdFromProviderType(kBingRoad);
    auto provider = MapProviderRegistry::getMapProviderFromMapId(id);
    QVERIFY(provider != nullptr);
    QCOMPARE(provider->mapName(), kBingRoad);

    auto byType = MapProviderRegistry::getMapProviderFromProviderType(kBingRoad);
    QVERIFY(byType != nullptr);
    QCOMPARE(byType->mapName(), kBingRoad);
}

UT_REGISTER_TEST(MapProviderRegistryTest, TestLabel::Unit)
