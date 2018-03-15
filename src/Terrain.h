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

Q_DECLARE_LOGGING_CATEGORY(ElevationProviderLog)

class ElevationProvider;

/// Used internally by ElevationProvider to batch requests together
class TerrainBatchManager : public QObject {
    Q_OBJECT

public:
    TerrainBatchManager(void);

    void addQuery(ElevationProvider* elevationProvider, const QList<QGeoCoordinate>& coordinates);

private slots:
    void _requestFinished   (void);
    void _fetchedTile       (void);                             /// slot to handle fetched elevation tiles

private:
    typedef struct {
        ElevationProvider*      elevationProvider;
        QList<QGeoCoordinate>   coordinates;
    } QueuedRequestInfo_t;

    enum class State {
        Idle,
        Downloading,
    };

    void _tileFailed(void);
    bool _getAltitudesForCoordinates(const QList<QGeoCoordinate>& coordinates, QList<float>& altitudes);
    QString _getTileHash(const QGeoCoordinate& coordinate);     /// Method to create a unique string for each tile

    QList<QueuedRequestInfo_t>  _requestQueue;
    State                       _state = State::Idle;
    QNetworkAccessManager       _networkManager;

    QMutex                      _tilesMutex;
    QHash<QString, TerrainTile> _tiles;
    QStringList                 _tileDownloadQueue;
};

class ElevationProvider : public QObject
{
    Q_OBJECT
public:
    ElevationProvider(QObject* parent = NULL);

    /**
     * Async elevation query for a list of lon,lat coordinates. When the query is done, the terrainData() signal
     * is emitted. This call caches local elevation tables for faster lookup in the future.
     * @param coordinates
     * @return true on success
     */
    bool queryTerrainData(const QList<QGeoCoordinate>& coordinates);

    /// Internal method
    void _signalTerrainData(bool success, QList<float>& altitudes);

signals:
    /// signal returning requested elevation data
    void terrainData(bool success, QList<float> altitudes);
};
