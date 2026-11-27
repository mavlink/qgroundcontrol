#pragma once

#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

class QGCGeoTiledMappingManagerEngine : public QGeoTiledMappingManagerEngine
{
    Q_OBJECT

public:
    QGCGeoTiledMappingManagerEngine(QGeoServiceProvider::Error* error, QString* errorString, QObject* parent = nullptr);
    ~QGCGeoTiledMappingManagerEngine();

    QGeoMap* createMap() final;

    /// Hands the tiles of the zoom level of the view of \a map to the tile fetcher nearest the centre first, ahead of
    /// the other tiles that the map prefetches: the fetcher asks for the tiles of a call in the order of a hash, and
    /// for those of later calls after them
    void updateTileRequests(QGeoTiledMap* map, const QSet<QGeoTileSpec>& tilesAdded,
                            const QSet<QGeoTileSpec>& tilesRemoved) override;

protected:
    /// How the maps prefetch the tiles around their views: the maps that exist, and those created later
    void setPrefetchStyle(QGeoTiledMap::PrefetchStyle style);

private:
    static constexpr int kTileVersion = 1;
};
