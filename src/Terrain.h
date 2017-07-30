/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QNetworkAccessManager>

/* usage example:
    ElevationProvider *p = new ElevationProvider();
    QList<QGeoCoordinate> coordinates;
    QGeoCoordinate c(47.379243, 8.548265);
    coordinates.push_back(c);
    c.setLatitude(c.latitude()+0.01);
    coordinates.push_back(c);
    p->queryTerrainData(coordinates);
 */


class ElevationProvider : public QObject
{
    Q_OBJECT
public:
    ElevationProvider(QObject* parent = NULL);

    /**
     * Async elevation query for a list of lon,lat coordinates. When the query is done, the terrainData() signal
     * is emitted.
     * @param coordinates
     * @return true on success
     */
    bool queryTerrainData(const QList<QGeoCoordinate>& coordinates);

signals:
    void terrainData(bool success, QList<float> altitudes);

private slots:
    void _requestFinished();
private:

    enum class State {
        Idle,
        Downloading,
    };

    State                   _state = State::Idle;
    QNetworkAccessManager   _networkManager;
};
