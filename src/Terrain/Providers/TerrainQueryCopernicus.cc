#include "TerrainQueryCopernicus.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtPositioning/QGeoCoordinate>

#include "CopernicusTileFormat.h"
#include "ElevationMapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(TerrainQueryCopernicusLog, "Terrain.TerrainQueryCopernicus")

TerrainQueryCopernicus::TerrainQueryCopernicus(QObject *parent)
    : TerrainOnlineQuery(parent)
{
    qCDebug(TerrainQueryCopernicusLog) << this;
}

TerrainQueryCopernicus::~TerrainQueryCopernicus()
{
    qCDebug(TerrainQueryCopernicusLog) << this;
}

void TerrainQueryCopernicus::requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    qCWarning(TerrainQueryCopernicusLog) << "The server has no query for the heights of coordinates";
    _queryMode = TerrainQuery::QueryModeCoordinates;
    _requestFailed();
}

void TerrainQueryCopernicus::requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord)
{
    Q_UNUSED(fromCoord);
    Q_UNUSED(toCoord);

    qCWarning(TerrainQueryCopernicusLog) << "The server has no query for the heights of a path";
    _queryMode = TerrainQuery::QueryModePath;
    _requestFailed();
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
    qCDebug(TerrainQueryCopernicusLog) << url;

    const QNetworkRequest request(url);
    QNetworkReply* const networkReply = _networkManager->get(request);
    if (!networkReply) {
        qCWarning(TerrainQueryCopernicusLog) << "QNetworkManager::Get did not return QNetworkReply";
        _requestFailed();
        return;
    }

    QGCNetworkHelper::ignoreSslErrorsIfNeeded(networkReply);

    (void) connect(networkReply, &QNetworkReply::finished, this, &TerrainQueryCopernicus::_requestFinished);
    (void) connect(networkReply, &QNetworkReply::sslErrors, this, &TerrainQueryCopernicus::_sslErrors);
    (void) connect(networkReply, &QNetworkReply::errorOccurred, this, &TerrainQueryCopernicus::_requestError);
}

void TerrainQueryCopernicus::_requestFinished()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        qCWarning(TerrainQueryCopernicusLog) << "null reply";
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(TerrainQueryCopernicusLog) << "error:url:data" << reply->error() << reply->url() << reply->readAll();
        reply->deleteLater();
        _requestFailed();
        return;
    }

    const QByteArray responseBytes = reply->readAll();
    reply->deleteLater();

    const QJsonValue jsonData = CopernicusTileFormat::responseData(responseBytes);
    if (jsonData.isNull()) {
        _requestFailed();
        return;
    }

    qCDebug(TerrainQueryCopernicusLog) << "success";
    _parseCarpetData(jsonData);
}

void TerrainQueryCopernicus::_parseCarpetData(const QJsonValue& carpetJson)
{
    const QJsonObject dataObject = carpetJson.toObject();

    const QJsonObject statsObject = dataObject["stats"].toObject();
    const QJsonValue minHeight = statsObject["min"];
    const QJsonValue maxHeight = statsObject["max"];
    if (!minHeight.isDouble() || !maxHeight.isDouble()) {
        qCWarning(TerrainQueryCopernicusLog) << "Carpet without its lowest and highest elevation";
        _requestFailed();
        return;
    }

    QList<QList<double>> carpet;
    if (!_carpetStatsOnly) {
        const QJsonArray carpetDataArray = dataObject["carpet"].toArray();

        for (qsizetype i = 0; i < carpetDataArray.count(); i++) {
            const QJsonArray rowArray = carpetDataArray[i].toArray();
            (void) carpet.append(QList<double>());

            for (qsizetype j = 0; j < rowArray.count(); j++) {
                const double height = rowArray[j].toDouble();
                (void) carpet.last().append(height);
            }
        }
    }

    emit carpetHeightsReceived(true, minHeight.toDouble(), maxHeight.toDouble(), carpet);
}
