#pragma once

#include <QtLocation/private/qgeotiledmapreply_p.h>

class QGCTileReply;
class QNetworkAccessManager;

/// Presents a QGCTileReply as the reply QtLocation's tile fetcher expects
class QGeoTiledMapReplyQGC : public QGeoTiledMapReply
{
    Q_OBJECT

public:
    explicit QGeoTiledMapReplyQGC(QNetworkAccessManager* networkManager, const QGeoTileSpec& spec,
                                  QObject* parent = nullptr);
    ~QGeoTiledMapReplyQGC();

    bool init();

private:
    void _tileReplyFinished();

    QGCTileReply* _tileReply = nullptr;
};
