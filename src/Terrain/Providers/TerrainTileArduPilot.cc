/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainTileArduPilot.h"
#include "JsonHelper.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(TerrainTileArduPilotLog, "qgc.terrain.terraintileardupilot");

TerrainTileArduPilot::TerrainTileArduPilot()
{
    // qCDebug(TerrainTileArduPilotLog) << Q_FUNC_INFO << this;
}

TerrainTileArduPilot::TerrainTileArduPilot(const QByteArray &byteArray)
    : TerrainTile(byteArray)
{
    // qCDebug(TerrainTileArduPilotLog) << Q_FUNC_INFO << this;
}

TerrainTileArduPilot::~TerrainTileArduPilot()
{
    // qCDebug(TerrainTileArduPilotLog) << Q_FUNC_INFO << this;
}

QList<double> TerrainTileArduPilot::parseCoordinateData(const QString &name, const QByteArray &hgtData)
{
    QString baseName = name.section('.', 0, 0);
    bool ok = false;
    int lat = baseName.mid(1, 2).toInt(&ok);
    int lon = baseName.mid(4, 3).toInt(&ok);
    if (!ok) {
        qCDebug(TerrainQueryArduPilotLog) << "Unable to convert HGT File Name";
        return QList<double>();
    }

    if (baseName.startsWith('S')) {
        lat = -lat;
    }
    if (baseName.mid(3, 1) == 'W') {
        lon = -lon;
    }

    constexpr int size = TerrainTileArduPilot::kTotalPoints * 2;
    if (hgtData.size() != size) {
        qCDebug(TerrainQueryArduPilotLog) << "Invalid HGT file size!";
        return QList<double>();
    }

    constexpr int resolution = TerrainTileArduPilot::kTileDimension;

    QDataStream stream(hgtData);
    stream.setByteOrder(QDataStream::BigEndian);

    QList<QGeoCoordinate> coordinates;
    QList<double> heights;
    for (int row = 0; row < resolution; ++row) {
        for (int col = 0; col < resolution; ++col) {
            qint16 elevation;
            stream >> elevation;

            const double currentLat = lat + (1.0 * row / (resolution - 1));
            const double currentLon = lon + (1.0 * col / (resolution - 1));

            const QGeoCoordinate coordinate(currentLat, currentLon, elevation);
            (void) coordinates.append(coordinate);
            (void) heights.append(coordinate.altitude());
        }
    }

    return heights;
}

QByteArray TerrainTileArduPilot::serializeFromData(const QByteArray &input)
{
    // TODO: Need Name
    const QList<double> heights = TerrainTileArduPilot::parseCoordinateData("", input);

    // TODO: Finish this
    TerrainTile::TileInfo_t tileInfo = {0};

    constexpr int cTileNumHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    const int cTileNumDataBytes = static_cast<int>(sizeof(int16_t)) * tileInfo.gridSizeLat * tileInfo.gridSizeLon;

    QByteArray result(cTileNumHeaderBytes + cTileNumDataBytes, Qt::Uninitialized);
    TileInfo_t *pTileInfo = reinterpret_cast<TileInfo_t*>(result.data());
    int16_t* const pTileData = reinterpret_cast<int16_t*>(&reinterpret_cast<uint8_t*>(result.data())[cTileNumHeaderBytes]);

    *pTileInfo = tileInfo;

    return result;
}
