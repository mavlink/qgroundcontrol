#include "MapProviderTable.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>

#include "Fact.h"
#include "MapProvider.h"
#include "MapTilesSettings.h"

namespace {

class TableMapProvider;

/// What the maps of one service share
struct MapFamily
{
    /// Names the servers of the maps (MapProvider::service()): families with one name ask the same servers
    const char* service = nullptr;
    /// Makes the URL of a tile. An empty one for a tile that the service has not, or cannot be asked for. Null for
    /// maps without a server (MapProvider::hasServer()).
    QString (*url)(const TableMapProvider& map, int x, int y, int zoom);
    /// For TableMapProvider::templateUrl(): {layer}, {z}, {x}, {y}, {tmsY} and {format} are replaced
    const char* urlTemplate = nullptr;
    const char* referrer = "";
    const char* attribution = "";
    /// Where the service lists who else is credited for a map, with %1 for the layer of the map
    const char* areaAttributionUrl = nullptr;
    QByteArray (*token)() = nullptr;
    QList<const Fact*> (*sourceSettings)() = nullptr;
    /// The setting of the language of the labels of the tiles, for maps whose URL names it
    const Fact* (*labelLanguageSetting)() = nullptr;
    bool (*isNoTileImage)(QByteArrayView image) = nullptr;
    bool (*isRefusalImage)(QByteArrayView image) = nullptr;
    bool applicationUserAgent = false;
};

/// A map: a row of kMapRows
struct MapRow
{
    int id;
    const char* name;
    const MapFamily& family;
    /// What tells the service which of its maps is asked for
    const char* layer;
    const char* imageFormat;
    quint32 averageSize;
    MapProvider::MapStyle style;
    /// The deepest zoom level of which the service has tiles of the map anywhere, where that is known
    int maxZoom = QGC_MAX_MAP_ZOOM;
    /// True for a map in dark colors
    bool night = false;
    /// False for a map that stays for the tiles and the offline sets that users have of it, and is offered no more
    bool offered = true;
};

class TableMapProvider final : public MapProvider
{
public:
    explicit TableMapProvider(const MapRow& row)
        : MapProvider(QString::fromUtf8(row.name), QString::fromUtf8(row.family.referrer),
                      QString::fromUtf8(row.imageFormat), row.averageSize, row.style, row.id)
        , _row(row)
    {}

    bool requiresApplicationUserAgent() const final { return _row.family.applicationUserAgent; }

    QByteArray token() const final { return _row.family.token ? _row.family.token() : QByteArray(); }

    bool isRefusalImage(QByteArrayView image) const final
    {
        return _row.family.isRefusalImage && _row.family.isRefusalImage(image);
    }

    bool isNoTileImage(QByteArrayView image) const final
    {
        return _row.family.isNoTileImage && _row.family.isNoTileImage(image);
    }

    int maxZoom() const final { return _row.maxZoom; }

    bool isNight() const final { return _row.night; }

    bool isUserSelectable() const final { return _row.offered; }

    bool hasServer() const final { return _row.family.url != nullptr; }

    QString attribution() const final { return QString::fromUtf8(_row.family.attribution); }

    QUrl areaAttributionUrl() const final
    {
        return _row.family.areaAttributionUrl
                   ? QUrl(QString::fromUtf8(_row.family.areaAttributionUrl).arg(QLatin1StringView(_row.layer)))
                   : QUrl();
    }

    QString service() const final
    {
        return _row.family.service ? QString::fromUtf8(_row.family.service) : MapProvider::service();
    }

    QList<const Fact*> sourceSettings() const final
    {
        return _row.family.sourceSettings ? _row.family.sourceSettings() : QList<const Fact*>();
    }

    const Fact* labelLanguageSetting() const final
    {
        return _row.family.labelLanguageSetting ? _row.family.labelLanguageSetting() : nullptr;
    }

    static QString templateUrl(const TableMapProvider& map, int x, int y, int zoom);
    static QString googleUrl(const TableMapProvider& map, int x, int y, int zoom);
    static QString bingUrl(const TableMapProvider& map, int x, int y, int zoom);
    static QString tiandituUrl(const TableMapProvider& map, int x, int y, int zoom);
    static QString mapboxUrl(const TableMapProvider& map, int x, int y, int zoom);
    static QString mapboxCustomUrl(const TableMapProvider& map, int x, int y, int zoom);
    static QString vworldUrl(const TableMapProvider& map, int x, int y, int zoom);
    static QString openaipUrl(const TableMapProvider& map, int x, int y, int zoom);
    static QString customUrl(const TableMapProvider& map, int x, int y, int zoom);

    /// The "418 Access blocked" picture, which every tile request got while the requests did not follow the tile
    /// usage policy. The picture is not kept here, only what tells it: its size and its SHA-256.
    static bool osmRefusalImage(QByteArrayView image)
    {
        constexpr qsizetype kAccessBlockedSize = 6672;
        constexpr QByteArrayView kAccessBlockedSha256(
            "ed65ab236c17a3d866bd5e097c7a79b433b8bba2d590c8e5d3ff1649437c7ff1");
        return _hasSha256(image, kAccessBlockedSize, kAccessBlockedSha256);
    }

    /// The "Map data not yet available" picture, which Esri's servers send with the HTTP status of a tile where a map
    /// has none, as below the deepest zoom level it has of a place. Told as osmRefusalImage() tells its picture.
    static bool esriNoTileImage(QByteArrayView image)
    {
        constexpr qsizetype kNoDataSize = 2521;
        constexpr QByteArrayView kNoDataSha256("9eafd300d61393184a4abc1d458564cfd1cd9b6f9c4e9c74687045c0a0e5b858");
        return _hasSha256(image, kNoDataSize, kNoDataSha256);
    }

private:
    QString _getURL(int x, int y, int zoom) const final
    {
        return _row.family.url ? _row.family.url(*this, x, y, zoom) : QString();
    }

    QLatin1StringView _layer() const { return QLatin1StringView(_row.layer); }

    const MapRow& _row;
};

/// The key of a tile in the quadtree of Bing: a digit for each zoom level, which tells the quarter of the tile above
QString mapTableQuadKey(int x, int y, int zoom)
{
    QString quadKey;
    for (int i = zoom; i > 0; i--) {
        char digit = '0';
        const int mask = 1 << (i - 1);

        if ((x & mask) != 0) {
            digit++;
        }
        if ((y & mask) != 0) {
            digit += 2;
        }

        (void) quadKey.append(digit);
    }

    return quadKey;
}

/// Which of the \a serverCount servers of a service is asked for a tile: neighboring tiles ask different ones
int mapTableServerNumber(int x, int y, int serverCount)
{
    return (x + 2 * y) % serverCount;
}

/// Bing answers with this picture, not with an HTTP error, where it has no imagery
bool bingNoTileImage(QByteArrayView image)
{
    static const QByteArray noTileImage = [] {
        QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
        return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
    }();

    return !noTileImage.isEmpty() && (image == noTileImage);
}

QByteArray esriToken()
{
    return MapTilesSettings::esriToken().toUtf8();
}

/// The account and the style the user set
QList<const Fact*> mapboxCustomSourceSettings()
{
    return {MapTilesSettings::mapboxAccount(), MapTilesSettings::mapboxStyle()};
}

/// The URL template the user set
QList<const Fact*> customUrlSourceSettings()
{
    return {MapTilesSettings::customUrl()};
}

/// The language of QGC
const Fact* mapTableLanguageSetting()
{
    return MapTilesSettings::language();
}

#ifndef QGC_NO_GOOGLE_MAPS
// Google's layers: h roads only, m standard roadmap, p terrain, r altered roadmap, s satellite only, t terrain only,
// y hybrid (s,h). Its tiles are 256x256 pixels, to zoom 22, from mt0 to mt3.google.com.
constexpr MapFamily kMapFamilyGoogle = {
    .service = "Google",
    .url = TableMapProvider::googleUrl,
    .referrer = "https://www.google.com/maps/preview",
    .attribution = "Google Maps",
    .labelLanguageSetting = mapTableLanguageSetting,
};

// The satellite layer has no labels
constexpr MapFamily kMapFamilyGoogleSatellite = {
    .service = "Google",
    .url = TableMapProvider::googleUrl,
    .referrer = kMapFamilyGoogle.referrer,
    .attribution = kMapFamilyGoogle.attribution,
};
#endif

constexpr MapFamily kMapFamilyBing = {
    .service = "Bing",
    .url = TableMapProvider::bingUrl,
    .referrer = "https://www.bing.com/maps/",
    .attribution = "© Microsoft",
    .labelLanguageSetting = mapTableLanguageSetting,
    .isNoTileImage = bingNoTileImage,
};

// The aerial imagery has no labels
constexpr MapFamily kMapFamilyBingSatellite = {
    .service = "Bing",
    .url = TableMapProvider::bingUrl,
    .referrer = kMapFamilyBing.referrer,
    .attribution = kMapFamilyBing.attribution,
    .isNoTileImage = bingNoTileImage,
};

constexpr MapFamily kMapFamilyTianDiTu = {
    .service = "TianDiTu",
    .url = TableMapProvider::tiandituUrl,
    .referrer = "https://map.tianditu.gov.cn/",
    .attribution = "© Tianditu",
};

constexpr MapFamily kMapFamilyStatkart = {
    .service = "Kartverket",
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://cache.kartverket.no/v1/wmts/1.0.0/{layer}/default/webmercator/{z}/{y}/{x}.png",
    .referrer = "https://norgeskart.no/",
    .attribution = "© Kartverket",
};

constexpr MapFamily kMapFamilySvalbard = {
    .service = "Norwegian Polar Institute",
    .url = TableMapProvider::templateUrl,
    .urlTemplate =
        "https://geodata.npolar.no/arcgis/rest/services/Basisdata/NP_Basiskart_Svalbard_WMTS_3857/MapServer/WMTS/tile/"
        "1.0.0/Basisdata_NP_Basiskart_Svalbard_WMTS_3857/default/default028mm/{z}/{y}/{x}",
    .referrer = "https://www.npolar.no/",
    .attribution = "© Norwegian Polar Institute",
};

constexpr MapFamily kMapFamilyEniro = {
    .service = "Eniro",
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://map.eniro.com/geowebcache/service/tms1.0.0/map/{z}/{x}/{tmsY}.{format}",
    .referrer = "https://www.eniro.se/",
    .attribution = "© Eniro",
};

// Esri publishes a list of the contributors of a map service, with the areas and zoom levels that each is credited
// for. Its server has none for World_Terrain_Base.
constexpr MapFamily kMapFamilyEsri = {
    .service = "Esri",
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://services.arcgisonline.com/ArcGIS/rest/services/{layer}/MapServer/tile/{z}/{y}/{x}",
    .attribution = "Powered by Esri",
    .areaAttributionUrl = "https://static.arcgis.com/attribution/%1?f=json",
    .token = esriToken,
    .isNoTileImage = TableMapProvider::esriNoTileImage,
};

constexpr MapFamily kMapFamilyEsriWithoutList = {
    .service = "Esri",
    .url = TableMapProvider::templateUrl,
    .urlTemplate = kMapFamilyEsri.urlTemplate,
    .attribution = kMapFamilyEsri.attribution,
    .token = esriToken,
    .isNoTileImage = TableMapProvider::esriNoTileImage,
};

constexpr MapFamily kMapFamilyMapbox = {
    .service = "Mapbox",
    .url = TableMapProvider::mapboxUrl,
    .referrer = "https://www.mapbox.com/",
    .attribution = "© Mapbox © OpenStreetMap",
};

// Mapbox asks for Maxar's credit as well on its satellite styles
constexpr MapFamily kMapFamilyMapboxSatellite = {
    .service = "Mapbox",
    .url = TableMapProvider::mapboxUrl,
    .referrer = kMapFamilyMapbox.referrer,
    .attribution = "© Mapbox © OpenStreetMap © Maxar",
};

constexpr MapFamily kMapFamilyMapboxCustom = {
    .service = "Mapbox",
    .url = TableMapProvider::mapboxCustomUrl,
    .referrer = kMapFamilyMapbox.referrer,
    .attribution = kMapFamilyMapbox.attribution,
    .sourceSettings = mapboxCustomSourceSettings,
};

constexpr MapFamily kMapFamilyVWorld = {
    .service = "VWorld",
    .url = TableMapProvider::vworldUrl,
    .referrer = "www.vworld.kr",
    .attribution = "© VWorld",
};

constexpr MapFamily kMapFamilyJapan = {
    .service = "GSI Japan",
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://cyberjapandata.gsi.go.jp/xyz/{layer}/{z}/{x}/{y}.{format}",
    .referrer = "https://cyberjapandata.gsi.go.jp/xyz/std",
    .attribution = "Source: Geospatial Information Authority of Japan",
};

constexpr MapFamily kMapFamilyLinz = {
    .service = "LINZ",
    .url = TableMapProvider::templateUrl,
    .urlTemplate =
        "https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/{z}/{x}/{y}.{format}?api=d01ev80nqcjxddfvc6amyvkk1ka",
    .referrer = "https://basemaps.linz.govt.nz/v1/tiles/aerial",
    .attribution = "© LINZ CC BY 4.0 © Imagery Basemap contributors",
};

// The tile usage policy of the OpenStreetMap servers (https://operations.osmfoundation.org/policies/tiles/) asks for
// HTTPS, for the name of the application as the user agent and, of an application, for no Referer. The servers
// answer other requests with a picture of a refusal, with the HTTP status of a tile.
constexpr MapFamily kMapFamilyOpenStreetMap = {
    .service = "OpenStreetMap",
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
    .attribution = "© OpenStreetMap contributors",
    .isRefusalImage = TableMapProvider::osmRefusalImage,
    .applicationUserAgent = true,
};

constexpr MapFamily kMapFamilyOpenAip = {
    .service = "openAIP",
    .url = TableMapProvider::openaipUrl,
    .referrer = "https://www.openaip.net",
    .attribution = "© openAIP (CC BY-NC 4.0)",
};

constexpr MapFamily kMapFamilyCustomUrl = {
    .service = "Custom URL",
    .url = TableMapProvider::customUrl,
    .sourceSettings = customUrlSourceSettings,
};

// The tiles of the MBTiles files that the user imported (TileCacheDatabase::importMBTiles()), which are in the tile
// cache only
constexpr MapFamily kMapFamilyImported = {.url = nullptr};

constexpr quint32 kMapTableAverageSize = QGC_AVERAGE_TILE_SIZE;

// The first number is the permanent id of the provider (MapProvider::mapId()), a key of the tile cache:
// - never change or reuse one
// - a new provider takes the next unused number: 40 and 41 are the elevation providers (ElevationMapProvider.h)
// - the number of a removed provider stays retired: 27 and 28 were the maps of MapQuest, whose tile servers are gone
// The name is in the settings and in the offline sets that users have: it stays as it is, too.
// The last number, of the maps that have one, is the deepest zoom level of the map: its server answers a request for a
// deeper tile with HTTP 400, and that of the Japan-GSI maps with 404. The VWorld maps have no URL for a deeper tile.
constexpr MapRow kMapRows[] = {
#ifndef QGC_NO_GOOGLE_MAPS
    {1, "Google Street Map", kMapFamilyGoogle, "lyrs=m", "png", 4913, MapProvider::StreetMap, 22},
    {2, "Google Satellite", kMapFamilyGoogleSatellite, "lyrs=s", "jpg", 56887, MapProvider::SatelliteMapDay, 22},
    {3, "Google Terrain", kMapFamilyGoogle, "v=t,r", "png", 19391, MapProvider::TerrainMap, 22},
    {4, "Google Hybrid", kMapFamilyGoogle, "lyrs=y", "png", 56887, MapProvider::HybridMap, 22},
    {5, "Google Labels", kMapFamilyGoogle, "lyrs=h", "png", kMapTableAverageSize, MapProvider::CustomMap, 22},
#endif
    {6, "Bing Road", kMapFamilyBing, "r", "png", 1297, MapProvider::StreetMap},
    {7, "Bing Satellite", kMapFamilyBingSatellite, "a", "jpg", 19597, MapProvider::SatelliteMapDay},
    {8, "Bing Hybrid", kMapFamilyBing, "h", "jpg", 19597, MapProvider::HybridMap},

    {9, "TianDiTu Road", kMapFamilyTianDiTu, "cia_w", "png", 1297, MapProvider::StreetMap},
    {10, "TianDiTu Satellite", kMapFamilyTianDiTu, "img_w", "jpg", 19597, MapProvider::SatelliteMapDay},

    {11, "Statkart Topo", kMapFamilyStatkart, "topo", "png", kMapTableAverageSize, MapProvider::StreetMap, 18},
    // Kartverket's cache service has no base map layer: this map shows the tiles of Statkart Topo. It stays, as
    // cached tiles and offline sets carry its id, and is offered no more.
    {12, "Statkart Basemap", kMapFamilyStatkart, "topo", "png", kMapTableAverageSize, MapProvider::StreetMap, 18, false,
     false},
    {13, "Svalbard Topo", kMapFamilySvalbard, "", "png", kMapTableAverageSize, MapProvider::StreetMap, 15},

    {14, "Eniro Topo", kMapFamilyEniro, "", "png", kMapTableAverageSize, MapProvider::StreetMap, 20},

    {15, "Esri World Street", kMapFamilyEsri, "World_Street_Map", "", kMapTableAverageSize, MapProvider::StreetMap},
    {16, "Esri World Satellite", kMapFamilyEsri, "World_Imagery", "", kMapTableAverageSize,
     MapProvider::SatelliteMapDay},
    {17, "Esri Terrain", kMapFamilyEsriWithoutList, "World_Terrain_Base", "", kMapTableAverageSize,
     MapProvider::TerrainMap},

    {18, "Mapbox Streets", kMapFamilyMapbox, "streets-v10", "jpg", 5648, MapProvider::StreetMap},
    {19, "Mapbox Light", kMapFamilyMapbox, "light-v9", "jpg", kMapTableAverageSize, MapProvider::CustomMap},
    {20, "Mapbox Dark", kMapFamilyMapbox, "dark-v9", "jpg", kMapTableAverageSize, MapProvider::CustomMap,
     QGC_MAX_MAP_ZOOM, true},
    {21, "Mapbox Satellite", kMapFamilyMapboxSatellite, "satellite-v9", "jpg", 15739, MapProvider::SatelliteMapDay},
    {22, "Mapbox Hybrid", kMapFamilyMapboxSatellite, "satellite-streets-v10", "jpg", 15739, MapProvider::HybridMap},
    {23, "Mapbox StreetsBasic", kMapFamilyMapbox, "basic-v9", "jpg", kMapTableAverageSize, MapProvider::StreetMap},
    {24, "Mapbox Outdoors", kMapFamilyMapbox, "outdoors-v10", "jpg", kMapTableAverageSize, MapProvider::CustomMap},
    {25, "Mapbox Bright", kMapFamilyMapbox, "bright-v9", "jpg", kMapTableAverageSize, MapProvider::CustomMap},
    {26, "Mapbox Custom", kMapFamilyMapboxCustom, "", "jpg", kMapTableAverageSize, MapProvider::CustomMap},

    {29, "VWorld Street Map", kMapFamilyVWorld, "Base", "png", kMapTableAverageSize, MapProvider::StreetMap, 19},
    {30, "VWorld Satellite Map", kMapFamilyVWorld, "Satellite", "jpeg", kMapTableAverageSize,
     MapProvider::SatelliteMapDay, 19},

    {31, "Japan-GSI Contour", kMapFamilyJapan, "std", "png", kMapTableAverageSize, MapProvider::StreetMap, 18},
    {32, "Japan-GSI Seamless", kMapFamilyJapan, "seamlessphoto", "jpg", kMapTableAverageSize,
     MapProvider::SatelliteMapDay, 18},
    {33, "Japan-GSI Anaglyph", kMapFamilyJapan, "anaglyphmap_color", "png", kMapTableAverageSize,
     MapProvider::StreetMap, 16},
    {34, "Japan-GSI Slope", kMapFamilyJapan, "slopemap", "png", kMapTableAverageSize, MapProvider::StreetMap, 15},
    {35, "Japan-GSI Relief", kMapFamilyJapan, "relief", "png", kMapTableAverageSize, MapProvider::StreetMap, 15},

    {36, "LINZ Basemap", kMapFamilyLinz, "", "png", kMapTableAverageSize, MapProvider::SatelliteMapDay},

    {37, "Street Map", kMapFamilyOpenStreetMap, "", "png", kMapTableAverageSize, MapProvider::StreetMap, 19},

    {38, "OpenAIP", kMapFamilyOpenAip, "", "png", kMapTableAverageSize, MapProvider::CustomMap},

    {39, "CustomURL Custom", kMapFamilyCustomUrl, "", "", kMapTableAverageSize, MapProvider::CustomMap},

    {MapProviderTable::kImportedMapId, "MBTiles Imported", kMapFamilyImported, "", "", kMapTableAverageSize,
     MapProvider::CustomMap},
};

QString TableMapProvider::templateUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    QString url = QString::fromUtf8(map._row.family.urlTemplate);
    (void) url.replace(QLatin1StringView("{layer}"), map._layer());
    (void) url.replace(QLatin1StringView("{format}"), map._imageFormat);
    (void) url.replace(QLatin1StringView("{z}"), QString::number(zoom));
    (void) url.replace(QLatin1StringView("{x}"), QString::number(x));
    (void) url.replace(QLatin1StringView("{y}"), QString::number(y));
    // TMS counts the rows from the south
    (void) url.replace(QLatin1StringView("{tmsY}"), QString::number((1 << zoom) - 1 - y));
    return url;
}

QString TableMapProvider::googleUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    // After &x=... and after &s=...
    const QLatin1StringView sec1((y >= 10000) && (y < 100000) ? "&s=" : "");
    const QString sec2 = QStringLiteral("Galileo").left(((x * 3) + y) % 8);
    return QStringLiteral("https://mt%1.google.com/vt/%2&hl=%3&x=%4%5&y=%6&z=%7&s=%8&scale=1")
        .arg(mapTableServerNumber(x, y, 4))
        .arg(map._layer(), map._labelLanguage())
        .arg(x)
        .arg(sec1)
        .arg(y)
        .arg(zoom)
        .arg(sec2);
}

QString TableMapProvider::bingUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    return QStringLiteral("https://ecn.t%1.tiles.virtualearth.net/tiles/%2%3.%4?g=2981&mkt=%5")
        .arg(mapTableServerNumber(x, y, 4))
        .arg(map._layer(), mapTableQuadKey(x, y, zoom), map._imageFormat, map._labelLanguage());
}

QString TableMapProvider::tiandituUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    const QString token = MapTilesSettings::tiandituToken();
    if (token.isEmpty()) {
        return QString();
    }
    return QStringLiteral("https://t%1.tianditu.gov.cn/DataServer?tk=%2&T=%3&x=%4&y=%5&l=%6")
        .arg(mapTableServerNumber(x, y, 8))
        .arg(token, map._layer())
        .arg(x)
        .arg(y)
        .arg(zoom);
}

QString TableMapProvider::mapboxUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    const QString token = MapTilesSettings::mapboxToken();
    if (token.isEmpty()) {
        return QString();
    }
    return QStringLiteral("https://api.mapbox.com/styles/v1/mapbox/%1/tiles/%2/%3/%4?access_token=%5")
        .arg(map._layer())
        .arg(zoom)
        .arg(x)
        .arg(y)
        .arg(token);
}

QString TableMapProvider::mapboxCustomUrl(const TableMapProvider&, int x, int y, int zoom)
{
    const QString token = MapTilesSettings::mapboxToken();
    const QString account = MapTilesSettings::mapboxAccount()->rawValue().toString();
    const QString style = MapTilesSettings::mapboxStyle()->rawValue().toString();
    if (token.isEmpty() || account.isEmpty() || style.isEmpty()) {
        return QString();
    }
    return QStringLiteral("https://api.mapbox.com/styles/v1/%1/%2/tiles/256/%3/%4/%5?access_token=%6")
        .arg(account, style)
        .arg(zoom)
        .arg(x)
        .arg(y)
        .arg(token);
}

QString TableMapProvider::vworldUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    // VWorld has the tiles of Korea only: at its first zoom level those of the columns 53 and 54 and of the rows 22
    // to 25, and at a deeper one the tiles of these, with a margin to the east and to the south
    constexpr int kFirstZoom = 6;
    constexpr int kFirstColumn = 53;
    constexpr int kEndColumn = 55;
    constexpr int kFirstRow = 22;
    constexpr int kEndRow = 26;

    if ((zoom < kFirstZoom) || (zoom > map.maxZoom())) {
        return QString();
    }

    const int depth = zoom - kFirstZoom;
    const int margin = (2 * depth) - 1;
    if ((x < (kFirstColumn << depth)) || (x > ((kEndColumn << depth) + margin))) {
        return QString();
    }
    if ((y < (kFirstRow << depth)) || (y > ((kEndRow << depth) + margin))) {
        return QString();
    }

    const QString token = MapTilesSettings::vworldToken();
    if (token.isEmpty()) {
        return QString();
    }
    return QStringLiteral("http://api.vworld.kr/req/wmts/1.0.0/%1/%2/%3/%4/%5.%6")
        .arg(token, map._layer())
        .arg(zoom)
        .arg(y)
        .arg(x)
        .arg(map._imageFormat);
}

QString TableMapProvider::openaipUrl(const TableMapProvider&, int x, int y, int zoom)
{
    QString url = QStringLiteral("https://api.tiles.openaip.net/api/data/openaip/%1/%2/%3.png").arg(zoom).arg(x).arg(y);

    const QString apiKey = MapTilesSettings::openaipToken();
    if (!apiKey.isEmpty()) {
        url += QStringLiteral("?apiKey=%1").arg(apiKey);
    }

    return url;
}

QString TableMapProvider::customUrl(const TableMapProvider&, int x, int y, int zoom)
{
    QString url = MapTilesSettings::customUrl()->rawValue().toString();
    (void) url.replace("{x}", QString::number(x));
    (void) url.replace("{y}", QString::number(y));
    static const QRegularExpression zoomRegExp("\\{(z|zoom)\\}");
    (void) url.replace(zoomRegExp, QString::number(zoom));
    return url;
}

}  // namespace

namespace MapProviderTable {

QList<std::shared_ptr<const MapProvider>> providers()
{
    QList<std::shared_ptr<const MapProvider>> providers;
    for (const MapRow& row : kMapRows) {
        providers.append(std::make_shared<TableMapProvider>(row));
    }
    return providers;
}

}  // namespace MapProviderTable
