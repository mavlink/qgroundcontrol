#pragma once

#include <QtLocation/private/qgeotilefetcher_p.h>

class QGeoTiledMappingManagerEngineQGC;
class QGeoTiledMapReplyQGC;
class QGeoTileSpec;
class QNetworkAccessManager;

class QGeoTileFetcherQGC : public QGeoTileFetcher
{
    Q_OBJECT

public:
    explicit QGeoTileFetcherQGC(QNetworkAccessManager* networkManager,
                                QGeoTiledMappingManagerEngineQGC* parent = nullptr);
    ~QGeoTileFetcherQGC();

private:
    QGeoTiledMapReply* getTileImage(const QGeoTileSpec& spec) final;

    QNetworkAccessManager* m_networkManager = nullptr;
};
