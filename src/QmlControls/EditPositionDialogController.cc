/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "EditPositionDialogController.h"
#include "QGCGeo.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(EditPositionDialogControllerLog, "qgc.qmlcontrols.editpositiondialogcontroller")

QMap<QString, FactMetaData*> EditPositionDialogController::_metaDataMap;

EditPositionDialogController::EditPositionDialogController(QObject *parent)
    : QObject(parent)
    , _latitudeFact(new Fact(0, _latitudeFactName, FactMetaData::valueTypeDouble, this))
    , _longitudeFact(new Fact(0, _longitudeFactName, FactMetaData::valueTypeDouble, this))
    , _zoneFact(new Fact(0, _zoneFactName, FactMetaData::valueTypeUint8, this))
    , _hemisphereFact(new Fact(0, _hemisphereFactName, FactMetaData::valueTypeUint8, this))
    , _eastingFact(new Fact(0, _eastingFactName, FactMetaData::valueTypeDouble, this))
    , _northingFact(new Fact(0, _northingFactName, FactMetaData::valueTypeDouble, this))
    , _mgrsFact(new Fact(0, _mgrsFactName, FactMetaData::valueTypeString, this))
{
    // qCDebug(EditPositionDialogControllerLog) << Q_FUNC_INFO << this;

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/EditPositionDialog.FactMetaData.json"), nullptr /* QObject parent */);
    }

    _latitudeFact->setMetaData(_metaDataMap[_latitudeFactName]);
    _longitudeFact->setMetaData(_metaDataMap[_longitudeFactName]);
    _zoneFact->setMetaData(_metaDataMap[_zoneFactName]);
    _hemisphereFact->setMetaData(_metaDataMap[_hemisphereFactName]);
    _eastingFact->setMetaData(_metaDataMap[_eastingFactName]);
    _northingFact->setMetaData(_metaDataMap[_northingFactName]);
    _mgrsFact->setMetaData(_metaDataMap[_mgrsFactName]);
}

EditPositionDialogController::~EditPositionDialogController()
{
    // qCDebug(EditPositionDialogControllerLog) << Q_FUNC_INFO << this;
}

void EditPositionDialogController::setCoordinate(QGeoCoordinate coordinate)
{
    if (coordinate != _coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(coordinate);
    }
}

void EditPositionDialogController::initValues()
{
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

void EditPositionDialogController::setFromGeo()
{
    _coordinate.setLatitude(_latitudeFact->rawValue().toDouble());
    _coordinate.setLongitude(_longitudeFact->rawValue().toDouble());
    emit coordinateChanged(_coordinate);
}

void EditPositionDialogController::setFromUTM()
{
    qCDebug(EditPositionDialogControllerLog) << _eastingFact->rawValue().toDouble() << _northingFact->rawValue().toDouble() << _zoneFact->rawValue().toInt() << (_hemisphereFact->rawValue().toInt() == 1);
    if (QGCGeo::convertUTMToGeo(_eastingFact->rawValue().toDouble(), _northingFact->rawValue().toDouble(), _zoneFact->rawValue().toInt(), _hemisphereFact->rawValue().toInt() == 1, _coordinate)) {
        qCDebug(EditPositionDialogControllerLog) << _eastingFact->rawValue().toDouble() << _northingFact->rawValue().toDouble() << _zoneFact->rawValue().toInt() << (_hemisphereFact->rawValue().toInt() == 1) << _coordinate;
        emit coordinateChanged(_coordinate);
    } else {
        initValues();
    }
}

void EditPositionDialogController::setFromMGRS()
{
    if (QGCGeo::convertMGRSToGeo(_mgrsFact->rawValue().toString(), _coordinate)) {
        emit coordinateChanged(_coordinate);
    } else {
        initValues();
    }
}

void EditPositionDialogController::setFromVehicle()
{
    setCoordinate(MultiVehicleManager::instance()->activeVehicle()->coordinate());
}

