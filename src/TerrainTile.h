#ifndef TERRAINTILE_H
#define TERRAINTILE_H

#include "QGCLoggingCategory.h"

#include "cpl_conv.h" // for CPLMalloc()
#include "gdal_priv.h"
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
  TerrainTile()
      : _minElevation(-1.0), _maxElevation(-1.0), _avgElevation(-1.0),
        _isValid(false) {}

  virtual ~TerrainTile(){};

  /**
   * Constructor from serialized elevation data (either from file or web)
   *
   * @param document
   */
   //TerrainTile(QByteArray byteArray);

  /**
   * Check for whether a coordinate lies within this tile
   *
   * @param coordinate
   * @return true if within
   */
  virtual bool isIn(const QGeoCoordinate& coordinate){
      Q_UNUSED(coordinate);
      qDebug() << "TerrainTile::isIn : Empty function Do Not USE"; 
      return false;};

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
  virtual double elevation(const QGeoCoordinate& coordinate){Q_UNUSED(coordinate);return 0.;};

  /**
   * Accessor for the minimum elevation of the tile
   *
   * @return minimum elevation
   */
  double minElevation(void) { return _minElevation; }

  /**
   * Accessor for the maximum elevation of the tile
   *
   * @return maximum elevation
   */
  double maxElevation(void) { return _maxElevation; }

  /**
   * Accessor for the average elevation of the tile
   *
   * @return average elevation
   */
  double avgElevation(void) {return _avgElevation; }

  /**
   * Accessor for the center coordinate
   *
   * @return center coordinate
   */
  virtual QGeoCoordinate centerCoordinate(void){return QGeoCoordinate(0,0);};

  /// Approximate spacing of the elevation data measurement points
  static constexpr double terrainAltitudeSpacing = 30.0;
protected:
    int16_t             _minElevation;                                  /// Minimum elevation in tile
    int16_t             _maxElevation;                                  /// Maximum elevation in tile
    double              _avgElevation;                                  /// Average elevation of the tile
    bool                _isValid;                                       /// data loaded is valid
};

class AirmapTerrainTile : public TerrainTile {

public :
    //AirmapTerrainTile();
    AirmapTerrainTile(QByteArray byteArray);
    ~AirmapTerrainTile();
    bool isIn(const QGeoCoordinate& coordinate) override;
    double elevation(const QGeoCoordinate& coordinate) override;
    QGeoCoordinate centerCoordinate(void) override;

  /**
   * Serialize data
   *
   * @return serialized data
   */
  static QByteArray serialize(QByteArray input);

private:
    typedef struct {
        double  swLat,swLon, neLat, neLon;
        int16_t minElevation;
        int16_t maxElevation;
        double  avgElevation;
        int16_t gridSizeLat;
        int16_t gridSizeLon;
    } TileInfo_t;

    int _latToDataIndex(double latitude);
    int _lonToDataIndex(double longitude);

    QGeoCoordinate      _southWest;                                     /// South west corner of the tile
    QGeoCoordinate      _northEast;                                     /// North east corner of the tile


    int16_t**           _data;                                          /// 2D elevation data array
    int16_t             _gridSizeLat;                                   /// data grid size in latitude direction
    int16_t             _gridSizeLon;                                   /// data grid size in longitude direction

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

class GeotiffTerrainTile : public TerrainTile {

public :
    GeotiffTerrainTile(QByteArray byteArray);
    ~GeotiffTerrainTile();
    bool isIn(const QGeoCoordinate& coordinate) override;
    double elevation(const QGeoCoordinate& coordinate) override;
    QGeoCoordinate centerCoordinate(void) override;

  static QByteArray serialize(QByteArray input);

private:
  
  int lonlatToxy(const QGeoCoordinate& c);
  int xyTolonlat(const QGeoCoordinate& c);
  int lonlatToPixel(const QGeoCoordinate& c, int xy[2]);

  GDALDataset *poDataset;
  GDALRasterBand *poBand;
  double adfGeoTransform[6];
  QString fname;

  OGRCoordinateTransformation *lonlatToxyTransformation;
  OGRCoordinateTransformation *xyTolonlatTransformation;
};

class GeotiffDatasetTerrainTile : public TerrainTile {

public :
    GeotiffDatasetTerrainTile(QByteArray byteArray);
    ~GeotiffDatasetTerrainTile();
    bool isIn(const QGeoCoordinate& coordinate) override;
    double elevation(const QGeoCoordinate& coordinate) override;
    QGeoCoordinate centerCoordinate(void) override;

  static QByteArray serialize(QByteArray input);

private:

  QList<GeotiffTerrainTile *> _tiles;

};

#endif // TERRAINTILE_H
