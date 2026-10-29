#include "RallyPoint.h"

#include "TerrainQuery.h"

QMap<QString, FactMetaData*> RallyPoint::_metaDataMap;

RallyPoint::RallyPoint(const QGeoCoordinate& coordinate, QGroundControlQmlGlobal::AltitudeFrame altitudeFrame,
                       QObject* parent)
    : QObject(parent)
    , _dirty(false)
    , _longitudeFact(0, _longitudeFactName, FactMetaData::valueTypeDouble)
    , _latitudeFact(0, _latitudeFactName, FactMetaData::valueTypeDouble)
    , _altitudeFact(0, _altitudeFactName, FactMetaData::valueTypeDouble)
    , _altitudeFrame(altitudeFrame)
{
    // Using setCoordinate() would set dirty
    _longitudeFact.setRawValue(coordinate.longitude());
    _latitudeFact.setRawValue(coordinate.latitude());
    _altitudeFact.setRawValue(coordinate.altitude());

    _factSetup();
}

RallyPoint::RallyPoint(const RallyPoint& other, QObject* parent)
    : QObject(parent)
    , _dirty(false)
    , _longitudeFact(0, _longitudeFactName, FactMetaData::valueTypeDouble)
    , _latitudeFact(0, _latitudeFactName, FactMetaData::valueTypeDouble)
    , _altitudeFact(0, _altitudeFactName, FactMetaData::valueTypeDouble)
    , _altitudeFrame(other._altitudeFrame)
{
    _longitudeFact.setRawValue(other._longitudeFact.rawValue());
    _latitudeFact.setRawValue(other._latitudeFact.rawValue());
    _altitudeFact.setRawValue(other._altitudeFact.rawValue());

    _factSetup();
}

const RallyPoint& RallyPoint::operator=(const RallyPoint& other)
{
    _longitudeFact.setRawValue(other._longitudeFact.rawValue());
    _latitudeFact.setRawValue(other._latitudeFact.rawValue());
    _altitudeFact.setRawValue(other._altitudeFact.rawValue());

    setAltitudeFrame(other._altitudeFrame);

    emit coordinateChanged(coordinate());

    return *this;
}

RallyPoint::~RallyPoint()
{

}

void RallyPoint::_factSetup(void)
{
    _cacheFactMetadata();

    _latitudeFact.setMetaData(_metaDataMap[_latitudeFactName]);
    _longitudeFact.setMetaData(_metaDataMap[_longitudeFactName]);
    _altitudeFact.setMetaData(_metaDataMap[_altitudeFactName]);

    connect(&_latitudeFact, &Fact::valueChanged, this, &RallyPoint::_sendCoordinateChanged);
    connect(&_longitudeFact, &Fact::valueChanged, this, &RallyPoint::_sendCoordinateChanged);
    connect(&_altitudeFact, &Fact::valueChanged, this, &RallyPoint::_sendCoordinateChanged);
    connect(&_altitudeFact, &Fact::rawValueChanged, this, &RallyPoint::_altitudeFactRawValueChanged);

    // Several quick moves (a drag) result in a single terrain request
    _terrainQueryTimer.setInterval(500);
    _terrainQueryTimer.setSingleShot(true);
    connect(&_terrainQueryTimer, &QTimer::timeout, this, &RallyPoint::_requestTerrainAltitude);
    connect(this, &RallyPoint::coordinateChanged, this, [this](const QGeoCoordinate& coordinate) {
        if (_followTerrain && (coordinate.latitude() != _lastTerrainQueryLatitude ||
                               coordinate.longitude() != _lastTerrainQueryLongitude)) {
            _terrainQueryTimer.start();
        }
    });

    // Raw, not cooked: a units change must not dirty the point
    for (Fact* const fact : {&_latitudeFact, &_longitudeFact, &_altitudeFact}) {
        connect(fact, &Fact::rawValueChanged, this, [this]() { setDirty(true); });
    }
}

void RallyPoint::_cacheFactMetadata() {
    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/RallyPoint.FactMetaData.json"), nullptr /* metaDataParent */);
    }
}

void RallyPoint::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != this->coordinate()) {
        _longitudeFact.setRawValue(coordinate.longitude());
        _latitudeFact.setRawValue(coordinate.latitude());
        _altitudeFact.setRawValue(coordinate.altitude());
        emit coordinateChanged(coordinate);
        setDirty(true);
    }
}

void RallyPoint::setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrame altitudeFrame)
{
    if (altitudeFrame != _altitudeFrame) {
        _altitudeFrame = altitudeFrame;
        emit altitudeFrameChanged();
        setDirty(true);
    }
}

void RallyPoint::setDirty(bool dirty)
{
    if (dirty != _dirty) {
        _dirty = dirty;
        emit dirtyChanged(dirty);
    }
}

void RallyPoint::changeAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrame altitudeFrame, double seedAltitude,
                                     bool seedFromTerrain)
{
    const bool terrainRequest = (altitudeFrame == QGroundControlQmlGlobal::AltitudeFrameAbsolute) && seedFromTerrain;

    // An answer for an AMSL request must not override a later selection, even when it is the current frame
    if (!terrainRequest) {
        _stopFollowingTerrain();
        _setTerrainQueryFailed(false);
    }

    if (altitudeFrame == _altitudeFrame) {
        return;
    }

    if (terrainRequest) {
        useTerrainAltitude(true /* switchToAbsoluteOnFailure */);
        return;
    }

    setAltitudeFrame(altitudeFrame);
    _altitudeFact.setRawValue(seedAltitude);
}

void RallyPoint::useTerrainAltitude(bool switchToAbsoluteOnFailure)
{
    _terrainQueryTimer.stop();
    _setTerrainQueryFailed(false);
    _switchToAbsoluteOnTerrainFailure = switchToAbsoluteOnFailure;
    _requestTerrainAltitude();
}

void RallyPoint::_requestTerrainAltitude(void)
{
    const QGeoCoordinate coord = coordinate();
    if (!coord.isValid()) {
        return;
    }

    _lastTerrainQueryLatitude = coord.latitude();
    _lastTerrainQueryLongitude = coord.longitude();

    if (_terrainQuery) {
        disconnect(_terrainQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this,
                   &RallyPoint::_terrainDataReceived);
    }
    _terrainQuery = new TerrainAtCoordinateQuery(true /* autoDelete */);
    connect(_terrainQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &RallyPoint::_terrainDataReceived);
    _terrainQuery->requestData({coord});
}

void RallyPoint::_terrainDataReceived(bool success, const QList<double>& heights)
{
    _terrainQuery = nullptr;

    const QGeoCoordinate coord = coordinate();
    if (coord.latitude() != _lastTerrainQueryLatitude || coord.longitude() != _lastTerrainQueryLongitude) {
        // The point moved while the request was in flight, the answer is for the old position
        _requestTerrainAltitude();
        return;
    }

    const bool switchToAbsoluteOnFailure = _switchToAbsoluteOnTerrainFailure;
    _switchToAbsoluteOnTerrainFailure = false;

    if (!success || heights.isEmpty()) {
        if (switchToAbsoluteOnFailure) {
            setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameAbsolute);
        }
        _setTerrainQueryFailed(true);
        return;
    }

    _setTerrainQueryFailed(false);
    _followTerrain = true;
    _setAltitudeFromTerrain(heights.first());
}

void RallyPoint::_setAltitudeFromTerrain(double altitude)
{
    _applyingTerrainAltitude = true;
    setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameAbsolute);
    _altitudeFact.setRawValue(altitude);
    _applyingTerrainAltitude = false;
}

void RallyPoint::_altitudeFactRawValueChanged(void)
{
    if (_applyingTerrainAltitude) {
        return;
    }

    _stopFollowingTerrain();
    _setTerrainQueryFailed(false);
}

void RallyPoint::_stopFollowingTerrain(void)
{
    _followTerrain = false;
    _switchToAbsoluteOnTerrainFailure = false;
    _terrainQueryTimer.stop();

    // A late answer must not overwrite an altitude the user has taken over
    if (_terrainQuery) {
        disconnect(_terrainQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this,
                   &RallyPoint::_terrainDataReceived);
        _terrainQuery = nullptr;
    }
}

void RallyPoint::_setTerrainQueryFailed(bool failed)
{
    if (failed != _terrainQueryFailed) {
        _terrainQueryFailed = failed;
        emit terrainQueryFailedChanged();
    }
}

MAV_FRAME RallyPoint::mavFrame(QGroundControlQmlGlobal::AltitudeFrame altitudeFrame)
{
    switch (altitudeFrame) {
        case QGroundControlQmlGlobal::AltitudeFrameAbsolute:
            return MAV_FRAME_GLOBAL;
        case QGroundControlQmlGlobal::AltitudeFrameTerrain:
            return MAV_FRAME_GLOBAL_TERRAIN_ALT;
        default:
            return MAV_FRAME_GLOBAL_RELATIVE_ALT;
    }
}

QGroundControlQmlGlobal::AltitudeFrame RallyPoint::altitudeFrameFromMavFrame(MAV_FRAME mavFrame)
{
    switch (mavFrame) {
        case MAV_FRAME_GLOBAL:
        case MAV_FRAME_GLOBAL_INT:
            return QGroundControlQmlGlobal::AltitudeFrameAbsolute;
        case MAV_FRAME_GLOBAL_TERRAIN_ALT:
        case MAV_FRAME_GLOBAL_TERRAIN_ALT_INT:
            return QGroundControlQmlGlobal::AltitudeFrameTerrain;
        default:
            return QGroundControlQmlGlobal::AltitudeFrameRelative;
    }
}

double RallyPoint::getDefaultFactAltitude() {
    _cacheFactMetadata();
    auto it = _metaDataMap.find(QString(_altitudeFactName));
    if(it != _metaDataMap.end() && (*it)->defaultValueAvailable()) {
        return (*it)->rawDefaultValue().toDouble();
    }
    return 0.0;
}

QGeoCoordinate RallyPoint::coordinate(void) const
{
    return QGeoCoordinate(_latitudeFact.rawValue().toDouble(), _longitudeFact.rawValue().toDouble(), _altitudeFact.rawValue().toDouble());
}

void RallyPoint::_sendCoordinateChanged(void)
{
    emit coordinateChanged(coordinate());
}
