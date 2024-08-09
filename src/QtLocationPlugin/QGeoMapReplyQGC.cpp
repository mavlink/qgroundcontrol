#include "QGeoMapReplyQGC.h"

#include "ElevationMapProvider.h"
#include "MapProvider.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "QGeoFileTileCacheQGC.h"

#include <DeviceInfo.h>
#include <QGCFileDownload.h>
#include <QGCLoggingCategory.h>

#include <QtCore/QFile>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QSslError>

QGC_LOGGING_CATEGORY(QGeoTiledMapReplyQGCLog, "qgc.qtlocationplugin.qgeomapreplyqgc")

QByteArray QGeoTiledMapReplyQGC::_bingNoTileImage;
QByteArray QGeoTiledMapReplyQGC::_badTile;

QGeoTiledMapReplyQGC::QGeoTiledMapReplyQGC(QNetworkAccessManager *networkManager, const QNetworkRequest &request, const QGeoTileSpec &spec, QObject *parent)
    : QGeoTiledMapReply(spec, parent)
    , _networkManager(networkManager)
    , _request(request)
{
    // qCDebug(QGeoTiledMapReplyQGCLog) << Q_FUNC_INFO << this;

    _initDataFromResources();

    (void) connect(this, &QGeoTiledMapReplyQGC::errorOccurred, this, [this](QGeoTiledMapReply::Error error, const QString &errorString) {
        qCWarning(QGeoTiledMapReplyQGCLog) << error << errorString;
        setMapImageData(_badTile);
        setMapImageFormat("png");
        setCached(false);
    }, Qt::AutoConnection);

    QGCFetchTileTask* const task = QGeoFileTileCacheQGC::createFetchTileTask(UrlFactory::getProviderTypeFromQtMapId(spec.mapId()), spec.x(), spec.y(), spec.zoom());
    (void) connect(task, &QGCFetchTileTask::tileFetched, this, &QGeoTiledMapReplyQGC::_cacheReply);
    (void) connect(task, &QGCMapTask::error, this, &QGeoTiledMapReplyQGC::_cacheError);
    getQGCMapEngine()->addTask(task);
}

QGeoTiledMapReplyQGC::~QGeoTiledMapReplyQGC()
{
    // qCDebug(QGeoTiledMapReplyQGCLog) << Q_FUNC_INFO << this;
}

void QGeoTiledMapReplyQGC::_initDataFromResources()
{
    if (_bingNoTileImage.isEmpty()) {
        QFile file("://res/BingNoTileBytes.dat");
        if (file.open(QFile::ReadOnly)) {
            _bingNoTileImage = file.readAll();
            file.close();
        }
    }

    if (_badTile.isEmpty()) {
        QFile file("://res/images/notile.png");
        if (file.open(QFile::ReadOnly)) {
            _badTile = file.readAll();
            file.close();
        }
    }
}

void QGeoTiledMapReplyQGC::_networkReplyFinished()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        setError(QGeoTiledMapReply::UnknownError, QStringLiteral("Unexpected Error"));
        return;
    }
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    if (!reply->isOpen()) {
        setError(QGeoTiledMapReply::ParseError, QStringLiteral("Empty Reply"));
        return;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if ((statusCode < HTTP_Response::SUCCESS_OK) || (statusCode >= HTTP_Response::REDIRECTION_MULTIPLE_CHOICES)) {
        setError(QGeoTiledMapReply::CommunicationError, reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
        return;
    }

    QByteArray image = reply->readAll();
    if (image.isEmpty()) {
        setError(QGeoTiledMapReply::ParseError, QStringLiteral("Image is Empty"));
        return;
    }

    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromQtMapId(tileSpec().mapId());
    Q_CHECK_PTR(mapProvider);

    if (mapProvider->isBingProvider() && (image == _bingNoTileImage)) {
        setError(QGeoTiledMapReply::CommunicationError, QStringLiteral("Bing Tile Above Zoom Level"));
        return;
    }

    if (mapProvider->isElevationProvider()) {
        const SharedElevationProvider elevationProvider = std::dynamic_pointer_cast<const ElevationProvider>(mapProvider);
        image = elevationProvider->serialize(image);
        if (image.isEmpty()) {
            setError(QGeoTiledMapReply::ParseError, QStringLiteral("Failed to Serialize Terrain Tile"));
            return;
        }
    }
    setMapImageData(image);

    const QString format = mapProvider->getImageFormat(image);
    if (format.isEmpty()) {
        setError(QGeoTiledMapReply::ParseError, QStringLiteral("Unknown Format"));
        return;
    }
    setMapImageFormat(format);

    QGeoFileTileCacheQGC::cacheTile(mapProvider->getMapName(), tileSpec().x(), tileSpec().y(), tileSpec().zoom(), image, format);

    setFinished(true);
}

void QGeoTiledMapReplyQGC::_networkReplyError(QNetworkReply::NetworkError error)
{
    if (error != QNetworkReply::OperationCanceledError) {
        const QNetworkReply* const reply = qobject_cast<const QNetworkReply*>(sender());
        if (!reply) {
            setError(QGeoTiledMapReply::CommunicationError, QStringLiteral("Invalid Reply"));
        } else {
            setError(QGeoTiledMapReply::CommunicationError, reply->errorString());
        }
    } else {
        setFinished(true);
    }
}

#if QT_CONFIG(ssl)
void QGeoTiledMapReplyQGC::_networkReplySslErrors(const QList<QSslError> &errors)
{
    QString errorString;
    for (const QSslError &error : errors) {
        if (!errorString.isEmpty()) {
            (void) errorString.append('\n');
        }
        (void) errorString.append(error.errorString());
    }

    if (!errorString.isEmpty()) {
        setError(QGeoTiledMapReply::CommunicationError, errorString);
    }
}
#endif

void QGeoTiledMapReplyQGC::_cacheReply(QGCCacheTile *tile)
{
    if (tile) {
        setMapImageData(tile->img());
        setMapImageFormat(tile->format());
        setCached(true);
        setFinished(true);
        delete tile;
    } else {
        setError(QGeoTiledMapReply::UnknownError, QStringLiteral("Invalid Cache Tile"));
    }
}

void QGeoTiledMapReplyQGC::_cacheError(QGCMapTask::TaskType type, QStringView errorString)
{
    Q_UNUSED(errorString);

    Q_ASSERT(type == QGCMapTask::taskFetchTile);

    if (!QGCDeviceInfo::isInternetAvailable()) {
        setError(QGeoTiledMapReply::CommunicationError, QStringLiteral("Network Not Available"));
        return;
    }

    _request.setOriginatingObject(this);

    QNetworkReply* const reply = _networkManager->get(_request);
    reply->setParent(this);
    QGCFileDownload::setIgnoreSSLErrorsIfNeeded(*reply);

    (void) connect(reply, &QNetworkReply::finished, this, &QGeoTiledMapReplyQGC::_networkReplyFinished);
    (void) connect(reply, &QNetworkReply::errorOccurred, this, &QGeoTiledMapReplyQGC::_networkReplyError);
#if QT_CONFIG(ssl)
    (void) connect(reply, &QNetworkReply::sslErrors, this, &QGeoTiledMapReplyQGC::_networkReplySslErrors);
#endif
    (void) connect(this, &QGeoTiledMapReplyQGC::aborted, reply, &QNetworkReply::abort);
}

void QGeoTiledMapReplyQGC::abort()
{
    QGeoTiledMapReply::abort();
}
