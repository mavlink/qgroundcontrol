#ifndef TERRAINCACHESERVER_H
#define TERRAINCACHESERVER_H

#include "TerrainTile.h"

class TerrainCacheTileServer
{
public:
    TerrainCacheTileServer();

    bool cacheTerrainData(const QGeoCoordinate& southWest, const QGeoCoordinate& northEast);

    bool cached(const QGeoCoordinate& coord);

private:
    QStringList             _downloadQueue;
};

#endif // TERRAINCACHESERVER_H
