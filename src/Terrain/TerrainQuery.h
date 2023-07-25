/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include <QGeoRectangle>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QtLocation/private/qgeotiledmapreply_p.h>

Q_DECLARE_LOGGING_CATEGORY(TerrainQueryLog)
Q_DECLARE_LOGGING_CATEGORY(TerrainQueryVerboseLog)

class TerrainAtCoordinateQuery;

/// Base class for offline/online terrain queries
class TerrainQueryInterface : public QObject
{
    Q_OBJECT

public:
    TerrainQueryInterface(QObject* parent) : QObject(parent) { }

    /// Request terrain heights for specified coodinates.
    /// Signals: coordinateHeights when data is available
    virtual void requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates) = 0;

    /// Requests terrain heights along the path specified by the two coordinates.
    /// Signals: pathHeights
    ///     @param coordinates to query
    virtual void requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord) = 0;

    /// Request terrain heights for the rectangular area specified.
    /// Signals: carpetHeights when data is available
    ///     @param swCoord South-West bound of rectangular area to query
    ///     @param neCoord North-East bound of rectangular area to query
    ///     @param statsOnly true: Return only stats, no carpet data
    virtual void requestCarpetHeights(const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly) = 0;

signals:
    void coordinateHeightsReceived(bool success, QList<double> heights);
    void pathHeightsReceived(bool success, double distanceBetween, double finalDistanceBetween, const QList<double>& heights);
    void carpetHeightsReceived(bool success, double minHeight, double maxHeight, const QList<QList<double>>& carpet);
};

/// AirMap online implementation of terrain queries
class TerrainAirMapQuery : public TerrainQueryInterface {
    Q_OBJECT

public:
    TerrainAirMapQuery(QObject* parent = nullptr);

    // Overrides from TerrainQueryInterface
    void requestCoordinateHeights   (const QList<QGeoCoordinate>& coordinates) final;
    void requestPathHeights         (const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord) final;
    void requestCarpetHeights       (const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly) final;

private slots:
    void _requestError              (QNetworkReply::NetworkError code);
    void _requestFinished           (void);
    void _sslErrors                 (const QList<QSslError> &errors);

private:
    void _sendQuery                 (const QString& path, const QUrlQuery& urlQuery);
    void _requestFailed             (void);
    void _parseCoordinateData       (const QJsonValue& coordinateJson);
    void _parsePathData             (const QJsonValue& pathJson);
    void _parseCarpetData           (const QJsonValue& carpetJson);

    enum QueryMode {
        QueryModeCoordinates,
        QueryModePath,
        QueryModeCarpet
    };

    QNetworkAccessManager   _networkManager;
    QueryMode               _queryMode;
    bool                    _carpetStatsOnly;
};

/// AirMap offline cachable implementation of terrain queries
class TerrainOfflineAirMapQuery : public TerrainQueryInterface {
    Q_OBJECT

public:
    TerrainOfflineAirMapQuery(QObject* parent = nullptr);

    // Overrides from TerrainQueryInterface
    void requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates) final;
    void requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord) final;
    void requestCarpetHeights(const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly) final;

    // Internal methods
    void _signalCoordinateHeights(bool success, QList<double> heights);
    void _signalPathHeights(bool success, double distanceBetween, double finalDistanceBetween, const QList<double>& heights);
    void _signalCarpetHeights(bool success, double minHeight, double maxHeight, const QList<QList<double>>& carpet);
};

/// Used internally by TerrainOfflineAirMapQuery to manage terrain tiles
class TerrainTileManager : public QObject {
    Q_OBJECT

public:
    TerrainTileManager(void);

    void addCoordinateQuery         (TerrainOfflineAirMapQuery* terrainQueryInterface, const QList<QGeoCoordinate>& coordinates);
    void addPathQuery               (TerrainOfflineAirMapQuery* terrainQueryInterface, const QGeoCoordinate& startPoint, const QGeoCoordinate& endPoint);
    bool getAltitudesForCoordinates (const QList<QGeoCoordinate>& coordinates, QList<double>& altitudes, bool& error);

    static QList<QGeoCoordinate> pathQueryToCoords(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord, double& distanceBetween, double& finalDistanceBetween);

private slots:
    void _terrainDone(QByteArray responseBytes, QNetworkReply::NetworkError error);

private:
    enum class State {
        Idle,
        Downloading,
    };

    enum QueryMode {
        QueryModeCoordinates,
        QueryModePath,
        QueryModeCarpet
    };

    typedef struct {
        TerrainOfflineAirMapQuery*  terrainQueryInterface;
        QueryMode                   queryMode;
        double                      distanceBetween;        // Distance between each returned height
        double                      finalDistanceBetween;   // Distance between for final height
        QList<QGeoCoordinate>       coordinates;
    } QueuedRequestInfo_t;

    void    _tileFailed                         (void);
    QString _getTileHash                        (const QGeoCoordinate& coordinate);

    QList<QueuedRequestInfo_t>  _requestQueue;
    State                       _state = State::Idle;
    QNetworkAccessManager       _networkManager;

    QMutex                      _tilesMutex;
    QHash<QString, TerrainTile> _tiles;
};

/// Used internally by TerrainAtCoordinateQuery to batch coordinate requests together
class TerrainAtCoordinateBatchManager : public QObject {
    Q_OBJECT

public:
    TerrainAtCoordinateBatchManager(void);

    void addQuery(TerrainAtCoordinateQuery* terrainAtCoordinateQuery, const QList<QGeoCoordinate>& coordinates);

private slots:
    void _sendNextBatch         (void);
    void _queryObjectDestroyed  (QObject* elevationProvider);
    void _coordinateHeights     (bool success, QList<double> heights);

private:
    typedef struct {
        TerrainAtCoordinateQuery*   terrainAtCoordinateQuery;
        QList<QGeoCoordinate>       coordinates;
    } QueuedRequestInfo_t;

    typedef struct {
        TerrainAtCoordinateQuery*   terrainAtCoordinateQuery;
        bool                        queryObjectDestroyed;
        int                         cCoord;
    } SentRequestInfo_t;


    enum class State {
        Idle,
        Downloading,
    };

    void _batchFailed(void);
    QString _stateToString(State state);

    QList<QueuedRequestInfo_t>  _requestQueue;
    QList<SentRequestInfo_t>    _sentRequests;
    State                       _state = State::Idle;
    const int                   _batchTimeout = 500;
    QTimer                      _batchTimer;
    TerrainOfflineAirMapQuery   _terrainQuery;
};

// IMPORTANT NOTE: The terrain query objects below must continue to live until the the terrain system signals data back through them.
// Because of that it makes object lifetime tricky. Normally you would use autoDelete = true such they delete themselves when they
// complete. The case for using autoDelete=false is where the query has not been "newed" as a standalone object.
//
// Another typical use case is to query some terrain data and while you are waiting for it to come back the underlying reason
// for that query changes and you end up needed to query again for a new set of data. In this case you are no longer intersted
// in the results of the previous query. The way to do that is to disconnect the data received signal on the old stale query
// when you create the new query.

/// NOTE: TerrainAtCoordinateQuery is not thread safe. All instances/calls to ElevationProvider must be on main thread.
class TerrainAtCoordinateQuery : public QObject
{
    Q_OBJECT
public:
    /// @param autoDelete true: object will delete itself after it signals results
    TerrainAtCoordinateQuery(bool autoDelete);

    /// Async terrain query for a list of lon,lat coordinates. When the query is done, the terrainData() signal
    /// is emitted.
    ///     @param coordinates to query
    void requestData(const QList<QGeoCoordinate>& coordinates);

    /// Either returns altitudes from cache or queues database request
    ///     @param[out] error true: altitude not returned due to error, false: altitudes returned
    /// @return true: altitude returned (check error as well), false: database query queued (altitudes not returned)
    static bool getAltitudesForCoordinates(const QList<QGeoCoordinate>& coordinates, QList<double>& altitudes, bool& error);

    // Internal method
    void _signalTerrainData(bool success, QList<double>& heights);

signals:
    void terrainDataReceived(bool success, QList<double> heights);

private:
    bool _autoDelete;
};

class TerrainPathQuery : public QObject
{
    Q_OBJECT

public:
    /// @param autoDelete true: object will delete itself after it signals results
    TerrainPathQuery(bool autoDelete);

    /// Async terrain query for terrain heights between two lat/lon coordinates. When the query is done, the terrainData() signal
    /// is emitted.
    ///     @param coordinates to query
    void requestData(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord);

    typedef struct {
        double          distanceBetween;        ///< Distance between each height value
        double          finalDistanceBetween;   ///< Distance between final two height values
        QList<double>   heights;                ///< Terrain heights along path
    } PathHeightInfo_t;

signals:
    /// Signalled when terrain data comes back from server
    void terrainDataReceived(bool success, const PathHeightInfo_t& pathHeightInfo);

private slots:
    void _pathHeights(bool success, double distanceBetween, double finalDistanceBetween, const QList<double>& heights);

private:
    bool                        _autoDelete;
    TerrainOfflineAirMapQuery   _terrainQuery;
};

Q_DECLARE_METATYPE(TerrainPathQuery::PathHeightInfo_t)

class TerrainPolyPathQuery : public QObject
{
    Q_OBJECT

public:
    /// @param autoDelete true: object will delete itself after it signals results
    TerrainPolyPathQuery(bool autoDelete);

    /// Async terrain query for terrain heights for the paths between each specified QGeoCoordinate.
    /// When the query is done, the terrainData() signal is emitted.
    ///     @param polyPath List of QGeoCoordinate
    void requestData(const QVariantList& polyPath);
    void requestData(const QList<QGeoCoordinate>& polyPath);

signals:
    /// Signalled when terrain data comes back from server
    void terrainDataReceived(bool success, const QList<TerrainPathQuery::PathHeightInfo_t>& rgPathHeightInfo);

private slots:
    void _terrainDataReceived(bool success, const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo);

private:
    bool                                        _autoDelete;
    int                                         _curIndex = 0;
    QList<QGeoCoordinate>                       _rgCoords;
    QList<TerrainPathQuery::PathHeightInfo_t>   _rgPathHeightInfo;
    TerrainPathQuery                            _pathQuery;
};

/// @brief Provides unit test terrain query responses.
/// @details It provides preset, emulated, 1 arc-second (SRTM1) resolution regions that are either
/// flat or sloped in a fashion that aids testing terrain-sensitive functionality. All emulated
/// regions are positioned around Point Nemo - should real terrain became useful and checked in one day.
class UnitTestTerrainQuery : public TerrainQueryInterface {
public:

    static constexpr double regionSizeDeg     = 0.1;      // all regions are 0.1deg (~11km) square
    static constexpr double one_second_deg    = 1.0/3600;
    static constexpr double earths_radius_mts = 6371000.;

    /// Point Nemo is a point on Earth furthest from land
    static const QGeoCoordinate pointNemo;

    /// Region with constant 10m terrain elevation
    struct Flat10Region : public QGeoRectangle
    {
        Flat10Region(const QGeoRectangle& region)
            : QGeoRectangle(region)
        {

        }

        static const double amslElevation;
    };
    static const Flat10Region flat10Region;

    /// Region with a linear west to east slope raising at a rate of 100 meters per kilometer (-100m to 1000m)
    struct LinearSlopeRegion : public QGeoRectangle
    {
        LinearSlopeRegion(const QGeoRectangle& region)
            : QGeoRectangle(region)
        {

        }

        static const double minAMSLElevation;
        static const double maxAMSLElevation;
        static const double totalElevationChange;
    };
    static const LinearSlopeRegion linearSlopeRegion;

    /// Region with a hill (top half of a sphere) in the center.
    struct HillRegion : public QGeoRectangle
    {
        HillRegion(const QGeoRectangle& region)
            : QGeoRectangle(region)
        {
        }

        static const double radius;
    };
    static const HillRegion hillRegion;

    UnitTestTerrainQuery(TerrainQueryInterface* parent = nullptr);

    // Overrides from TerrainQueryInterface
    void requestCoordinateHeights   (const QList<QGeoCoordinate>& coordinates) override;
    void requestPathHeights         (const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord) override;
    void requestCarpetHeights       (const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly) override;

private:
    typedef struct {
        QList<QGeoCoordinate>   rgCoords;
        QList<double>           rgHeights;
        double                  distanceBetween;
        double                  finalDistanceBetween;
    } PathHeightInfo_t;

    QList<double> _requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates);
    PathHeightInfo_t _requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord);
};

