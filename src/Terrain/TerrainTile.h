#pragma once

#include <QtCore/QList>

class QGeoCoordinate;
class TerrainTileTest;


class TerrainTile
{
    friend class TerrainTileTest;

public:
    /// Constructor from serialized elevation data (either from file or web)
    ///    @param document
    explicit TerrainTile(const QByteArray &byteArray);
    virtual ~TerrainTile();

    /// Check whether valid data is loaded
    ///    @return true if data is valid
    bool isValid() const { return _isValid; }

    /// Evaluates the elevation at the given coordinate
    ///    @param coordinate
    ///    @return elevation
    double elevation(const QGeoCoordinate &coordinate) const;

    /// Accessor for the minimum elevation of the tile
    ///    @return minimum elevation
    double minElevation() const { return (_isValid ? static_cast<double>(_tileInfo.minElevation) : qQNaN()); }

    /// Accessor for the maximum elevation of the tile
    ///    @return maximum elevation
    double maxElevation() const { return (_isValid ? static_cast<double>(_tileInfo.maxElevation) : qQNaN()); }

    /// Accessor for the average elevation of the tile
    ///    @return average elevation
    double avgElevation() const { return (_isValid ? _tileInfo.avgElevation : qQNaN()); }

protected:
    struct TileInfo_t {
        double  swLat, swLon, neLat, neLon;
        int16_t minElevation, maxElevation;
        double  avgElevation;
        int16_t gridSizeLat, gridSizeLon;
    } Q_PACKED;

private:
    TileInfo_t _tileInfo{};
    QList<QList<int16_t>> _elevationData;   ///< 2D elevation data array
    double _cellSizeLat = 0.0;              ///< data grid size in latitude direction
    double _cellSizeLon = 0.0;              ///< data grid size in longitude direction
    bool _isValid = false;                  ///< data loaded is valid
};
