/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionCommandUIInfo.h"
#include "JsonHelper.h"
#include "FactMetaData.h"
#include "QGCLoggingCategory.h"

const char* MissionCommandUIInfo::_categoryJsonKey              = "category";
const char* MissionCommandUIInfo::_decimalPlacesJsonKey         = "decimalPlaces";
const char* MissionCommandUIInfo::_defaultJsonKey               = "default";
const char* MissionCommandUIInfo::_descriptionJsonKey           = "description";
const char* MissionCommandUIInfo::_enumStringsJsonKey           = "enumStrings";
const char* MissionCommandUIInfo::_enumValuesJsonKey            = "enumValues";
const char* MissionCommandUIInfo::_nanUnchangedJsonKey          = "nanUnchanged";
const char* MissionCommandUIInfo::_friendlyEditJsonKey          = "friendlyEdit";
const char* MissionCommandUIInfo::_friendlyNameJsonKey          = "friendlyName";
const char* MissionCommandUIInfo::_idJsonKey                    = "id";
const char* MissionCommandUIInfo::_labelJsonKey                 = "label";
const char* MissionCommandUIInfo::_mavCmdInfoJsonKey            = "mavCmdInfo";
const char* MissionCommandUIInfo::_param1JsonKey                = "param1";
const char* MissionCommandUIInfo::_param2JsonKey                = "param2";
const char* MissionCommandUIInfo::_param3JsonKey                = "param3";
const char* MissionCommandUIInfo::_param4JsonKey                = "param4";
const char* MissionCommandUIInfo::_param5JsonKey                = "param5";
const char* MissionCommandUIInfo::_param6JsonKey                = "param6";
const char* MissionCommandUIInfo::_param7JsonKey                = "param7";
const char* MissionCommandUIInfo::_paramJsonKeyFormat           = "param%1";
const char* MissionCommandUIInfo::_paramRemoveJsonKey           = "paramRemove";
const char* MissionCommandUIInfo::_rawNameJsonKey               = "rawName";
const char* MissionCommandUIInfo::_standaloneCoordinateJsonKey  = "standaloneCoordinate";
const char* MissionCommandUIInfo::_specifiesCoordinateJsonKey   = "specifiesCoordinate";
const char* MissionCommandUIInfo::_specifiesAltitudeOnlyJsonKey = "specifiesAltitudeOnly";
const char* MissionCommandUIInfo::_unitsJsonKey                 = "units";
const char* MissionCommandUIInfo::_commentJsonKey               = "comment";
const char* MissionCommandUIInfo::_advancedCategory             = "Advanced";

MissionCmdParamInfo::MissionCmdParamInfo(QObject* parent)
    : QObject(parent)
{

}

MissionCmdParamInfo::MissionCmdParamInfo(const MissionCmdParamInfo& other, QObject* parent)
    : QObject(parent)
{
    *this = other;
}

const MissionCmdParamInfo& MissionCmdParamInfo::operator=(const MissionCmdParamInfo& other)
{
    _decimalPlaces =    other._decimalPlaces;
    _defaultValue =     other._defaultValue;
    _enumStrings =      other._enumStrings;
    _enumValues =       other._enumValues;
    _label =            other._label;
    _param =            other._param;
    _units =            other._units;
    _nanUnchanged =     other._nanUnchanged;

    return *this;
}

MissionCommandUIInfo::MissionCommandUIInfo(QObject* parent)
    : QObject(parent)
{

}

MissionCommandUIInfo::MissionCommandUIInfo(const MissionCommandUIInfo& other, QObject* parent)
    : QObject(parent)
{
    *this = other;
}

const MissionCommandUIInfo& MissionCommandUIInfo::operator=(const MissionCommandUIInfo& other)
{
    _command =          other._command;
    _infoMap =          other._infoMap;
    _paramRemoveList =  other._paramRemoveList;

    for (int index: other._paramInfoMap.keys()) {
        _paramInfoMap[index] = new MissionCmdParamInfo(*other._paramInfoMap[index], this);
    }

    return *this;
}

QString MissionCommandUIInfo::category(void) const
{
    if (_infoMap.contains(_categoryJsonKey)) {
        return _infoMap[_categoryJsonKey].toString();
    } else {
        return _advancedCategory;
    }
}

QString MissionCommandUIInfo::description(void) const
{
    if (_infoMap.contains(_descriptionJsonKey)) {
        return _infoMap[_descriptionJsonKey].toString();
    } else {
        return QString();
    }
}

bool MissionCommandUIInfo::friendlyEdit(void) const
{
    if (_infoMap.contains(_friendlyEditJsonKey)) {
        return _infoMap[_friendlyEditJsonKey].toBool();
    } else {
        return false;
    }
}

QString MissionCommandUIInfo::friendlyName(void) const
{
    if (_infoMap.contains(_friendlyNameJsonKey)) {
        return _infoMap[_friendlyNameJsonKey].toString();
    } else {
        return QString();
    }
}

QString MissionCommandUIInfo::rawName(void) const
{
    if (_infoMap.contains(_rawNameJsonKey)) {
        return _infoMap[_rawNameJsonKey].toString();
    } else {
        return QString();
    }
}

bool MissionCommandUIInfo::isStandaloneCoordinate(void) const
{
    if (_infoMap.contains(_standaloneCoordinateJsonKey)) {
        return _infoMap[_standaloneCoordinateJsonKey].toBool();
    } else {
        return false;
    }
}

bool MissionCommandUIInfo::specifiesCoordinate(void) const
{
    if (_infoMap.contains(_specifiesCoordinateJsonKey)) {
        return _infoMap[_specifiesCoordinateJsonKey].toBool();
    } else {
        return false;
    }
}

bool MissionCommandUIInfo::specifiesAltitudeOnly(void) const
{
    if (_infoMap.contains(_specifiesAltitudeOnlyJsonKey)) {
        return _infoMap[_specifiesAltitudeOnlyJsonKey].toBool();
    } else {
        return false;
    }
}

void MissionCommandUIInfo::_overrideInfo(MissionCommandUIInfo* uiInfo)
{
    // Override info values
    for (const QString& valueKey: uiInfo->_infoMap.keys()) {
        _setInfoValue(valueKey, uiInfo->_infoMap[valueKey]);
    }

    // Add to the remove params list
    for (int removeIndex: uiInfo->_paramRemoveList) {
        if (!_paramRemoveList.contains(removeIndex)) {
            _paramRemoveList.append(removeIndex);
        }
    }

    // Override param info
    for (const int paramIndex: uiInfo->_paramInfoMap.keys()) {
        _paramRemoveList.removeOne(paramIndex);
        // MissionCmdParamInfo objects are owned by MissionCommandTree are are in existence for the entire run so
        // we can just use the same pointer reference.
        _paramInfoMap[paramIndex] = uiInfo->_paramInfoMap[paramIndex];
    }
}

QString MissionCommandUIInfo::_loadErrorString(const QString& errorString) const
{
    return QString("%1 %2").arg(_infoValue(_rawNameJsonKey).toString()).arg(errorString);
}

bool MissionCommandUIInfo::loadJsonInfo(const QJsonObject& jsonObject, bool requireFullObject, QString& errorString)
{
    QString internalError;

    QStringList allKeys;
    allKeys << _idJsonKey << _rawNameJsonKey << _friendlyNameJsonKey << _descriptionJsonKey << _standaloneCoordinateJsonKey << _specifiesCoordinateJsonKey
            <<_friendlyEditJsonKey << _param1JsonKey << _param2JsonKey << _param3JsonKey << _param4JsonKey << _param5JsonKey << _param6JsonKey << _param7JsonKey
            << _paramRemoveJsonKey << _categoryJsonKey << _specifiesAltitudeOnlyJsonKey;

    // Look for unknown keys in top level object
    for (const QString& key: jsonObject.keys()) {
        if (!allKeys.contains(key) && key != _commentJsonKey) {
            errorString = _loadErrorString(QString("Unknown key: %1").arg(key));
            return false;
        }
    }

    // Make sure we have the required keys
    QStringList requiredKeys;
    requiredKeys << _idJsonKey;
    if (requireFullObject) {
        requiredKeys << _rawNameJsonKey;
    }
    if (!JsonHelper::validateRequiredKeys(jsonObject, requiredKeys, internalError)) {
        errorString = _loadErrorString(internalError);
        return false;
    }

    // Only the full object should specify rawName, friendlyName
    if (!requireFullObject && (jsonObject.contains(_rawNameJsonKey) || jsonObject.contains(_friendlyNameJsonKey))) {
        errorString = _loadErrorString(QStringLiteral("Only the full object should specify rawName or friendlyName"));
        return false;
    }

    // Validate key types

    QList<QJsonValue::Type> types;
    types << QJsonValue::Double << QJsonValue::String << QJsonValue::String<< QJsonValue::String << QJsonValue::Bool << QJsonValue::Bool << QJsonValue::Bool
          << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object
          << QJsonValue::String << QJsonValue::String << QJsonValue::Bool;
    if (!JsonHelper::validateKeyTypes(jsonObject, allKeys, types, internalError)) {
        errorString = _loadErrorString(internalError);
        return false;
    }

    // Read in top level values

    _command = (MAV_CMD)jsonObject.value(_idJsonKey).toInt();

    if (jsonObject.contains(_categoryJsonKey)) {
        _infoMap[_categoryJsonKey] = jsonObject.value(_categoryJsonKey).toVariant();
    }
    if (jsonObject.contains(_rawNameJsonKey)) {
        _infoMap[_rawNameJsonKey] = jsonObject.value(_rawNameJsonKey).toVariant();
    }
    if (jsonObject.contains(_friendlyNameJsonKey)) {
        _infoMap[_friendlyNameJsonKey] = jsonObject.value(_friendlyNameJsonKey).toVariant();
    }
    if (jsonObject.contains(_descriptionJsonKey)) {
        _infoMap[_descriptionJsonKey] = jsonObject.value(_descriptionJsonKey).toVariant();
    }
    if (jsonObject.contains(_standaloneCoordinateJsonKey)) {
        _infoMap[_standaloneCoordinateJsonKey] = jsonObject.value(_standaloneCoordinateJsonKey).toVariant();
    }
    if (jsonObject.contains(_specifiesCoordinateJsonKey)) {
        _infoMap[_specifiesCoordinateJsonKey] = jsonObject.value(_specifiesCoordinateJsonKey).toVariant();
    }
    if (jsonObject.contains(_specifiesAltitudeOnlyJsonKey)) {
        _infoMap[_specifiesAltitudeOnlyJsonKey] = jsonObject.value(_specifiesAltitudeOnlyJsonKey).toBool();
    }
    if (jsonObject.contains(_friendlyEditJsonKey)) {
        _infoMap[_friendlyEditJsonKey] = jsonObject.value(_friendlyEditJsonKey).toVariant();
    }
    if (jsonObject.contains(_paramRemoveJsonKey)) {
        QStringList indexList = jsonObject.value(_paramRemoveJsonKey).toString().split(QStringLiteral(","));
        for (const QString& indexString: indexList) {
            _paramRemoveList.append(indexString.toInt());
        }
    }

    if (requireFullObject) {
        // Since this is the base of the hierarchy it must contain valid defaults for all values.
        if (!_infoAvailable(_categoryJsonKey)) {
            _setInfoValue(_categoryJsonKey, _advancedCategory);
        }
        if (!_infoAvailable(_friendlyNameJsonKey)) {
            _setInfoValue(_friendlyNameJsonKey, _infoValue(_rawNameJsonKey));
        }
        if (!_infoAvailable(_descriptionJsonKey)) {
            _setInfoValue(_descriptionJsonKey, QStringLiteral(""));
        }
        if (!_infoAvailable(_standaloneCoordinateJsonKey)) {
            _setInfoValue(_standaloneCoordinateJsonKey, false);
        }
        if (!_infoAvailable(_specifiesCoordinateJsonKey)) {
            _setInfoValue(_specifiesCoordinateJsonKey, false);
        }
        if (!_infoAvailable(_friendlyEditJsonKey)) {
            _setInfoValue(_friendlyEditJsonKey, false);
        }
    }

    if (requireFullObject) {
        if (_infoAvailable(_friendlyEditJsonKey) && _infoValue(_friendlyEditJsonKey).toBool()) {
            if (!_infoAvailable(_descriptionJsonKey)) {
                errorString = _loadErrorString(QStringLiteral("Missing description for friendly edit"));
                return false;
            }
            if (_infoValue(_rawNameJsonKey).toString() == _infoValue(_friendlyNameJsonKey).toString()) {
                errorString = _loadErrorString(QStringLiteral("Missing friendlyName for friendly edit"));
                return false;
            }
        }
    }

    QString debugOutput;
    for (const QString& infoKey: _infoMap.keys()) {
        debugOutput.append(QString("MavCmdInfo %1: %2 ").arg(infoKey).arg(_infoMap[infoKey].toString()));
    }
    qCDebug(MissionCommandsLog) << debugOutput;

    // Read params

    for (int i=1; i<=7; i++) {
        QString paramKey = QString(_paramJsonKeyFormat).arg(i);

        if (jsonObject.contains(paramKey)) {
            QJsonObject paramObject = jsonObject.value(paramKey).toObject();

            QStringList allParamKeys;
            allParamKeys << _defaultJsonKey << _decimalPlacesJsonKey << _enumStringsJsonKey << _enumValuesJsonKey << _labelJsonKey << _unitsJsonKey << _nanUnchangedJsonKey;

            // Look for unknown keys in param object
            for (const QString& key: paramObject.keys()) {
                if (!allParamKeys.contains(key) && key != _commentJsonKey) {
                    errorString = _loadErrorString(QString("Unknown param key: %1").arg(key));
                    return false;
                }
            }

            // Validate key types
            QList<QJsonValue::Type> types;
            types << QJsonValue::Null <<  QJsonValue::Double << QJsonValue::String << QJsonValue::String << QJsonValue::String << QJsonValue::String << QJsonValue::Bool;
            if (!JsonHelper::validateKeyTypes(jsonObject, allParamKeys, types, internalError)) {
                errorString = _loadErrorString(internalError);
                return false;
            }

            _setInfoValue(_friendlyEditJsonKey, true); // Assume friendly edit if we have params

            if (!paramObject.contains(_labelJsonKey)) {
                internalError = QString("param object missing label key: %1").arg(paramKey);
                errorString = _loadErrorString(internalError);
                return false;
            }

            MissionCmdParamInfo* paramInfo = new MissionCmdParamInfo(this);

            paramInfo->_label =         paramObject.value(_labelJsonKey).toString();
            paramInfo->_decimalPlaces = paramObject.value(_decimalPlacesJsonKey).toInt(FactMetaData::kUnknownDecimalPlaces);
            paramInfo->_enumStrings =   paramObject.value(_enumStringsJsonKey).toString().split(",", QString::SkipEmptyParts);
            paramInfo->_param =         i;
            paramInfo->_units =         paramObject.value(_unitsJsonKey).toString();
            paramInfo->_nanUnchanged =  paramObject.value(_nanUnchangedJsonKey).toBool(false);

            if (paramObject.contains(_defaultJsonKey)) {
                if (paramInfo->_nanUnchanged) {
                    paramInfo->_defaultValue = JsonHelper::possibleNaNJsonValue(paramObject[_defaultJsonKey]);
                } else {
                    if (paramObject[_defaultJsonKey].type() == QJsonValue::Null) {
                        errorString = QString("Param %1 default value was null/NaN but NaN is not allowed");
                        return false;
                    }
                    paramInfo->_defaultValue = paramObject.value(_defaultJsonKey).toDouble(0.0);
                }
            } else {
                paramInfo->_defaultValue = paramInfo->_nanUnchanged ? std::numeric_limits<double>::quiet_NaN() : 0;
            }

            QStringList enumValues = paramObject.value(_enumValuesJsonKey).toString().split(",", QString::SkipEmptyParts);
            for (const QString &enumValue: enumValues) {
                bool    convertOk;
                double  value = enumValue.toDouble(&convertOk);

                if (!convertOk) {
                    internalError = QString("Bad enumValue: %1").arg(enumValue);
                    errorString = _loadErrorString(internalError);
                    return false;
                }

                paramInfo->_enumValues << QVariant(value);
            }
            if (paramInfo->_enumValues.count() != paramInfo->_enumStrings.count()) {
                internalError = QString("enum strings/values count mismatch: %1, %2").arg(paramInfo->_enumStrings.count()).arg(paramInfo->_enumValues.count());
                errorString = _loadErrorString(internalError);
                return false;
            }

            qCDebug(MissionCommandsLog) << "Param"
                                        << paramInfo->_label
                                        << paramInfo->_defaultValue
                                        << paramInfo->_decimalPlaces
                                        << paramInfo->_param
                                        << paramInfo->_units
                                        << paramInfo->_enumStrings
                                        << paramInfo->_enumValues
                                        << paramInfo->_nanUnchanged;

            _paramInfoMap[i] = paramInfo;
        }
    }

    return true;
}

const MissionCmdParamInfo* MissionCommandUIInfo::getParamInfo(int index, bool& showUI) const
{
    const MissionCmdParamInfo* paramInfo = nullptr;

    if (_paramInfoMap.contains(index)) {
        paramInfo = _paramInfoMap[index];
    }

    showUI = (paramInfo != nullptr) && !_paramRemoveList.contains(index);

    return paramInfo;
}
