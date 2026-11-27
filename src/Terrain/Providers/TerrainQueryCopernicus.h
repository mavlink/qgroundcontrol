#pragma once

#include <QtCore/QObject>

#include "TerrainQueryInterface.h"

class QGeoCoordinate;

/// Asks https://terrain-ce.suite.auterion.com/api/v1/ itself, without the tile cache. The server answers the query
/// for a carpet only: the two other queries fail without a request.
class TerrainQueryCopernicus : public TerrainOnlineQuery
{
    Q_OBJECT

public:
    explicit TerrainQueryCopernicus(QObject *parent = nullptr);
    ~TerrainQueryCopernicus();

    void requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates) final;
    void requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord) final;
    void requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly) final;

private slots:
    void _requestFinished() final;

private:
    void _sendQuery(const QString &path, const QUrlQuery &urlQuery);
    void _parseCarpetData(const QJsonValue &carpetJson);

    bool _carpetStatsOnly = false;
};
