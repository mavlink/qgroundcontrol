/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include "MissionCommands.h"
#include "FactMetaData.h"
#include "Vehicle.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"

#include <QStringList>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>
#include <QFile>

QGC_LOGGING_CATEGORY(MissionCommandsLog, "MissionCommandsLog")

const QString MissionCommands::_categoryJsonKey             (QStringLiteral("category"));
const QString MissionCommands::_decimalPlacesJsonKey        (QStringLiteral("decimalPlaces"));
const QString MissionCommands::_defaultJsonKey              (QStringLiteral("default"));
const QString MissionCommands::_descriptionJsonKey          (QStringLiteral("description"));
const QString MissionCommands::_enumStringsJsonKey          (QStringLiteral("enumStrings"));
const QString MissionCommands::_enumValuesJsonKey           (QStringLiteral("enumValues"));
const QString MissionCommands::_friendlyEditJsonKey         (QStringLiteral("friendlyEdit"));
const QString MissionCommands::_friendlyNameJsonKey         (QStringLiteral("friendlyName"));
const QString MissionCommands::_idJsonKey                   (QStringLiteral("id"));
const QString MissionCommands::_labelJsonKey                (QStringLiteral("label"));
const QString MissionCommands::_mavCmdInfoJsonKey           (QStringLiteral("mavCmdInfo"));
const QString MissionCommands::_param1JsonKey               (QStringLiteral("param1"));
const QString MissionCommands::_param2JsonKey               (QStringLiteral("param2"));
const QString MissionCommands::_param3JsonKey               (QStringLiteral("param3"));
const QString MissionCommands::_param4JsonKey               (QStringLiteral("param4"));
const QString MissionCommands::_paramJsonKeyFormat          (QStringLiteral("param%1"));
const QString MissionCommands::_rawNameJsonKey              (QStringLiteral("rawName"));
const QString MissionCommands::_standaloneCoordinateJsonKey (QStringLiteral("standaloneCoordinate"));
const QString MissionCommands::_specifiesCoordinateJsonKey  (QStringLiteral("specifiesCoordinate"));
const QString MissionCommands::_unitsJsonKey                (QStringLiteral("units"));
const QString MissionCommands::_versionJsonKey              (QStringLiteral("version"));

MissionCommands::MissionCommands(QGCApplication* app)
    : QGCTool(app)
{

}

void MissionCommands::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    _loadMavCmdInfoJson();
    _createFirmwareSpecificLists();
}

bool MissionCommands::_validateKeyTypes(QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types)
{
    for (int i=0; i<keys.count(); i++) {
        if (jsonObject.contains(keys[i])) {
            if (jsonObject.value(keys[i]).type() != types[i]) {
                qWarning() << "Incorrect type key:type:expected" << keys[i] << jsonObject.value(keys[i]).type() << types[i];
                return false;
            }
        }
    }

    return true;
}

void MissionCommands::_loadMavCmdInfoJson(void)
{
    QFile jsonFile(":/json/MavCmdInfo.json");
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open MavCmdInfo.json" << jsonFile.errorString();
        return;
    }

    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() <<  "Unable to open json document" << jsonParseError.errorString();
        return;
    }

    QJsonObject json = doc.object();

    int version = json.value(_versionJsonKey).toInt();
    if (version != 1) {
        qWarning() << "Invalid version" << version;
        return;
    }

    QJsonValue jsonValue = json.value(_mavCmdInfoJsonKey);
    if (!jsonValue.isArray()) {
        qWarning() << "mavCmdInfo not array";
        return;
    }

    QJsonArray jsonArray = jsonValue.toArray();
    foreach(QJsonValue info, jsonArray) {
        if (!info.isObject()) {
            qWarning() << "mavCmdArray should contain objects";
            return;
        }
        QJsonObject jsonObject = info.toObject();

        // Make sure we have the required keys
        QStringList requiredKeys;
        requiredKeys << _idJsonKey << _rawNameJsonKey;
        foreach (const QString &key, requiredKeys) {
            if (!jsonObject.contains(key)) {
                qWarning() << "Mission required key" << key;
                return;
            }
        }

        // Validate key types

        QStringList             keys;
        QList<QJsonValue::Type> types;
        keys << _idJsonKey << _rawNameJsonKey << _friendlyNameJsonKey << _descriptionJsonKey << _standaloneCoordinateJsonKey << _specifiesCoordinateJsonKey <<_friendlyEditJsonKey
             << _param1JsonKey << _param2JsonKey << _param3JsonKey << _param4JsonKey << _categoryJsonKey;
        types << QJsonValue::Double << QJsonValue::String << QJsonValue::String<< QJsonValue::String << QJsonValue::Bool << QJsonValue::Bool << QJsonValue::Bool
              << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::String;
        if (!_validateKeyTypes(jsonObject, keys, types)) {
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
            qWarning() << "Duplicate command" << mavCmdInfo->command();
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
                if (!_validateKeyTypes(paramObject, keys, types)) {
                    return;
                }

                mavCmdInfo->_friendlyEdit = true; // Assume friendly edit if we have params

                if (!paramObject.contains(_labelJsonKey)) {
                    qWarning() << "param object missing label key" << mavCmdInfo->rawName() << paramKey;
                    return;
                }

                MavCmdParamInfo* paramInfo = new MavCmdParamInfo(this);

                paramInfo->_label =         paramObject.value(_labelJsonKey).toString();
                paramInfo->_defaultValue =  paramObject.value(_defaultJsonKey).toDouble(0.0);
                paramInfo->_decimalPlaces = paramObject.value(_decimalPlacesJsonKey).toInt(FactMetaData::defaultDecimalPlaces);
                paramInfo->_enumStrings =   paramObject.value(_enumStringsJsonKey).toString().split(",", QString::SkipEmptyParts);
                paramInfo->_param =         i;
                paramInfo->_units =         paramObject.value(_unitsJsonKey).toString();

                QStringList enumValues = paramObject.value(_enumValuesJsonKey).toString().split(",", QString::SkipEmptyParts);
                foreach (const QString &enumValue, enumValues) {
                    bool    convertOk;
                    double  value = enumValue.toDouble(&convertOk);

                    if (!convertOk) {
                        qWarning() << "Bad enumValue" << enumValue;
                        return;
                    }

                    paramInfo->_enumValues << QVariant(value);
                }
                if (paramInfo->_enumValues.count() != paramInfo->_enumStrings.count()) {
                    qWarning() << "enum strings/values count mismatch" << paramInfo->_enumStrings.count() << paramInfo->_enumValues.count();
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

        // We don't add categories till down here, since friendly edit isn't valid till here
        if (mavCmdInfo->_command != MAV_CMD_NAV_LAST) {
            // Don't add fake home position command to categories

            if (mavCmdInfo->friendlyEdit()) {
                // Only friendly edit commands go in category list. We use MAV_AUTOPILOT_GENERIC key to store full list.
                if (!_categoryToMavCmdInfoListMap.contains(MAV_AUTOPILOT_GENERIC) || !_categoryToMavCmdInfoListMap[MAV_AUTOPILOT_GENERIC].contains(mavCmdInfo->category())) {
                    qCDebug(MissionCommandsLog) << "Adding new category";
                    _categoryToMavCmdInfoListMap[MAV_AUTOPILOT_GENERIC][mavCmdInfo->category()] = new QmlObjectListModel(this);
                }
                _categoryToMavCmdInfoListMap[MAV_AUTOPILOT_GENERIC][mavCmdInfo->category()]->append(mavCmdInfo);
            }
        }

        if (mavCmdInfo->friendlyEdit()) {
            if (mavCmdInfo->description().isEmpty()) {
                qWarning() << "Missing description" << mavCmdInfo->rawName();
                return;
            }
            if (mavCmdInfo->rawName() ==  mavCmdInfo->friendlyName()) {
                qWarning() << "Missing friendly name" << mavCmdInfo->rawName() << mavCmdInfo->friendlyName();
                return;
            }
        }
    }
}

MAV_AUTOPILOT MissionCommands::_firmwareTypeFromVehicle(Vehicle* vehicle) const
{
    if (vehicle) {
        return vehicle->firmwareType();
    } else {
        QSettings settings;

        // FIXME: Hack duplicated code from QGroundControlQmlGlobal. Had to do this for now since
        // QGroundControlQmlGlobal is not available from C++ side.

        return (MAV_AUTOPILOT)settings.value("OfflineEditingFirmwareType", MAV_AUTOPILOT_ARDUPILOTMEGA).toInt();
    }
}

QString MissionCommands::categoryFromCommand(MavlinkQmlSingleton::Qml_MAV_CMD command) const
{
    return _mavCmdInfoMap[(MAV_CMD)command]->category();
}

QVariant MissionCommands::getCommandsForCategory(Vehicle* vehicle, const QString& category) const
{
    return QVariant::fromValue(_categoryToMavCmdInfoListMap[_firmwareTypeFromVehicle(vehicle)][category]);
}

const QStringList MissionCommands::categories(Vehicle* vehicle) const
{
    QStringList list;

    foreach (const QString &category, _categoryToMavCmdInfoListMap[_firmwareTypeFromVehicle(vehicle)].keys()) {
        list << category;
    }

    return list;
}

void MissionCommands::_createFirmwareSpecificLists(void)
{
    QList<MAV_AUTOPILOT>    firmwareList;

    firmwareList << MAV_AUTOPILOT_PX4 << MAV_AUTOPILOT_ARDUPILOTMEGA;

    foreach (MAV_AUTOPILOT firmwareType, firmwareList) {
        FirmwarePlugin* plugin = _toolbox->firmwarePluginManager()->firmwarePluginForAutopilot(firmwareType, MAV_TYPE_QUADROTOR);

        QList<MAV_CMD> cmdList = plugin->supportedMissionCommands();
        foreach (MAV_CMD command, cmdList) {
            MavCmdInfo* mavCmdInfo = _mavCmdInfoMap[command];

            if (mavCmdInfo->friendlyEdit()) {
                if (!_categoryToMavCmdInfoListMap.contains(firmwareType) || !_categoryToMavCmdInfoListMap[firmwareType].contains(mavCmdInfo->category())) {
                    qCDebug(MissionCommandsLog) << "Adding new category" << firmwareType;
                    _categoryToMavCmdInfoListMap[firmwareType][mavCmdInfo->category()] = new QmlObjectListModel(this);
                }
                _categoryToMavCmdInfoListMap[firmwareType][mavCmdInfo->category()]->append(mavCmdInfo);
            } else {
                qWarning() << "Attempt to add non friendly edit supported command";
            }
        }
    }
}
