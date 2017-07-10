/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "QGCApplication.h"

#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkProxy>
#include <QSet>

QGC_LOGGING_CATEGORY(AirMapManagerLog, "AirMapManagerLog")


AirMapLogin::AirMapLogin(QNetworkAccessManager& networkManager, const QString& APIKey)
    : _networkManager(networkManager), _APIKey(APIKey)
{
}

void AirMapLogin::setCredentials(const QString& clientID, const QString& userName, const QString& password)
{
    logout();
    _clientID = clientID;
    _userName = userName;
    _password = password;
}

void AirMapLogin::login()
{
    if (isLoggedIn() || _isLoginInProgress) {
        return;
    }
    _isLoginInProgress = true;

    QUrlQuery postData;
    postData.addQueryItem(QStringLiteral("grant_type"), "password");
    postData.addQueryItem(QStringLiteral("client_id"), _clientID);
    postData.addQueryItem(QStringLiteral("connection"), "Username-Password-Authentication");
    postData.addQueryItem(QStringLiteral("username"), _userName);
    postData.addQueryItem(QStringLiteral("password"), _password);
    postData.addQueryItem(QStringLiteral("scope"), "openid offline_access");
    postData.addQueryItem(QStringLiteral("device"), "test");

    QUrl url(QStringLiteral("https://sso.airmap.io/oauth/ro"));

    _post(url, postData.toString(QUrl::FullyEncoded).toUtf8());
}

void AirMapLogin::_requestFinished(void)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    _isLoginInProgress = false;

    QByteArray responseBytes = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QNetworkReply::NetworkError networkError = QNetworkReply::NetworkError::UnknownNetworkError;
        emit loginFailure(networkError, "", "");
        return;
    }
    QJsonObject rootObject = responseJson.object();

    // When an error occurs we still end up here
    if (reply->error() != QNetworkReply::NoError) {
        QJsonValue errorDescription = rootObject.value("error_description");
        QString serverError = "";
        if (errorDescription.isString()) {
            serverError = errorDescription.toString();
        }
        emit loginFailure(reply->error(), reply->errorString(), serverError);
        return;
    }

    _JWTToken = rootObject["id_token"].toString();

    if (_JWTToken == "") { // make sure we got a token
        QNetworkReply::NetworkError networkError = QNetworkReply::NetworkError::AuthenticationRequiredError;
        emit loginFailure(networkError, "", "");
        return;
    }

    emit loginSuccess();
}

void AirMapLogin::_requestError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    // handled in _requestFinished()
}

void AirMapLogin::_post(QUrl url, const QByteArray& postData)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    request.setRawHeader("X-API-Key", _APIKey.toUtf8());

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager.setProxy(tProxy);

    QNetworkReply* networkReply = _networkManager.post(request, postData);
    if (!networkReply) {
        QNetworkReply::NetworkError networkError = QNetworkReply::NetworkError::UnknownNetworkError;
        emit loginFailure(networkError, "", "");
        return;
    }

    connect(networkReply, &QNetworkReply::finished,                                                                 this, &AirMapLogin::_requestFinished);
    connect(networkReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &AirMapLogin::_requestError);
}


AirMapNetworking::AirMapNetworking(SharedData& networkingData)
    : _networkingData(networkingData)
{
}

void AirMapNetworking::post(QUrl url, const QByteArray& postData, bool isJsonData, bool requiresLogin)
{
    QNetworkRequest request(url);
    if (isJsonData) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    } else {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    }

    request.setRawHeader("X-API-Key", _networkingData.airmapAPIKey.toUtf8());

    if (requiresLogin) {
        if (_networkingData.login.isLoggedIn()) {
            request.setRawHeader("Authorization", (QString("Bearer ")+_networkingData.login.JWTToken()).toUtf8());
        } else {
            connect(&_networkingData.login, &AirMapLogin::loginSuccess, this, &AirMapNetworking::_loginSuccess);
            connect(&_networkingData.login, &AirMapLogin::loginFailure, this, &AirMapNetworking::_loginFailure);
            _pendingRequest.type = RequestType::POST;
            _pendingRequest.url = url;
            _pendingRequest.postData = postData;
            _pendingRequest.isJsonData = isJsonData;
            _pendingRequest.requiresLogin = requiresLogin;
            _networkingData.login.login();
            return;
        }
    }

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkingData.networkManager.setProxy(tProxy);

    QNetworkReply* networkReply = _networkingData.networkManager.post(request, postData);
    if (!networkReply) {
        QNetworkReply::NetworkError networkError = QNetworkReply::NetworkError::UnknownNetworkError;
        emit error(networkError, "", "");
        return;
    }

    connect(networkReply, &QNetworkReply::finished, this, &AirMapNetworking::_requestFinished);
}

void AirMapNetworking::_loginSuccess()
{
    disconnect(&_networkingData.login, &AirMapLogin::loginSuccess, this, &AirMapNetworking::_loginSuccess);
    disconnect(&_networkingData.login, &AirMapLogin::loginFailure, this, &AirMapNetworking::_loginFailure);

    if (_pendingRequest.type == RequestType::GET) {
        get(_pendingRequest.url, _pendingRequest.requiresLogin);
    } else if (_pendingRequest.type == RequestType::POST) {
        post(_pendingRequest.url, _pendingRequest.postData, _pendingRequest.isJsonData, _pendingRequest.requiresLogin);
    }
    _pendingRequest.type = RequestType::None;
}
void AirMapNetworking::_loginFailure(QNetworkReply::NetworkError networkError, const QString& errorString, const QString& serverErrorMessage)
{
    disconnect(&_networkingData.login, &AirMapLogin::loginSuccess, this, &AirMapNetworking::_loginSuccess);
    disconnect(&_networkingData.login, &AirMapLogin::loginFailure, this, &AirMapNetworking::_loginFailure);
    emit error(networkError, errorString, serverErrorMessage);
}

void AirMapNetworking::get(QUrl url, bool requiresLogin)
{
    QNetworkRequest request(url);

    request.setRawHeader("X-API-Key", _networkingData.airmapAPIKey.toUtf8());

    if (requiresLogin) {
        if (_networkingData.login.isLoggedIn()) {
            request.setRawHeader("Authorization", (QString("Bearer ")+_networkingData.login.JWTToken()).toUtf8());
        } else {
            connect(&_networkingData.login, &AirMapLogin::loginSuccess, this, &AirMapNetworking::_loginSuccess);
            connect(&_networkingData.login, &AirMapLogin::loginFailure, this, &AirMapNetworking::_loginFailure);
            _pendingRequest.type = RequestType::GET;
            _pendingRequest.url = url;
            _pendingRequest.requiresLogin = requiresLogin;
            _networkingData.login.login();
            return;
        }
    }

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkingData.networkManager.setProxy(tProxy);

    QNetworkReply* networkReply = _networkingData.networkManager.get(request);
    if (!networkReply) {
        QNetworkReply::NetworkError networkError = QNetworkReply::NetworkError::UnknownNetworkError;
        emit error(networkError, "", "");
        return;
    }

    connect(networkReply, &QNetworkReply::finished, this, &AirMapNetworking::_requestFinished);
}

void AirMapNetworking::_requestFinished(void)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

    // When an error occurs we still end up here
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray responseBytes = reply->readAll();

        QJsonParseError parseError;
        QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
        QJsonObject rootObject = responseJson.object();
        QString serverError = "";
        if (rootObject.contains("data")) { // eg. in case of a conflict message
            serverError = rootObject["data"].toObject()["message"].toString();
        } else if (rootObject.contains("error_description")) { // eg. login failure
            serverError = rootObject["error_description"].toString();
        } else if (rootObject.contains("message")) { // eg. api key failure
            serverError = rootObject["message"].toString();
        }
        emit error(reply->error(), reply->errorString(), serverError);
        return;
    }

    // Check for redirection
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectionTarget.isNull()) {
        QUrl redirectUrl = reply->url().resolved(redirectionTarget.toUrl());
        get(redirectUrl);
        reply->deleteLater();
        return;
    }

    QByteArray responseBytes = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(AirMapManagerLog) << "JSON parse error:" << parseError.errorString();
    }
    emit finished(parseError, responseJson);
}



AirspaceRestrictionManager::AirspaceRestrictionManager(AirMapNetworking::SharedData& sharedData)
    : _networking(sharedData)
{
    connect(&_networking, &AirMapNetworking::finished, this, &AirspaceRestrictionManager::_parseAirspaceJson);
    connect(&_networking, &AirMapNetworking::error, this, &AirspaceRestrictionManager::_error);
}

void AirspaceRestrictionManager::updateROI(const QGeoCoordinate& center, double radiusMeters)
{
    if (_state != State::Idle) {
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

    airspaceQuery.addQueryItem(QStringLiteral("latitude"), QString::number(center.latitude(), 'f', 10));
    airspaceQuery.addQueryItem(QStringLiteral("longitude"), QString::number(center.longitude(), 'f', 10));
    airspaceQuery.addQueryItem(QStringLiteral("weather"), QStringLiteral("true"));
    airspaceQuery.addQueryItem(QStringLiteral("buffer"), QString::number(radiusMeters, 'f', 0));

    QUrl airMapAirspaceUrl(QStringLiteral("https://api.airmap.com/status/alpha/point"));
    airMapAirspaceUrl.setQuery(airspaceQuery);

    _state = State::RetrieveList;
    _networking.get(airMapAirspaceUrl);
}

void AirspaceRestrictionManager::_parseAirspaceJson(QJsonParseError parseError, QJsonDocument airspaceDoc)
{

    QJsonObject rootObject = airspaceDoc.object();

    switch(_state) {
        case State::RetrieveList:
        {
            QSet<QString> advisorySet;
            const QJsonArray& advisoriesArray = rootObject["data"].toObject()["advisories"].toArray();
            for (int i=0; i< advisoriesArray.count(); i++) {
                const QJsonObject& advisoryObject = advisoriesArray[i].toObject();
                QString advisoryId(advisoryObject["id"].toString());
                qCDebug(AirMapManagerLog) << "Advisory id: " << advisoryId;
                advisorySet.insert(advisoryId);
            }

            for (const auto& advisoryId : advisorySet) {
                QUrl url(QStringLiteral("https://api.airmap.com/airspace/v2/")+advisoryId);
                _networking.get(url);
            }
            _numAwaitingItems = advisorySet.size();
            _state = State::RetrieveItems;
        }
            break;

        case State::RetrieveItems:
        {
            const QJsonArray& airspaceArray = rootObject["data"].toArray();
            for (int i = 0; i < airspaceArray.count(); i++) {
                const QJsonObject& airspaceObject = airspaceArray[i].toObject();
                QString airspaceType(airspaceObject["type"].toString());
                QString airspaceId(airspaceObject["id"].toString());
                QString airspaceName(airspaceObject["name"].toString());
                const QJsonObject& airspaceGeometry(airspaceObject["geometry"].toObject());
                QString geometryType(airspaceGeometry["type"].toString());
                qCDebug(AirMapManagerLog) << "Airspace ID:" << airspaceId << "name:" << airspaceName << "type:" << airspaceType << "geometry:" << geometryType;

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
                        //TODO
                        qWarning() << errorString;
                    }

                } else {
                    // TODO: are there any circles?
                    qWarning() << "Unknown geometry type:" << geometryType;
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
            qCDebug(AirMapManagerLog) << "Error: wrong state";
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

void AirspaceRestrictionManager::_error(QNetworkReply::NetworkError code, const QString& errorString,
        const QString& serverErrorMessage)
{
    qCWarning(AirMapManagerLog) << "AirspaceRestrictionManager::_error" << code << serverErrorMessage;

    if (_state == State::RetrieveItems) {
        if (--_numAwaitingItems == 0) {
            _state = State::Idle;
            // TODO: handle properly: update _polygonList...
        }
    } else {
        _state = State::Idle;
    }
    emit networkError(code, errorString, serverErrorMessage);
}

AirMapFlightManager::AirMapFlightManager(AirMapNetworking::SharedData& sharedData)
    : _networking(sharedData)
{
    connect(&_networking, &AirMapNetworking::finished, this, &AirMapFlightManager::_parseJson);
    connect(&_networking, &AirMapNetworking::error, this, &AirMapFlightManager::_error);
}

void AirMapFlightManager::createFlight(const QList<MissionItem*>& missionItems)
{
    if (!_networking.getLogin().hasCredentials()) {
        qCDebug(AirMapManagerLog) << "Login Credentials not set: will not send flight";
        return;
    }

    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapFlightManager::createFlight: State not idle";
        return;
    }

    QList<QGeoCoordinate> flight;

    // get the flight trajectory
    for(const auto &item : missionItems) {
        switch(item->command()) {
            case MAV_CMD_NAV_WAYPOINT:
            case MAV_CMD_NAV_LAND:
            case MAV_CMD_NAV_TAKEOFF:
                // TODO: others too?
            {
                // TODO: handle different coordinate frames?
                double lat = item->param5();
                double lon = item->param6();
                double alt = item->param7();
                flight.append(QGeoCoordinate(lat, lon, alt));
            }
                break;
            default:
                break;
        }
    }
    if (flight.empty()) {
        return;
    }

    qCDebug(AirMapManagerLog) << "uploading flight";

    QJsonObject root;
    root.insert("latitude", QJsonValue::fromVariant(flight[0].latitude()));
    root.insert("longitude", QJsonValue::fromVariant(flight[0].longitude()));
    QJsonObject geometryObject;
    geometryObject.insert("type", "LineString");
    QJsonArray coordinatesArray;
    for (const auto& coord : flight) {
        QJsonArray coordinate;
        coordinate.push_back(coord.longitude());
        coordinate.push_back(coord.latitude());
        coordinatesArray.push_back(coordinate);
    }
    geometryObject.insert("coordinates", coordinatesArray);

    root.insert("geometry", geometryObject);

    root.insert("public", QJsonValue::fromVariant(true));
    root.insert("notify", QJsonValue::fromVariant(true));

    _state = State::FlightUpload;

    QUrl url(QStringLiteral("https://api.airmap.com/flight/v2/path"));

    //qCDebug(AirMapManagerLog) << root;
    _networking.post(url, QJsonDocument(root).toJson(), true, true);
}


void AirMapFlightManager::_parseJson(QJsonParseError parseError, QJsonDocument doc)
{

    QJsonObject rootObject = doc.object();

    switch(_state) {
        case State::FlightUpload:
        {
            qDebug() << "flight uploaded:" << rootObject;
            const QJsonObject& dataObject = rootObject["data"].toObject();
            _currentFlightId = dataObject["id"].toString();
            qCDebug(AirMapManagerLog) << "Got Flight ID:" << _currentFlightId;
            _state = State::Idle;
        }
            break;
        default:
            qCDebug(AirMapManagerLog) << "Error: wrong state";
            break;
    }
}

void AirMapFlightManager::_error(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage)
{
    qCWarning(AirMapManagerLog) << "AirMapFlightManager::_error" << code << serverErrorMessage;
    _state = State::Idle;
    emit networkError(code, errorString, serverErrorMessage);
}

AirMapManager::AirMapManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox), _airspaceRestrictionManager(_networkingData), _flightManager(_networkingData)
{
    _updateTimer.setInterval(2000);
    _updateTimer.setSingleShot(true);
    connect(&_updateTimer, &QTimer::timeout, this, &AirMapManager::_updateToROI);
    connect(&_airspaceRestrictionManager, &AirspaceRestrictionManager::networkError, this, &AirMapManager::_networkError);
    connect(&_flightManager, &AirMapFlightManager::networkError, this, &AirMapManager::_networkError);
}

AirMapManager::~AirMapManager()
{

}

void AirMapManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    _networkingData.airmapAPIKey = toolbox->settingsManager()->appSettings()->airMapKey()->rawValueString();

    // TODO: set login credentials from config
    QString clientID = "";
    QString userName = "";
    QString password = "";
    _networkingData.login.setCredentials(clientID, userName, password);
}

void AirMapManager::setROI(QGeoCoordinate& center, double radiusMeters)
{
    _roiCenter = center;
    _roiRadius = radiusMeters;
    _updateTimer.start();
}

void AirMapManager::_updateToROI(void)
{
    if (!hasAPIKey()) {
        qCDebug(AirMapManagerLog) << "API key not set. Not updating Airspace";
        return;
    }
    _airspaceRestrictionManager.updateROI(_roiCenter, _roiRadius);
}

void AirMapManager::_networkError(QNetworkReply::NetworkError code, const QString& errorString, const QString& serverErrorMessage)
{
    QString errorDetails = "";
    if (code == QNetworkReply::NetworkError::ContentOperationNotPermittedError) {
        errorDetails = " (invalid API key?)";
    } else if (code == QNetworkReply::NetworkError::AuthenticationRequiredError) {
        errorDetails = " (authentication failure)";
    }
    if (serverErrorMessage.length() > 0) {
        // the errorString is a bit verbose and redundant with serverErrorMessage. So we just show the server error.
        qgcApp()->showMessage(QString("AirMap error%1. Response from Server: %2").arg(errorDetails).arg(serverErrorMessage));
    } else {
        qgcApp()->showMessage(QString("AirMap error: %1%2").arg(errorString).arg(errorDetails));
    }
}

void AirMapManager::createFlight(const QList<MissionItem*>& missionItems)
{
    if (!hasAPIKey()) {
        qCDebug(AirMapManagerLog) << "API key not set. Will not create a flight";
        return;
    }
    _flightManager.createFlight(missionItems);
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
