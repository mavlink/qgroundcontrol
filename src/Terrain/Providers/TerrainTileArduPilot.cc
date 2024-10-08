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

QByteArray TerrainTileArduPilot::serializeFromData(const QByteArray &input)
{
    TerrainTile::TileInfo_t tileInfo;

    constexpr int cTileNumHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    const int cTileNumDataBytes = static_cast<int>(sizeof(int16_t)) * tileInfo.gridSizeLat * tileInfo.gridSizeLon;

    QByteArray result(cTileNumHeaderBytes + cTileNumDataBytes, Qt::Uninitialized);
    TileInfo_t *pTileInfo = reinterpret_cast<TileInfo_t*>(result.data());
    int16_t* const pTileData = reinterpret_cast<int16_t*>(&reinterpret_cast<uint8_t*>(result.data())[cTileNumHeaderBytes]);

    *pTileInfo = tileInfo;

    return result;
}
