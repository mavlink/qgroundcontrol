#pragma once

#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QtNetwork/QNetworkRequest>

class QGeoTiledMappingManagerEngineQGC;
class QGeoTiledMapReplyQGC;
class QGeoTileSpec;
class QNetworkAccessManager;

class QGeoTileFetcherQGC : public QGeoTileFetcher
{
    Q_OBJECT

public:
    explicit QGeoTileFetcherQGC(QNetworkAccessManager *networkManager, const QVariantMap &parameters, QGeoTiledMappingManagerEngineQGC *parent = nullptr);
    ~QGeoTileFetcherQGC();

    static QNetworkRequest getNetworkRequest(int mapId, int x, int y, int zoom);
    /* Note: QNetworkAccessManager queues the requests it receives. The number of requests executed in parallel is dependent on the protocol.
     * Currently, for the HTTP protocol on desktop platforms, 6 requests are executed in parallel for one host/port combination. */
    static int concurrentDownloads(const QString &type) { Q_UNUSED(type); return 6; }

private:
    QGeoTiledMapReply* getTileImage(const QGeoTileSpec &spec) final;
    bool initialized() const final;
    bool fetchingEnabled() const final;
    void timerEvent(QTimerEvent *event) final;
    void handleReply(QGeoTiledMapReply *reply, const QGeoTileSpec &spec) final;

    QNetworkAccessManager *m_networkManager = nullptr;
};
