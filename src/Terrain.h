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
#include "QGCMapEngineData.h"
#include "QGCLoggingCategory.h"

#include <QObject>
#include <QGeoCoordinate>
#include <QNetworkAccessManager>
#include <QHash>
#include <QMutex>
#include <QtLocation/private/qgeotiledmapreply_p.h>

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
     * is emitted. This call directly looks elevations up online.
     * @param coordinates
     * @return true on success
     */
    bool queryTerrainDataPoints(const QList<QGeoCoordinate>& coordinates);

    /**
     * Async elevation query for a list of lon,lat coordinates. When the query is done, the terrainData() signal
     * is emitted. This call caches local elevation tables for faster lookup in the future.
     * @param coordinates
     * @return true on success
     */
    bool queryTerrainData(const QList<QGeoCoordinate>& coordinates);

signals:
    /// signal returning requested elevation data
    void terrainData(bool success, QList<float> altitudes);

private slots:
    void _requestFinished();                                    /// slot to handle download of elevation of list of coordinates
    void _fetchedTile();                                        /// slot to handle fetched elevation tiles

private:

    QString _getTileHash(const QGeoCoordinate& coordinate);     /// Method to create a unique string for each tile. Format: south_west_north_east as floats.

    enum class State {
        Idle,
        Downloading,
    };

    State                       _state = State::Idle;
    QNetworkAccessManager       _networkManager;
    QList<QGeoCoordinate>       _coordinates;

    static QMutex                       _tilesMutex;
    static QHash<QString, TerrainTile>  _tiles;
    static QStringList                  _downloadQueue;
};
