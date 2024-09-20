/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QVariant>
#include <QtPositioning/QGeoCoordinate>

#include "TerrainQueryInterface.h"

class QTimer;

Q_DECLARE_LOGGING_CATEGORY(TerrainQueryLog)
Q_DECLARE_LOGGING_CATEGORY(TerrainQueryVerboseLog)

// IMPORTANT NOTE: The terrain query objects below must continue to live until the the terrain system signals data back through them.
// Because of that it makes object lifetime tricky. Normally you would use autoDelete = true such they delete themselves when they
// complete. The case for using autoDelete=false is where the query has not been "newed" as a standalone object.
//
// Another typical use case is to query some terrain data and while you are waiting for it to come back the underlying reason
// for that query changes and you end up needed to query again for a new set of data. In this case you are no longer intersted
// in the results of the previous query. The way to do that is to disconnect the data received signal on the old stale query
// when you create the new query.

/*===========================================================================*/

class TerrainAtCoordinateQuery;

class TerrainAtCoordinateBatchManager : public QObject
{
    Q_OBJECT

public:
    explicit TerrainAtCoordinateBatchManager(QObject *parent = nullptr);
    ~TerrainAtCoordinateBatchManager();

    static TerrainAtCoordinateBatchManager *instance();

    void addQuery(TerrainAtCoordinateQuery *terrainAtCoordinateQuery, const QList<QGeoCoordinate> &coordinates);

private slots:
    void _sendNextBatch();
    void _queryObjectDestroyed(QObject *elevationProvider);
    void _coordinateHeights(bool success, const QList<double> &heights);

private:
    struct QueuedRequestInfo_t {
        TerrainAtCoordinateQuery *terrainAtCoordinateQuery;
        QList<QGeoCoordinate> coordinates;
    };

    struct SentRequestInfo_t {
        TerrainAtCoordinateQuery *terrainAtCoordinateQuery;
        bool queryObjectDestroyed;
        qsizetype cCoord;
    };

    void _batchFailed();
    QString _stateToString(TerrainQuery::State state);

    QQueue<QueuedRequestInfo_t> _requestQueue;
    QList<SentRequestInfo_t> _sentRequests;
    TerrainQuery::State _state = TerrainQuery::State::Idle;
    QTimer *_batchTimer = nullptr;
    TerrainQueryInterface *_terrainQuery = nullptr;
    static constexpr int _batchTimeout = 500;
};

/*===========================================================================*/

/// NOTE: TerrainAtCoordinateQuery is not thread safe. All instances/calls to ElevationProvider must be on main thread.
class TerrainAtCoordinateQuery : public QObject
{
    Q_OBJECT

public:
    /// @param autoDelete true: object will delete itself after it signals results
    explicit TerrainAtCoordinateQuery(bool autoDelete, QObject *parent = nullptr);
    ~TerrainAtCoordinateQuery();

    /// Async terrain query for a list of lon,lat coordinates. When the query is done, the terrainData() signal
    /// is emitted.
    ///     @param coordinates to query
    void requestData(const QList<QGeoCoordinate> &coordinates);

    /// Either returns altitudes from cache or queues database request
    ///     @param[out] error true: altitude not returned due to error, false: altitudes returned
    /// @return true: altitude returned (check error as well), false: database query queued (altitudes not returned)
    static bool getAltitudesForCoordinates(const QList<QGeoCoordinate> &coordinates, QList<double> &altitudes, bool &error);

    void signalTerrainData(bool success, const QList<double> &heights);

signals:
    void terrainDataReceived(bool success, const QList<double> &heights);

private:
    bool _autoDelete = false;
};

/*===========================================================================*/

class TerrainPathQuery : public QObject
{
    Q_OBJECT

public:
    /// @param autoDelete true: object will delete itself after it signals results
    explicit TerrainPathQuery(bool autoDelete, QObject *parent = nullptr);
    ~TerrainPathQuery();

    /// Async terrain query for terrain heights between two lat/lon coordinates. When the query is done, the terrainData() signal
    /// is emitted.
    ///     @param coordinates to query
    void requestData(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord);

    struct PathHeightInfo_t {
        double distanceBetween;        ///< Distance between each height value
        double finalDistanceBetween;   ///< Distance between final two height values
        QList<double> heights;                ///< Terrain heights along path
    };

signals:
    /// Signalled when terrain data comes back from server
    void terrainDataReceived(bool success, const TerrainPathQuery::PathHeightInfo_t &pathHeightInfo);

private slots:
    void _pathHeights(bool success, double distanceBetween, double finalDistanceBetween, const QList<double> &heights);

private:
    bool _autoDelete = false;
    TerrainQueryInterface *_terrainQuery = nullptr;
};
Q_DECLARE_METATYPE(TerrainPathQuery::PathHeightInfo_t)

/*===========================================================================*/

class TerrainPolyPathQuery : public QObject
{
    Q_OBJECT

public:
    ///     @param autoDelete true: object will delete itself after it signals results
    explicit TerrainPolyPathQuery(bool autoDelete, QObject *parent = nullptr);
    ~TerrainPolyPathQuery();

    /// Async terrain query for terrain heights for the paths between each specified QGeoCoordinate.
    /// When the query is done, the terrainData() signal is emitted.
    ///     @param polyPath List of QGeoCoordinate
    void requestData(const QVariantList &polyPath);
    void requestData(const QList<QGeoCoordinate> &polyPath);

signals:
    /// Signalled when terrain data comes back from server
    void terrainDataReceived(bool success, const QList<TerrainPathQuery::PathHeightInfo_t> &rgPathHeightInfo);

private slots:
    void _terrainDataReceived(bool success, const TerrainPathQuery::PathHeightInfo_t &pathHeightInfo);

private:
    bool _autoDelete = false;
    int _curIndex = 0;
    QList<QGeoCoordinate> _rgCoords;
    QList<TerrainPathQuery::PathHeightInfo_t> _rgPathHeightInfo;
    TerrainPathQuery *_pathQuery = nullptr;
};
