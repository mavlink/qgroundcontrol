#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

#include "TerrainQueryInterface.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainQueryOpenElevationLog)

class QGeoCoordinate;

class TerrainQueryOpenElevation : public TerrainOnlineQuery
{
    Q_OBJECT

public:
    explicit TerrainQueryOpenElevation(QObject *parent = nullptr);
    ~TerrainQueryOpenElevation();

    void requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates) final;
    void requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord) final;
    void requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly) final;

private slots:
    void _requestFinished() final;

private:
    void _sendQuery(const QList<QGeoCoordinate> &coordinates);
    void _parseCoordinateData(const QList<double> &elevations);
    void _parsePathData(const QList<double> &elevations);
    void _parseCarpetData(const QList<double> &elevations);

    bool _carpetStatsOnly = false;
    double _pathDistanceBetween = 0;
    double _pathFinalDistanceBetween = 0;
    int _carpetGridSizeLat = 0;
    int _carpetGridSizeLon = 0;
    int _expectedResultCount = 0;
};
