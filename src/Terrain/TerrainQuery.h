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
#include <QNetworkReply>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(TerrainQueryLog)

class TerrainAtCoordinateQuery;

// Base class for all terrain query objects
class TerrainQuery : public QObject {
    Q_OBJECT

public:
    TerrainQuery(QObject* parent = NULL);

protected:
    // Send a query to AirMap terrain servers. Data and errors are returned back from super class virtual implementations.
    //  @param path Additional path to add to url
    //  @param urlQuery Query to send
    void _sendQuery(const QString& path, const QUrlQuery& urlQuery);

    virtual void _getNetworkReplyFailed     (void) = 0;                                 ///< QNetworkManager::get failed to return QNetworkReplay object
    virtual void _requestFailed             (QNetworkReply::NetworkError error) = 0;    ///< QNetworkReply::finished returned error
    virtual void _requestJsonParseFailed    (const QString& errorString) = 0;           ///< Parsing of returned json failed
    virtual void _requestAirmapStatusFailed (const QString& status) = 0;                ///< AirMap status was not "success"
    virtual void _requestSucess             (const QJsonValue& dataJsonValue) = 0;      ///< Successful reqest, data returned

private slots:
    void _requestFinished(void);

private:
    QNetworkAccessManager _networkManager;
};

/// Used internally by TerrainAtCoordinateQuery to batch coordinate requests together
class TerrainAtCoordinateBatchManager : public TerrainQuery {
    Q_OBJECT

public:
    TerrainAtCoordinateBatchManager(void);

    void addQuery(TerrainAtCoordinateQuery* terrainAtCoordinateQuery, const QList<QGeoCoordinate>& coordinates);

protected:
    void _getNetworkReplyFailed     (void) final;
    void _requestFailed             (QNetworkReply::NetworkError error) final;
    void _requestJsonParseFailed    (const QString& errorString) final;
    void _requestAirmapStatusFailed (const QString& status) final;
    void _requestSucess             (const QJsonValue& dataJsonValue) final;

private slots:
    void _sendNextBatch         (void);
    void _queryObjectDestroyed  (QObject* elevationProvider);

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
};

/// NOTE: TerrainAtCoordinateQuery is not thread safe. All instances/calls to ElevationProvider must be on main thread.
class TerrainAtCoordinateQuery : public QObject
{
    Q_OBJECT
public:
    TerrainAtCoordinateQuery(QObject* parent = NULL);

     /// Async terrain query for a list of lon,lat coordinates. When the query is done, the terrainData() signal
     /// is emitted.
     ///    @param coordinates to query
    void requestData(const QList<QGeoCoordinate>& coordinates);

    // Internal method
    void _signalTerrainData(bool success, QList<float>& altitudes);

signals:
    void terrainData(bool success, QList<float> altitudes);
};

class TerrainPathQuery : public TerrainQuery
{
    Q_OBJECT

public:
    TerrainPathQuery(QObject* parent = NULL);

     /// Async terrain query for terrain heights between two lat/lon coordinates. When the query is done, the terrainData() signal
     /// is emitted.
     ///    @param coordinates to query
    void requestData(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord);

protected:
    void _getNetworkReplyFailed     (void) final;
    void _requestFailed             (QNetworkReply::NetworkError error) final;
    void _requestJsonParseFailed    (const QString& errorString) final;
    void _requestAirmapStatusFailed (const QString& status) final;
    void _requestSucess             (const QJsonValue& dataJsonValue) final;

signals:
    /// Signalled when terrain data comes back from server
    ///     @param latStep Amount of latitudinal distance between each returned height
    ///     @param lonStep Amount of longitudinal distance between each returned height
    ///     @param altitudes Altitudes along specified path
    void terrainData(bool success, double latStep, double lonStep, QList<double> altitudes);
};
