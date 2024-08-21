#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>

#include "TerrainTile.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainTileCopernicusLog)

/// Implements an interface for https://developers.airmap.com/v2.0/docs/elevation-api
class TerrainTileCopernicus : public TerrainTile
{
public:
    TerrainTileCopernicus();

    /// Constructor from serialized elevation data (either from file or web)
    ///    @param document
    explicit TerrainTileCopernicus(const QByteArray &byteArray);
    ~TerrainTileCopernicus();

    static QByteArray serializeFromJson(const QByteArray &input);

    static constexpr double tileSizeDegrees = 0.01;                 ///< Each terrain tile represents a square area .01 degrees in lat/lon
    static constexpr double tileValueSpacingDegrees = (1.0 / 3600); ///< 1 Arc-Second spacing of elevation values
    static constexpr double tileValueSpacingMeters = 30.0;

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
