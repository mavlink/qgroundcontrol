#pragma once

#include <QtCore/QVector>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

#include "TerrainTile.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainTileArduPilotLog)

class TerrainTileArduPilot : public TerrainTile
{
    friend class TerrainTileTest;

public:
    /// Constructor from serialized elevation data (either from file or web)
    ///    @param byteArray Binary data containing TileInfo_t followed by elevation data
    explicit TerrainTileArduPilot(const QByteArray &byteArray);
    ~TerrainTileArduPilot() override = default;

    /// Parses elevation data from a .hgt or .hgt.zip file
    ///    @param name Filename of the HGT file
    ///    @param coordinateData Raw elevation data
    ///    @return Vector of QGeoCoordinate with elevation as altitude
    static QVector<QGeoCoordinate> parseCoordinateData(const QString &name, const QByteArray &coordinateData);

    /// Serializes elevation data into a QByteArray compatible with TerrainTile
    ///    @param name Filename of the HGT file
    ///    @param hgtData Raw elevation data
    ///    @return Serialized QByteArray containing TileInfo_t and elevation data
    static QByteArray serializeFromData(const QString &name, const QByteArray &hgtData);

    /// SRTM-1 (1 arc-second resolution, 30 meters): 3601 × 3601 grid of elevations (12,967,201 data points). 0-60 degrees N/S.
    static constexpr double kTileSizeDegrees = 1.0;                       ///< Each terrain tile represents a square area of 1 degree in lat/lon
    static constexpr double kTileValueSpacingDegrees = (1.0 / 3600);      ///< 1 Arc-Second spacing of elevation values
    static constexpr double kTileValueSpacingMeters = 30.0;               ///< SRTM1 Resolution
    static constexpr int kTileDimension = 3601;                           ///< SRTM1 Total Points per side
    static constexpr int kTotalPoints = kTileDimension * kTileDimension;  ///< SRTM1 Total Points

private:
    /// Parses the filename to extract the southwest coordinate
    ///    @param name Filename of the HGT file
    ///    @return QGeoCoordinate representing the southwest corner
    static QGeoCoordinate _parseFileName(const QString &name);
};
