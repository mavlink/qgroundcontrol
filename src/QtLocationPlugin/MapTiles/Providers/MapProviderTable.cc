#include "MapProviderTable.h"

#include <cmath>

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>

#include "AppSettings.h"
#include "MapProvider.h"
#include "SettingsManager.h"

namespace {

class TableMapProvider;

/// What the maps of one service share
struct MapFamily
{
    /// Makes the URL of a tile. An empty one for a tile that the service has not, or cannot be asked for.
    QString (*url)(const TableMapProvider& map, int x, int y, int zoom);
    /// For TableMapProvider::templateUrl(): {layer}, {z}, {x}, {y}, {tmsY}, {format} and {server4} are replaced
    const char* urlTemplate = nullptr;
    const char* referrer = "";
    const char* attribution = "";
    /// Where the service lists who else is credited for a map, with %1 for the layer of the map
    const char* areaAttributionUrl = nullptr;
    QByteArray (*token)() = nullptr;
    QList<Fact*> (*sourceSettings)() = nullptr;
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

    QByteArray getToken() const final { return _row.family.token ? _row.family.token() : QByteArray(); }

    bool isRefusalImage(QByteArrayView image) const final
    {
        return _row.family.isRefusalImage && _row.family.isRefusalImage(image);
    }

    bool isNoTileImage(QByteArrayView image) const final
    {
        return _row.family.isNoTileImage && _row.family.isNoTileImage(image);
    }

    QString attribution() const final { return QString::fromUtf8(_row.family.attribution); }

    QUrl areaAttributionUrl() const final
    {
        return _row.family.areaAttributionUrl
                   ? QUrl(QString::fromUtf8(_row.family.areaAttributionUrl).arg(QLatin1StringView(_row.layer)))
                   : QUrl();
    }

    QList<Fact*> sourceSettings() const final
    {
        return _row.family.sourceSettings ? _row.family.sourceSettings() : QList<Fact*>();
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

private:
    QString _getURL(int x, int y, int zoom) const final { return _row.family.url(*this, x, y, zoom); }

    QLatin1StringView _layer() const { return QLatin1StringView(_row.layer); }

    const MapRow& _row;
};

AppSettings* mapTableAppSettings()
{
    return SettingsManager::instance()->appSettings();
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
    return mapTableAppSettings()->esriToken()->rawValue().toString().toUtf8();
}

/// The account and the style the user set
QList<Fact*> mapboxCustomSourceSettings()
{
    return {mapTableAppSettings()->mapboxAccount(), mapTableAppSettings()->mapboxStyle()};
}

/// The URL template the user set
QList<Fact*> customUrlSourceSettings()
{
    return {mapTableAppSettings()->customURL()};
}

// Google's layers: h roads only, m standard roadmap, p terrain, r altered roadmap, s satellite only, t terrain only,
// y hybrid (s,h). Its tiles are 256x256 pixels, to zoom 20, from mt0 to mt3.google.com.
constexpr MapFamily kMapFamilyGoogle = {
    .url = TableMapProvider::googleUrl,
    .referrer = "https://www.google.com/maps/preview",
    .attribution = "Google Maps",
};

constexpr MapFamily kMapFamilyBing = {
    .url = TableMapProvider::bingUrl,
    .referrer = "https://www.bing.com/maps/",
    .attribution = "© Microsoft",
    .isNoTileImage = bingNoTileImage,
};

constexpr MapFamily kMapFamilyTianDiTu = {
    .url = TableMapProvider::tiandituUrl,
    .referrer = "https://map.tianditu.gov.cn/",
    .attribution = "© Tianditu",
};

constexpr MapFamily kMapFamilyStatkart = {
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://cache.kartverket.no/v1/wmts/1.0.0/{layer}/default/webmercator/{z}/{y}/{x}.png",
    .referrer = "https://norgeskart.no/",
    .attribution = "© Kartverket",
};

constexpr MapFamily kMapFamilySvalbard = {
    .url = TableMapProvider::templateUrl,
    .urlTemplate =
        "https://geodata.npolar.no/arcgis/rest/services/Basisdata/NP_Basiskart_Svalbard_WMTS_3857/MapServer/WMTS/tile/"
        "1.0.0/Basisdata_NP_Basiskart_Svalbard_WMTS_3857/default/default028mm/{z}/{y}/{x}",
    .referrer = "https://www.npolar.no/",
    .attribution = "© Norwegian Polar Institute",
};

constexpr MapFamily kMapFamilyEniro = {
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://map.eniro.com/geowebcache/service/tms1.0.0/map/{z}/{x}/{tmsY}.{format}",
    .referrer = "https://www.eniro.se/",
    .attribution = "© Eniro",
};

// Esri publishes a list of the contributors of a map service, with the areas and zoom levels that each is credited
// for. Its server has none for World_Terrain_Base.
constexpr MapFamily kMapFamilyEsri = {
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://services.arcgisonline.com/ArcGIS/rest/services/{layer}/MapServer/tile/{z}/{y}/{x}",
    .attribution = "Powered by Esri",
    .areaAttributionUrl = "https://static.arcgis.com/attribution/%1?f=json",
    .token = esriToken,
};

constexpr MapFamily kMapFamilyEsriWithoutList = {
    .url = TableMapProvider::templateUrl,
    .urlTemplate = kMapFamilyEsri.urlTemplate,
    .attribution = kMapFamilyEsri.attribution,
    .token = esriToken,
};

constexpr MapFamily kMapFamilyMapbox = {
    .url = TableMapProvider::mapboxUrl,
    .referrer = "https://www.mapbox.com/",
    .attribution = "© Mapbox © OpenStreetMap",
};

// Mapbox asks for Maxar's credit as well on its satellite styles
constexpr MapFamily kMapFamilyMapboxSatellite = {
    .url = TableMapProvider::mapboxUrl,
    .referrer = kMapFamilyMapbox.referrer,
    .attribution = "© Mapbox © OpenStreetMap © Maxar",
};

constexpr MapFamily kMapFamilyMapboxCustom = {
    .url = TableMapProvider::mapboxCustomUrl,
    .referrer = kMapFamilyMapbox.referrer,
    .attribution = kMapFamilyMapbox.attribution,
    .sourceSettings = mapboxCustomSourceSettings,
};

constexpr MapFamily kMapFamilyMapQuest = {
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "http://otile{server4}.mqcdn.com/tiles/1.0.0/{layer}/{z}/{x}/{y}.{format}",
    .referrer = "https://mapquest.com",
    .attribution = "© MapQuest",
};

constexpr MapFamily kMapFamilyVWorld = {
    .url = TableMapProvider::vworldUrl,
    .referrer = "www.vworld.kr",
    .attribution = "© VWorld",
};

constexpr MapFamily kMapFamilyJapan = {
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://cyberjapandata.gsi.go.jp/xyz/{layer}/{z}/{x}/{y}.{format}",
    .referrer = "https://cyberjapandata.gsi.go.jp/xyz/std",
    .attribution = "Source: Geospatial Information Authority of Japan",
};

constexpr MapFamily kMapFamilyLinz = {
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
    .url = TableMapProvider::templateUrl,
    .urlTemplate = "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
    .attribution = "© OpenStreetMap contributors",
    .isRefusalImage = TableMapProvider::osmRefusalImage,
    .applicationUserAgent = true,
};

constexpr MapFamily kMapFamilyOpenAip = {
    .url = TableMapProvider::openaipUrl,
    .referrer = "https://www.openaip.net",
    .attribution = "© openAIP (CC BY-NC 4.0)",
};

constexpr MapFamily kMapFamilyCustomUrl = {
    .url = TableMapProvider::customUrl,
    .sourceSettings = customUrlSourceSettings,
};

constexpr quint32 kMapTableAverageSize = QGC_AVERAGE_TILE_SIZE;

// The first number is the permanent id of the provider (MapProvider::getMapId()), a key of the tile cache:
// - never change or reuse one
// - a new provider takes the next unused number: 40 and 41 are the elevation providers (ElevationMapProvider.h)
// - the number of a removed provider stays retired
// The name is in the settings and in the offline sets that users have: it stays as it is, too.
constexpr MapRow kMapRows[] = {
#ifndef QGC_NO_GOOGLE_MAPS
    {1, "Google Street Map", kMapFamilyGoogle, "lyrs=m", "png", 4913, MapProvider::StreetMap},
    {2, "Google Satellite", kMapFamilyGoogle, "lyrs=s", "jpg", 56887, MapProvider::SatelliteMapDay},
    {3, "Google Terrain", kMapFamilyGoogle, "v=t,r", "png", 19391, MapProvider::TerrainMap},
    {4, "Google Hybrid", kMapFamilyGoogle, "lyrs=y", "png", 56887, MapProvider::HybridMap},
    {5, "Google Labels", kMapFamilyGoogle, "lyrs=h", "png", kMapTableAverageSize, MapProvider::CustomMap},
#endif
    {6, "Bing Road", kMapFamilyBing, "r", "png", 1297, MapProvider::StreetMap},
    {7, "Bing Satellite", kMapFamilyBing, "a", "jpg", 19597, MapProvider::SatelliteMapDay},
    {8, "Bing Hybrid", kMapFamilyBing, "h", "jpg", 19597, MapProvider::HybridMap},

    {9, "TianDiTu Road", kMapFamilyTianDiTu, "cia_w", "png", 1297, MapProvider::StreetMap},
    {10, "TianDiTu Satellite", kMapFamilyTianDiTu, "img_w", "jpg", 19597, MapProvider::SatelliteMapDay},

    {11, "Statkart Topo", kMapFamilyStatkart, "topo", "png", kMapTableAverageSize, MapProvider::StreetMap},
    // Kartverket's cache service has no base map layer: this map shows the tiles of Statkart Topo. It stays, as
    // cached tiles and offline sets carry its id.
    {12, "Statkart Basemap", kMapFamilyStatkart, "topo", "png", kMapTableAverageSize, MapProvider::StreetMap},
    {13, "Svalbard Topo", kMapFamilySvalbard, "", "png", kMapTableAverageSize, MapProvider::StreetMap},

    {14, "Eniro Topo", kMapFamilyEniro, "", "png", kMapTableAverageSize, MapProvider::StreetMap},

    {15, "Esri World Street", kMapFamilyEsri, "World_Street_Map", "", kMapTableAverageSize, MapProvider::StreetMap},
    {16, "Esri World Satellite", kMapFamilyEsri, "World_Imagery", "", kMapTableAverageSize,
     MapProvider::SatelliteMapDay},
    {17, "Esri Terrain", kMapFamilyEsriWithoutList, "World_Terrain_Base", "", kMapTableAverageSize,
     MapProvider::TerrainMap},

    {18, "Mapbox Streets", kMapFamilyMapbox, "streets-v10", "jpg", 5648, MapProvider::StreetMap},
    {19, "Mapbox Light", kMapFamilyMapbox, "light-v9", "jpg", kMapTableAverageSize, MapProvider::CustomMap},
    {20, "Mapbox Dark", kMapFamilyMapbox, "dark-v9", "jpg", kMapTableAverageSize, MapProvider::CustomMap},
    {21, "Mapbox Satellite", kMapFamilyMapboxSatellite, "satellite-v9", "jpg", 15739, MapProvider::SatelliteMapDay},
    {22, "Mapbox Hybrid", kMapFamilyMapboxSatellite, "satellite-streets-v10", "jpg", 15739, MapProvider::HybridMap},
    {23, "Mapbox StreetsBasic", kMapFamilyMapbox, "basic-v9", "jpg", kMapTableAverageSize, MapProvider::StreetMap},
    {24, "Mapbox Outdoors", kMapFamilyMapbox, "outdoors-v10", "jpg", kMapTableAverageSize, MapProvider::CustomMap},
    {25, "Mapbox Bright", kMapFamilyMapbox, "bright-v9", "jpg", kMapTableAverageSize, MapProvider::CustomMap},
    {26, "Mapbox Custom", kMapFamilyMapboxCustom, "", "jpg", kMapTableAverageSize, MapProvider::CustomMap},

    {27, "MapQuest Map", kMapFamilyMapQuest, "map", "jpg", kMapTableAverageSize, MapProvider::StreetMap},
    {28, "MapQuest Sat", kMapFamilyMapQuest, "sat", "jpg", kMapTableAverageSize, MapProvider::SatelliteMapDay},

    {29, "VWorld Street Map", kMapFamilyVWorld, "Base", "png", kMapTableAverageSize, MapProvider::StreetMap},
    {30, "VWorld Satellite Map", kMapFamilyVWorld, "Satellite", "jpeg", kMapTableAverageSize,
     MapProvider::SatelliteMapDay},

    {31, "Japan-GSI Contour", kMapFamilyJapan, "std", "png", kMapTableAverageSize, MapProvider::StreetMap},
    {32, "Japan-GSI Seamless", kMapFamilyJapan, "seamlessphoto", "jpg", kMapTableAverageSize,
     MapProvider::SatelliteMapDay},
    {33, "Japan-GSI Anaglyph", kMapFamilyJapan, "anaglyphmap_color", "png", kMapTableAverageSize,
     MapProvider::StreetMap},
    {34, "Japan-GSI Slope", kMapFamilyJapan, "slopemap", "png", kMapTableAverageSize, MapProvider::StreetMap},
    {35, "Japan-GSI Relief", kMapFamilyJapan, "relief", "png", kMapTableAverageSize, MapProvider::StreetMap},

    {36, "LINZ Basemap", kMapFamilyLinz, "", "png", kMapTableAverageSize, MapProvider::SatelliteMapDay},

    {37, "Street Map", kMapFamilyOpenStreetMap, "", "png", kMapTableAverageSize, MapProvider::StreetMap},

    {38, "OpenAIP", kMapFamilyOpenAip, "", "png", kMapTableAverageSize, MapProvider::CustomMap},

    {39, "CustomURL Custom", kMapFamilyCustomUrl, "", "", kMapTableAverageSize, MapProvider::CustomMap},
};

QString TableMapProvider::templateUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    QString url = QString::fromUtf8(map._row.family.urlTemplate);
    (void) url.replace(QLatin1StringView("{layer}"), map._layer());
    (void) url.replace(QLatin1StringView("{format}"), map._imageFormat);
    (void) url.replace(QLatin1StringView("{server4}"), QString::number(map._getServerNum(x, y, 4)));
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
        .arg(map._getServerNum(x, y, 4))
        .arg(map._layer(), map._language)
        .arg(x)
        .arg(sec1)
        .arg(y)
        .arg(zoom)
        .arg(sec2);
}

QString TableMapProvider::bingUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    return QStringLiteral("https://ecn.t%1.tiles.virtualearth.net/tiles/%2%3.%4?g=2981&mkt=%5")
        .arg(map._getServerNum(x, y, 4))
        .arg(map._layer(), map._tileXYToQuadKey(x, y, zoom), map._imageFormat, map._language);
}

QString TableMapProvider::tiandituUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    const QString token = mapTableAppSettings()->tiandituToken()->rawValue().toString();
    if (token.isEmpty()) {
        return QString();
    }
    return QStringLiteral("https://t%1.tianditu.gov.cn/DataServer?tk=%2&T=%3&x=%4&y=%5&l=%6")
        .arg(map._getServerNum(x, y, 8))
        .arg(token, map._layer())
        .arg(x)
        .arg(y)
        .arg(zoom);
}

QString TableMapProvider::mapboxUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    const QString token = mapTableAppSettings()->mapboxToken()->rawValue().toString();
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
    AppSettings* const settings = mapTableAppSettings();
    const QString token = settings->mapboxToken()->rawValue().toString();
    if (token.isEmpty()) {
        return QString();
    }
    return QStringLiteral("https://api.mapbox.com/styles/v1/%1/%2/tiles/256/%3/%4/%5?access_token=%6")
        .arg(settings->mapboxAccount()->rawValue().toString())
        .arg(settings->mapboxStyle()->rawValue().toString())
        .arg(zoom)
        .arg(x)
        .arg(y)
        .arg(token);
}

QString TableMapProvider::vworldUrl(const TableMapProvider& map, int x, int y, int zoom)
{
    if ((zoom < 5) || (zoom > 19)) {
        return QString();
    }

    // VWorld has the tiles of Korea only
    const int gap = zoom - 6;

    const int x_min = 53 * pow(2, gap);
    const int x_max = (55 * pow(2, gap)) + (2 * gap - 1);
    if ((x < x_min) || (x > x_max)) {
        return QString();
    }

    const int y_min = 22 * pow(2, gap);
    const int y_max = (26 * pow(2, gap)) + (2 * gap - 1);
    if ((y < y_min) || (y > y_max)) {
        return QString();
    }

    const QString token = mapTableAppSettings()->vworldToken()->rawValue().toString();
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

    const QString apiKey = mapTableAppSettings()->openaipToken()->rawValue().toString();
    if (!apiKey.isEmpty()) {
        url += QStringLiteral("?apiKey=%1").arg(apiKey);
    }

    return url;
}

QString TableMapProvider::customUrl(const TableMapProvider&, int x, int y, int zoom)
{
    QString url = mapTableAppSettings()->customURL()->rawValue().toString();
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
