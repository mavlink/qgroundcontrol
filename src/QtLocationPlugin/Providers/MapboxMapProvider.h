#pragma once

#include "MapProvider.h"

static constexpr const quint32 AVERAGE_MAPBOX_SAT_MAP = 15739;
static constexpr const quint32 AVERAGE_MAPBOX_STREET_MAP = 5648;

class MapboxMapProvider : public MapProvider
{
public:
    MapboxMapProvider(const QString& mapName, const QString& mapTypeId, quint32 averageSize,
                      MapProvider::MapStyle mapType)
        : MapProvider(mapName, QStringLiteral("https://www.mapbox.com/"), QStringLiteral("jpg"), averageSize, mapType),
          _mapTypeId(mapTypeId)
    {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
};
