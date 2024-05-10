#include "QGeoMapReplyQGC.h"
#include "QGCMapEngine.h"
#include "QGeoTileFetcherQGC.h"
#include "TerrainTile.h"
#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "DeviceInfo.h"
#include "QGeoFileTileCacheQGC.h"
#include <QGCLoggingCategory.h>

#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtCore/QFile>

QGC_LOGGING_CATEGORY(QGeoTiledMapReplyQGCLog, "qgc.qtlocationplugin.qgeomapreplyqgc")

QByteArray QGeoTiledMapReplyQGC::s_bingNoTileImage;
QByteArray QGeoTiledMapReplyQGC::s_badTile;

QGeoTiledMapReplyQGC::QGeoTiledMapReplyQGC(QNetworkAccessManager *networkManager, const QGeoTileSpec &spec, QObject *parent)
    : QGeoTiledMapReply(spec, parent)
    , m_networkManager(networkManager)
{
    _initDataFromResources();

    connect(this, &QGeoTiledMapReplyQGC::errorOccurred, this, [this](QGeoTiledMapReply::Error error, const QString &errorString){
        Q_UNUSED(error); Q_UNUSED(errorString);
        setMapImageData(s_badTile);
        setMapImageFormat("png");
        setCached(false);
    });

    QGCFetchTileTask* const task = QGeoFileTileCacheQGC::createFetchTileTask(UrlFactory::getProviderTypeFromQtMapId(spec.mapId()), spec.x(), spec.y(), spec.zoom());
    connect(task, &QGCFetchTileTask::tileFetched, this, &QGeoTiledMapReplyQGC::_cacheReply);
    connect(task, &QGCMapTask::error, this, &QGeoTiledMapReplyQGC::_cacheError);
    getQGCMapEngine()->addTask(task);

    // qCDebug(QGeoTiledMapReplyQGCLog) << Q_FUNC_INFO << this;
}

QGeoTiledMapReplyQGC::~QGeoTiledMapReplyQGC()
{
    // qCDebug(QGeoTiledMapReplyQGCLog) << Q_FUNC_INFO << this;
}

void QGeoTiledMapReplyQGC::_initDataFromResources()
{
    if (s_bingNoTileImage.isEmpty()) {
        QFile file("://res/BingNoTileBytes.dat");
        if (file.open(QFile::ReadOnly)) {
            s_bingNoTileImage = file.readAll();
            file.close();
        }
    }

    if (s_badTile.isEmpty()) {
        QFile file("://res/images/notile.png");
        if(file.open(QFile::ReadOnly)) {
            s_badTile = file.readAll();
            file.close();
        }
    }
}

void QGeoTiledMapReplyQGC::_networkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (!reply) {
        setError(QGeoTiledMapReply::UnknownError, "Unexpected Error");
        return;
    }
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setError(QGeoTiledMapReply::UnknownError, "Unexpected Error");
        return;
    }

    QByteArray image = reply->readAll();
    if (image.isEmpty()) {
        setError(QGeoTiledMapReply::ParseError, "Image is Empty");
        return;
    }

    const int mapId = tileSpec().mapId();
    const SharedMapProvider mapProvider = UrlFactory::getProviderFromQtMapId(mapId);

    if (mapProvider->isBingProvider() && (image == s_bingNoTileImage)) {
        setError(QGeoTiledMapReply::CommunicationError, "Bing Tile Above Zoom Level");
        return;
    }

    if (mapProvider->isElevationProvider()) {
        image = TerrainTile::serializeFromAirMapJson(image);
    }
    setMapImageData(image);

    const QString format = mapProvider->getImageFormat(image);
    if (format.isEmpty()) {
        setError(QGeoTiledMapReply::ParseError, "Unknown Format");
        return;
    }
    setMapImageFormat(format);

    const QString mapType = UrlFactory::getProviderTypeFromQtMapId(mapId);
    QGeoFileTileCacheQGC::cacheTile(mapType, tileSpec().x(), tileSpec().y(), tileSpec().zoom(), image, format);

    setFinished(true);
}

void QGeoTiledMapReplyQGC::_networkReplyError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (!reply) {
        setError(QGeoTiledMapReply::CommunicationError, "Invalid Reply");
        return;
    }
    reply->deleteLater();

    if (error != QNetworkReply::OperationCanceledError) {
        qCWarning(QGeoTiledMapReplyQGCLog) << Q_FUNC_INFO << reply->errorString();
        setError(QGeoTiledMapReply::CommunicationError, reply->errorString());
    } else {
        setFinished(true);
    }
}

void QGeoTiledMapReplyQGC::_cacheReply(QGCCacheTile* tile)
{
    if (tile) {
        setMapImageData(tile->img());
        setMapImageFormat(tile->format());
        setCached(true);
        setFinished(true);
        delete tile;
    } else {
        setError(QGeoTiledMapReply::UnknownError, "Invalid Cache Tile");
    }
}

void QGeoTiledMapReplyQGC::_cacheError(QGCMapTask::TaskType type, QStringView /*errorString*/)
{
    if (!QGCDeviceInfo::isInternetAvailable()) {
        setError(QGeoTiledMapReply::CommunicationError, "Network Not Available");
        return;
    }

    if (type != QGCMapTask::taskFetchTile) {
        qCWarning(QGeoTiledMapReplyQGCLog) << Q_FUNC_INFO << "Wrong Task";
        setError(QGeoTiledMapReply::UnknownError, "Wrong Task");
        return;
    }

    QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(tileSpec().mapId(), tileSpec().x(), tileSpec().y(), tileSpec().zoom());
    request.setOriginatingObject(this);

#if !defined(__mobile__)
    const QNetworkProxy proxy = m_networkManager->proxy();
    QNetworkProxy tempProxy;
    tempProxy.setType(QNetworkProxy::DefaultProxy);
    m_networkManager->setProxy(tempProxy);
#endif

    QNetworkReply* reply = m_networkManager->get(request);
    reply->setParent(this);
    connect(reply, &QNetworkReply::finished, this, &QGeoTiledMapReplyQGC::_networkReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, &QGeoTiledMapReplyQGC::_networkReplyError);
    connect(this, &QGeoTiledMapReplyQGC::aborted, reply, &QNetworkReply::abort);
    connect(this, &QGeoTiledMapReplyQGC::destroyed, reply, &QNetworkReply::deleteLater);

#if !defined(__mobile__)
    m_networkManager->setProxy(proxy);
#endif
}

/*void QGeoTiledMapReplyQGC::abort()
{
    QGeoTiledMapReply::abort();
}*/
