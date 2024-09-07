#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>

#include "TerrainTile.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainTileArduPilotLog)

class TerrainTileArduPilot : public TerrainTile
{
public:
    TerrainTileArduPilot();

    /// Constructor from serialized elevation data (either from file or web)
    ///    @param byteArray
    explicit TerrainTileArduPilot(const QByteArray &byteArray);
    ~TerrainTileArduPilot();

    static QByteArray serializeFromData(const QByteArray &input);

    /// SRTM-1 (1 arc-second resolution, 30 meters): 3601 × 3601 grid of elevations (129,672,001 data points). 0-84 degrees N/S.
    static constexpr double kTileSizeDegrees = 0.01;                 ///< Each terrain tile represents a square area .01 degrees in lat/lon
    static constexpr double kTileValueSpacingDegrees = (1.0 / 3600); ///< 1 Arc-Second spacing of elevation values
    static constexpr double kTileValueSpacingMeters = 30.0;          ///< SRTM1 Resolution
    static constexpr double kTileDimension = 3601;                   ///< SRTM1 Total Points
    static constexpr double kTotalPoints = kTileDimension * kTileDimension;   ///< SRTM1 Total Points

    /// SRTM-3 (3 arc-second resolution, 90 meters): 1201 × 1201 grid of elevations (1,442,401 data points). 0-84 degrees N/S. >60N & <60S filled from SRTM1
    // static constexpr double tileValueSpacingMeters = 90.0;       ///< SRTM3 Resolution
};
