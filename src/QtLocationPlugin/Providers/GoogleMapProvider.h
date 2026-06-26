#pragma once

#include "MapProvider.h"

// h: roads only
// m: standard roadmap
// p: terrain
// r: somehow altered roadmap
// s: satellite only
// t: terrain only
// y: hybrid (s,h)
// traffic
// transit
// bike
// mt.google.com: mt0, mt1, mt2 mt3
// size: 256x256
// maxZoom: 20

static constexpr const quint32 AVERAGE_GOOGLE_STREET_MAP = 4913;
static constexpr const quint32 AVERAGE_GOOGLE_SAT_MAP = 56887;
static constexpr const quint32 AVERAGE_GOOGLE_TERRAIN_MAP = 19391;

class GoogleMapProvider : public MapProvider
{
public:
    GoogleMapProvider(const QString& mapName, const QString& versionRequest, const QString& version,
                      const QString& imageFormat, quint32 averageSize, MapProvider::MapStyle mapType)
        : MapProvider(mapName, QStringLiteral("https://www.google.com/maps/preview"), imageFormat, averageSize,
                      mapType),
          _versionRequest(versionRequest),
          _version(version)
    {}

private:
    void _getSecGoogleWords(int x, int y, QString& sec1, QString& sec2) const;
    QString _getURL(int x, int y, int zoom) const final;

    const QString _versionRequest;
    const QString _version;
    const QString _mapUrl = QStringLiteral("https://mt%1.google.com/vt/%2=%3&hl=%4&x=%5%6&y=%7&z=%8&s=%9&scale=%10");
    const QString _secGoogleWord = QStringLiteral("Galileo");
    static constexpr int kServerCount = 4;
};
