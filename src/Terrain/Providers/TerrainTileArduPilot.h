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
    ~TerrainTileArduPilot();

    /// Parses raw HGT elevation data into a vector of QGeoCoordinate with elevation as altitude from a .hgt or .hgt.zip file
    ///    @param name Filename of the HGT file
    ///    @param coordinateData Raw elevation data
    ///    @return Vector of QGeoCoordinate with elevation as altitude
    static QVector<QGeoCoordinate> parseCoordinateData(const QString &name, const QByteArray &coordinateData);

    /// Serializes raw HGT elevation data into a QByteArray compatible with TerrainTile
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

    /// SRTM-3 (3 arc-second resolution, 90 meters): 1201 × 1201 grid of elevations (1,442,401 data points).
    static constexpr double kSRTM3ValueSpacingDegrees = (1.0 / 1200);    ///< 3 Arc-Second spacing of elevation values
    static constexpr double kSRTM3ValueSpacingMeters = 90.0;             ///< SRTM3 Resolution
    static constexpr int kSRTM3Dimension = 1201;                         ///< SRTM3 Total Points per side
    static constexpr int kSRTM3TotalPoints = kSRTM3Dimension * kSRTM3Dimension; ///< SRTM3 Total Points

    /// Auto-detect SRTM variant from raw HGT data size
    ///    @param dataSize Size of raw HGT data in bytes
    ///    @return Grid dimension (3601 for SRTM1, 1201 for SRTM3, or 0 if unrecognized)
    static int detectDimension(qsizetype dataSize);

private:
    /// Parses the filename to extract the southwest coordinate
    ///    @param name Filename of the HGT file
    ///    @return QGeoCoordinate representing the southwest corner
    static QGeoCoordinate _parseFileName(const QString &name);
};
