#include "QGCMapEngine.h"
#include "QGeoMapReplyQGC.h"
#include "TerrainTile.h"
#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "DeviceInfo.h"
#include <QGCLoggingCategory.h>

#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtCore/QFile>

QGC_LOGGING_CATEGORY(QGeoTiledMapReplyQGCLog, "qgc.qtlocationplugin.qgeomapreplyqgc")

QByteArray QGeoTiledMapReplyQGC::s_bingNoTileImage;
QByteArray QGeoTiledMapReplyQGC::s_badTile;

//-----------------------------------------------------------------------------
QGeoTiledMapReplyQGC::QGeoTiledMapReplyQGC(QNetworkAccessManager *networkManager, const QNetworkRequest &request, const QGeoTileSpec &spec, QObject *parent)
    : QGeoTiledMapReply(spec, parent)
    , m_request(request)
    , m_networkManager(networkManager)
{
    _initDataFromResources();

    if(m_request.url().isEmpty()) {
        setMapImageData(s_badTile);
        setMapImageFormat("png");
        setCached(false);
        setFinished(true);
    } else {
        QGCFetchTileTask* task = getQGCMapEngine()->createFetchTileTask(getQGCMapEngine()->urlFactory()->getProviderTypeFromQtMapId(spec.mapId()), spec.x(), spec.y(), spec.zoom());
        connect(task, &QGCFetchTileTask::tileFetched, this, &QGeoTiledMapReplyQGC::_cacheReply);
        connect(task, &QGCMapTask::error, this, &QGeoTiledMapReplyQGC::_cacheError);
        getQGCMapEngine()->addTask(task);
    }

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
        if(file.open(QFile::ReadOnly)) {
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
    if(!reply) {
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
    const MapProvider* const mapProvider = getQGCMapEngine()->urlFactory()->getMapProviderFromQtMapId(mapId);

    if (mapProvider->_isBingProvider() && (image == s_bingNoTileImage)) {
        setError(QGeoTiledMapReply::CommunicationError, "Bing Tile Above Zoom Level");
        return;
    }

    if (mapProvider->_isElevationProvider()) {
        image = TerrainTile::serializeFromAirMapJson(image);
    }
    setMapImageData(image);

    const QString format = mapProvider->getImageFormat(image);
    if (format.isEmpty()) {
        setError(QGeoTiledMapReply::ParseError, "Unknown Format");
        return;
    }
    setMapImageFormat(format);

    const QString mapType = getQGCMapEngine()->urlFactory()->getProviderTypeFromQtMapId(mapId);
    getQGCMapEngine()->cacheTile(mapType, tileSpec().x(), tileSpec().y(), tileSpec().zoom(), image, format);

    setFinished(true);
}

void QGeoTiledMapReplyQGC::_networkReplyError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (!reply) {
        setError(QGeoTiledMapReply::CommunicationError, "Invalid Reply");
        return;
    }
    reply->deleteLater();

    if (reply->error() != QNetworkReply::OperationCanceledError) {
        qCWarning(QGeoTiledMapReplyQGCLog) << Q_FUNC_INFO << reply->errorString();
        setError(QGeoTiledMapReply::CommunicationError, reply->errorString());
    } else {
        setFinished(true);
    }
}

void QGeoTiledMapReplyQGC::_cacheError(QGCMapTask::TaskType type, const QString& /*errorString*/)
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

#if !defined(__mobile__)
    const QNetworkProxy proxy = m_networkManager->proxy();
    QNetworkProxy tempProxy;
    tempProxy.setType(QNetworkProxy::DefaultProxy);
    m_networkManager->setProxy(tempProxy);
#endif

    QNetworkReply* reply = m_networkManager->get(m_request);
    reply->setParent(nullptr);
    connect(reply, &QNetworkReply::finished, this, &QGeoTiledMapReplyQGC::_networkReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, &QGeoTiledMapReplyQGC::_networkReplyError);
    connect(this, &QGeoTiledMapReplyQGC::aborted, reply, &QNetworkReply::abort);
    connect(this, &QGeoTiledMapReplyQGC::destroyed, reply, &QNetworkReply::deleteLater);

#if !defined(__mobile__)
    m_networkManager->setProxy(proxy);
#endif
}

void QGeoTiledMapReplyQGC::_cacheReply(QGCCacheTile* tile)
{
    setMapImageData(tile->img());
    setMapImageFormat(tile->format());
    setCached(true);
    setFinished(true);
    tile->deleteLater();
}
