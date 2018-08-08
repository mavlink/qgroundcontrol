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
    TerrainTile();
    ~TerrainTile();

    /**
    * Constructor from json doc with elevation data (either from file or web)
    *
    * @param document
    */
    TerrainTile(QJsonDocument document);

    /**
    * Constructor from serialized elevation data (either from file or web)
    *
    * @param document
    */
    TerrainTile(QByteArray byteArray);

    /**
    * Check for whether a coordinate lies within this tile
    *
    * @param coordinate
    * @return true if within
    */
    bool isIn(const QGeoCoordinate& coordinate) const;

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
    double minElevation(void) const { return _minElevation; }

    /**
    * Accessor for the maximum elevation of the tile
    *
    * @return maximum elevation
    */
    double maxElevation(void) const { return _maxElevation; }

    /**
    * Accessor for the average elevation of the tile
    *
    * @return average elevation
    */
    double avgElevation(void) const { return _avgElevation; }

    /**
    * Accessor for the center coordinate
    *
    * @return center coordinate
    */
    QGeoCoordinate centerCoordinate(void) const;

    /**
    * Serialize data
    *
    * @return serialized data
    */
    static QByteArray serialize(QByteArray input);

    /// Approximate spacing of the elevation data measurement points
    static constexpr double terrainAltitudeSpacing = 30.0;

private:
    typedef struct {
        double  swLat,swLon, neLat, neLon;
        int16_t minElevation;
        int16_t maxElevation;
        double  avgElevation;
        int16_t gridSizeLat;
        int16_t gridSizeLon;
    } TileInfo_t;

    inline int _latToDataIndex(double latitude) const;
    inline int _lonToDataIndex(double longitude) const;

    QGeoCoordinate      _southWest;                                     /// South west corner of the tile
    QGeoCoordinate      _northEast;                                     /// North east corner of the tile

    int16_t             _minElevation;                                  /// Minimum elevation in tile
    int16_t             _maxElevation;                                  /// Maximum elevation in tile
    double              _avgElevation;                                  /// Average elevation of the tile

    int16_t**           _data;                                          /// 2D elevation data array
    int16_t             _gridSizeLat;                                   /// data grid size in latitude direction
    int16_t             _gridSizeLon;                                   /// data grid size in longitude direction
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
