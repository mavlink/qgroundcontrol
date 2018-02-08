/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"

#include <QObject>
#include <QGeoCoordinate>
#include <QNetworkAccessManager>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(ElevationProviderLog)

class ElevationProvider;

/// Used internally by ElevationProvider to batch requests together
class TerrainBatchManager : public QObject {
    Q_OBJECT

public:
    TerrainBatchManager(void);

    void addQuery(ElevationProvider* elevationProvider, const QList<QGeoCoordinate>& coordinates);

private slots:
    void _sendNextBatch                 (void);
    void _requestFinished               (void);
    void _elevationProviderDestroyed    (QObject* elevationProvider);

private:
    typedef struct {
        ElevationProvider*      elevationProvider;
        QList<QGeoCoordinate>   coordinates;
    } QueuedRequestInfo_t;

    typedef struct {
        ElevationProvider*      elevationProvider;
        bool                    providerDestroyed;
        int                     cCoord;
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
    QNetworkAccessManager       _networkManager;
    const int                   _batchTimeout = 500;
    QTimer                      _batchTimer;
};

/// NOTE: ElevationProvider is not thread safe. All instances/calls to ElevationProvider must be on main thread.
class ElevationProvider : public QObject
{
    Q_OBJECT
public:
    ElevationProvider(QObject* parent = NULL);

     /// Async elevation query for a list of lon,lat coordinates. When the query is done, the terrainData() signal
     /// is emitted.
     ///    @param coordinates to query
    void queryTerrainData(const QList<QGeoCoordinate>& coordinates);

    // Internal method
    void _signalTerrainData(bool success, QList<float>& altitudes);

signals:
    void terrainData(bool success, QList<float> altitudes);
};
