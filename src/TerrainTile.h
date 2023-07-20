#ifndef TERRAINTILE_H
#define TERRAINTILE_H

#include "QGCLoggingCategory.h"

#include <QGeoCoordinate>

Q_DECLARE_LOGGING_CATEGORY(TerrainTileLog)

/**
 * @brief The TerrainTile class
 *
 * Implements an interface for https://developers.airmap.com/v2.0/docs/elevation-api
 */

class TerrainTile
{
public:
    TerrainTile() = default;
    ~TerrainTile();

    /**
    * Constructor from serialized elevation data (either from file or web)
    *
    * @param document
    */
    TerrainTile(const QByteArray& byteArray);

    /**
    * Check whether valid data is loaded
    *
    * @return true if data is valid
    */
    bool isValid(void) const { return _isValid; }

    /**
    * Evaluates the elevation at the given coordinate
    *
    * @param coordinate
    * @return elevation
    */
    double elevation(const QGeoCoordinate& coordinate) const;

    /**
    * Accessor for the minimum elevation of the tile
    *
    * @return minimum elevation
    */
    double minElevation(void) const { return _isValid ? static_cast<double>(_tileInfo.minElevation) : qQNaN(); }

    /**
    * Accessor for the maximum elevation of the tile
    *
    * @return maximum elevation
    */
    double maxElevation(void) const { return _isValid ? static_cast<double>(_tileInfo.maxElevation) : qQNaN(); }

    /**
    * Accessor for the average elevation of the tile
    *
    * @return average elevation
    */
    double avgElevation(void) const { return _isValid ? _tileInfo.avgElevation : qQNaN(); }

    /**
    * Accessor for the center coordinate
    *
    * @return center coordinate
    */

    static QByteArray serializeFromAirMapJson(const QByteArray& input);

    static constexpr double tileSizeDegrees         = 0.01;         ///< Each terrain tile represents a square area .01 degrees in lat/lon
    static constexpr double tileValueSpacingDegrees = 1.0 / 3600;   ///< 1 Arc-Second spacing of elevation values
    static constexpr double tileValueSpacingMeters  = 30.0;

private:
    typedef struct {
        double  swLat, swLon, neLat, neLon;
        int16_t minElevation;
        int16_t maxElevation;
        double  avgElevation;
        int16_t gridSizeLat;
        int16_t gridSizeLon;
    } TileInfo_t;

    TileInfo_t          _tileInfo;
    int16_t**           _data;                                          /// 2D elevation data array
    double              _cellSizeLat;                                   /// data grid size in latitude direction
    double              _cellSizeLon;                                   /// data grid size in longitude direction
    bool                _isValid;                                       /// data loaded is valid

    // Json keys
    static const char*  _jsonStatusKey;
    static const char*  _jsonDataKey;
    static const char*  _jsonBoundsKey;
    static const char*  _jsonSouthWestKey;
    static const char*  _jsonNorthEastKey;
    static const char*  _jsonStatsKey;
    static const char*  _jsonMaxElevationKey;
    static const char*  _jsonMinElevationKey;
    static const char*  _jsonAvgElevationKey;
    static const char*  _jsonCarpetKey;
};

#endif // TERRAINTILE_H
