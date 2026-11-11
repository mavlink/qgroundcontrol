#include "QGeoMapReplyQGC.h"

#include <QtLocation/private/qgeotilespec_p.h>

#include "QGCLoggingCategory.h"
#include "QGCTileReply.h"
#include "QGeoMapIdQGC.h"

QGC_LOGGING_CATEGORY(QGeoTiledMapReplyQGCLog, "QtLocationPlugin.QGeoTiledMapReplyQGC")

QGeoTiledMapReplyQGC::QGeoTiledMapReplyQGC(QNetworkAccessManager* networkManager, const QGeoTileSpec& spec,
                                           QObject* parent)
    : QGeoTiledMapReply(spec, parent)
    , _tileReply(new QGCTileReply(networkManager, QGeoMapIdQGC::toProviderId(spec.mapId()), spec.x(), spec.y(),
                                  spec.zoom(), this))
{
    qCDebug(QGeoTiledMapReplyQGCLog) << this;

    (void) connect(_tileReply, &QGCTileReply::finished, this, &QGeoTiledMapReplyQGC::_tileReplyFinished);
    (void) connect(this, &QGeoTiledMapReplyQGC::aborted, _tileReply, &QGCTileReply::abort);
}

QGeoTiledMapReplyQGC::~QGeoTiledMapReplyQGC()
{
    qCDebug(QGeoTiledMapReplyQGCLog) << this;
}

bool QGeoTiledMapReplyQGC::init()
{
    return _tileReply->start();
}

void QGeoTiledMapReplyQGC::_tileReplyFinished()
{
    const QGCTileReply::Error error = _tileReply->error();
    if (error == QGCTileReply::Error::NoError) {
        setMapImageData(_tileReply->data());
        setMapImageFormat(_tileReply->format());
        setCached(_tileReply->isCached());
        setFinished(true);
        return;
    }

    qCWarning(QGeoTiledMapReplyQGCLog) << error << _tileReply->errorString();
    setError((error == QGCTileReply::Error::DataError) ? QGeoTiledMapReply::ParseError
                                                       : QGeoTiledMapReply::CommunicationError,
             _tileReply->errorString());
}
