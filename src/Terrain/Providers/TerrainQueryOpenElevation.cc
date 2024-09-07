#include "TerrainQueryOpenElevation.h"
#include "ElevationMapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "QGCGeo.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtPositioning/QGeoCoordinate>

#include <QtCore/QtMath>
#include <limits>

QGC_LOGGING_CATEGORY(TerrainQueryOpenElevationLog, "Terrain.TerrainQueryOpenElevation")

namespace {
    constexpr int kMaxCarpetGridSize = 10000;
    constexpr int kMaxCarpetTotalPoints = 100000;
    constexpr double kDefaultSpacingDegrees = (1.0 / 3600);
    constexpr double kDefaultSpacingMeters = 30.0;
}

TerrainQueryOpenElevation::TerrainQueryOpenElevation(QObject *parent)
    : TerrainOnlineQuery(parent)
{
    qCDebug(TerrainQueryOpenElevationLog) << this;
}

TerrainQueryOpenElevation::~TerrainQueryOpenElevation()
{
    qCDebug(TerrainQueryOpenElevationLog) << this;
}

void TerrainQueryOpenElevation::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    _queryMode = TerrainQuery::QueryModeCoordinates;
    _sendQuery(coordinates);
}

void TerrainQueryOpenElevation::requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    const double totalDistance = QGCGeo::geodesicDistance(fromCoord, toCoord);
    const int numPoints = qMax(2, qCeil(totalDistance / kDefaultSpacingMeters) + 1);

    QList<QGeoCoordinate> coordinates = QGCGeo::interpolatePath(fromCoord, toCoord, numPoints);

    if (coordinates.size() >= 2) {
        _pathDistanceBetween = QGCGeo::geodesicDistance(coordinates[0], coordinates[1]);
        _pathFinalDistanceBetween = QGCGeo::geodesicDistance(coordinates[coordinates.size() - 2], coordinates.last());
    } else {
        _pathDistanceBetween = totalDistance;
        _pathFinalDistanceBetween = totalDistance;
    }

    _queryMode = TerrainQuery::QueryModePath;
    _sendQuery(coordinates);
}

void TerrainQueryOpenElevation::requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly)
{
    if (swCoord.longitude() > neCoord.longitude() || swCoord.latitude() > neCoord.latitude()) {
        qCWarning(TerrainQueryOpenElevationLog) << "Invalid carpet bounds: SW must be south-west of NE";
        _requestFailed();
        return;
    }

    _carpetStatsOnly = statsOnly;

    _carpetGridSizeLat = qCeil((neCoord.latitude() - swCoord.latitude()) / kDefaultSpacingDegrees);
    _carpetGridSizeLon = qCeil((neCoord.longitude() - swCoord.longitude()) / kDefaultSpacingDegrees);

    if (_carpetGridSizeLat <= 0 || _carpetGridSizeLon <= 0) {
        qCWarning(TerrainQueryOpenElevationLog) << "Carpet area too small";
        _requestFailed();
        return;
    }

    if (_carpetGridSizeLat > kMaxCarpetGridSize || _carpetGridSizeLon > kMaxCarpetGridSize) {
        qCWarning(TerrainQueryOpenElevationLog) << "Carpet area too large";
        _requestFailed();
        return;
    }

    const int totalPoints = (_carpetGridSizeLat + 1) * (_carpetGridSizeLon + 1);
    if (totalPoints > kMaxCarpetTotalPoints) {
        qCWarning(TerrainQueryOpenElevationLog) << "Carpet total points exceed limit:" << totalPoints << ">" << kMaxCarpetTotalPoints;
        _requestFailed();
        return;
    }

    QList<QGeoCoordinate> coordinates;
    for (int latIdx = 0; latIdx <= _carpetGridSizeLat; latIdx++) {
        const double lat = swCoord.latitude() + (latIdx * kDefaultSpacingDegrees);
        for (int lonIdx = 0; lonIdx <= _carpetGridSizeLon; lonIdx++) {
            const double lon = swCoord.longitude() + (lonIdx * kDefaultSpacingDegrees);
            (void) coordinates.append(QGeoCoordinate(lat, lon));
        }
    }
    _carpetGridSizeLat += 1;
    _carpetGridSizeLon += 1;

    _queryMode = TerrainQuery::QueryModeCarpet;
    _sendQuery(coordinates);
}

void TerrainQueryOpenElevation::_sendQuery(const QList<QGeoCoordinate> &coordinates)
{
    _expectedResultCount = coordinates.count();

    QJsonArray locationsArray;
    for (const QGeoCoordinate &coord : coordinates) {
        QJsonObject location;
        location[QStringLiteral("latitude")] = coord.latitude();
        location[QStringLiteral("longitude")] = coord.longitude();
        locationsArray.append(location);
    }

    QJsonObject requestBody;
    requestBody[QStringLiteral("locations")] = locationsArray;

    const QByteArray jsonData = QJsonDocument(requestBody).toJson(QJsonDocument::Compact);

    QUrl url(QString(OpenElevationProvider::kProviderURL) + QStringLiteral("/api/v1/lookup"));
    qCDebug(TerrainQueryOpenElevationLog) << "POST" << url << "locations:" << coordinates.count();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
#ifdef QT_DEBUG
    QSslConfiguration sslConf = request.sslConfiguration();
    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConf);
#endif

    QNetworkReply* const networkReply = _networkManager->post(request, jsonData);
    if (!networkReply) {
        qCWarning(TerrainQueryOpenElevationLog) << "QNetworkManager::post did not return QNetworkReply";
        _requestFailed();
        return;
    }

    QGCNetworkHelper::ignoreSslErrorsIfNeeded(networkReply);

    (void) connect(networkReply, &QNetworkReply::finished, this, &TerrainQueryOpenElevation::_requestFinished);
    (void) connect(networkReply, &QNetworkReply::sslErrors, this, &TerrainQueryOpenElevation::_sslErrors);
    (void) connect(networkReply, &QNetworkReply::errorOccurred, this, &TerrainQueryOpenElevation::_requestError);
}

void TerrainQueryOpenElevation::_requestFinished()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        qCWarning(TerrainQueryOpenElevationLog) << "null reply";
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(TerrainQueryOpenElevationLog) << "error:url:data" << reply->error() << reply->url() << reply->readAll();
        reply->deleteLater();
        _requestFailed();
        return;
    }

    const QByteArray responseBytes = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(TerrainQueryOpenElevationLog) << "JSON parse error:" << parseError.errorString();
        _requestFailed();
        return;
    }

    const QJsonObject rootObj = jsonDoc.object();
    const QJsonArray resultsArray = rootObj[QStringLiteral("results")].toArray();
    if (resultsArray.isEmpty()) {
        qCWarning(TerrainQueryOpenElevationLog) << "Empty results array in response";
        _requestFailed();
        return;
    }

    if (resultsArray.count() != _expectedResultCount) {
        qCWarning(TerrainQueryOpenElevationLog) << "Result count mismatch: expected" << _expectedResultCount << "got" << resultsArray.count();
        _requestFailed();
        return;
    }

    QList<double> elevations;
    elevations.reserve(resultsArray.count());
    for (const QJsonValue &val : resultsArray) {
        const QJsonObject result = val.toObject();
        (void) elevations.append(result[QStringLiteral("elevation")].toDouble());
    }

    qCDebug(TerrainQueryOpenElevationLog) << "received" << elevations.count() << "elevations";

    switch (_queryMode) {
    case TerrainQuery::QueryModeCoordinates:
        _parseCoordinateData(elevations);
        break;
    case TerrainQuery::QueryModePath:
        _parsePathData(elevations);
        break;
    case TerrainQuery::QueryModeCarpet:
        _parseCarpetData(elevations);
        break;
    default:
        break;
    }
}

void TerrainQueryOpenElevation::_parseCoordinateData(const QList<double> &elevations)
{
    emit coordinateHeightsReceived(true, elevations);
}

void TerrainQueryOpenElevation::_parsePathData(const QList<double> &elevations)
{
    emit pathHeightsReceived(true, _pathDistanceBetween, _pathFinalDistanceBetween, elevations);
}

void TerrainQueryOpenElevation::_parseCarpetData(const QList<double> &elevations)
{
    double minHeight = std::numeric_limits<double>::max();
    double maxHeight = std::numeric_limits<double>::lowest();

    QList<QList<double>> carpet;

    for (int latIdx = 0; latIdx < _carpetGridSizeLat; latIdx++) {
        QList<double> row;
        for (int lonIdx = 0; lonIdx < _carpetGridSizeLon; lonIdx++) {
            const int idx = (latIdx * _carpetGridSizeLon) + lonIdx;
            if (idx < elevations.count()) {
                const double h = elevations[idx];
                if (h < minHeight) minHeight = h;
                if (h > maxHeight) maxHeight = h;
                if (!_carpetStatsOnly) {
                    row.append(h);
                }
            }
        }
        if (!_carpetStatsOnly) {
            carpet.append(row);
        }
    }

    emit carpetHeightsReceived(true, minHeight, maxHeight, carpet);
}
