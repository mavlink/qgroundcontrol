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
#include "QGCQGeoCoordinate.h"

#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkProxy>
#include <QSet>

QGC_LOGGING_CATEGORY(AirMapManagerLog, "AirMapManagerLog")

AirMapManager::AirMapManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    _updateTimer.setInterval(2000);
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

    //qDebug() << url.toString(QUrl::FullyEncoded);
    //qDebug() << "Settings API key: " << _toolbox->settingsManager()->appSettings()->airMapKey()->rawValueString();

    request.setRawHeader("X-API-Key", _toolbox->settingsManager()->appSettings()->airMapKey()->rawValueString().toUtf8());

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

    if (_state == State::RetrieveItems) {
        if (--_numAwaitingItems == 0) {
            _state = State::Idle;
            // TODO: handle properly
        }
    }

    // FIXME
    qWarning() << errorMsg;
}

void AirMapManager::_updateToROI(void)
{
    if (_state != State::Idle) {
        qDebug() << "Error: state not idle (yet)";
        // restart timer?
        return;
    }

    // Build up the polygon for the query

//    QJsonObject     polygonJson;
//
//    polygonJson["type"] = "Polygon";
//
//    QGeoCoordinate left =   _roiCenter.atDistanceAndAzimuth(_roiRadius, -90);
//    QGeoCoordinate right =  _roiCenter.atDistanceAndAzimuth(_roiRadius, 90);
//    QGeoCoordinate top =    _roiCenter.atDistanceAndAzimuth(_roiRadius, 0);
//    QGeoCoordinate bottom = _roiCenter.atDistanceAndAzimuth(_roiRadius, 180);
//
//    QGeoCoordinate topLeft(top.latitude(), left.longitude());
//    QGeoCoordinate topRight(top.latitude(), right.longitude());
//    QGeoCoordinate bottomLeft(bottom.latitude(), left.longitude());
//    QGeoCoordinate bottomRight(bottom.latitude(), left.longitude());
//
//    QJsonValue  coordValue;
//    QJsonArray  rgVertex;
//
//    // GeoJson polygons are right handed and include duplicate first and last vertex
//    JsonHelper::saveGeoJsonCoordinate(topLeft, false /* writeAltitude */, coordValue);
//    rgVertex.append(coordValue);
//    JsonHelper::saveGeoJsonCoordinate(bottomLeft, false /* writeAltitude */, coordValue);
//    rgVertex.append(coordValue);
//    JsonHelper::saveGeoJsonCoordinate(bottomRight, false /* writeAltitude */, coordValue);
//    rgVertex.append(coordValue);
//    JsonHelper::saveGeoJsonCoordinate(topRight, false /* writeAltitude */, coordValue);
//    rgVertex.append(coordValue);
//    JsonHelper::saveGeoJsonCoordinate(topLeft, false /* writeAltitude */, coordValue);
//    rgVertex.append(coordValue);
//
//    QJsonArray rgPolygon;
//    rgPolygon.append(rgVertex);
//
//    polygonJson["coordinates"] = rgPolygon;
//    QJsonDocument polygonJsonDoc(polygonJson);

    // Build up the http query

    QUrlQuery airspaceQuery;

    airspaceQuery.addQueryItem(QStringLiteral("latitude"), QString::number(_roiCenter.latitude(), 'f', 10));
    airspaceQuery.addQueryItem(QStringLiteral("longitude"), QString::number(_roiCenter.longitude(), 'f', 10));
    airspaceQuery.addQueryItem(QStringLiteral("weather"), QStringLiteral("true"));
    airspaceQuery.addQueryItem(QStringLiteral("buffer"), QString::number(_roiRadius, 'f', 0));

    QUrl airMapAirspaceUrl(QStringLiteral("https://api.airmap.com/status/alpha/point"));
    airMapAirspaceUrl.setQuery(airspaceQuery);

    _state = State::RetrieveList;
    _get(airMapAirspaceUrl);
}

void AirMapManager::_parseAirspaceJson(const QJsonDocument& airspaceDoc)
{
    if (!airspaceDoc.isObject()) {
        // FIXME
        return;
    }

    QJsonObject rootObject = airspaceDoc.object();

    switch(_state) {
        case State::RetrieveList:
        {
            QSet<QString> advisorySet;
            const QJsonArray& advisoriesArray = rootObject["data"].toObject()["advisories"].toArray();
            for (int i=0; i< advisoriesArray.count(); i++) {
                const QJsonObject& advisoryObject = advisoriesArray[i].toObject();
                QString advisoryId(advisoryObject["id"].toString());
                qDebug() << "Adivsory id: " << advisoryId;
                advisorySet.insert(advisoryId);
            }

            for (const auto& advisoryId : advisorySet) {
                QUrl url(QStringLiteral("https://api.airmap.com/airspace/v2/")+advisoryId);
                _get(url);
            }
            _numAwaitingItems = advisorySet.size();
            _state = State::RetrieveItems;
        }
            break;

        case State::RetrieveItems:
        {
            qDebug() << "got item";
            const QJsonArray& airspaceArray = rootObject["data"].toArray();
            for (int i=0; i< airspaceArray.count(); i++) {
                const QJsonObject& airspaceObject = airspaceArray[i].toObject();
                QString airspaceType(airspaceObject["type"].toString());
                QString airspaceId(airspaceObject["id"].toString());
                QString airspaceName(airspaceObject["name"].toString());
                const QJsonObject& airspaceGeometry(airspaceObject["geometry"].toObject());
                QString geometryType(airspaceGeometry["type"].toString());
                qDebug() << "Airspace ID:" << airspaceId << "name:" << airspaceName << "type:" << airspaceType << "geometry:" << geometryType;
                if (geometryType == "Polygon") {

                    const QJsonArray& airspaceCoordinates(airspaceGeometry["coordinates"].toArray()[0].toArray());
                    QString errorString;
                    QmlObjectListModel list;
                    if (JsonHelper::loadPolygon(airspaceCoordinates, list, this, errorString)) {
                        QVariantList polygon;
                        for (int i = 0; i < list.count(); ++i) {
                            polygon.append(QVariant::fromValue(((QGCQGeoCoordinate*)list[i])->coordinate()));
                        }
                        list.clearAndDeleteContents();
                        _nextPolygonList.append(new PolygonAirspaceRestriction(polygon));
                    } else {
                        //FIXME
                        qDebug() << errorString;
                    }

                } else {
                    // TODO: are there any circles?
                    qDebug() << "Unknown geometry type:" << geometryType;
                }
            }

            if (--_numAwaitingItems == 0) {
                _polygonList.clearAndDeleteContents();
                _circleList.clearAndDeleteContents();
                for (const auto& circle : _nextcircleList) {
                    _circleList.append(circle);
                }
                _nextcircleList.clear();
                for (const auto& polygon : _nextPolygonList) {
                    _polygonList.append(polygon);
                }
                _nextPolygonList.clear();

                _state = State::Idle;
            }
        }
            break;
        default:
            qDebug() << "Error: wrong state";
            break;
    }

    // https://api.airmap.com/airspace/v2/search API
//    const QJsonArray& airspaceArray = rootObject["data"].toArray();
//    for (int i=0; i< airspaceArray.count(); i++) {
//        const QJsonObject& airspaceObject = airspaceArray[i].toObject();
//        QString airspaceType(airspaceObject["type"].toString());
//        qDebug() << airspaceType;
//        qDebug() << airspaceObject["name"].toString();
//        QGeoCoordinate center(airspaceObject["latitude"].toDouble(), airspaceObject["longitude"].toDouble());
//        qDebug() << center;
//        if (airspaceType == "airport") {
//            _circleList.append(new CircularAirspaceRestriction(center, 5000));
//        }
//    }
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
