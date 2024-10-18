/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

#include "TerrainQueryInterface.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainQueryCopernicusLog)

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
