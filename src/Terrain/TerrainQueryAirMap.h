/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>

#include "TerrainQueryInterface.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainQueryAirMapLog)
Q_DECLARE_LOGGING_CATEGORY(TerrainQueryAirMapVerboseLog)

class QGeoCoordinate;
class QNetworkAccessManager;

class TerrainAirMapQuery : public TerrainQueryInterface
{
    Q_OBJECT

public:
    explicit TerrainAirMapQuery(QObject *parent = nullptr);
    ~TerrainAirMapQuery();

    void requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates) final;
    void requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord) final;
    void requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly) final;

private slots:
    void _requestError(QNetworkReply::NetworkError code);
    void _requestFinished();
    void _sslErrors(const QList<QSslError> &errors);

private:
    void _sendQuery(const QString &path, const QUrlQuery &urlQuery);
    void _requestFailed();
    void _parseCoordinateData(const QJsonValue &coordinateJson);
    void _parsePathData(const QJsonValue &pathJson);
    void _parseCarpetData(const QJsonValue &carpetJson);

    bool _carpetStatsOnly = false;
    QNetworkAccessManager *_networkManager = nullptr;
};

/*===========================================================================*/

class TerrainOfflineAirMapQuery : public TerrainQueryInterface
{
    Q_OBJECT

public:
    explicit TerrainOfflineAirMapQuery(QObject *parent = nullptr);
    ~TerrainOfflineAirMapQuery();

    void requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates) final;
    void requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord) final;
};
