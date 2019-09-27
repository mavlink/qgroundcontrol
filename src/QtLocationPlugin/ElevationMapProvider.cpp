#include "ElevationMapProvider.h"
#if defined(DEBUG_GOOGLE_MAPS)
#include <QFile>
#include <QStandardPaths>
#endif
#include "QGCMapEngine.h"

ElevationProvider::ElevationProvider(QString imageFormat, quint32 averageSize,
                                     QGeoMapType::MapStyle mapType,
                                     QObject*              parent)
    : MapProvider(QString("https://api.airmap.com/"), imageFormat, averageSize,
                  mapType, parent) {}
ElevationProvider::~ElevationProvider() {}


//-----------------------------------------------------------------------------
QByteArray AirmapElevationProvider::serialize(QByteArray buf){
    return AirmapTerrainTile::serialize(buf);
}
//-----------------------------------------------------------------------------
TerrainTile * AirmapElevationProvider::newTerrainTile(QByteArray buf){
    TerrainTile * t = new AirmapTerrainTile(buf);
    return t;
}

//-----------------------------------------------------------------------------
int AirmapElevationProvider::long2tileX(double lon, int z) {
    Q_UNUSED(z);
    return static_cast<int>(floor((lon + 180.0) / srtm1TileSize));
}

//-----------------------------------------------------------------------------
int AirmapElevationProvider::lat2tileY(double lat, int z) {
    Q_UNUSED(z);
    return static_cast<int>(floor((lat + 90.0) / srtm1TileSize));
}

QString
AirmapElevationProvider::_getURL(int x, int y, int zoom,
                                 QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    Q_UNUSED(zoom);
    return QString("https://api.airmap.com/elevation/v1/ele/"
                   "carpet?points=%1,%2,%3,%4")
        .arg(static_cast<double>(y) * srtm1TileSize - 90.0)
        .arg(static_cast<double>(x) * srtm1TileSize - 180.0)
        .arg(static_cast<double>(y + 1) * srtm1TileSize - 90.0)
        .arg(static_cast<double>(x + 1) * srtm1TileSize - 180.0);
}

QGCTileSet AirmapElevationProvider::getTileCount(int zoom, double topleftLon,
                                                 double topleftLat,
                                                 double bottomRightLon,
                                                 double bottomRightLat) {
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(bottomRightLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(topleftLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;

    return set;
}




QByteArray GeotiffElevationProvider::serialize(QByteArray buf){
    return GeotiffTerrainTile::serialize(buf);
}
//-----------------------------------------------------------------------------
TerrainTile * GeotiffElevationProvider::newTerrainTile(QByteArray buf){
    TerrainTile * t = new GeotiffTerrainTile(buf);
    return t;
}

double GeotiffElevationProvider::tilex2long(int x, int z) 
{
	return x / (double)(1 << z) * 360.0 - 180;
}

double GeotiffElevationProvider::tiley2lat(int y, int z) 
{
	double n = M_PI - 2.0 * M_PI * y / (double)(1 << z);
	return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}


QString
GeotiffElevationProvider::_getURL(int x, int y, int zoom,
                                 QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    Q_UNUSED(zoom);

    // Params
    int resolution = 512;
    QString layer = "QGCTerrainTest:dtm";

    // Infos here https://wiki.openstreetmap.org/wiki/Zoom_levels

    double lon = tilex2long(x,zoom);
    double lat = tiley2lat(y,zoom);

    double delta_lat  = (40075016.686 * cos(lat*M_PI/180.))/pow(2,zoom);
    double delta_long = 360./pow(2,zoom);

    return QString("https://services.aeronavics.com/geoserver/QGCTerrainTest/wms?service=WMS&version=1.1.0&request=GetMap&layers=%1&bbox=%2,%3,%4,%5&width=%6&height=%7&srs=EPSG:32760&format=image/geotiff")
        .arg(layer)
        .arg(lon)
        .arg(lat)
        .arg(fmod((lon+delta_lat+180.),360) - 180)
        .arg(fmod((lat+delta_long+180.),360) - 180.)
        .arg(resolution)
        .arg(resolution);
}
