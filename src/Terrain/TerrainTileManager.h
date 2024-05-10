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

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QMutex>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(TerrainTileManagerLog)

class TerrainOfflineAirMapQuery;

/// Used internally by TerrainOfflineAirMapQuery to manage terrain tiles
class TerrainTileManager : public QObject {
    Q_OBJECT

public:
    TerrainTileManager(QObject* parent = nullptr);

    void addCoordinateQuery         (TerrainOfflineAirMapQuery* terrainQueryInterface, const QList<QGeoCoordinate>& coordinates);
    void addPathQuery               (TerrainOfflineAirMapQuery* terrainQueryInterface, const QGeoCoordinate& startPoint, const QGeoCoordinate& endPoint);
    bool getAltitudesForCoordinates (const QList<QGeoCoordinate>& coordinates, QList<double>& altitudes, bool& error);

    static TerrainTileManager* instance();
    static QList<QGeoCoordinate> pathQueryToCoords(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord, double& distanceBetween, double& finalDistanceBetween);

private slots:
    void _terrainDone();

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
