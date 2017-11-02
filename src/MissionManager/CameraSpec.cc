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

CameraSpec::CameraSpec(const CameraSpec& other, QObject* parent)
    : QObject                   (parent)
    , _dirty                    (false)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CameraSpec.FactMetaData.json"), this))
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
    json[_sensorWidthName] =        _sensorWidthFact.rawValue().toDouble();
    json[_sensorHeightName] =       _sensorHeightFact.rawValue().toDouble();
    json[_imageWidthName] =         _imageWidthFact.rawValue().toDouble();
    json[_imageHeightName] =        _imageHeightFact.rawValue().toDouble();
    json[_focalLengthName] =        _focalLengthFact.rawValue().toDouble();
    json[_landscapeName] =          _landscapeFact.rawValue().toBool();
    json[_fixedOrientationName] =   _fixedOrientationFact.rawValue().toBool();
    json[_minTriggerIntervalName] = _minTriggerIntervalFact.rawValue().toDouble();
}

bool CameraSpec::load(const QJsonObject& json, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { _sensorWidthName,         QJsonValue::Double, true },
        { _sensorHeightName,        QJsonValue::Double, true },
        { _imageWidthName,          QJsonValue::Double, true },
        { _imageHeightName,         QJsonValue::Double, true },
        { _focalLengthName,         QJsonValue::Double, true },
        { _landscapeName,           QJsonValue::Bool,   true },
        { _fixedOrientationName,    QJsonValue::Bool,   true },
        { _minTriggerIntervalName,  QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(json, keyInfoList, errorString)) {
        return false;
    }

    _sensorWidthFact.setRawValue        (json[_sensorWidthName].toDouble());
    _sensorHeightFact.setRawValue       (json[_sensorHeightName].toDouble());
    _imageWidthFact.setRawValue         (json[_imageWidthName].toDouble());
    _imageHeightFact.setRawValue        (json[_imageHeightName].toDouble());
    _focalLengthFact.setRawValue        (json[_focalLengthName].toDouble());
    _landscapeFact.setRawValue          (json[_landscapeName].toBool());
    _fixedOrientationFact.setRawValue   (json[_fixedOrientationName].toBool());
    _minTriggerIntervalFact.setRawValue (json[_minTriggerIntervalName].toDouble());

    return true;
}
