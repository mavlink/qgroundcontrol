/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQueryInterface.h"
#include "QGCLoggingCategory.h"

#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainQueryInterfaceLog, "qgc.terrain.terrainqueryinterface")

void TerrainQueryInterface::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    Q_UNUSED(coordinates);
    qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "Not Supported";
}

void TerrainQueryInterface::requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    Q_UNUSED(fromCoord);
    Q_UNUSED(toCoord);
    qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "Not Supported";
}

void TerrainQueryInterface::requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly)
{
    Q_UNUSED(swCoord);
    Q_UNUSED(neCoord);
    Q_UNUSED(statsOnly);
    qCWarning(TerrainQueryInterfaceLog) << Q_FUNC_INFO << "Not Supported";
}

void TerrainQueryInterface::signalCoordinateHeights(bool success, const QList<double> &heights)
{
    emit coordinateHeightsReceived(success, heights);
}

void TerrainQueryInterface::signalPathHeights(bool success, double distanceBetween, double finalDistanceBetween, const QList<double> &heights)
{
    emit pathHeightsReceived(success, distanceBetween, finalDistanceBetween, heights);
}

void TerrainQueryInterface::signalCarpetHeights(bool success, double minHeight, double maxHeight, const QList<QList<double>> &carpet)
{
    emit carpetHeightsReceived(success, minHeight, maxHeight, carpet);
}
