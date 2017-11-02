/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraCalc.h"
#include "JsonHelper.h"
#include "Vehicle.h"
#include "CameraMetaData.h"

#include <QQmlEngine>

const char* CameraCalc::_valueSetIsDistanceName =       "ValueSetIsDistance";
const char* CameraCalc::_distanceToSurfaceName =        "DistanceToSurface";
const char* CameraCalc::_imageDensityName =             "ImageDensity";
const char* CameraCalc::_frontalOverlapName =           "FrontalOverlap";
const char* CameraCalc::_sideOverlapName =              "SideOverlap";
const char* CameraCalc::_adjustedFootprintFrontalName = "AdjustedFootprintFrontal";
const char* CameraCalc::_adjustedFootprintSideName =    "AdjustedFootprintSide";
const char* CameraCalc::_jsonCameraSpecTypeKey =        "CameraSpecType";
const char* CameraCalc::_jsonKnownCameraNameKey =       "CameraName";

CameraCalc::CameraCalc(Vehicle* vehicle, QObject* parent)
    : CameraSpec                    (parent)
    , _vehicle                      (vehicle)
    , _dirty                        (false)
    , _cameraSpecType               (CameraSpecNone)
    , _disableRecalc                (false)
    , _metaDataMap                  (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CameraCalc.FactMetaData.json"), this))
    , _valueSetIsDistanceFact       (0, _valueSetIsDistanceName,        FactMetaData::valueTypeBool)
    , _distanceToSurfaceFact        (0, _distanceToSurfaceName,         FactMetaData::valueTypeDouble)
    , _imageDensityFact             (0, _imageDensityName,              FactMetaData::valueTypeDouble)
    , _frontalOverlapFact           (0, _frontalOverlapName,            FactMetaData::valueTypeDouble)
    , _sideOverlapFact              (0, _sideOverlapName,               FactMetaData::valueTypeDouble)
    , _adjustedFootprintSideFact    (0, _adjustedFootprintSideName,     FactMetaData::valueTypeDouble)
    , _adjustedFootprintFrontalFact (0, _adjustedFootprintFrontalName,  FactMetaData::valueTypeDouble)
    , _imageFootprintSide           (0)
    , _imageFootprintFrontal        (0)
    , _knownCameraList              (_vehicle->staticCameraList())
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    _valueSetIsDistanceFact.setMetaData         (_metaDataMap[_valueSetIsDistanceName],         true /* setDefaultFromMetaData */);
    _distanceToSurfaceFact.setMetaData          (_metaDataMap[_distanceToSurfaceName],          true);
    _imageDensityFact.setMetaData               (_metaDataMap[_imageDensityName],               true);
    _frontalOverlapFact.setMetaData             (_metaDataMap[_frontalOverlapName],             true);
    _sideOverlapFact.setMetaData                (_metaDataMap[_sideOverlapName],                true);
    _adjustedFootprintSideFact.setMetaData      (_metaDataMap[_adjustedFootprintSideName],      true);
    _adjustedFootprintFrontalFact.setMetaData   (_metaDataMap[_adjustedFootprintFrontalName],   true);

    connect(this, &CameraCalc::knownCameraNameChanged, this, &CameraCalc::_knownCameraNameChanged);

    connect(this, &CameraCalc::cameraSpecTypeChanged, this, &CameraCalc::_recalcTriggerDistance);

    connect(&_distanceToSurfaceFact,    &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_imageDensityFact,         &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_frontalOverlapFact,       &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_sideOverlapFact,          &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(landscape(),                &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);

    _recalcTriggerDistance();
}

void CameraCalc::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void CameraCalc::setCameraSpecType(CameraSpecType cameraSpecType)
{
    if (cameraSpecType != _cameraSpecType) {
        _cameraSpecType = cameraSpecType;
        emit cameraSpecTypeChanged(_cameraSpecType);
    }
}

void CameraCalc::setKnownCameraName(QString knownCameraName)
{
    if (knownCameraName != _knownCameraName) {
        _knownCameraName = knownCameraName;
        emit knownCameraNameChanged(_knownCameraName);
    }
}

void CameraCalc::_knownCameraNameChanged(QString knownCameraName)
{
    if (_cameraSpecType == CameraSpecKnown) {
        CameraMetaData* cameraMetaData = NULL;

        // Update camera specs to new camera
        for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
            cameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
            if (knownCameraName == cameraMetaData->name()) {
                break;
            }
        }

        _disableRecalc = true;
        if (cameraMetaData) {
            sensorWidth()->setRawValue          (cameraMetaData->sensorWidth());
            sensorHeight()->setRawValue         (cameraMetaData->sensorHeight());
            imageWidth()->setRawValue           (cameraMetaData->imageWidth());
            imageHeight()->setRawValue          (cameraMetaData->imageHeight());
            focalLength()->setRawValue          (cameraMetaData->focalLength());
            landscape()->setRawValue            (cameraMetaData->landscape());
            fixedOrientation()->setRawValue     (cameraMetaData->fixedOrientation());
            minTriggerInterval()->setRawValue   (cameraMetaData->minTriggerInterval());
        } else {
            // We don't know this camera, switch back to custom
            _cameraSpecType = CameraSpecCustom;
            emit cameraSpecTypeChanged(_cameraSpecType);
        }
        _disableRecalc = false;

        _recalcTriggerDistance();
    }
}

void CameraCalc::_recalcTriggerDistance(void)
{
    if (_disableRecalc || _cameraSpecType == CameraSpecNone) {
        return;
    }

    _disableRecalc = true;

    double focalLength =    this->focalLength()->rawValue().toDouble();
    double sensorWidth =    this->sensorWidth()->rawValue().toDouble();
    double sensorHeight =   this->sensorHeight()->rawValue().toDouble();
    double imageWidth =     this->imageWidth()->rawValue().toDouble();
    double imageHeight =    this->imageHeight()->rawValue().toDouble();
    double imageDensity =   _imageDensityFact.rawValue().toDouble();

    if (focalLength <= 0 || sensorWidth <= 0 || sensorHeight <= 0 || imageWidth <= 0 || imageHeight <= 0 || imageDensity <= 0) {
        return;
    }

    if (_valueSetIsDistanceFact.rawValue().toBool()) {
        _imageDensityFact.setRawValue((_distanceToSurfaceFact.rawValue().toDouble() * sensorWidth * 100.0) / (imageWidth * focalLength));
    } else {
        _distanceToSurfaceFact.setRawValue((imageWidth * _imageDensityFact.rawValue().toDouble() * focalLength) / (sensorWidth * 100.0));
    }

    imageDensity = _imageDensityFact.rawValue().toDouble();

    if (landscape()->rawValue().toBool()) {
        _imageFootprintSide =       (imageWidth  * imageDensity) / 100.0;
        _imageFootprintFrontal =    (imageHeight * imageDensity) / 100.0;
    } else {
        _imageFootprintSide  =      (imageHeight * imageDensity) / 100.0;
        _imageFootprintFrontal =    (imageWidth  * imageDensity) / 100.0;
    }
    _adjustedFootprintSideFact.setRawValue      (_imageFootprintSide * ((100.0 - _sideOverlapFact.rawValue().toDouble()) / 100.0));
    _adjustedFootprintFrontalFact.setRawValue   (_imageFootprintFrontal * ((100.0 - _frontalOverlapFact.rawValue().toDouble()) / 100.0));

    emit imageFootprintSideChanged      (_imageFootprintSide);
    emit imageFootprintFrontalChanged   (_imageFootprintFrontal);

    _disableRecalc = false;
}

void CameraCalc::save(QJsonObject& json) const
{
    json[_jsonCameraSpecTypeKey] =          (int)_cameraSpecType;
    json[_adjustedFootprintSideName] =      _adjustedFootprintSideFact.rawValue().toDouble();
    json[_adjustedFootprintFrontalName] =   _adjustedFootprintFrontalFact.rawValue().toDouble();
    json[_distanceToSurfaceName] =  _distanceToSurfaceFact.rawValue().toDouble();

    if (_cameraSpecType != CameraSpecNone) {
        CameraSpec::save(json);
        json[_jsonKnownCameraNameKey] = _knownCameraName;
        json[_valueSetIsDistanceName] = _valueSetIsDistanceFact.rawValue().toBool();
        json[_imageDensityName] =       _imageDensityFact.rawValue().toDouble();
        json[_frontalOverlapName] =     _frontalOverlapFact.rawValue().toDouble();
        json[_sideOverlapName] =        _sideOverlapFact.rawValue().toDouble();
    }
}

bool CameraCalc::load(const QJsonObject& json, QString& errorString)
{    
    QList<JsonHelper::KeyValidateInfo> keyInfoList1 = {
        { _jsonCameraSpecTypeKey,           QJsonValue::Double, true },
        { _adjustedFootprintSideName,       QJsonValue::Double, true },
        { _adjustedFootprintFrontalName,    QJsonValue::Double, true },
        { _distanceToSurfaceName,           QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(json, keyInfoList1, errorString)) {
        return false;
    }

    int cameraSpecType = json[_jsonCameraSpecTypeKey].toInt();
    switch (cameraSpecType) {
    case CameraSpecNone:
    case CameraSpecCustom:
    case CameraSpecKnown:
        break;
    default:
        errorString = tr("Unsupported CameraSpecType %d").arg(cameraSpecType);
        return false;
    }

    _disableRecalc = true;

    setCameraSpecType((CameraSpecType)cameraSpecType);
    _adjustedFootprintSideFact.setRawValue      (json[_adjustedFootprintSideName].toDouble());
    _adjustedFootprintFrontalFact.setRawValue   (json[_adjustedFootprintFrontalName].toDouble());
    _distanceToSurfaceFact.setRawValue          (json[_distanceToSurfaceName].toDouble());

    if (_cameraSpecType != CameraSpecNone) {
        QList<JsonHelper::KeyValidateInfo> keyInfoList2 = {
            { _jsonKnownCameraNameKey,          QJsonValue::String, true },
            { _valueSetIsDistanceName,          QJsonValue::Bool,   true },
            { _imageDensityName,                QJsonValue::Double, true },
            { _frontalOverlapName,              QJsonValue::Double, true },
            { _sideOverlapName,                 QJsonValue::Double, true },
        };
        if (!JsonHelper::validateKeys(json, keyInfoList2, errorString)) {
            return false;
            _disableRecalc = false;
        }

        setKnownCameraName(json[_jsonKnownCameraNameKey].toString());
        _valueSetIsDistanceFact.setRawValue (json[_valueSetIsDistanceName].toBool());
        _imageDensityFact.setRawValue       (json[_imageDensityName].toDouble());
        _frontalOverlapFact.setRawValue     (json[_frontalOverlapName].toDouble());
        _sideOverlapFact.setRawValue        (json[_sideOverlapName].toDouble());

        if (!CameraSpec::load(json, errorString)) {
            return false;
        }
    }

    _disableRecalc = false;

    return true;
}
