#ifndef TERRAINTILE_H
#define TERRAINTILE_H

#include "QGCLoggingCategory.h"

#include <QGeoCoordinate>

#define TERRAIN_TILE_SIZE 90

Q_DECLARE_LOGGING_CATEGORY(TerrainTileLog)

class TerrainTile
{
public:
    TerrainTile();
    ~TerrainTile();

    /**
    * Constructor from json doc with elevation data (either from file or web)
    *
    * @param json doc
    */
    TerrainTile(QJsonDocument doc);

    /**
    * Check for whether a coordinate lies within this tile
    *
    * @param coordinate
    * @return true if within
    */
    bool isIn(QGeoCoordinate coord);

    /**
    * Check whether valid data is loaded
    *
    * @return true if data is valid
    */
    bool isValid(void) { return _isValid; }

    /**
    * Evaluates the elevation at the given coordinate
    *
    * @param coordinate
    * @return elevation
    */
    float elevation(const QGeoCoordinate& coord);

    /**
    * Accessor for the minimum elevation of the tile
    *
    * @return minimum elevation
    */
    float minElevation(void) { return _minElevation; }

    /**
    * Accessor for the maximum elevation of the tile
    *
    * @return maximum elevation
    */
    float maxElevation(void) { return _maxElevation; }

    /**
    * Accessor for the average elevation of the tile
    *
    * @return average elevation
    */
    float avgElevation(void) { return _avgElevation; }

private:
    QGeoCoordinate      _southWest;                                     /// South west corner of the tile
    QGeoCoordinate      _northEast;                                     /// North east corner of the tile

    float               _minElevation;                                  /// Minimum elevation in tile
    float               _maxElevation;                                  /// Maximum elevation in tile
    float               _avgElevation;                                  /// Average elevation of the tile

    float               _data[TERRAIN_TILE_SIZE][TERRAIN_TILE_SIZE];    /// elevation data
    bool                _isValid;                                       /// data loaded is valid
    static const int    _gridSize = TERRAIN_TILE_SIZE;                  /// tile grid size in lat and lon
    static const float  _srtm1Increment = 1.0 / (60.0 * 60.0);          /// grid spacing in degree

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
