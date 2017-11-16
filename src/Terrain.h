/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TerrainTile.h"
#include "QGCLoggingCategory.h"

#include <QObject>
#include <QGeoCoordinate>
#include <QNetworkAccessManager>
#include <QHash>
#include <QMutex>

/* usage example:
    ElevationProvider *p = new ElevationProvider();
    QList<QGeoCoordinate> coordinates;
    QGeoCoordinate c(47.379243, 8.548265);
    coordinates.push_back(c);
    c.setLatitude(c.latitude()+0.01);
    coordinates.push_back(c);
    p->queryTerrainData(coordinates);
 */

Q_DECLARE_LOGGING_CATEGORY(TerrainLog)

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

    /**
     * Cache all data in rectangular region given by list of coordinates.
     *
     * @param coordinates
     * @return true on successful scheduling for download
     */
    bool cacheTerrainTiles(const QList<QGeoCoordinate>& coordinates);

    /**
     * Cache all data in rectangular region given by south west and north east corner.
     *
     * @param southWest
     * @param northEast
     * @return true on successful scheduling for download
     */
    bool cacheTerrainTiles(const QGeoCoordinate& southWest, const QGeoCoordinate& northEast);

signals:
    void terrainData(bool success, QList<float> altitudes);

private slots:
    void _requestFinished();
    void _requestFinishedTile();

private:

    QString _uniqueTileId(const QGeoCoordinate& coordinate);
    void    _downloadTiles(void);

    enum class State {
        Idle,
        Downloading,
    };

    State                       _state = State::Idle;
    QNetworkAccessManager       _networkManager;

    static QMutex                       _tilesMutex;
    static QHash<QString, TerrainTile>  _tiles;
    static QStringList                  _downloadQueue;
};
