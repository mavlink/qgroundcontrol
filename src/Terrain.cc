/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Terrain.h"

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ElevationProvider::ElevationProvider(QObject* parent)
    : QObject(parent)
{

}

bool ElevationProvider::queryTerrainData(const QList<QGeoCoordinate>& coordinates)
{
    if (_state != State::Idle || coordinates.length() == 0) {
        return false;
    }

    QUrlQuery query;
    QString points = "";
    for (const auto& coordinate : coordinates) {
        points += QString::number(coordinate.latitude(), 'f', 10) + ","
                + QString::number(coordinate.longitude(), 'f', 10) + ",";
    }
    points = points.mid(0, points.length() - 1); // remove the last ','

    query.addQueryItem(QStringLiteral("points"), points);
    QUrl url(QStringLiteral("https://api.airmap.com/elevation/stage/srtm1/ele"));
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager.setProxy(tProxy);

    QNetworkReply* networkReply = _networkManager.get(request);
    if (!networkReply) {
        return false;
    }

    connect(networkReply, &QNetworkReply::finished, this, &ElevationProvider::_requestFinished);

    _state = State::Downloading;
    return true;
}

void ElevationProvider::_requestFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    QList<float> altitudes;
    _state = State::Idle;

    // When an error occurs we still end up here
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray responseBytes = reply->readAll();

        QJsonParseError parseError;
        QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
        qDebug() << responseJson;
        emit terrainData(false, altitudes);
        return;
    }

    QByteArray responseBytes = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit terrainData(false, altitudes);
        return;
    }
    QJsonObject rootObject = responseJson.object();
    QString status = rootObject["status"].toString();
    if (status == "success") {
        const QJsonArray& dataArray = rootObject["data"].toArray();
        for (int i = 0; i < dataArray.count(); i++) {
            altitudes.push_back(dataArray[i].toDouble());
        }

        emit terrainData(true, altitudes);
    }
    emit terrainData(false, altitudes);
}
