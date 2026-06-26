#pragma once

#include "MapProvider.h"

class EsriMapProvider : public MapProvider
{
public:
    EsriMapProvider(const QString& mapName, const QString& mapTypeId, quint32 averageSize,
                    MapProvider::MapStyle mapType)
        : MapProvider(mapName, QStringLiteral(""), QStringLiteral(""), averageSize, mapType), _mapTypeId(mapTypeId)
    {}

    QByteArray getToken() const final;

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl =
        QStringLiteral("https://services.arcgisonline.com/ArcGIS/rest/services/%1/MapServer/tile/%2/%3/%4");
};
