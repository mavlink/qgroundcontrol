#include "TransformPositionController.h"

#include "Fact.h"
#include "FactMetaData.h"
#include "MultiVehicleManager.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(TransformPositionControllerLog, "QMLControls.TransformPositionController")

QMap<QString, FactMetaData*> TransformPositionController::_metaDataMap;

TransformPositionController::TransformPositionController(QObject *parent)
    : QObject(parent)
    , _latitudeFact(new Fact(0, _latitudeFactName, FactMetaData::valueTypeDouble, this))
    , _longitudeFact(new Fact(0, _longitudeFactName, FactMetaData::valueTypeDouble, this))
    , _zoneFact(new Fact(0, _zoneFactName, FactMetaData::valueTypeUint8, this))
    , _hemisphereFact(new Fact(0, _hemisphereFactName, FactMetaData::valueTypeUint8, this))
    , _eastingFact(new Fact(0, _eastingFactName, FactMetaData::valueTypeDouble, this))
    , _northingFact(new Fact(0, _northingFactName, FactMetaData::valueTypeDouble, this))
    , _mgrsFact(new Fact(0, _mgrsFactName, FactMetaData::valueTypeString, this))
    , _offsetEastFact(new Fact(0, _offsetEastFactName, FactMetaData::valueTypeDouble, this))
    , _offsetNorthFact(new Fact(0, _offsetNorthFactName, FactMetaData::valueTypeDouble, this))
    , _offsetUpFact(new Fact(0, _offsetUpFactName, FactMetaData::valueTypeDouble, this))
    , _rotateDegreesCWFact(new Fact(0, _rotateDegreesCWFactName, FactMetaData::valueTypeDouble, this))
{
    // qCDebug(TransformPositionControllerLog) << Q_FUNC_INFO << this;

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/TransformPositionController.FactMetaData.json"), nullptr /* QObject parent */);
    }

    _latitudeFact->setMetaData(_metaDataMap[_latitudeFactName]);
    _longitudeFact->setMetaData(_metaDataMap[_longitudeFactName]);
    _zoneFact->setMetaData(_metaDataMap[_zoneFactName]);
    _hemisphereFact->setMetaData(_metaDataMap[_hemisphereFactName]);
    _eastingFact->setMetaData(_metaDataMap[_eastingFactName]);
    _northingFact->setMetaData(_metaDataMap[_northingFactName]);
    _mgrsFact->setMetaData(_metaDataMap[_mgrsFactName]);
    _offsetEastFact->setMetaData(_metaDataMap[_offsetEastFactName]);
    _offsetNorthFact->setMetaData(_metaDataMap[_offsetNorthFactName]);
    _offsetUpFact->setMetaData(_metaDataMap[_offsetUpFactName]);
    _rotateDegreesCWFact->setMetaData(_metaDataMap[_rotateDegreesCWFactName]);
}

TransformPositionController::~TransformPositionController()
{
    // qCDebug(TransformPositionControllerLog) << Q_FUNC_INFO << this;
}

void TransformPositionController::setCoordinate(QGeoCoordinate coordinate)
{
    if (!coordinate.isValid()) {
        qCWarning(TransformPositionControllerLog) << "Attempt to set invalid coordinate";
        return;
    }

    if (coordinate == _coordinate) {
        return;
    }

    const bool wasInvalid = !_coordinate.isValid();
    _coordinate = coordinate;

    // Do not emit on the initial invalid->valid transition to prevent
    // onCoordinateChanged handlers from firing on initial dialog setup.
    if (!wasInvalid) {
        emit coordinateChanged(_coordinate);
    }
}

void TransformPositionController::initValues()
{
    if (!_coordinate.isValid()) {
        return;
    }

    _latitudeFact->setRawValue(_coordinate.latitude());
    _longitudeFact->setRawValue(_coordinate.longitude());

    double easting, northing;
    const int zone = QGCGeo::convertGeoToUTM(_coordinate, easting, northing);
    if ((zone >= 1) && (zone <= 60)) {
        _zoneFact->setRawValue(zone);
        _hemisphereFact->setRawValue(_coordinate.latitude() < 0);
        _eastingFact->setRawValue(easting);
        _northingFact->setRawValue(northing);
    }

    const QString mgrs = QGCGeo::convertGeoToMGRS(_coordinate);
    if (!mgrs.isEmpty()) {
        _mgrsFact->setRawValue(mgrs);
    }
}

void TransformPositionController::setFromGeo()
{
    QGeoCoordinate newCoordinate = _coordinate;
    newCoordinate.setLatitude(_latitudeFact->rawValue().toDouble());
    newCoordinate.setLongitude(_longitudeFact->rawValue().toDouble());
    setCoordinate(newCoordinate);
}

void TransformPositionController::setFromUTM()
{
    qCDebug(TransformPositionControllerLog) << _eastingFact->rawValue().toDouble() << _northingFact->rawValue().toDouble() << _zoneFact->rawValue().toInt() << (_hemisphereFact->rawValue().toInt() == 1);
    QGeoCoordinate newCoordinate;
    if (QGCGeo::convertUTMToGeo(_eastingFact->rawValue().toDouble(), _northingFact->rawValue().toDouble(), _zoneFact->rawValue().toInt(), _hemisphereFact->rawValue().toInt() == 1, newCoordinate)) {
        qCDebug(TransformPositionControllerLog) << _eastingFact->rawValue().toDouble() << _northingFact->rawValue().toDouble() << _zoneFact->rawValue().toInt() << (_hemisphereFact->rawValue().toInt() == 1) << newCoordinate;
        setCoordinate(newCoordinate);
    } else {
        initValues();
    }
}

void TransformPositionController::setFromMGRS()
{
    QGeoCoordinate newCoordinate;
    if (QGCGeo::convertMGRSToGeo(_mgrsFact->rawValue().toString(), newCoordinate)) {
        setCoordinate(newCoordinate);
    } else {
        initValues();
    }
}

void TransformPositionController::setFromVehicle()
{
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    if (!activeVehicle) {
        qCWarning(TransformPositionControllerLog) << "Cannot set coordinate from vehicle: no active vehicle";
        return;
    }

    setCoordinate(activeVehicle->coordinate());
}
