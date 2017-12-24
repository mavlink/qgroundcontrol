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
const char* CameraCalc::_jsonCameraNameKey =            "CameraName";
const char* CameraCalc::_jsonCameraSpecTypeKey =        "CameraSpecType";

CameraCalc::CameraCalc(Vehicle* vehicle, QObject* parent)
    : CameraSpec                    (parent)
    , _vehicle                      (vehicle)
    , _dirty                        (false)
    , _cameraName                   (manualCameraName())
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

    connect(this, &CameraCalc::cameraNameChanged, this, &CameraCalc::_recalcTriggerDistance);

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

void CameraCalc::setCameraName(QString cameraName)
{
    if (cameraName != _cameraName) {
        _cameraName = cameraName;

        if (_cameraName == manualCameraName() || _cameraName == customCameraName()) {
            // These values are unknown for these types
            fixedOrientation()->setRawValue(false);
            minTriggerInterval()->setRawValue(0);
            if (_cameraName == manualCameraName()) {
                valueSetIsDistance()->setRawValue(false);
            }
        } else {
            // This should be a known camera name. Update camera specs to new camera

            bool foundKnownCamera = false;
            CameraMetaData* cameraMetaData = NULL;
            for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
                cameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
                if (cameraName == cameraMetaData->name()) {
                    foundKnownCamera = true;
                    break;
                }
            }

            _disableRecalc = true;
            if (foundKnownCamera) {
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
                _cameraName = customCameraName();
                fixedOrientation()->setRawValue(false);
                minTriggerInterval()->setRawValue(0);
            }
            _disableRecalc = false;
        }

        emit cameraNameChanged(_cameraName);
    }
}

void CameraCalc::_recalcTriggerDistance(void)
{
    if (_disableRecalc || _cameraName == manualCameraName()) {
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
    json[JsonHelper::jsonVersionKey] =      1;
    json[_adjustedFootprintSideName] =      _adjustedFootprintSideFact.rawValue().toDouble();
    json[_adjustedFootprintFrontalName] =   _adjustedFootprintFrontalFact.rawValue().toDouble();
    json[_distanceToSurfaceName] =  _distanceToSurfaceFact.rawValue().toDouble();
    json[_jsonCameraNameKey] = _cameraName;

    if (_cameraName != manualCameraName()) {
        CameraSpec::save(json);
        json[_valueSetIsDistanceName] = _valueSetIsDistanceFact.rawValue().toBool();
        json[_imageDensityName] =       _imageDensityFact.rawValue().toDouble();
        json[_frontalOverlapName] =     _frontalOverlapFact.rawValue().toDouble();
        json[_sideOverlapName] =        _sideOverlapFact.rawValue().toDouble();
    }
}

bool CameraCalc::load(const QJsonObject& json, QString& errorString)
{
    QJsonObject v1Json = json;

    if (!v1Json.contains(JsonHelper::jsonVersionKey)) {
        // Version 0 file. Differences from Version 1 for conversion:
        //  JsonHelper::jsonVersionKey not stored
        //  _jsonCameraSpecTypeKey stores CameraSpecType
        //  _jsonCameraNameKey only set if CameraSpecKnown
        int cameraSpec = v1Json[_jsonCameraSpecTypeKey].toInt(CameraSpecNone);
        if (cameraSpec == CameraSpecCustom) {
            v1Json[_jsonCameraNameKey] = customCameraName();
        } else if (cameraSpec == CameraSpecNone) {
            v1Json[_jsonCameraNameKey] = manualCameraName();
        }
        v1Json.remove(_jsonCameraSpecTypeKey);
        v1Json[JsonHelper::jsonVersionKey] = 1;
    }

    int version = v1Json[JsonHelper::jsonVersionKey].toInt();
    if (version != 1) {
        errorString = tr("CameraCalc section version %1 not supported").arg(version);
        return false;
    }

    QList<JsonHelper::KeyValidateInfo> keyInfoList1 = {
        { _jsonCameraNameKey,               QJsonValue::String, true },
        { _adjustedFootprintSideName,       QJsonValue::Double, true },
        { _adjustedFootprintFrontalName,    QJsonValue::Double, true },
        { _distanceToSurfaceName,           QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(v1Json, keyInfoList1, errorString)) {
        return false;
    }

    _disableRecalc = true;

    setCameraName(v1Json[_jsonCameraNameKey].toString());
    _adjustedFootprintSideFact.setRawValue      (v1Json[_adjustedFootprintSideName].toDouble());
    _adjustedFootprintFrontalFact.setRawValue   (v1Json[_adjustedFootprintFrontalName].toDouble());
    _distanceToSurfaceFact.setRawValue          (v1Json[_distanceToSurfaceName].toDouble());

    if (_cameraName != manualCameraName()) {
        QList<JsonHelper::KeyValidateInfo> keyInfoList2 = {
            { _valueSetIsDistanceName,          QJsonValue::Bool,   true },
            { _imageDensityName,                QJsonValue::Double, true },
            { _frontalOverlapName,              QJsonValue::Double, true },
            { _sideOverlapName,                 QJsonValue::Double, true },
        };
        if (!JsonHelper::validateKeys(v1Json, keyInfoList2, errorString)) {
            return false;
            _disableRecalc = false;
        }

        _valueSetIsDistanceFact.setRawValue (v1Json[_valueSetIsDistanceName].toBool());
        _imageDensityFact.setRawValue       (v1Json[_imageDensityName].toDouble());
        _frontalOverlapFact.setRawValue     (v1Json[_frontalOverlapName].toDouble());
        _sideOverlapFact.setRawValue        (v1Json[_sideOverlapName].toDouble());

        if (!CameraSpec::load(v1Json, errorString)) {
            return false;
        }
    }

    _disableRecalc = false;

    return true;
}

QString CameraCalc::customCameraName(void)
{
    return tr("Custom Camera");
}

QString CameraCalc::manualCameraName(void)
{
    return tr("Manual (no camera specs)");
}
