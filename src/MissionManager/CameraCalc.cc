/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraCalc.h"
#include "JsonHelper.h"
#include "Vehicle.h"
#include "CameraMetaData.h"
#include "PlanMasterController.h"
#include <cmath>
#include <algorithm>

#include <QQmlEngine>

const char* CameraCalc::cameraNameName =                    "CameraName";
const char* CameraCalc::valueSetIsDistanceName =            "ValueSetIsDistance";
const char* CameraCalc::distanceToSurfaceName =             "DistanceToSurface";
const char* CameraCalc::distanceToSurfaceRelativeName =     "DistanceToSurfaceRelative";
const char* CameraCalc::imageDensityName =                  "ImageDensity";
const char* CameraCalc::frontalOverlapName =                "FrontalOverlap";
const char* CameraCalc::sideOverlapName =                   "SideOverlap";
const char* CameraCalc::adjustedFootprintFrontalName =      "AdjustedFootprintFrontal";
const char* CameraCalc::adjustedFootprintSideName =         "AdjustedFootprintSide";

const char* CameraCalc::camposName =                 "Campos";
const char* CameraCalc::camposSurvey2DName =         "CamposSurvey2D";
const char* CameraCalc::camposPositionsName =        "CamposPositions";
const char* CameraCalc::camposMinIntervalName =      "CamposMinInterval";
const char* CameraCalc::camposRollAngleName =        "CamposRollAngle";
const char* CameraCalc::camposPitchAngleName =       "CamposPitchAngle";

const char* CameraCalc::_jsonCameraSpecTypeKey =            "CameraSpecType";

const char* CameraCalc::_jsonCamposKey =                         "campos";
const char* CameraCalc::_jsonCamposSurvey2DKey =                 "camposSurvey2D";
const char* CameraCalc::_jsonCamposPositionsKey =                "camposPositions";
const char* CameraCalc::_jsonCamposMinIntervalKey =              "camposMinInterval";
const char* CameraCalc::_jsonCamposRollAngleKey =                "camposRollAngle";
const char* CameraCalc::_jsonCamposPitchAngleKey =               "camposPitchAngle";

CameraCalc::CameraCalc(PlanMasterController* masterController, const QString& settingsGroup, QObject* parent)
    : CameraSpec                    (settingsGroup, parent)
    , _knownCameraList              (masterController->controllerVehicle()->staticCameraList())
    , _metaDataMap                  (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CameraCalc.FactMetaData.json"), this))
    , _cameraNameFact               (settingsGroup, _metaDataMap[cameraNameName])
    , _valueSetIsDistanceFact       (settingsGroup, _metaDataMap[valueSetIsDistanceName])
    , _distanceToSurfaceFact        (settingsGroup, _metaDataMap[distanceToSurfaceName])
    , _imageDensityFact             (settingsGroup, _metaDataMap[imageDensityName])
    , _frontalOverlapFact           (settingsGroup, _metaDataMap[frontalOverlapName])
    , _sideOverlapFact              (settingsGroup, _metaDataMap[sideOverlapName])
    , _adjustedFootprintSideFact    (settingsGroup, _metaDataMap[adjustedFootprintSideName])
    , _adjustedFootprintFrontalFact (settingsGroup, _metaDataMap[adjustedFootprintFrontalName])

    , _camposFact                   (settingsGroup, _metaDataMap[camposName])
    , _camposSurvey2DFact           (settingsGroup, _metaDataMap[camposSurvey2DName])
    , _camposPositionsFact          (settingsGroup, _metaDataMap[camposPositionsName])
    , _camposMinIntervalFact        (settingsGroup, _metaDataMap[camposMinIntervalName])
    , _camposRollAngleFact          (settingsGroup, _metaDataMap[camposRollAngleName])
    , _camposPitchAngleFact         (settingsGroup, _metaDataMap[camposPitchAngleName])
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    connect(&_valueSetIsDistanceFact,       &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_distanceToSurfaceFact,        &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_imageDensityFact,             &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_frontalOverlapFact,           &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_sideOverlapFact,              &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_adjustedFootprintSideFact,    &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_adjustedFootprintFrontalFact, &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_cameraNameFact,               &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(this,                           &CameraCalc::distanceToSurfaceRelativeChanged,  this, &CameraCalc::_setDirty);

    connect(&_camposFact,                   &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_camposSurvey2DFact,           &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_camposPositionsFact,          &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_camposMinIntervalFact,        &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_camposRollAngleFact,          &Fact::valueChanged,                            this, &CameraCalc::_setDirty);
    connect(&_camposPitchAngleFact,         &Fact::valueChanged,                            this, &CameraCalc::_setDirty);

    connect(&_cameraNameFact,               &Fact::valueChanged,                            this, &CameraCalc::_cameraNameChanged);
    connect(&_cameraNameFact,               &Fact::valueChanged,                            this, &CameraCalc::isManualCameraChanged);
    connect(&_cameraNameFact,               &Fact::valueChanged,                            this, &CameraCalc::isCustomCameraChanged);

    connect(&_distanceToSurfaceFact,    &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_imageDensityFact,         &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_frontalOverlapFact,       &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_sideOverlapFact,          &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(sensorWidth(),              &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(sensorHeight(),             &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(imageWidth(),               &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(imageHeight(),              &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(focalLength(),              &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(landscape(),                &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);

    connect(&_camposFact,               &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_camposSurvey2DFact,       &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_camposPositionsFact,      &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_camposMinIntervalFact,    &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_camposRollAngleFact,      &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);
    connect(&_camposPitchAngleFact,     &Fact::rawValueChanged, this, &CameraCalc::_recalcTriggerDistance);

    // Build the brand list from known cameras
    _cameraBrandList.append(xlatManualCameraName());
    _cameraBrandList.append(xlatCustomCameraName());
    for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
        CameraMetaData* cameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
        if (!_cameraBrandList.contains(cameraMetaData->brand)) {
            _cameraBrandList.append(cameraMetaData->brand);
        }
    }

    _cameraNameChanged();
    _setBrandModelFromCanonicalName(_cameraNameFact.rawValue().toString());

    setDirty(false);
}

void CameraCalc::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void CameraCalc::_cameraNameChanged(void)
{
    if (_disableRecalc) {
        return;
    }

    QString cameraName = _cameraNameFact.rawValue().toString();

    if (isManualCamera() || isCustomCamera()) {
        fixedOrientation()->setRawValue(false);
        minTriggerInterval()->setRawValue(0);
        if (isManualCamera() && !valueSetIsDistance()->rawValue().toBool()) {
            valueSetIsDistance()->setRawValue(true);
        }
    } else {
        // Look for known camera
        CameraMetaData* knownCameraMetaData = nullptr;
        for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
            CameraMetaData* cameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
            if (cameraName == cameraMetaData->canonicalName) {
                knownCameraMetaData = cameraMetaData;
                break;
            }
        }

        if (!knownCameraMetaData) {
            // Lookup failed. Force to custom as fallback.
            // This will cause another camera changed signal which will recurse back into this routine
            _cameraNameFact.setRawValue(canonicalCustomCameraName());
            return;
        }

        _disableRecalc = true;

        sensorWidth()->setRawValue          (knownCameraMetaData->sensorWidth);
        sensorHeight()->setRawValue         (knownCameraMetaData->sensorHeight);
        imageWidth()->setRawValue           (knownCameraMetaData->imageWidth);
        imageHeight()->setRawValue          (knownCameraMetaData->imageHeight);
        focalLength()->setRawValue          (knownCameraMetaData->focalLength);
        landscape()->setRawValue            (knownCameraMetaData->landscape);
        fixedOrientation()->setRawValue     (knownCameraMetaData->fixedOrientation);
        minTriggerInterval()->setRawValue   (knownCameraMetaData->minTriggerInterval);

        _disableRecalc = false;
    }

    _recalcTriggerDistance();
    _adjustDistanceToSurfaceRelative();
}

void CameraCalc::_recalcTriggerDistance(void)
{
    if (_disableRecalc || isManualCamera()) {
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
    } else if (!_camposFact.rawValue().toBool() || !_camposSurvey2DFact.rawValue().toBool()){
        _distanceToSurfaceFact.setRawValue((imageWidth * _imageDensityFact.rawValue().toDouble() * focalLength) / (sensorWidth * 100.0));
    } else {
        //this guarantees that the pixels in the used area of the image (the complement of sidelap) are <= GSD
        //height will vary as it tries to keep only the used area below the set GSD
        double halfAOV = std::min(atan(sensorWidth / (2.0 * focalLength)) + _camposRollAngleFact.rawValue().toDouble() * M_PI / 180.0, 75.0 * M_PI / 180.0);
        double GSD = _imageDensityFact.rawValue().toDouble()/100.0;
        double sidelap = _sideOverlapFact.rawValue().toDouble()/100.0;
        double pixelAngle = 2.0 * atan(sensorWidth / (2.0 * focalLength)) / imageWidth;
        double height = GSD / (tan(atan((1-sidelap)*tan(halfAOV))+pixelAngle)-((1-sidelap)*tan(halfAOV)));
        _distanceToSurfaceFact.setRawValue(height);
    }

    imageDensity = _imageDensityFact.rawValue().toDouble();

    if (landscape()->rawValue().toBool()) {
        _imageFootprintSide =       (imageWidth  * imageDensity) / 100.0;
        _imageFootprintFrontal =    (imageHeight * imageDensity) / 100.0;
    } else {
        _imageFootprintSide  =      (imageHeight * imageDensity) / 100.0;
        _imageFootprintFrontal =    (imageWidth  * imageDensity) / 100.0;
    }

    if (_camposFact.rawValue().toBool() && _camposSurvey2DFact.rawValue().toBool()) {
        //find half the angle of view, accounting for the additional roll angles, upper bounded for a 150° AoV camera, as with greater values the GSD at the extremes decays too rapidly.
        double halfAOV = std::min(atan(sensorWidth / (2.0 * focalLength)) + _camposRollAngleFact.rawValue().toDouble() * M_PI / 180.0, 75.0 * M_PI / 180.0);

        //compute the adjusted horizontal field of view with the addition of the roll angle set
        double camposHFOV = 2 * tan(halfAOV) * _distanceToSurfaceFact.rawValue().toDouble();

        _imageFootprintSide = camposHFOV;

        double halfVerticalAOV = atan(sensorHeight / (2.0 * focalLength));

        //compute the frontal footprint from height and angle and divide by the number of positions.
        _imageFootprintFrontal = 2 * tan(halfVerticalAOV) * _distanceToSurfaceFact.rawValue().toDouble() / _camposPositionsFact.rawValue().toDouble();
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
    json[adjustedFootprintSideName] =       _adjustedFootprintSideFact.rawValue().toDouble();
    json[adjustedFootprintFrontalName] =    _adjustedFootprintFrontalFact.rawValue().toDouble();
    json[distanceToSurfaceName] =           _distanceToSurfaceFact.rawValue().toDouble();
    json[distanceToSurfaceRelativeName] =   _distanceToSurfaceRelative;
    json[cameraNameName] =                  _cameraNameFact.rawValue().toString();

    json[_jsonCamposKey] =                  _camposFact.rawValue().toBool();
    json[_jsonCamposSurvey2DKey] =          _camposSurvey2DFact.rawValue().toBool();
    json[_jsonCamposPositionsKey] =         _camposPositionsFact.rawValue().toInt();
    json[_jsonCamposMinIntervalKey] =       _camposMinIntervalFact.rawValue().toInt();
    json[_jsonCamposRollAngleKey] =         _camposRollAngleFact.rawValue().toDouble();
    json[_jsonCamposPitchAngleKey] =        _camposPitchAngleFact.rawValue().toDouble();

    if (!isManualCamera()) {
        CameraSpec::save(json);
        json[valueSetIsDistanceName] = _valueSetIsDistanceFact.rawValue().toBool();
        json[imageDensityName] =       _imageDensityFact.rawValue().toDouble();
        json[frontalOverlapName] =     _frontalOverlapFact.rawValue().toDouble();
        json[sideOverlapName] =        _sideOverlapFact.rawValue().toDouble();
    }
}

bool CameraCalc::load(const QJsonObject& json, QString& errorString)
{
    QJsonObject v1Json = json;

    if (!json.contains(JsonHelper::jsonVersionKey)) {
        // Version 0 file. Differences from Version 1 for conversion:
        //  JsonHelper::jsonVersionKey not stored
        //  _jsonCameraSpecTypeKey stores CameraSpecType
        //  _jsonCameraNameKey only set if CameraSpecKnown
        int cameraSpec = v1Json[_jsonCameraSpecTypeKey].toInt(CameraSpecNone);
        if (cameraSpec == CameraSpecCustom) {
            v1Json[cameraNameName] = canonicalCustomCameraName();
        } else if (cameraSpec == CameraSpecNone) {
            v1Json[cameraNameName] = canonicalManualCameraName();
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
        { cameraNameName,                   QJsonValue::String, true },
        { adjustedFootprintSideName,        QJsonValue::Double, true },
        { adjustedFootprintFrontalName,     QJsonValue::Double, true },
        { distanceToSurfaceName,            QJsonValue::Double, true },
        { distanceToSurfaceRelativeName,    QJsonValue::Bool,   true },
        
        { _jsonCamposKey,                   QJsonValue::Bool,   false },
        { _jsonCamposSurvey2DKey,           QJsonValue::Bool,   false },
        { _jsonCamposPositionsKey,          QJsonValue::Double, false },
        { _jsonCamposMinIntervalKey,        QJsonValue::Double, false },
        { _jsonCamposRollAngleKey,          QJsonValue::Double, false },
        { _jsonCamposPitchAngleKey,         QJsonValue::Double, false },
    };
    if (!JsonHelper::validateKeys(v1Json, keyInfoList1, errorString)) {
        return false;
    }

    _disableRecalc = true;

    _distanceToSurfaceRelative = v1Json[distanceToSurfaceRelativeName].toBool();

    _adjustedFootprintSideFact.setRawValue      (v1Json[adjustedFootprintSideName].toDouble());
    _adjustedFootprintFrontalFact.setRawValue   (v1Json[adjustedFootprintFrontalName].toDouble());
    _distanceToSurfaceFact.setRawValue          (v1Json[distanceToSurfaceName].toDouble());

    // We have to clean up camera names. Older builds incorrectly used translated the camera names in the persisted plan file.
    // Newer builds use a canonical english camera name in plan files.
    QString canonicalCameraName = _validCanonicalCameraName(v1Json[cameraNameName].toString());
    _cameraNameFact.setRawValue(canonicalCameraName);

    if (!isManualCamera()) {
        QList<JsonHelper::KeyValidateInfo> keyInfoList2 = {
            { valueSetIsDistanceName,   QJsonValue::Bool,   true },
            { imageDensityName,         QJsonValue::Double, true },
            { frontalOverlapName,       QJsonValue::Double, true },
            { sideOverlapName,          QJsonValue::Double, true },
        };
        if (!JsonHelper::validateKeys(v1Json, keyInfoList2, errorString)) {
            _disableRecalc = false;
            return false;
        }

        _valueSetIsDistanceFact.setRawValue (v1Json[valueSetIsDistanceName].toBool());
        _frontalOverlapFact.setRawValue     (v1Json[frontalOverlapName].toDouble());
        _sideOverlapFact.setRawValue        (v1Json[sideOverlapName].toDouble());
        _imageDensityFact.setRawValue       (v1Json[imageDensityName].toDouble());

        if (!CameraSpec::load(v1Json, errorString)) {
            _disableRecalc = false;
            return false;
        }
    }

    _disableRecalc = false;

    _setBrandModelFromCanonicalName(canonicalCameraName);

    return true;
}

QString CameraCalc::canonicalCustomCameraName(void)
{
    // This string should NOT be translated
    return "Custom Camera";
}

QString CameraCalc::canonicalManualCameraName(void)
{
    // This string should NOT be translated
    return "Manual (no camera specs)";
}

QString CameraCalc::xlatCustomCameraName(void)
{
    return tr("Custom Camera");
}

QString CameraCalc::xlatManualCameraName(void)
{
    return tr("Manual (no camera specs)");
}

void CameraCalc::_adjustDistanceToSurfaceRelative(void)
{
    if (!isManualCamera()) {
        setDistanceToSurfaceRelative(true);
    }
}

void CameraCalc::setDistanceToSurfaceRelative(bool distanceToSurfaceRelative)
{
    if (distanceToSurfaceRelative != _distanceToSurfaceRelative) {
        _distanceToSurfaceRelative = distanceToSurfaceRelative;
        emit distanceToSurfaceRelativeChanged(distanceToSurfaceRelative);
    }
}

void CameraCalc::_setDirty(void)
{
    setDirty(true);
}

void CameraCalc::setCameraBrand(const QString& cameraBrand)
{
    // Note that cameraBrand can also be manual or custom camera

    if (cameraBrand != _cameraBrand) {
        QString newCameraName = cameraBrand;

        _cameraBrand = cameraBrand;
        _cameraModel.clear();

        if (_cameraBrand != xlatManualCameraName() && _cameraBrand != xlatCustomCameraName()) {
            CameraMetaData* firstCameraMetaData = nullptr;
            for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
                firstCameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
                if (firstCameraMetaData->brand == _cameraBrand) {
                    break;
                }
            }
            newCameraName = firstCameraMetaData->canonicalName;
            _cameraModel = firstCameraMetaData->model;
        }
        emit cameraBrandChanged();
        emit cameraModelChanged();

        _rebuildCameraModelList();

        _cameraNameFact.setRawValue(newCameraName);
    }
}

void CameraCalc::setCameraModel(const QString& cameraModel)
{
    if (cameraModel != _cameraModel) {
        _cameraModel = cameraModel;
        emit cameraModelChanged();

        for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
            CameraMetaData* cameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
            if (cameraMetaData->brand == _cameraBrand && cameraMetaData->model == _cameraModel) {
                _cameraNameFact.setRawValue(cameraMetaData->canonicalName);
                break;
            }
        }

    }
}

void CameraCalc::_setBrandModelFromCanonicalName(const QString& cameraName)
{
    _cameraBrand = cameraName;
    _cameraModel.clear();
    _cameraModelList.clear();

    if (cameraName != canonicalManualCameraName() && cameraName != canonicalCustomCameraName()) {
        for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
            CameraMetaData* cameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
            if (cameraMetaData->canonicalName == cameraName) {
                _cameraBrand = cameraMetaData->brand;
                _cameraModel = cameraMetaData->model;
                break;
            }
        }
    }
    emit cameraBrandChanged();
    emit cameraModelChanged();

    _rebuildCameraModelList();
}

void CameraCalc::_rebuildCameraModelList(void)
{
    _cameraModelList.clear();

    for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
        CameraMetaData* cameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
        if (cameraMetaData->brand == _cameraBrand) {
            _cameraModelList.append(cameraMetaData->model);
        }
    }

    emit cameraModelListChanged();
}

void CameraCalc::_setCameraNameFromV3TransectLoad(const QString& cameraName)
{
    // We don't recalc here since the rest of the camera values are already loaded from the json
    _disableRecalc = true;
    QString canonicalCameraName = _validCanonicalCameraName(cameraName);
    _cameraNameFact.setRawValue(cameraName);
    _disableRecalc = true;

    _setBrandModelFromCanonicalName(canonicalCameraName);
}

QString CameraCalc::_validCanonicalCameraName(const QString& cameraName)
{
    QString canonicalCameraName = cameraName;

    if (canonicalCameraName != canonicalCustomCameraName() && canonicalCameraName != canonicalManualCameraName()) {
        if (cameraName == xlatManualCameraName()) {
            canonicalCameraName = canonicalManualCameraName();
        } else if (cameraName == xlatCustomCameraName()) {
            canonicalCameraName = canonicalCustomCameraName();
        } else {
            // Look for known camera
            for (int cameraIndex=0; cameraIndex<_knownCameraList.count(); cameraIndex++) {
                CameraMetaData* cameraMetaData = _knownCameraList[cameraIndex].value<CameraMetaData*>();
                if (cameraName == cameraMetaData->canonicalName || cameraName == cameraMetaData->deprecatedTranslatedName) {
                    return cameraMetaData->canonicalName;
                }
            }

            canonicalCameraName = canonicalCustomCameraName();
        }
    }

    return canonicalCameraName;
}
