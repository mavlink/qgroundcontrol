/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQueryCopernicus.h"
#include "TerrainTileCopernicus.h"
#include "ElevationMapProvider.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainQueryCopernicusLog, "qgc.terrain.terrainquerycopernicus")

TerrainQueryCopernicus::TerrainQueryCopernicus(QObject *parent)
    : TerrainOnlineQuery(parent)
{
    // qCDebug(TerrainQueryCopernicusLog) << Q_FUNC_INFO << this;
}

TerrainQueryCopernicus::~TerrainQueryCopernicus()
{
    // qCDebug(TerrainQueryCopernicusLog) << Q_FUNC_INFO << this;
}

void TerrainQueryCopernicus::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    QString points;
    for (const QGeoCoordinate &coord: coordinates) {
        const QString point = QString::number(coord.latitude(), 'f', 10) + "," + QString::number(coord.longitude(), 'f', 10);
        points += (point + ",");
    }
    points = points.mid(0, points.length() - 1); // remove the last ',' from string

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("points"), points);

    _queryMode = TerrainQuery::QueryModeCoordinates;
    _sendQuery(QString(), query);
}

void TerrainQueryCopernicus::requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    QString points;
    points += QString::number(fromCoord.latitude(), 'f', 10) + ","
            + QString::number(fromCoord.longitude(), 'f', 10) + ",";
    points += QString::number(toCoord.latitude(), 'f', 10) + ","
            + QString::number(toCoord.longitude(), 'f', 10);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("points"), points);

    _queryMode = TerrainQuery::QueryModePath;
    _sendQuery(QStringLiteral("/path"), query);
}

void TerrainQueryCopernicus::requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly)
{
    QString points;
    points += QString::number(swCoord.latitude(), 'f', 10) + ","
            + QString::number(swCoord.longitude(), 'f', 10) + ",";
    points += QString::number(neCoord.latitude(), 'f', 10) + ","
            + QString::number(neCoord.longitude(), 'f', 10);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("points"), points);

    _queryMode = TerrainQuery::QueryModeCarpet;
    _carpetStatsOnly = statsOnly;

    _sendQuery(QStringLiteral("/carpet"), query);
}

void TerrainQueryCopernicus::_sendQuery(const QString &path, const QUrlQuery &urlQuery)
{
    QUrl url(QString(CopernicusElevationProvider::kProviderURL) + QStringLiteral("/api/v1") + path);
    url.setQuery(urlQuery);
    qCDebug(TerrainQueryCopernicusLog) << Q_FUNC_INFO << url;

    QNetworkRequest request(url);
    QSslConfiguration sslConf = request.sslConfiguration();
    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConf);

    QNetworkReply* const networkReply = _networkManager->get(request);
    if (!networkReply) {
        qCWarning(TerrainQueryCopernicusLog) << "QNetworkManager::Get did not return QNetworkReply";
        _requestFailed();
        return;
    }

    QGCFileDownload::setIgnoreSSLErrorsIfNeeded(*networkReply);

    (void) connect(networkReply, &QNetworkReply::finished, this, &TerrainQueryCopernicus::_requestFinished);
    (void) connect(networkReply, &QNetworkReply::sslErrors, this, &TerrainQueryCopernicus::_sslErrors);
    (void) connect(networkReply, &QNetworkReply::errorOccurred, this, &TerrainQueryCopernicus::_requestError);
}

void TerrainQueryCopernicus::_requestFinished()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        qCWarning(TerrainQueryCopernicusLog) << Q_FUNC_INFO << "null reply";
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(TerrainQueryCopernicusLog) << Q_FUNC_INFO << "error:url:data" << reply->error() << reply->url() << reply->readAll();
        reply->deleteLater();
        _requestFailed();
        return;
    }

    const QByteArray responseBytes = reply->readAll();
    reply->deleteLater();

    const QJsonValue &jsonData = TerrainTileCopernicus::getJsonFromData(responseBytes);
    if (jsonData.isNull()) {
        _requestFailed();
        return;
    }

    qCDebug(TerrainQueryCopernicusLog) << Q_FUNC_INFO << "success";
    switch (_queryMode) {
    case TerrainQuery::QueryModeCoordinates:
        _parseCoordinateData(jsonData);
        break;
    case TerrainQuery::QueryModePath:
        _parsePathData(jsonData);
        break;
    case TerrainQuery::QueryModeCarpet:
        _parseCarpetData(jsonData);
        break;
    default:
        break;
    }
}

void TerrainQueryCopernicus::_parseCoordinateData(const QJsonValue &coordinateJson)
{
    const QJsonArray dataArray = coordinateJson.toArray();

    QList<double> heights;
    for (const QJsonValue &dataValue: dataArray) {
        (void) heights.append(dataValue.toDouble());
    }

    emit coordinateHeightsReceived(true, heights);
}

void TerrainQueryCopernicus::_parsePathData(const QJsonValue &pathJson)
{
    const QJsonObject jsonObject = pathJson.toArray()[0].toObject();
    const QJsonArray stepArray = jsonObject["step"].toArray();
    const QJsonArray profileArray = jsonObject["profile"].toArray();

    const double latStep = stepArray[0].toDouble();
    const double lonStep = stepArray[1].toDouble();

    QList<double> heights;
    for (const QJsonValue &profileValue: profileArray) {
        (void) heights.append(profileValue.toDouble());
    }

    emit pathHeightsReceived(true, latStep, lonStep, heights);
}

void TerrainQueryCopernicus::_parseCarpetData(const QJsonValue &carpetJson)
{
    const QJsonObject jsonObject = carpetJson.toArray()[0].toObject();

    const QJsonObject statsObject = jsonObject["stats"].toObject();
    const double minHeight = statsObject["min"].toDouble();
    const double maxHeight = statsObject["max"].toDouble();

    QList<QList<double>> carpet;
    if (!_carpetStatsOnly) {
        const QJsonArray carpetArray = jsonObject["carpet"].toArray();

        for (qsizetype i = 0; i < carpetArray.count(); i++) {
            const QJsonArray rowArray = carpetArray[i].toArray();
            (void) carpet.append(QList<double>());

            for (qsizetype j = 0; j < rowArray.count(); j++) {
                const double height = rowArray[j].toDouble();
                (void) carpet.last().append(height);
            }
        }
    }

    emit carpetHeightsReceived(true, minHeight, maxHeight, carpet);
}
