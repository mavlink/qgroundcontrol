/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQueryAirMap.h"
#include "TerrainTileManager.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainQueryAirMapLog, "qgc.terrain.terrainqueryairmap")
QGC_LOGGING_CATEGORY(TerrainQueryAirMapVerboseLog, "qgc.terrain.terrainqueryairmap.verbose")

TerrainAirMapQuery::TerrainAirMapQuery(QObject *parent)
    : TerrainQueryInterface(parent)
    , _networkManager(new QNetworkAccessManager(this))
{
    // qCDebug(TerrainAirMapQuery) << Q_FUNC_INFO << this;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QNetworkProxy proxy = _networkManager->proxy();
    proxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager->setProxy(proxy);
#endif

    qCDebug(TerrainQueryAirMapVerboseLog) << "supportsSsl" << QSslSocket::supportsSsl() << "sslLibraryBuildVersionString" << QSslSocket::sslLibraryBuildVersionString();
}

TerrainAirMapQuery::~TerrainAirMapQuery()
{
    // qCDebug(TerrainAirMapQuery) << Q_FUNC_INFO << this;
}

void TerrainAirMapQuery::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    QString points;
    for (const QGeoCoordinate &coord: coordinates) {
            points += QString::number(coord.latitude(), 'f', 10) + ","
                    + QString::number(coord.longitude(), 'f', 10) + ",";
    }
    points = points.mid(0, points.length() - 1); // remove the last ',' from string

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("points"), points);

    _queryMode = TerrainQuery::QueryModeCoordinates;
    _sendQuery(QString(), query);
}

void TerrainAirMapQuery::requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
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

void TerrainAirMapQuery::requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly)
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

void TerrainAirMapQuery::_sendQuery(const QString &path, const QUrlQuery &urlQuery)
{
    // TODO: Invalid URL
    // https://terrain-ce.suite.auterion.com/api/v1/
    QUrl url(QStringLiteral("https://api.airmap.com/elevation/v1/ele") + path);
    qCDebug(TerrainQueryAirMapLog) << Q_FUNC_INFO << url;
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    QSslConfiguration sslConf = request.sslConfiguration();
    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConf);

    QNetworkReply* const networkReply = _networkManager->get(request);
    if (!networkReply) {
        qCWarning(TerrainQueryAirMapLog) << "QNetworkManager::Get did not return QNetworkReply";
        _requestFailed();
        return;
    }

    QGCFileDownload::setIgnoreSSLErrorsIfNeeded(*networkReply);

    (void) connect(networkReply, &QNetworkReply::finished, this, &TerrainAirMapQuery::_requestFinished);
    (void) connect(networkReply, &QNetworkReply::sslErrors, this, &TerrainAirMapQuery::_sslErrors);
    (void) connect(networkReply, &QNetworkReply::errorOccurred, this, &TerrainAirMapQuery::_requestError);
}

void TerrainAirMapQuery::_requestError(QNetworkReply::NetworkError code)
{
    if (code != QNetworkReply::NoError) {
        QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
        qCWarning(TerrainQueryAirMapLog) << Q_FUNC_INFO << "error:url:data" << reply->error() << reply->url() << reply->readAll();
    }
}

void TerrainAirMapQuery::_sslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &error : errors) {
        qCWarning(TerrainQueryAirMapLog) << "SSL error:" << error.errorString();

        const QSslCertificate &certificate = error.certificate();
        if (!certificate.isNull()) {
            qCWarning(TerrainQueryAirMapLog) << "SSL Certificate problem:" << certificate.toText();
        }
    }
}

void TerrainAirMapQuery::_requestFinished()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        qCWarning(TerrainQueryAirMapLog) << Q_FUNC_INFO << "null reply";
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(TerrainQueryAirMapLog) << Q_FUNC_INFO << "error:url:data" << reply->error() << reply->url() << reply->readAll();
        reply->deleteLater();
        _requestFailed();
        return;
    }

    const QByteArray responseBytes = reply->readAll();
    reply->deleteLater();

    // Convert the response to Json
    QJsonParseError parseError;
    const QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(TerrainQueryAirMapLog) << Q_FUNC_INFO << "unable to parse json:" << parseError.errorString();
        _requestFailed();
        return;
    }

    // Check airmap reponse status
    const QJsonObject rootObject = responseJson.object();
    const QString status = rootObject["status"].toString();
    if (status != "success") {
        qCWarning(TerrainQueryAirMapLog) << Q_FUNC_INFO << "status != success:" << status;
        _requestFailed();
        return;
    }

    // Send back data
    const QJsonValue &jsonData = rootObject["data"];
    qCDebug(TerrainQueryAirMapLog) << Q_FUNC_INFO << "success";
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

void TerrainAirMapQuery::_requestFailed()
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
        break;
    }
}

void TerrainAirMapQuery::_parseCoordinateData(const QJsonValue &coordinateJson)
{
    const QJsonArray dataArray = coordinateJson.toArray();

    QList<double> heights;
    for (const QJsonValue &dataValue: dataArray) {
        (void) heights.append(dataValue.toDouble());
    }

    emit coordinateHeightsReceived(true, heights);
}

void TerrainAirMapQuery::_parsePathData(const QJsonValue &pathJson)
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

void TerrainAirMapQuery::_parseCarpetData(const QJsonValue &carpetJson)
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

/*===========================================================================*/

TerrainOfflineAirMapQuery::TerrainOfflineAirMapQuery(QObject *parent)
    : TerrainQueryInterface(parent)
{
    // qCDebug(AudioOutputLog) << Q_FUNC_INFO << this;

    qCDebug(TerrainQueryAirMapVerboseLog) << "supportsSsl" << QSslSocket::supportsSsl() << "sslLibraryBuildVersionString" << QSslSocket::sslLibraryBuildVersionString();
}

TerrainOfflineAirMapQuery::~TerrainOfflineAirMapQuery()
{
    // qCDebug(AudioOutputLog) << Q_FUNC_INFO << this;
}

void TerrainOfflineAirMapQuery::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    _queryMode = TerrainQuery::QueryModeCoordinates;
    TerrainTileManager::instance()->addCoordinateQuery(this, coordinates);
}

void TerrainOfflineAirMapQuery::requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    _queryMode = TerrainQuery::QueryModePath;
    TerrainTileManager::instance()->addPathQuery(this, fromCoord, toCoord);
}
