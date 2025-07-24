/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TerrainQueryInterface.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtPositioning/QGeoCoordinate>

class TerrainTile;
class QNetworkAccessManager;
class UnitTestTerrainQuery;

Q_DECLARE_LOGGING_CATEGORY(TerrainTileManagerLog)

class TerrainTileManager : public QObject
{
    Q_OBJECT

    friend class UnitTestTerrainQuery;
public:
    explicit TerrainTileManager(QObject *parent = nullptr);
    ~TerrainTileManager();

    static TerrainTileManager *instance();

    /// Either returns altitudes from cache or queues database request
    ///     @param[out] error true: altitude not returned due to error, false: altitudes returned
    ///     @return true: altitude returned (check error as well), false: database query queued (altitudes not returned)
    bool getAltitudesForCoordinates(const QList<QGeoCoordinate> &coordinates, QList<double> &altitudes, bool &error);

    void addCoordinateQuery(TerrainQueryInterface *terrainQueryInterface, const QList<QGeoCoordinate> &coordinates);
    void addPathQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &startPoint, const QGeoCoordinate &endPoint);

private slots:
    void _terrainDone();

private:
    /// Returns a list of individual coordinates along the requested path spaced according to the terrain tile value spacing
    static QList<QGeoCoordinate> _pathQueryToCoords(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord, double &distanceBetween, double &finalDistanceBetween);
    void _tileFailed();
    void _cacheTile(const QByteArray &data, const QString &hash);
    TerrainTile *_getCachedTile(const QString &hash);

    struct QueuedRequestInfo_t {
        TerrainQueryInterface *terrainQueryInterface;
        TerrainQuery::QueryMode queryMode;
        double distanceBetween;                         ///< Distance between each returned height
        double finalDistanceBetween;                    ///< Distance between for final height
        QList<QGeoCoordinate> coordinates;
    };

    QQueue<QueuedRequestInfo_t> _requestQueue;
    TerrainQuery::State _state = TerrainQuery::State::Idle;

    QMutex _tilesMutex;
    QHash<QString, TerrainTile*> _tiles;

    QNetworkAccessManager *_networkManager = nullptr;
};
