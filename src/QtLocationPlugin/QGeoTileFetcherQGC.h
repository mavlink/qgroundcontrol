#pragma once

#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGeoTileFetcherQGCLog)

class QGeoTiledMappingManagerEngineQGC;
class QGeoTiledMapReplyQGC;
class QGeoTileSpec;
class QNetworkAccessManager;

class QGeoTileFetcherQGC : public QGeoTileFetcher
{
    Q_OBJECT

public:
    explicit QGeoTileFetcherQGC(QNetworkAccessManager* networkManager, QGeoTiledMappingManagerEngineQGC *parent = nullptr);
    ~QGeoTileFetcherQGC();

private:
    QGeoTiledMapReply* getTileImage(const QGeoTileSpec &spec) final;
    bool initialized() const final;
    bool fetchingEnabled() const final;
    void timerEvent(QTimerEvent *event) final;
    void handleReply(QGeoTiledMapReply *reply, const QGeoTileSpec &spec) final;

    QNetworkAccessManager* m_networkManager = nullptr;
};
