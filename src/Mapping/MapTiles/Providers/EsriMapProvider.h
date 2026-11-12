#pragma once

#include "MapProvider.h"

class EsriMapProvider : public MapProvider
{
protected:
    /// \a hasAreaAttribution: Esri publishes a list of the contributors of the map service \a mapTypeId, with the
    /// areas and zoom levels that each is credited for
    EsriMapProvider(const QString& mapName, const QString& mapTypeId, quint32 averageSize,
                    MapProvider::MapStyle mapType, bool hasAreaAttribution)
        : MapProvider(mapName, QStringLiteral(""), QStringLiteral(""), averageSize, mapType)
        , _mapTypeId(mapTypeId)
        , _areaAttributionUrl(
              hasAreaAttribution
                  ? QUrl(QStringLiteral("https://static.arcgis.com/attribution/%1?f=json").arg(mapTypeId))
                  : QUrl())
    {}

public:
    QByteArray getToken() const final;

    QString attribution() const final { return QStringLiteral("Powered by Esri"); }

    QUrl areaAttributionUrl() const final { return _areaAttributionUrl; }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl =
        QStringLiteral("https://services.arcgisonline.com/ArcGIS/rest/services/%1/MapServer/tile/%2/%3/%4");
    const QUrl _areaAttributionUrl;
};

class EsriWorldStreetMapProvider : public EsriMapProvider
{
public:
    EsriWorldStreetMapProvider()
        : EsriMapProvider(QStringLiteral("Esri World Street"), QStringLiteral("World_Street_Map"),
                          QGC_AVERAGE_TILE_SIZE, MapProvider::StreetMap, /*hasAreaAttribution*/ true)
    {}
};

class EsriWorldSatelliteMapProvider : public EsriMapProvider
{
public:
    EsriWorldSatelliteMapProvider()
        : EsriMapProvider(QStringLiteral("Esri World Satellite"), QStringLiteral("World_Imagery"),
                          QGC_AVERAGE_TILE_SIZE, MapProvider::SatelliteMapDay, /*hasAreaAttribution*/ true)
    {}
};

class EsriTerrainMapProvider : public EsriMapProvider
{
public:
    // Esri's server has no list of contributors for this map service
    EsriTerrainMapProvider()
        : EsriMapProvider(QStringLiteral("Esri Terrain"), QStringLiteral("World_Terrain_Base"), QGC_AVERAGE_TILE_SIZE,
                          MapProvider::TerrainMap, /*hasAreaAttribution*/ false)
    {}
};
