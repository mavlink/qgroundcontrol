#pragma once

#include <QtCore/QObject>

#include "TerrainQueryInterface.h"


class QGeoCoordinate;

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
    void _parseCoordinateData(const QJsonValue &coordinateJson);
    void _parsePathData(const QJsonValue &pathJson);
    void _parseCarpetData(const QJsonValue &carpetJson);

    bool _carpetStatsOnly = false;
};
