#pragma once

#include "TerrainQueryInterface.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QQueue>

#include <memory>
#include <QtPositioning/QGeoCoordinate>

class ElevationProvider;
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
    ~TerrainTileManager() override;

    static TerrainTileManager *instance();

    /// Either returns altitudes from cache or queues database request
    ///     @param[out] error true: altitude not returned due to error, false: altitudes returned
    ///     @return true: altitude returned (check error as well), false: database query queued (altitudes not returned)
    bool getAltitudesForCoordinates(const QList<QGeoCoordinate> &coordinates, QList<double> &altitudes, bool &error);

    void addCoordinateQuery(TerrainQueryInterface *terrainQueryInterface, const QList<QGeoCoordinate> &coordinates);
    void addPathQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &startPoint, const QGeoCoordinate &endPoint);
    void addCarpetQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly);

private slots:
    void _terrainDone();

private:
    /// Returns the currently selected elevation provider from settings
    static std::shared_ptr<const ElevationProvider> _getElevationProvider();
    /// Returns a list of individual coordinates along the requested path spaced according to the terrain tile value spacing
    static QList<QGeoCoordinate> _pathQueryToCoords(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord, double spacingMeters, double &distanceBetween, double &finalDistanceBetween);
    void _tileFailed();
    void _cacheTile(const QByteArray &data, const QString &hash);
    TerrainTile *_getCachedTile(const QString &hash);
    static void _processCarpetResults(const QList<double> &altitudes, int gridSizeLat, int gridSizeLon,
                                      bool statsOnly, double &minHeight, double &maxHeight, QList<QList<double>> &carpet);

    struct QueuedRequestInfo_t {
        QPointer<TerrainQueryInterface> terrainQueryInterface;
        TerrainQuery::QueryMode queryMode;
        double distanceBetween;                         ///< Distance between each returned height
        double finalDistanceBetween;                    ///< Distance between for final height
        QList<QGeoCoordinate> coordinates;
        bool carpetStatsOnly;                           ///< For carpet queries: return only stats
        int carpetGridSizeLat;                          ///< For carpet queries: number of rows
        int carpetGridSizeLon;                          ///< For carpet queries: number of columns
    };

    QQueue<QueuedRequestInfo_t> _requestQueue;
    TerrainQuery::State _state = TerrainQuery::State::Idle;

    QMutex _tilesMutex;
    QHash<QString, TerrainTile*> _tiles;

    QNetworkAccessManager *_networkManager = nullptr;
};
