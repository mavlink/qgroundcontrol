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
    _adjustedFootprintSideFact.setMetaData      (_metaDataMap[_adjustedFootprintSideName],      false);
    _adjustedFootprintFrontalFact.setMetaData   (_metaDataMap[_adjustedFootprintFrontalName],   false);

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
    Q_UNUSED(json);

#if 0
    QJsonObject saveObject;

    saveObject[JsonHelper::jsonVersionKey] =                    3;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;
    saveObject[_jsonManualGridKey] =                            _manualGridFact.rawValue().toBool();
    saveObject[_jsonFixedValueIsAltitudeKey] =                  _fixedValueIsAltitudeFact.rawValue().toBool();
    saveObject[_jsonHoverAndCaptureKey] =                       _hoverAndCaptureFact.rawValue().toBool();
    saveObject[_jsonRefly90DegreesKey] =                        _refly90Degrees;
    saveObject[_jsonCameraTriggerDistanceKey] =                 _cameraTriggerDistanceFact.rawValue().toDouble();
    saveObject[_jsonCameraTriggerInTurnaroundKey] =             _cameraTriggerInTurnaroundFact.rawValue().toBool();

    QJsonObject gridObject;
    gridObject[_jsonGridAltitudeKey] =          _gridAltitudeFact.rawValue().toDouble();
    gridObject[_jsonGridAltitudeRelativeKey] =  _gridAltitudeRelativeFact.rawValue().toBool();
    gridObject[_jsonGridAngleKey] =             _gridAngleFact.rawValue().toDouble();
    gridObject[_jsonGridSpacingKey] =           _gridSpacingFact.rawValue().toDouble();
    gridObject[_jsonGridEntryLocationKey] =     _gridEntryLocationFact.rawValue().toDouble();
    gridObject[_jsonTurnaroundDistKey] =        _turnaroundDistFact.rawValue().toDouble();

    saveObject[_jsonGridObjectKey] = gridObject;

    if (!_manualGridFact.rawValue().toBool()) {
        QJsonObject cameraObject;
        cameraObject[_jsonCameraNameKey] =                  _cameraFact.rawValue().toString();
        cameraObject[_jsonCameraOrientationLandscapeKey] =  _cameraOrientationLandscapeFact.rawValue().toBool();
        cameraObject[_jsonCameraSensorWidthKey] =           _cameraSensorWidthFact.rawValue().toDouble();
        cameraObject[_jsonCameraSensorHeightKey] =          _cameraSensorHeightFact.rawValue().toDouble();
        cameraObject[_jsonCameraResolutionWidthKey] =       _cameraResolutionWidthFact.rawValue().toDouble();
        cameraObject[_jsonCameraResolutionHeightKey] =      _cameraResolutionHeightFact.rawValue().toDouble();
        cameraObject[_jsonCameraFocalLengthKey] =           _cameraFocalLengthFact.rawValue().toDouble();
        cameraObject[_jsonCameraMinTriggerIntervalKey] =    _cameraMinTriggerInterval;
        cameraObject[_jsonGroundResolutionKey] =            _groundResolutionFact.rawValue().toDouble();
        cameraObject[_jsonFrontalOverlapKey] =              _frontalOverlapFact.rawValue().toInt();
        cameraObject[_jsonSideOverlapKey] =                 _sideOverlapFact.rawValue().toInt();

        saveObject[_jsonCameraObjectKey] = cameraObject;
    }

    // Polygon shape
    _mapPolygon.saveToJson(saveObject);

    missionItems.append(saveObject);
#endif
}

bool CameraCalc::load(const QJsonObject& complexObject, QString& errorString)
{
    Q_UNUSED(complexObject);
    Q_UNUSED(errorString);
#if 0
    QJsonObject v2Object = complexObject;

    // We need to pull version first to determine what validation/conversion needs to be performed.
    QList<JsonHelper::KeyValidateInfo> versionKeyInfoList = {
        { JsonHelper::jsonVersionKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(v2Object, versionKeyInfoList, errorString)) {
        return false;
    }

    int version = v2Object[JsonHelper::jsonVersionKey].toInt();
    if (version != 2 && version != 3) {
        errorString = tr("%1 does not support this version of survey items").arg(qgcApp()->applicationName());
        return false;
    }
    if (version == 2) {
        // Convert to v3
        if (v2Object.contains(VisualMissionItem::jsonTypeKey) && v2Object[VisualMissionItem::jsonTypeKey].toString() == QStringLiteral("survey")) {
            v2Object[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeComplexItemValue;
            v2Object[ComplexMissionItem::jsonComplexItemTypeKey] = jsonComplexItemTypeValue;
        }
    }

    QList<JsonHelper::KeyValidateInfo> mainKeyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { QGCMapPolygon::jsonPolygonKey,                QJsonValue::Array,  true },
        { _jsonGridObjectKey,                           QJsonValue::Object, true },
        { _jsonCameraObjectKey,                         QJsonValue::Object, false },
        { _jsonCameraTriggerDistanceKey,                QJsonValue::Double, true },
        { _jsonManualGridKey,                           QJsonValue::Bool,   true },
        { _jsonFixedValueIsAltitudeKey,                 QJsonValue::Bool,   true },
        { _jsonHoverAndCaptureKey,                      QJsonValue::Bool,   false },
        { _jsonRefly90DegreesKey,                       QJsonValue::Bool,   false },
        { _jsonCameraTriggerInTurnaroundKey,            QJsonValue::Bool,   false },    // Should really be required, but it was missing from initial code due to bug
    };
    if (!JsonHelper::validateKeys(v2Object, mainKeyInfoList, errorString)) {
        return false;
    }

    QString itemType = v2Object[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = v2Object[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    _ignoreRecalc = true;

    _mapPolygon.clear();

    setSequenceNumber(sequenceNumber);

    _manualGridFact.setRawValue                 (v2Object[_jsonManualGridKey].toBool(true));
    _fixedValueIsAltitudeFact.setRawValue       (v2Object[_jsonFixedValueIsAltitudeKey].toBool(true));
    _gridAltitudeRelativeFact.setRawValue       (v2Object[_jsonGridAltitudeRelativeKey].toBool(true));
    _hoverAndCaptureFact.setRawValue            (v2Object[_jsonHoverAndCaptureKey].toBool(false));
    _cameraTriggerInTurnaroundFact.setRawValue  (v2Object[_jsonCameraTriggerInTurnaroundKey].toBool(true));

    _refly90Degrees = v2Object[_jsonRefly90DegreesKey].toBool(false);

    QList<JsonHelper::KeyValidateInfo> gridKeyInfoList = {
        { _jsonGridAltitudeKey,                 QJsonValue::Double, true },
        { _jsonGridAltitudeRelativeKey,         QJsonValue::Bool,   true },
        { _jsonGridAngleKey,                    QJsonValue::Double, true },
        { _jsonGridSpacingKey,                  QJsonValue::Double, true },
        { _jsonGridEntryLocationKey,            QJsonValue::Double, false },
        { _jsonTurnaroundDistKey,               QJsonValue::Double, true },
    };
    QJsonObject gridObject = v2Object[_jsonGridObjectKey].toObject();
    if (!JsonHelper::validateKeys(gridObject, gridKeyInfoList, errorString)) {
        return false;
    }
    _gridAltitudeFact.setRawValue           (gridObject[_jsonGridAltitudeKey].toDouble());
    _gridAngleFact.setRawValue              (gridObject[_jsonGridAngleKey].toDouble());
    _gridSpacingFact.setRawValue            (gridObject[_jsonGridSpacingKey].toDouble());
    _turnaroundDistFact.setRawValue         (gridObject[_jsonTurnaroundDistKey].toDouble());
    _cameraTriggerDistanceFact.setRawValue  (v2Object[_jsonCameraTriggerDistanceKey].toDouble());
    if (gridObject.contains(_jsonGridEntryLocationKey)) {
        _gridEntryLocationFact.setRawValue(gridObject[_jsonGridEntryLocationKey].toDouble());
    } else {
        _gridEntryLocationFact.setRawValue(_gridEntryLocationFact.rawDefaultValue());
    }

    if (!_manualGridFact.rawValue().toBool()) {
        if (!v2Object.contains(_jsonCameraObjectKey)) {
            errorString = tr("%1 but %2 object is missing").arg("manualGrid = false").arg("camera");
            return false;
        }

        QJsonObject cameraObject = v2Object[_jsonCameraObjectKey].toObject();

        // Older code had typo on "imageSideOverlap" incorrectly being "imageSizeOverlap"
        QString incorrectImageSideOverlap = "imageSizeOverlap";
        if (cameraObject.contains(incorrectImageSideOverlap)) {
            cameraObject[_jsonSideOverlapKey] = cameraObject[incorrectImageSideOverlap];
            cameraObject.remove(incorrectImageSideOverlap);
        }

        QList<JsonHelper::KeyValidateInfo> cameraKeyInfoList = {
            { _jsonGroundResolutionKey,             QJsonValue::Double, true },
            { _jsonFrontalOverlapKey,               QJsonValue::Double, true },
            { _jsonSideOverlapKey,                  QJsonValue::Double, true },
            { _jsonCameraSensorWidthKey,            QJsonValue::Double, true },
            { _jsonCameraSensorHeightKey,           QJsonValue::Double, true },
            { _jsonCameraResolutionWidthKey,        QJsonValue::Double, true },
            { _jsonCameraResolutionHeightKey,       QJsonValue::Double, true },
            { _jsonCameraFocalLengthKey,            QJsonValue::Double, true },
            { _jsonCameraNameKey,                   QJsonValue::String, true },
            { _jsonCameraOrientationLandscapeKey,   QJsonValue::Bool,   true },
            { _jsonCameraMinTriggerIntervalKey,     QJsonValue::Double, false },
        };
        if (!JsonHelper::validateKeys(cameraObject, cameraKeyInfoList, errorString)) {
            return false;
        }

        _cameraFact.setRawValue(cameraObject[_jsonCameraNameKey].toString());
        _cameraOrientationLandscapeFact.setRawValue(cameraObject[_jsonCameraOrientationLandscapeKey].toBool(true));

        _groundResolutionFact.setRawValue       (cameraObject[_jsonGroundResolutionKey].toDouble());
        _frontalOverlapFact.setRawValue         (cameraObject[_jsonFrontalOverlapKey].toInt());
        _sideOverlapFact.setRawValue            (cameraObject[_jsonSideOverlapKey].toInt());
        _cameraSensorWidthFact.setRawValue      (cameraObject[_jsonCameraSensorWidthKey].toDouble());
        _cameraSensorHeightFact.setRawValue     (cameraObject[_jsonCameraSensorHeightKey].toDouble());
        _cameraResolutionWidthFact.setRawValue  (cameraObject[_jsonCameraResolutionWidthKey].toDouble());
        _cameraResolutionHeightFact.setRawValue (cameraObject[_jsonCameraResolutionHeightKey].toDouble());
        _cameraFocalLengthFact.setRawValue      (cameraObject[_jsonCameraFocalLengthKey].toDouble());
        _cameraMinTriggerInterval =             cameraObject[_jsonCameraMinTriggerIntervalKey].toDouble(0);
    }

    // Polygon shape
    /// Load a polygon from json
    ///     @param json Json object to load from
    ///     @param required true: no polygon in object will generate error
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    if (!_mapPolygon.loadFromJson(v2Object, true /* required */, errorString)) {
        _mapPolygon.clear();
        return false;
    }

    _ignoreRecalc = false;
    _generateGrid();

    return true;
#endif
    return false;
}
