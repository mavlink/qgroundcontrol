/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraSpec.h"
#include "JsonHelper.h"

#include <QQmlEngine>

const char* CameraSpec::_jsonSensorWidthKey =           "sensorWidth";
const char* CameraSpec::_jsonSensorHeightKey =          "sensorHeight";
const char* CameraSpec::_jsonImageWidthKey =            "imageWidth";
const char* CameraSpec::_jsonImageHeightKey =           "imageHeight";
const char* CameraSpec::_jsonFocalLengthKey =           "focalLength";
const char* CameraSpec::_jsonMinTriggerIntervalKey =    "minTriggerInterval";
const char* CameraSpec::_jsonNameKey =                  "name";
const char* CameraSpec::_jsonLandscapeKey =             "orientationLandscape";

const char* CameraSpec::_nameName =                 "Camera";
const char* CameraSpec::_sensorWidthName =          "SensorWidth";
const char* CameraSpec::_sensorHeightName =         "SensorHeight";
const char* CameraSpec::_imageWidthName =           "ImageWidth";
const char* CameraSpec::_imageHeightName =          "ImageHeight";
const char* CameraSpec::_focalLengthName =          "FocalLength";
const char* CameraSpec::_landscapeName =            "Landscape";
const char* CameraSpec::_fixedOrientationName =     "FixedOrientation";
const char* CameraSpec::_minTriggerIntervalName =   "MinTriggerInterval";

CameraSpec::CameraSpec(QObject* parent)
    : QObject                   (parent)
    , _dirty                    (false)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CameraSpec.FactMetaData.json"), this))
    , _nameFact                 (0, _nameName,                  FactMetaData::valueTypeString)
    , _sensorWidthFact          (0, _sensorWidthName,           FactMetaData::valueTypeDouble)
    , _sensorHeightFact         (0, _sensorHeightName,          FactMetaData::valueTypeDouble)
    , _imageWidthFact           (0, _imageWidthName,            FactMetaData::valueTypeUint32)
    , _imageHeightFact          (0, _imageHeightName,           FactMetaData::valueTypeUint32)
    , _focalLengthFact          (0, _focalLengthName,           FactMetaData::valueTypeDouble)
    , _landscapeFact            (0, _landscapeName,             FactMetaData::valueTypeBool)
    , _fixedOrientationFact     (0, _fixedOrientationName,      FactMetaData::valueTypeBool)
    , _minTriggerIntervalFact   (0, _minTriggerIntervalName,    FactMetaData::valueTypeDouble)
{
    _init(true);
}

CameraSpec::CameraSpec(const QString&   name,
           double           sensorWidth,
           double           sensorHeight,
           double           imageWidth,
           double           imageHeight,
           double           focalLength,
           bool             landscape,
           bool             fixedOrientation,
           double           minTriggerInterval,
           QObject*         parent)
    : QObject                   (parent)
    , _dirty                    (false)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CameraSpec.FactMetaData.json"), this))
    , _nameFact                 (0, _nameName,                  FactMetaData::valueTypeString)
    , _sensorWidthFact          (0, _sensorWidthName,           FactMetaData::valueTypeDouble)
    , _sensorHeightFact         (0, _sensorHeightName,          FactMetaData::valueTypeDouble)
    , _imageWidthFact           (0, _imageWidthName,            FactMetaData::valueTypeUint32)
    , _imageHeightFact          (0, _imageHeightName,           FactMetaData::valueTypeUint32)
    , _focalLengthFact          (0, _focalLengthName,           FactMetaData::valueTypeDouble)
    , _landscapeFact            (0, _landscapeName,             FactMetaData::valueTypeBool)
    , _fixedOrientationFact     (0, _fixedOrientationName,      FactMetaData::valueTypeBool)
    , _minTriggerIntervalFact   (0, _minTriggerIntervalName,    FactMetaData::valueTypeDouble)
{
    _init(false);

    _nameFact.setRawValue               (name);
    _sensorWidthFact.setRawValue        (sensorWidth);
    _sensorHeightFact.setRawValue       (sensorHeight);
    _imageWidthFact.setRawValue         (imageWidth);
    _imageHeightFact.setRawValue        (imageHeight);
    _focalLengthFact.setRawValue        (focalLength);
    _landscapeFact.setRawValue          (landscape);
    _fixedOrientationFact.setRawValue   (fixedOrientation);
    _minTriggerIntervalFact.setRawValue (minTriggerInterval);
}

CameraSpec::CameraSpec(const CameraSpec& other, QObject* parent)
    : QObject                   (parent)
    , _dirty                    (false)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CameraSpec.FactMetaData.json"), this))
    , _nameFact                 (0, _nameName,                  FactMetaData::valueTypeString)
    , _sensorWidthFact          (0, _sensorWidthName,           FactMetaData::valueTypeDouble)
    , _sensorHeightFact         (0, _sensorHeightName,          FactMetaData::valueTypeDouble)
    , _imageWidthFact           (0, _imageWidthName,            FactMetaData::valueTypeUint32)
    , _imageHeightFact          (0, _imageHeightName,           FactMetaData::valueTypeUint32)
    , _focalLengthFact          (0, _focalLengthName,           FactMetaData::valueTypeDouble)
    , _landscapeFact            (0, _landscapeName,             FactMetaData::valueTypeBool)
    , _fixedOrientationFact     (0, _fixedOrientationName,      FactMetaData::valueTypeBool)
    , _minTriggerIntervalFact   (0, _minTriggerIntervalName,    FactMetaData::valueTypeDouble)
{
    _init(false);

    *this = other;
}

const CameraSpec& CameraSpec::operator=(const CameraSpec& other)
{
    _nameFact.setRawValue               (other._nameFact.rawValue());
    _sensorWidthFact.setRawValue        (other._sensorWidthFact.rawValue());
    _sensorHeightFact.setRawValue       (other._sensorHeightFact.rawValue());
    _imageWidthFact.setRawValue         (other._imageWidthFact.rawValue());
    _imageHeightFact.setRawValue        (other._imageHeightFact.rawValue());
    _focalLengthFact.setRawValue        (other._focalLengthFact.rawValue());
    _landscapeFact.setRawValue          (other._landscapeFact.rawValue());
    _fixedOrientationFact.setRawValue   (other._fixedOrientationFact.rawValue());
    _minTriggerIntervalFact.setRawValue (other._minTriggerIntervalFact.rawValue());

    return *this;
}

void CameraSpec::_init(bool setDefaults)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    _nameFact.setMetaData               (_metaDataMap[_nameName],               setDefaults /* setDefaultFromMetaData */);
    _sensorWidthFact.setMetaData        (_metaDataMap[_sensorWidthName],        setDefaults);
    _sensorHeightFact.setMetaData       (_metaDataMap[_sensorHeightName],       setDefaults);
    _imageWidthFact.setMetaData         (_metaDataMap[_imageWidthName],         setDefaults);
    _imageHeightFact.setMetaData        (_metaDataMap[_imageHeightName],        setDefaults);
    _focalLengthFact.setMetaData        (_metaDataMap[_focalLengthName],        setDefaults);
    _landscapeFact.setMetaData          (_metaDataMap[_landscapeName],          setDefaults);
    _fixedOrientationFact.setMetaData   (_metaDataMap[_fixedOrientationName],   setDefaults);
    _minTriggerIntervalFact.setMetaData (_metaDataMap[_minTriggerIntervalName], setDefaults);
}

void CameraSpec::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void CameraSpec::save(QJsonObject& json) const
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

bool CameraSpec::load(const QJsonObject& complexObject, QString& errorString)
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

#if 0
void CameraSpec::recalcImageOnGround(bool valueIsAltitude, double value)
{
    var focalLength     = missionItem.cameraFocalLength.rawValue
    var sensorWidth     = missionItem.cameraSensorWidth.rawValue
    var sensorHeight    = missionItem.cameraSensorHeight.rawValue
    var imageWidth      = missionItem.cameraResolutionWidth.rawValue
    var imageHeight     = missionItem.cameraResolutionHeight.rawValue

    var altitude        = missionItem.gridAltitude.rawValue
    var groundResolution= missionItem.groundResolution.rawValue
    var frontalOverlap  = missionItem.frontalOverlap.rawValue
    var sideOverlap     = missionItem.sideOverlap.rawValue

    if (focalLength <= 0 || sensorWidth <= 0 || sensorHeight <= 0 || imageWidth <= 0 || imageHeight <= 0 || groundResolution <= 0) {
        return
    }

    var imageSizeSideGround     //size in side (non flying) direction of the image on the ground
    var imageSizeFrontGround    //size in front (flying) direction of the image on the ground
    var gridSpacing
    var cameraTriggerDistance

    if (missionItem.fixedValueIsAltitude.value) {
        groundResolution = (altitude * sensorWidth * 100) / (imageWidth * focalLength)
    } else {
        altitude = (imageWidth * groundResolution * focalLength) / (sensorWidth * 100)
    }

    if (missionItem.cameraOrientationLandscape.value) {
        imageSizeSideGround  = (imageWidth  * groundResolution) / 100
        imageSizeFrontGround = (imageHeight * groundResolution) / 100
    } else {
        imageSizeSideGround  = (imageHeight * groundResolution) / 100
        imageSizeFrontGround = (imageWidth  * groundResolution) / 100
    }

    gridSpacing = imageSizeSideGround * ( (100-sideOverlap) / 100 )
    cameraTriggerDistance = imageSizeFrontGround * ( (100-frontalOverlap) / 100 )

    if (missionItem.fixedValueIsAltitude.value) {
        missionItem.groundResolution.rawValue = groundResolution
    } else {
        missionItem.gridAltitude.rawValue = altitude
    }
    missionItem.gridSpacing.rawValue = gridSpacing
    missionItem.cameraTriggerDistance.rawValue = cameraTriggerDistance
}
#endif
