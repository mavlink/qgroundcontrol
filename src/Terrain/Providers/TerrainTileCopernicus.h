/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>

#include "TerrainTile.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainTileCopernicusLog)

/// Implements an interface for https://terrain-ce.suite.auterion.com/api/v1/
class TerrainTileCopernicus : public TerrainTile
{
    friend class TerrainTileTest;

public:
    /// Constructor from serialized elevation data (either from file or web)
    ///    @param byteArray
    explicit TerrainTileCopernicus(const QByteArray &byteArray);
    ~TerrainTileCopernicus();

    static QByteArray serializeFromData(const QByteArray &input);
    static QJsonValue getJsonFromData(const QByteArray &input);

    static constexpr double kTileSizeDegrees = 0.01;                ///< Each terrain tile represents a square area .01 degrees in lat/lon
    static constexpr double kTleValueSpacingDegrees = (1.0 / 3600); ///< 1 Arc-Second spacing of elevation values
    static constexpr double kTileValueSpacingMeters = 30.0;

private:
    static constexpr const char *_jsonStatusKey = "status";
    static constexpr const char *_jsonDataKey = "data";
    static constexpr const char *_jsonBoundsKey = "bounds";
    static constexpr const char *_jsonSouthWestKey = "sw";
    static constexpr const char *_jsonNorthEastKey = "ne";
    static constexpr const char *_jsonStatsKey = "stats";
    static constexpr const char *_jsonMaxElevationKey = "max";
    static constexpr const char *_jsonMinElevationKey = "min";
    static constexpr const char *_jsonAvgElevationKey = "avg";
    static constexpr const char *_jsonCarpetKey = "carpet";
};
