/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MissionCommandList.h"
#include "FactMetaData.h"
#include "Vehicle.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"
#include "JsonHelper.h"

#include <QStringList>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>
#include <QFile>

const QString MissionCommandList::_categoryJsonKey             (QStringLiteral("category"));
const QString MissionCommandList::_decimalPlacesJsonKey        (QStringLiteral("decimalPlaces"));
const QString MissionCommandList::_defaultJsonKey              (QStringLiteral("default"));
const QString MissionCommandList::_descriptionJsonKey          (QStringLiteral("description"));
const QString MissionCommandList::_enumStringsJsonKey          (QStringLiteral("enumStrings"));
const QString MissionCommandList::_enumValuesJsonKey           (QStringLiteral("enumValues"));
const QString MissionCommandList::_friendlyEditJsonKey         (QStringLiteral("friendlyEdit"));
const QString MissionCommandList::_friendlyNameJsonKey         (QStringLiteral("friendlyName"));
const QString MissionCommandList::_idJsonKey                   (QStringLiteral("id"));
const QString MissionCommandList::_labelJsonKey                (QStringLiteral("label"));
const QString MissionCommandList::_mavCmdInfoJsonKey           (QStringLiteral("mavCmdInfo"));
const QString MissionCommandList::_param1JsonKey               (QStringLiteral("param1"));
const QString MissionCommandList::_param2JsonKey               (QStringLiteral("param2"));
const QString MissionCommandList::_param3JsonKey               (QStringLiteral("param3"));
const QString MissionCommandList::_param4JsonKey               (QStringLiteral("param4"));
const QString MissionCommandList::_paramJsonKeyFormat          (QStringLiteral("param%1"));
const QString MissionCommandList::_rawNameJsonKey              (QStringLiteral("rawName"));
const QString MissionCommandList::_standaloneCoordinateJsonKey (QStringLiteral("standaloneCoordinate"));
const QString MissionCommandList::_specifiesCoordinateJsonKey  (QStringLiteral("specifiesCoordinate"));
const QString MissionCommandList::_unitsJsonKey                (QStringLiteral("units"));
const QString MissionCommandList::_versionJsonKey              (QStringLiteral("version"));

MissionCommandList::MissionCommandList(const QString& jsonFilename, QObject* parent)
    : QObject(parent)
{
    _loadMavCmdInfoJson(jsonFilename);
}

void MissionCommandList::_loadMavCmdInfoJson(const QString& jsonFilename)
{
    if (jsonFilename.isEmpty()) {
        return;
    }

    qCDebug(MissionCommandsLog) << "Loading" << jsonFilename;

    QFile jsonFile(jsonFilename);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file" << jsonFilename << jsonFile.errorString();
        return;
    }

    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() << jsonFilename << "Unable to open json document" << jsonParseError.errorString();
        return;
    }

    QJsonObject json = doc.object();

    int version = json.value(_versionJsonKey).toInt();
    if (version != 1) {
        qWarning() << jsonFilename << "Invalid version" << version;
        return;
    }

    QJsonValue jsonValue = json.value(_mavCmdInfoJsonKey);
    if (!jsonValue.isArray()) {
        qWarning() << jsonFilename << "mavCmdInfo not array";
        return;
    }

    QJsonArray jsonArray = jsonValue.toArray();
    foreach(QJsonValue info, jsonArray) {
        if (!info.isObject()) {
            qWarning() << jsonFilename << "mavCmdArray should contain objects";
            return;
        }
        QJsonObject jsonObject = info.toObject();

        // Make sure we have the required keys
        QString errorString;
        QStringList requiredKeys;
        requiredKeys << _idJsonKey << _rawNameJsonKey;
        if (!JsonHelper::validateRequiredKeys(jsonObject, requiredKeys, errorString)) {
            qWarning() << jsonFilename << errorString;
            return;
        }

        // Validate key types

        QStringList             keys;
        QList<QJsonValue::Type> types;
        keys << _idJsonKey << _rawNameJsonKey << _friendlyNameJsonKey << _descriptionJsonKey << _standaloneCoordinateJsonKey << _specifiesCoordinateJsonKey <<_friendlyEditJsonKey
             << _param1JsonKey << _param2JsonKey << _param3JsonKey << _param4JsonKey << _categoryJsonKey;
        types << QJsonValue::Double << QJsonValue::String << QJsonValue::String<< QJsonValue::String << QJsonValue::Bool << QJsonValue::Bool << QJsonValue::Bool
              << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::String;
        if (!JsonHelper::validateKeyTypes(jsonObject, keys, types, errorString)) {
            qWarning() << jsonFilename << errorString;
            return;
        }

        MavCmdInfo* mavCmdInfo = new MavCmdInfo(this);

        mavCmdInfo->_command = (MAV_CMD)      jsonObject.value(_idJsonKey).toInt();
        mavCmdInfo->_category =               jsonObject.value(_categoryJsonKey).toString("Advanced");
        mavCmdInfo->_rawName =                jsonObject.value(_rawNameJsonKey).toString();
        mavCmdInfo->_friendlyName =           jsonObject.value(_friendlyNameJsonKey).toString(QString());
        mavCmdInfo->_description =            jsonObject.value(_descriptionJsonKey).toString(QString());
        mavCmdInfo->_standaloneCoordinate =   jsonObject.value(_standaloneCoordinateJsonKey).toBool(false);
        mavCmdInfo->_specifiesCoordinate =    jsonObject.value(_specifiesCoordinateJsonKey).toBool(false);
        mavCmdInfo->_friendlyEdit =           jsonObject.value(_friendlyEditJsonKey).toBool(false);

        qCDebug(MissionCommandsLog) << "Command"
                                    << mavCmdInfo->_command
                                    << mavCmdInfo->_category
                                    << mavCmdInfo->_rawName
                                    << mavCmdInfo->_friendlyName
                                    << mavCmdInfo->_description
                                    << mavCmdInfo->_standaloneCoordinate
                                    << mavCmdInfo->_specifiesCoordinate
                                    << mavCmdInfo->_friendlyEdit;

        if (_mavCmdInfoMap.contains((MAV_CMD)mavCmdInfo->command())) {
            qWarning() << jsonFilename << "Duplicate command" << mavCmdInfo->command();
            return;
        }

        _mavCmdInfoMap[mavCmdInfo->_command] = mavCmdInfo;

        // Read params

        for (int i=1; i<=7; i++) {
            QString paramKey = QString(_paramJsonKeyFormat).arg(i);

            if (jsonObject.contains(paramKey)) {
                QJsonObject paramObject = jsonObject.value(paramKey).toObject();

                // Validate key types
                QStringList             keys;
                QList<QJsonValue::Type> types;
                keys << _defaultJsonKey << _decimalPlacesJsonKey << _enumStringsJsonKey << _enumValuesJsonKey << _labelJsonKey << _unitsJsonKey;
                types << QJsonValue::Double <<  QJsonValue::Double << QJsonValue::String << QJsonValue::String << QJsonValue::String << QJsonValue::String;
                if (!JsonHelper::validateKeyTypes(jsonObject, keys, types, errorString)) {
                    qWarning() << jsonFilename << errorString;
                    return;
                }

                mavCmdInfo->_friendlyEdit = true; // Assume friendly edit if we have params

                if (!paramObject.contains(_labelJsonKey)) {
                    qWarning() << jsonFilename << "param object missing label key" << mavCmdInfo->rawName() << paramKey;
                    return;
                }

                MavCmdParamInfo* paramInfo = new MavCmdParamInfo(this);

                paramInfo->_label =         paramObject.value(_labelJsonKey).toString();
                paramInfo->_defaultValue =  paramObject.value(_defaultJsonKey).toDouble(0.0);
                paramInfo->_decimalPlaces = paramObject.value(_decimalPlacesJsonKey).toInt(FactMetaData::unknownDecimalPlaces);
                paramInfo->_enumStrings =   paramObject.value(_enumStringsJsonKey).toString().split(",", QString::SkipEmptyParts);
                paramInfo->_param =         i;
                paramInfo->_units =         paramObject.value(_unitsJsonKey).toString();

                QStringList enumValues = paramObject.value(_enumValuesJsonKey).toString().split(",", QString::SkipEmptyParts);
                foreach (const QString &enumValue, enumValues) {
                    bool    convertOk;
                    double  value = enumValue.toDouble(&convertOk);

                    if (!convertOk) {
                        qWarning() << jsonFilename << "Bad enumValue" << enumValue;
                        return;
                    }

                    paramInfo->_enumValues << QVariant(value);
                }
                if (paramInfo->_enumValues.count() != paramInfo->_enumStrings.count()) {
                    qWarning() << jsonFilename << "enum strings/values count mismatch" << paramInfo->_enumStrings.count() << paramInfo->_enumValues.count();
                    return;
                }

                qCDebug(MissionCommandsLog) << "Param"
                                            << paramInfo->_label
                                            << paramInfo->_defaultValue
                                            << paramInfo->_decimalPlaces
                                            << paramInfo->_param
                                            << paramInfo->_units
                                            << paramInfo->_enumStrings
                                            << paramInfo->_enumValues;

                mavCmdInfo->_paramInfoMap[i] = paramInfo;
            }
        }

        if (mavCmdInfo->friendlyEdit()) {
            if (mavCmdInfo->description().isEmpty()) {
                qWarning() << jsonFilename << "Missing description" << mavCmdInfo->rawName();
                return;
            }
            if (mavCmdInfo->rawName() ==  mavCmdInfo->friendlyName()) {
                qWarning() << jsonFilename << "Missing friendly name" << mavCmdInfo->rawName() << mavCmdInfo->friendlyName();
                return;
            }
        }
    }
}

bool MissionCommandList::contains(MAV_CMD command) const
{
    return _mavCmdInfoMap.contains(command);
}

MavCmdInfo* MissionCommandList::getMavCmdInfo(MAV_CMD command) const
{
    if (!contains(command)) {
        return NULL;
    }

    return _mavCmdInfoMap[command];
}

QList<MAV_CMD> MissionCommandList::commandsIds(void) const
{
    QList<MAV_CMD> list;

    foreach (const MavCmdInfo* mavCmdInfo, _mavCmdInfoMap) {
        list << (MAV_CMD)mavCmdInfo->command();
    }

    return list;
}
