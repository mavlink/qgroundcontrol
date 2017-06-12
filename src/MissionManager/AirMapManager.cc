/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "JsonHelper.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkProxy>

QGC_LOGGING_CATEGORY(AirMapManagerLog, "AirMapManagerLog")

AirMapManager::AirMapManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    _updateTimer.setInterval(5000);
    _updateTimer.setSingleShot(true);
    connect(&_updateTimer, &QTimer::timeout, this, &AirMapManager::_updateToROI);
}

AirMapManager::~AirMapManager()
{

}

void AirMapManager::setROI(QGeoCoordinate& center, double radiusMeters)
{
    _roiCenter = center;
    _roiRadius = radiusMeters;
    _updateTimer.start();
}

void AirMapManager::_get(QUrl url)
{
    QNetworkRequest request(url);

    qDebug() << url.toString(QUrl::FullyEncoded);
    qDebug() << _toolbox->settingsManager()->appSettings()->airMapKey()->rawValueString();

    request.setRawHeader("X-API-Key",  _toolbox->settingsManager()->appSettings()->airMapKey()->rawValueString().toUtf8());

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager.setProxy(tProxy);

    QNetworkReply* networkReply = _networkManager.get(request);
    if (!networkReply) {
        // FIXME
        qWarning() << "QNetworkAccessManager::get failed";
        return;
    }

    connect(networkReply, &QNetworkReply::finished,                                                                 this, &AirMapManager::_getFinished);
    connect(networkReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &AirMapManager::_getError);
}

void AirMapManager::_getFinished(void)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

    // When an error occurs we still end up here. So bail out in those cases.
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    // Check for redirection
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectionTarget.isNull()) {
        QUrl redirectUrl = reply->url().resolved(redirectionTarget.toUrl());
        _get(redirectUrl);
        reply->deleteLater();
        return;
    }

    qDebug() << "_getFinished";
    QByteArray responseBytes = reply->readAll();
    //qDebug() << responseBytes;

    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    // FIXME: Error handling
    qDebug() << responseJson.isNull() << parseError.errorString();
    //qDebug().noquote() << responseJson.toJson();

    _parseAirspaceJson(responseJson);
}

void AirMapManager::_getError(QNetworkReply::NetworkError code)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

    QString errorMsg;

    if (code == QNetworkReply::OperationCanceledError) {
        errorMsg = "Download cancelled";

    } else if (code == QNetworkReply::ContentNotFoundError) {
        errorMsg = "Error: File Not Found";

    } else {
        errorMsg = QString("Error during download. Error: (%1, %2)").arg(code).arg(reply->errorString());
    }

    // FIXME
    qWarning() << errorMsg;
}

void AirMapManager::_updateToROI(void)
{
    // Build up the polygon for the query

    QJsonObject     polygonJson;

    polygonJson["type"] = "Polygon";

    QGeoCoordinate left =   _roiCenter.atDistanceAndAzimuth(_roiRadius, -90);
    QGeoCoordinate right =  _roiCenter.atDistanceAndAzimuth(_roiRadius, 90);
    QGeoCoordinate top =    _roiCenter.atDistanceAndAzimuth(_roiRadius, 0);
    QGeoCoordinate bottom = _roiCenter.atDistanceAndAzimuth(_roiRadius, 180);

    QGeoCoordinate topLeft(top.latitude(), left.longitude());
    QGeoCoordinate topRight(top.latitude(), right.longitude());
    QGeoCoordinate bottomLeft(bottom.latitude(), left.longitude());
    QGeoCoordinate bottomRight(bottom.latitude(), left.longitude());

    QJsonValue  coordValue;
    QJsonArray  rgVertex;

    // GeoJson polygons are right handed and include duplicate first and last vertex
    JsonHelper::saveGeoJsonCoordinate(topLeft, false /* writeAltitude */, coordValue);
    rgVertex.append(coordValue);
    JsonHelper::saveGeoJsonCoordinate(bottomLeft, false /* writeAltitude */, coordValue);
    rgVertex.append(coordValue);
    JsonHelper::saveGeoJsonCoordinate(bottomRight, false /* writeAltitude */, coordValue);
    rgVertex.append(coordValue);
    JsonHelper::saveGeoJsonCoordinate(topRight, false /* writeAltitude */, coordValue);
    rgVertex.append(coordValue);
    JsonHelper::saveGeoJsonCoordinate(topLeft, false /* writeAltitude */, coordValue);
    rgVertex.append(coordValue);

    QJsonArray rgPolygon;
    rgPolygon.append(rgVertex);

    polygonJson["coordinates"] = rgPolygon;
    QJsonDocument polygonJsonDoc(polygonJson);

    // Build up the http query

    QUrlQuery airspaceQuery;

    airspaceQuery.addQueryItem(QStringLiteral("geometry"), QString::fromUtf8(polygonJsonDoc.toJson(QJsonDocument::Compact)));
    airspaceQuery.addQueryItem(QStringLiteral("types"), QStringLiteral("airport,controlled_airspace,tfr,wildfire"));
    airspaceQuery.addQueryItem(QStringLiteral("full"), QStringLiteral("true"));
    airspaceQuery.addQueryItem(QStringLiteral("geometry_format"), QStringLiteral("geojson"));

    QUrl airMapAirspaceUrl(QStringLiteral("https://api.airmap.com/airspace/v2/search"));
    airMapAirspaceUrl.setQuery(airspaceQuery);

    _get(airMapAirspaceUrl);
}

void AirMapManager::_parseAirspaceJson(const QJsonDocument& airspaceDoc)
{
    if (!airspaceDoc.isObject()) {
        // FIXME
        return;
    }

    _polygonList.clearAndDeleteContents();
    _circleList.clearAndDeleteContents();

    QJsonObject rootObject = airspaceDoc.object();
    const QJsonArray& airspaceArray = rootObject["data"].toArray();
    for (int i=0; i< airspaceArray.count(); i++) {
        const QJsonObject& airspaceObject = airspaceArray[i].toObject();
        QString airspaceType(airspaceObject["type"].toString());
        qDebug() << airspaceType;
        qDebug() << airspaceObject["name"].toString();
        QGeoCoordinate center(airspaceObject["latitude"].toDouble(), airspaceObject["longitude"].toDouble());
        qDebug() << center;
        if (airspaceType == "airport") {
            _circleList.append(new CircularAirspaceRestriction(center, 8046.72));
        }
    }
}

AirspaceRestriction::AirspaceRestriction(QObject* parent)
    : QObject(parent)
{

}

PolygonAirspaceRestriction::PolygonAirspaceRestriction(const QVariantList& polygon, QObject* parent)
    : AirspaceRestriction(parent)
    , _polygon(polygon)
{

}

CircularAirspaceRestriction::CircularAirspaceRestriction(const QGeoCoordinate& center, double radius, QObject* parent)
    : AirspaceRestriction(parent)
    , _center(center)
    , _radius(radius)
{

}
