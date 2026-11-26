#pragma once

#include <QtCore/QJsonValue>
#include <QtCore/QList>

#include "TerrainTile.h"

/// \brief Implements an interface for https://terrain-ce.suite.auterion.com/api/v1/
///
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
    static constexpr double kTileValueSpacingDegrees = (1.0 / 3600); ///< 1 Arc-Second spacing of elevation values
    static constexpr double kTileValueSpacingMeters = 30.0;

    /// Largest response that is read, after it is decompressed. A tile carpet holds 37 x 37 elevations (0.01 degree
    /// at one arc-second). As JSON numbers with all the digits of a double, indented one to a line, they take about
    /// 60 KiB. The limit leaves a wide margin above that, and keeps a compressed response from expanding to the
    /// default 256 MiB on the GUI thread, which parses the tiles
    static constexpr qint64 kMaxResponseBytes = 1024 * 1024;

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
