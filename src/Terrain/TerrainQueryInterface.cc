/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQueryInterface.h"
#include "TerrainTileManager.h"
#include "QGCLoggingCategory.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainQueryInterfaceLog, "qgc.terrain.terrainqueryinterface")

TerrainQueryInterface::TerrainQueryInterface(QObject *parent)
    : QObject(parent)
{
    // qCDebug(TerrainQueryInterfaceLog) << Q_FUNC_INFO << this;
}

TerrainQueryInterface::~TerrainQueryInterface()
{
    // qCDebug(TerrainQueryInterfaceLog) << Q_FUNC_INFO << this;
}

void TerrainQueryInterface::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    Q_UNUSED(coordinates);
    qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "Not Supported";
}

void TerrainQueryInterface::requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    Q_UNUSED(fromCoord);
    Q_UNUSED(toCoord);
    qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "Not Supported";
}

void TerrainQueryInterface::requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly)
{
    Q_UNUSED(swCoord);
    Q_UNUSED(neCoord);
    Q_UNUSED(statsOnly);
    qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "Not Supported";
}

void TerrainQueryInterface::signalCoordinateHeights(bool success, const QList<double> &heights)
{
    emit coordinateHeightsReceived(success, heights);
}

void TerrainQueryInterface::signalPathHeights(bool success, double distanceBetween, double finalDistanceBetween, const QList<double> &heights)
{
    emit pathHeightsReceived(success, distanceBetween, finalDistanceBetween, heights);
}

void TerrainQueryInterface::signalCarpetHeights(bool success, double minHeight, double maxHeight, const QList<QList<double>> &carpet)
{
    emit carpetHeightsReceived(success, minHeight, maxHeight, carpet);
}

void TerrainQueryInterface::_requestFailed()
{
    switch (_queryMode) {
    case TerrainQuery::QueryModeCoordinates:
        emit coordinateHeightsReceived(false, QList<double>());
        break;
    case TerrainQuery::QueryModePath:
        emit pathHeightsReceived(false, qQNaN(), qQNaN(), QList<double>());
        break;
    case TerrainQuery::QueryModeCarpet:
        emit carpetHeightsReceived(false, qQNaN(), qQNaN(), QList<QList<double>>());
        break;
    default:
        qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "Query Mode Not Supported";
        break;
    }
}

/*===========================================================================*/

TerrainOfflineQuery::TerrainOfflineQuery(QObject *parent)
    : TerrainQueryInterface(parent)
{
    // qCDebug(TerrainQueryInterfaceLog) << Q_FUNC_INFO << this;
}

TerrainOfflineQuery::~TerrainOfflineQuery()
{
    // qCDebug(TerrainQueryInterfaceLog) << Q_FUNC_INFO << this;
}

void TerrainOfflineQuery::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    _queryMode = TerrainQuery::QueryModeCoordinates;
    TerrainTileManager::instance()->addCoordinateQuery(this, coordinates);
}

void TerrainOfflineQuery::requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    _queryMode = TerrainQuery::QueryModePath;
    TerrainTileManager::instance()->addPathQuery(this, fromCoord, toCoord);
}

/*===========================================================================*/

TerrainOnlineQuery::TerrainOnlineQuery(QObject *parent)
    : TerrainQueryInterface(parent)
    , _networkManager(new QNetworkAccessManager(this))
{
    // qCDebug(TerrainQueryInterfaceLog) << Q_FUNC_INFO << this;

    qCDebug(TerrainQueryInterfaceLog) << "supportsSsl" << QSslSocket::supportsSsl() << "sslLibraryBuildVersionString" << QSslSocket::sslLibraryBuildVersionString();

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QNetworkProxy proxy = _networkManager->proxy();
    proxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager->setProxy(proxy);
#endif
}

TerrainOnlineQuery::~TerrainOnlineQuery()
{
    // qCDebug(TerrainQueryInterfaceLog) << Q_FUNC_INFO << this;
}

void TerrainOnlineQuery::_requestFinished()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "null reply";
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "error:url:data" << reply->error() << reply->url() << reply->readAll();
        reply->deleteLater();
        _requestFailed();
        return;
    }

    const QByteArray responseBytes = reply->readAll();
    reply->deleteLater();

    qCDebug(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "success";
}

void TerrainOnlineQuery::_requestError(QNetworkReply::NetworkError code)
{
    if (code != QNetworkReply::NoError) {
        QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
        qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "error:url:data" << reply->error() << reply->url() << reply->readAll();
    }
}

void TerrainOnlineQuery::_sslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &error : errors) {
        qCWarning(TerrainQueryInterfaceLog) << "SSL error:" << error.errorString();

        const QSslCertificate &certificate = error.certificate();
        if (!certificate.isNull()) {
            qCWarning(TerrainQueryInterfaceLog) << "SSL Certificate problem:" << certificate.toText();
        }
    }
}
