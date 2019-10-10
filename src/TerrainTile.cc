#include "TerrainTile.h"
#include "JsonHelper.h"
#include "QGCMapEngine.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QDir>

QGC_LOGGING_CATEGORY(TerrainTileLog, "TerrainTileLog")

const char*  AirmapTerrainTile::_jsonStatusKey        = "status";
const char*  AirmapTerrainTile::_jsonDataKey          = "data";
const char*  AirmapTerrainTile::_jsonBoundsKey        = "bounds";
const char*  AirmapTerrainTile::_jsonSouthWestKey     = "sw";
const char*  AirmapTerrainTile::_jsonNorthEastKey     = "ne";
const char*  AirmapTerrainTile::_jsonStatsKey         = "stats";
const char*  AirmapTerrainTile::_jsonMaxElevationKey  = "max";
const char*  AirmapTerrainTile::_jsonMinElevationKey  = "min";
const char*  AirmapTerrainTile::_jsonAvgElevationKey  = "avg";
const char*  AirmapTerrainTile::_jsonCarpetKey        = "carpet";

//AirmapTerrainTile::AirmapTerrainTile()
//    :  TerrainTile()
//    , _data(nullptr)
//    , _gridSizeLat(-1)
//    , _gridSizeLon(-1)
//{
//}

AirmapTerrainTile::~AirmapTerrainTile()
{
    if (_data) {
        for (int i = 0; i < _gridSizeLat; i++) {
            delete _data[i];
        }
        delete _data;
        _data = nullptr;
    }
}

AirmapTerrainTile::AirmapTerrainTile(QByteArray byteArray)
    : TerrainTile()
    , _data(nullptr)
    , _gridSizeLat(-1)
    , _gridSizeLon(-1)
{
    int cTileHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    int cTileBytesAvailable = byteArray.size();

    if (cTileBytesAvailable < cTileHeaderBytes) {
        qWarning() << "Terrain tile binary data too small for TileInfo_s header";
        return;
    }

    const TileInfo_t* tileInfo = reinterpret_cast<const TileInfo_t*>(byteArray.constData());
    _southWest.setLatitude(tileInfo->swLat);
    _southWest.setLongitude(tileInfo->swLon);
    _northEast.setLatitude(tileInfo->neLat);
    _northEast.setLongitude(tileInfo->neLon);
    _minElevation = tileInfo->minElevation;
    _maxElevation = tileInfo->maxElevation;
    _avgElevation = tileInfo->avgElevation;
    _gridSizeLat = tileInfo->gridSizeLat;
    _gridSizeLon = tileInfo->gridSizeLon;

    qCDebug(TerrainTileLog) << "Loading terrain tile: " << _southWest << " - " << _northEast;
    qCDebug(TerrainTileLog) << "min:max:avg:sizeLat:sizeLon" << _minElevation << _maxElevation << _avgElevation << _gridSizeLat << _gridSizeLon;

    int cTileDataBytes = static_cast<int>(sizeof(int16_t)) * _gridSizeLat * _gridSizeLon;
    if (cTileBytesAvailable < cTileHeaderBytes + cTileDataBytes) {
        qWarning() << "Terrain tile binary data too small for tile data";
        return;
    }

    _data = new int16_t*[_gridSizeLat];
    for (int k = 0; k < _gridSizeLat; k++) {
        _data[k] = new int16_t[_gridSizeLon];
    }

    int valueIndex = 0;
    const int16_t* pTileData = reinterpret_cast<const int16_t*>(&reinterpret_cast<const uint8_t*>(byteArray.constData())[cTileHeaderBytes]);
    for (int i = 0; i < _gridSizeLat; i++) {
        for (int j = 0; j < _gridSizeLon; j++) {
            _data[i][j] = pTileData[valueIndex++];
        }
    }

    _isValid = true;

    return;
}


bool AirmapTerrainTile::isIn(const QGeoCoordinate& coordinate)
{
    if (!_isValid) {
        qCWarning(TerrainTileLog) << "isIn requested, but tile not valid";
        return false;
    }
    bool ret = coordinate.latitude() >= _southWest.latitude() && coordinate.longitude() >= _southWest.longitude() &&
            coordinate.latitude() <= _northEast.latitude() && coordinate.longitude() <= _northEast.longitude();
    qCDebug(TerrainTileLog) << "Checking isIn: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast << ": " << ret;
    return ret;
}

double AirmapTerrainTile::elevation(const QGeoCoordinate& coordinate)
{
    if (_isValid) {
        qCDebug(TerrainTileLog) << "elevation: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast;
        // Get the index at resolution of 1 arc second
        int indexLat = _latToDataIndex(coordinate.latitude());
        int indexLon = _lonToDataIndex(coordinate.longitude());
        if (indexLat == -1 || indexLon == -1) {
            qCWarning(TerrainTileLog) << "Internal error indexLat:indexLon == -1" << indexLat << indexLon;
            return qQNaN();
        }
        qCDebug(TerrainTileLog) << "indexLat:indexLon" << indexLat << indexLon << "elevation" << _data[indexLat][indexLon];
        return static_cast<double>(_data[indexLat][indexLon]);
    } else {
        qCWarning(TerrainTileLog) << "Asking for elevation, but no valid data.";
        return qQNaN();
    }
}

QGeoCoordinate AirmapTerrainTile::centerCoordinate(void)
{
    return _southWest.atDistanceAndAzimuth(_southWest.distanceTo(_northEast) / 2.0, _southWest.azimuthTo(_northEast));
}

QByteArray AirmapTerrainTile::serialize(QByteArray input)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(input, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QByteArray emptyArray;
        return emptyArray;
    }

    if (!document.isObject()) {
        qCDebug(TerrainTileLog) << "Terrain tile json doc is no object";
        QByteArray emptyArray;
        return emptyArray;
    }
    QJsonObject rootObject = document.object();

    QString errorString;
    QList<JsonHelper::KeyValidateInfo> rootVersionKeyInfoList = {
        { _jsonStatusKey, QJsonValue::String, true },
        { _jsonDataKey,   QJsonValue::Object, true },
    };
    if (!JsonHelper::validateKeys(rootObject, rootVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        QByteArray emptyArray;
        return emptyArray;
    }

    if (rootObject[_jsonStatusKey].toString() != "success") {
        qCDebug(TerrainTileLog) << "Invalid terrain tile.";
        QByteArray emptyArray;
        return emptyArray;
    }
    const QJsonObject& dataObject = rootObject[_jsonDataKey].toObject();
    QList<JsonHelper::KeyValidateInfo> dataVersionKeyInfoList = {
        { _jsonBoundsKey, QJsonValue::Object, true },
        { _jsonStatsKey,  QJsonValue::Object, true },
        { _jsonCarpetKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(dataObject, dataVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        QByteArray emptyArray;
        return emptyArray;
    }

    // Bounds
    const QJsonObject& boundsObject = dataObject[_jsonBoundsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> boundsVersionKeyInfoList = {
        { _jsonSouthWestKey, QJsonValue::Array, true },
        { _jsonNorthEastKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(boundsObject, boundsVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        QByteArray emptyArray;
        return emptyArray;
    }
    const QJsonArray& swArray = boundsObject[_jsonSouthWestKey].toArray();
    const QJsonArray& neArray = boundsObject[_jsonNorthEastKey].toArray();
    if (swArray.count() < 2 || neArray.count() < 2 ) {
        qCDebug(TerrainTileLog) << "Incomplete bounding location";
        QByteArray emptyArray;
        return emptyArray;
    }

    // Stats
    const QJsonObject& statsObject = dataObject[_jsonStatsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> statsVersionKeyInfoList = {
        { _jsonMinElevationKey, QJsonValue::Double, true },
        { _jsonMaxElevationKey, QJsonValue::Double, true },
        { _jsonAvgElevationKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(statsObject, statsVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        QByteArray emptyArray;
        return emptyArray;
    }

    // Carpet
    const QJsonArray& carpetArray = dataObject[_jsonCarpetKey].toArray();
    int gridSizeLat = carpetArray.count();
    int gridSizeLon = carpetArray[0].toArray().count();
    qCDebug(TerrainTileLog) << "Received tile has size in latitude direction: " << gridSizeLat;
    qCDebug(TerrainTileLog) << "Received tile has size in longitued direction: " << gridSizeLon;

    TileInfo_t tileInfo;

    tileInfo.swLat = swArray[0].toDouble();
    tileInfo.swLon = swArray[1].toDouble();
    tileInfo.neLat = neArray[0].toDouble();
    tileInfo.neLon = neArray[1].toDouble();
    tileInfo.minElevation = static_cast<int16_t>(statsObject[_jsonMinElevationKey].toInt());
    tileInfo.maxElevation = static_cast<int16_t>(statsObject[_jsonMaxElevationKey].toInt());
    tileInfo.avgElevation = statsObject[_jsonAvgElevationKey].toDouble();
    tileInfo.gridSizeLat = static_cast<int16_t>(gridSizeLat);
    tileInfo.gridSizeLon = static_cast<int16_t>(gridSizeLon);

    int cTileHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    int cTileDataBytes = static_cast<int>(sizeof(int16_t)) * gridSizeLat * gridSizeLon;

    QByteArray byteArray(cTileHeaderBytes + cTileDataBytes, 0);

    TileInfo_t* pTileInfo = reinterpret_cast<TileInfo_t*>(byteArray.data());
    int16_t*    pTileData = reinterpret_cast<int16_t*>(&reinterpret_cast<uint8_t*>(byteArray.data())[cTileHeaderBytes]);

    *pTileInfo = tileInfo;

    int valueIndex = 0;
    for (int i = 0; i < gridSizeLat; i++) {
        const QJsonArray& row = carpetArray[i].toArray();
        if (row.count() < gridSizeLon) {
            qCDebug(TerrainTileLog) << "Expected row array of " << gridSizeLon << ", instead got " << row.count();
            QByteArray emptyArray;
            return emptyArray;
        }
        for (int j = 0; j < gridSizeLon; j++) {
            pTileData[valueIndex++] = static_cast<int16_t>(row[j].toDouble());
        }
    }

    return byteArray;
}


int AirmapTerrainTile::_latToDataIndex(double latitude)
{
    if (isValid() && _southWest.isValid() && _northEast.isValid()) {
        return qRound((latitude - _southWest.latitude()) / (_northEast.latitude() - _southWest.latitude()) * (_gridSizeLat - 1));
    } else {
        qCWarning(TerrainTileLog) << "AirmapTerrainTile::_latToDataIndex internal error" << isValid() << _southWest.isValid() << _northEast.isValid();
        return -1;
    }
}

int AirmapTerrainTile::_lonToDataIndex(double longitude)
{
    if (isValid() && _southWest.isValid() && _northEast.isValid()) {
        return qRound((longitude - _southWest.longitude()) / (_northEast.longitude() - _southWest.longitude()) * (_gridSizeLon - 1));
    } else {
        qCWarning(TerrainTileLog) << "AirmapTerrainTile::_lonToDataIndex internal error" << isValid() << _southWest.isValid() << _northEast.isValid();
        return -1;
    }
}

//------------------------------------------------------------------------------

bool GeotiffTerrainTile::isIn(const QGeoCoordinate& coordinate) {
    int xy[2];
    lonlatToPixel(coordinate, xy);

    if (xy[0] < poBand->GetXSize() && xy[1] < poBand->GetYSize()) {
        int ret = poBand->GetDataCoverageStatus(xy[0], xy[1], 1, 1, 0, nullptr);
        if (ret == GDAL_DATA_COVERAGE_STATUS_DATA) {
            // If data is NoDataValue, return false
            if (elevation(coordinate) != poBand->GetNoDataValue())
                qCDebug(TerrainTileLog)
                    << "GeotiffTerrainTile isIn " << (elevation(coordinate))
                    << coordinate;
            return (elevation(coordinate) != poBand->GetNoDataValue());
        }
    }
    qCDebug(TerrainTileLog)
        << "GeotiffTerrainTile isIn "
        << "false" << coordinate.longitude() << coordinate.latitude();
    return false;
}

QGeoCoordinate GeotiffTerrainTile::centerCoordinate(void) {
    QGeoCoordinate result;
    double         xSize = poDataset->GetRasterXSize();
    double         ySize = poDataset->GetRasterYSize();

    // Fill coord with XY data before transformation
    result.setLongitude(adfGeoTransform[0] + xSize * adfGeoTransform[1] / 2);
    result.setLatitude(adfGeoTransform[3] + ySize * adfGeoTransform[5] / 2);

    xyTolonlat(result);
    return result;
}

int GeotiffTerrainTile::lonlatToxy(const QGeoCoordinate& c) {
    double lon = c.longitude();
    double lat = c.latitude();
    int    ret = lonlatToxyTransformation->Transform(1, &lon, &lat, NULL);
    return ret;
}

int GeotiffTerrainTile::xyTolonlat(const QGeoCoordinate& c) {
    double lon = c.longitude();
    double lat = c.latitude();
    int    ret = xyTolonlatTransformation->Transform(1, &lon, &lat, NULL);
    return ret;
}

int GeotiffTerrainTile::lonlatToPixel(const QGeoCoordinate& c, int xy[2]) {
    double lon = c.longitude();
    double lat = c.latitude();


    int    ret = lonlatToxyTransformation->Transform(1, &lon, &lat, NULL);

    if(adfGeoTransform[1] == 0 || adfGeoTransform[5] == 0){
        qCDebug(TerrainTileLog) << "Division by zero !";
    }
    xy[0] = (int)((lon - adfGeoTransform[0]) / adfGeoTransform[1]);
    xy[1] = (int)((lat - adfGeoTransform[3]) / adfGeoTransform[5]);

    return ret;
}

GeotiffTerrainTile::~GeotiffTerrainTile() { GDALClose(poDataset); }

GeotiffTerrainTile::GeotiffTerrainTile(QByteArray buff) {

    fname = QString(buff);

    qCDebug(TerrainTileLog)
        << "GeotiffTerrainTile : create with file: " << fname;

    GDALAllRegister();
    poDataset =
        (GDALDataset*)GDALOpen(fname.toStdString().c_str(), GA_ReadOnly);
    if (poDataset == NULL) {
    // TODO This part is WIP : if buff is not a filename then it's a cached buffer that contains a geotiff,
    // parse it using VSI Mem Buffer :
    //const char* filename = "vsiFile";
    //VSIFileFromMemBuffer(filename, (unsigned char*)(buff.data()), buff.size(), true);
    // Try if buff is an image array
    //poDataset =
    //    (GDALDataset*)GDALOpen(filename, GA_ReadOnly);

        _isValid = false;
        
    }

    if (poDataset->GetRasterCount() != 1){
        qCDebug(TerrainTileLog) << "GeotiffTerrainTile " << fname << " More than 1 band !";
        _isValid = false;
    }

    if (poDataset->GetProjectionRef() == NULL) {
        qCDebug(TerrainTileLog) << "GeotiffTerrainTile " << "GetProjectionRef KO";
        _isValid = false;
    }
    if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None) {
        qCDebug(TerrainTileLog) << "GeotiffTerrainTile " << "GetGeoTransform KO" << poDataset->GetGeoTransform(adfGeoTransform);
        _isValid = false;
    }
    qCDebug(TerrainTileLog) << "GeotiffTerrainTile " << "GetGeoTransform :" << 
        adfGeoTransform[0] << " " << 
        adfGeoTransform[1] << " " << 
        adfGeoTransform[2] << " " << 
        adfGeoTransform[3] << " " << 
        adfGeoTransform[4] << " " << 
        adfGeoTransform[5];
    _isValid = true;

    OGRSpatialReference src;
    src.SetWellKnownGeogCS("WGS84");
    OGRSpatialReference dest(poDataset->GetProjectionRef());

    lonlatToxyTransformation = OGRCreateCoordinateTransformation(&src, &dest);
    xyTolonlatTransformation = OGRCreateCoordinateTransformation(&dest, &src);
    if(lonlatToxyTransformation == NULL){
        qCDebug(TerrainTileLog) << "GeotiffTerrainTile : OGRCreateCoordinateTransformation lonlatToxyTransformation failed" ;
        _isValid = false;
    }
    if(xyTolonlatTransformation == NULL){
        qCDebug(TerrainTileLog) << "GeotiffTerrainTile : OGRCreateCoordinateTransformation xyTolonlatTransformation failed" ;
        _isValid = false;
    }

    // Fetch a raster band

    poBand = poDataset->GetRasterBand(1);

    double minElev_d, maxElev_d;
    poBand->ComputeStatistics(true, &minElev_d, &maxElev_d, &_avgElevation,
                              NULL, NULL, NULL);

    _minElevation = minElev_d;
    _maxElevation = maxElev_d;

    qCDebug(TerrainTileLog) << "GeotiffTerrainTile" << minElev_d << " " << maxElev_d;
}

double GeotiffTerrainTile::elevation(const QGeoCoordinate& coordinate) {
    int xy[2];
    lonlatToPixel(coordinate, xy);

    QGeoCoordinate curpoint;
    curpoint.setLongitude(coordinate.longitude());
    curpoint.setLatitude(coordinate.latitude());

    if (!(xy[0] < poBand->GetXSize() && xy[1] < poBand->GetYSize())) {
        qCDebug(TerrainTileLog) << "elevation error";
        return poBand->GetNoDataValue(nullptr);
    }
    // Reading raster data
    float* pafScanline;
    int    nXSize = poBand->GetXSize();
    pafScanline   = (float*)CPLMalloc(sizeof(float) * nXSize);
    int ret       = poBand->RasterIO(GF_Read, 0, xy[1], nXSize, 1, pafScanline,
                               nXSize, 1, GDT_Float32, 0, 0);
    if (ret != CE_None) {
        qCDebug(TerrainTileLog) << "elevation error : RasterIO" << ret;
        return poBand->GetNoDataValue(nullptr);
    }

    CPLFree(pafScanline);
    qCDebug(TerrainTileLog) << fname << " elevation for" << coordinate << " " << pafScanline[xy[0]];
    return pafScanline[xy[0]];
}

QByteArray GeotiffTerrainTile::serialize(QByteArray input) { return input; }

GeotiffDatasetTerrainTile::~GeotiffDatasetTerrainTile() {
    for (int i = 0; i < _tiles.size(); i++) {
        delete _tiles.at(i);    
    }

}

GeotiffDatasetTerrainTile::GeotiffDatasetTerrainTile(QByteArray buff) {

    QDir * dir = new QDir(QString(buff));
    QStringList filters;
    filters << "*.tiff"
            << "*.tif"
            << "*.geotiff";
    dir->setNameFilters(filters);

    // Recent files first
    dir->setSorting(QDir::Time);

    QFileInfoList list = dir->entryInfoList();

    for (int i = 0; i < list.size(); i++) {
        QFileInfo fileInfo = list.at(i);
        
        // Create a GeotiffTerrainTile and add it to our list
        qCDebug(TerrainTileLog) << "GeotiffDatasetTerrainTile Adding : " << fileInfo.absoluteFilePath();
        GeotiffTerrainTile * tile = new GeotiffTerrainTile(fileInfo.absoluteFilePath().toUtf8());
        _tiles.append(tile);
        
    }
}
QByteArray GeotiffDatasetTerrainTile::serialize(QByteArray input) {
    return input;
}

bool   GeotiffDatasetTerrainTile::isIn(const QGeoCoordinate& coordinate){
    for (int i = 0; i < _tiles.size(); i++) {
        if(_tiles[i]->isIn(coordinate)){
            return true;
        }
    }
    return false;
}
double GeotiffDatasetTerrainTile::elevation(const QGeoCoordinate& coordinate){
    for (int i = 0; i < _tiles.size(); i++) {
        if(_tiles[i]->isIn(coordinate)){
            return _tiles[i]->elevation(coordinate);
        }
    }
    qCDebug(TerrainTileLog) << "GeotiffDatasetTerrainTile Not found";
    return 0.;
}

QGeoCoordinate GeotiffDatasetTerrainTile::centerCoordinate(void){
    return QGeoCoordinate(0,0);
}
