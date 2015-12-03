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

#include <QStringList>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

#include "MissionItem.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(MissionItemLog, "MissionItemLog")

const double MissionItem::defaultTakeoffPitch =         15.0;
const double MissionItem::defaultHeading =              0.0;
const double MissionItem::defaultAltitude =             25.0;
const double MissionItem::defaultAcceptanceRadius =     3.0;
const double MissionItem::defaultLoiterOrbitRadius =    10.0;
const double MissionItem::defaultLoiterTurns =          1.0;

FactMetaData* MissionItem::_altitudeMetaData =          NULL;
FactMetaData* MissionItem::_commandMetaData =           NULL;
FactMetaData* MissionItem::_defaultParamMetaData =      NULL;
FactMetaData* MissionItem::_frameMetaData =             NULL;
FactMetaData* MissionItem::_latitudeMetaData =          NULL;
FactMetaData* MissionItem::_longitudeMetaData =         NULL;
FactMetaData* MissionItem::_supportedCommandMetaData =  NULL;

const QString MissionItem::_decimalPlacesJsonKey        (QStringLiteral("decimalPlaces"));
const QString MissionItem::_defaultJsonKey              (QStringLiteral("default"));
const QString MissionItem::_descriptionJsonKey          (QStringLiteral("description"));
const QString MissionItem::_enumStringsJsonKey          (QStringLiteral("enumStrings"));
const QString MissionItem::_enumValuesJsonKey           (QStringLiteral("enumValues"));
const QString MissionItem::_friendlyEditJsonKey         (QStringLiteral("friendlyEdit"));
const QString MissionItem::_friendlyNameJsonKey         (QStringLiteral("friendlyName"));
const QString MissionItem::_idJsonKey                   (QStringLiteral("id"));
const QString MissionItem::_labelJsonKey                (QStringLiteral("label"));
const QString MissionItem::_mavCmdInfoJsonKey           (QStringLiteral("mavCmdInfo"));
const QString MissionItem::_param1JsonKey               (QStringLiteral("param1"));
const QString MissionItem::_param2JsonKey               (QStringLiteral("param2"));
const QString MissionItem::_param3JsonKey               (QStringLiteral("param3"));
const QString MissionItem::_param4JsonKey               (QStringLiteral("param4"));
const QString MissionItem::_paramJsonKeyFormat          (QStringLiteral("param%1"));
const QString MissionItem::_rawNameJsonKey              (QStringLiteral("rawName"));
const QString MissionItem::_specifiesCoordinateJsonKey  (QStringLiteral("specifiesCoordinate"));
const QString MissionItem::_unitsJsonKey                (QStringLiteral("units"));
const QString MissionItem::_versionJsonKey              (QStringLiteral("version"));

const QString MissionItem::_degreesConvertUnits         (QStringLiteral("degreesConvert"));
const QString MissionItem::_degreesUnits                (QStringLiteral("degrees"));

QMap<MAV_CMD, MissionItem::MavCmdInfo_t> MissionItem::_mavCmdInfoMap;

struct EnumInfo_s {
    const char *    label;
    MAV_FRAME       frame;
};

static const struct EnumInfo_s _rgMavFrameInfo[] = {
    { "MAV_FRAME_GLOBAL",                   MAV_FRAME_GLOBAL },
    { "MAV_FRAME_LOCAL_NED",                MAV_FRAME_LOCAL_NED },
    { "MAV_FRAME_MISSION",                  MAV_FRAME_MISSION },
    { "MAV_FRAME_GLOBAL_RELATIVE_ALT",      MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { "MAV_FRAME_LOCAL_ENU",                MAV_FRAME_LOCAL_ENU },
    { "MAV_FRAME_GLOBAL_INT",               MAV_FRAME_GLOBAL_INT },
    { "MAV_FRAME_GLOBAL_RELATIVE_ALT_INT",  MAV_FRAME_GLOBAL_RELATIVE_ALT_INT },
    { "MAV_FRAME_LOCAL_OFFSET_NED",         MAV_FRAME_LOCAL_OFFSET_NED },
    { "MAV_FRAME_BODY_NED",                 MAV_FRAME_BODY_NED },
    { "MAV_FRAME_BODY_OFFSET_NED",          MAV_FRAME_BODY_OFFSET_NED },
    { "MAV_FRAME_GLOBAL_TERRAIN_ALT",       MAV_FRAME_GLOBAL_TERRAIN_ALT },
    { "MAV_FRAME_GLOBAL_TERRAIN_ALT_INT",   MAV_FRAME_GLOBAL_TERRAIN_ALT_INT },
};

QDebug operator<<(QDebug dbg, const MissionItem& missionItem)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "MissionItem(" << missionItem.coordinate() << ")";
    
    return dbg;
}

QDebug operator<<(QDebug dbg, const MissionItem* missionItem)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "MissionItem(" << missionItem->coordinate() << ")";
    
    return dbg;
}

MissionItem::MissionItem(QObject* parent)
    : QObject(parent)
    , _rawEdit(false)
    , _dirty(false)
    , _sequenceNumber(0)
    , _isCurrentItem(false)
    , _azimuth(0.0)
    , _distance(0.0)
    , _homePositionSpecialCase(false)
    , _homePositionValid(false)
    , _altitudeRelativeToHomeFact   (0, "Altitude is relative to home", FactMetaData::valueTypeUint32)
    , _autoContinueFact             (0, "AutoContinue",                 FactMetaData::valueTypeUint32)
    , _commandFact                  (0, "",                             FactMetaData::valueTypeUint32)
    , _frameFact                    (0, "",                             FactMetaData::valueTypeUint32)
    , _param1Fact                   (0, "Param1:",                      FactMetaData::valueTypeDouble)
    , _param2Fact                   (0, "Param2:",                      FactMetaData::valueTypeDouble)
    , _param3Fact                   (0, "Param3:",                      FactMetaData::valueTypeDouble)
    , _param4Fact                   (0, "Param4:",                      FactMetaData::valueTypeDouble)
    , _param5Fact                   (0, "Latitude:",                    FactMetaData::valueTypeDouble)
    , _param6Fact                   (0, "Longitude:",                   FactMetaData::valueTypeDouble)
    , _param7Fact                   (0, "Altitude:",                    FactMetaData::valueTypeDouble)
    , _supportedCommandFact         (0, "Command:",                     FactMetaData::valueTypeUint32)
    , _param1MetaData(FactMetaData::valueTypeDouble)
    , _param2MetaData(FactMetaData::valueTypeDouble)
    , _param3MetaData(FactMetaData::valueTypeDouble)
    , _param4MetaData(FactMetaData::valueTypeDouble)
    , _param5MetaData(FactMetaData::valueTypeDouble)
    , _param6MetaData(FactMetaData::valueTypeDouble)
    , _param7MetaData(FactMetaData::valueTypeDouble)
    , _syncingAltitudeRelativeToHomeAndFrame    (false)
    , _syncingHeadingDegreesAndParam4           (false)
    , _syncingSupportedCommandAndCommand        (false)
{
    _setupMetaData();
    _connectSignals();

    setAutoContinue(true);
}

MissionItem::MissionItem(int             sequenceNumber,
                         MAV_CMD         command,
                         MAV_FRAME       frame,
                         double          param1,
                         double          param2,
                         double          param3,
                         double          param4,
                         double          param5,
                         double          param6,
                         double          param7,
                         bool            autoContinue,
                         bool            isCurrentItem,
                         QObject*        parent)
    : QObject(parent)
    , _rawEdit(false)
    , _dirty(false)
    , _sequenceNumber(sequenceNumber)
    , _isCurrentItem(isCurrentItem)
    , _azimuth(0.0)
    , _distance(0.0)
    , _homePositionSpecialCase(false)
    , _homePositionValid(false)
    , _altitudeRelativeToHomeFact   (0, "Altitude is relative to home", FactMetaData::valueTypeUint32)
    , _commandFact                  (0, "Command:",                     FactMetaData::valueTypeUint32)
    , _frameFact                    (0, "Frame:",                       FactMetaData::valueTypeUint32)
    , _param1Fact                   (0, "Param1:",                      FactMetaData::valueTypeDouble)
    , _param2Fact                   (0, "Param2:",                      FactMetaData::valueTypeDouble)
    , _param3Fact                   (0, "Param3:",                      FactMetaData::valueTypeDouble)
    , _param4Fact                   (0, "Param4:",                      FactMetaData::valueTypeDouble)
    , _param5Fact                   (0, "Lat/X:",                       FactMetaData::valueTypeDouble)
    , _param6Fact                   (0, "Lon/Y:",                       FactMetaData::valueTypeDouble)
    , _param7Fact                   (0, "Alt/Z:",                       FactMetaData::valueTypeDouble)
    , _supportedCommandFact         (0, "Command:",                     FactMetaData::valueTypeUint32)
    , _param1MetaData(FactMetaData::valueTypeDouble)
    , _param2MetaData(FactMetaData::valueTypeDouble)
    , _param3MetaData(FactMetaData::valueTypeDouble)
    , _param4MetaData(FactMetaData::valueTypeDouble)
    , _param5MetaData(FactMetaData::valueTypeDouble)
    , _param6MetaData(FactMetaData::valueTypeDouble)
    , _param7MetaData(FactMetaData::valueTypeDouble)
    , _syncingAltitudeRelativeToHomeAndFrame    (false)
    , _syncingHeadingDegreesAndParam4           (false)
    , _syncingSupportedCommandAndCommand        (false)
{
    _setupMetaData();
    _connectSignals();

    setCommand(command);
    setFrame(frame);
    setAutoContinue(autoContinue);

    _syncFrameToAltitudeRelativeToHome();
    _syncCommandToSupportedCommand(QVariant(this->command()));

    _param1Fact.setRawValue(param1);
    _param2Fact.setRawValue(param2);
    _param3Fact.setRawValue(param3);
    _param4Fact.setRawValue(param4);
    _param5Fact.setRawValue(param5);
    _param6Fact.setRawValue(param6);
    _param7Fact.setRawValue(param7);
}

MissionItem::MissionItem(const MissionItem& other, QObject* parent)
    : QObject(parent)
    , _rawEdit(false)
    , _dirty(false)
    , _sequenceNumber(0)
    , _isCurrentItem(false)
    , _azimuth(0.0)
    , _distance(0.0)
    , _homePositionSpecialCase(false)
    , _homePositionValid(false)
    , _altitudeRelativeToHomeFact   (0, "Altitude is relative to home", FactMetaData::valueTypeUint32)
    , _commandFact                  (0, "Command:",                     FactMetaData::valueTypeUint32)
    , _frameFact                    (0, "Frame:",                       FactMetaData::valueTypeUint32)
    , _param1Fact                   (0, "Param1:",                      FactMetaData::valueTypeDouble)
    , _param2Fact                   (0, "Param2:",                      FactMetaData::valueTypeDouble)
    , _param3Fact                   (0, "Param3:",                      FactMetaData::valueTypeDouble)
    , _param4Fact                   (0, "Param4:",                      FactMetaData::valueTypeDouble)
    , _param5Fact                   (0, "Lat/X:",                       FactMetaData::valueTypeDouble)
    , _param6Fact                   (0, "Lon/Y:",                       FactMetaData::valueTypeDouble)
    , _param7Fact                   (0, "Alt/Z:",                       FactMetaData::valueTypeDouble)
    , _supportedCommandFact         (0, "Command:",                     FactMetaData::valueTypeUint32)
    , _param1MetaData(FactMetaData::valueTypeDouble)
    , _param2MetaData(FactMetaData::valueTypeDouble)
    , _param3MetaData(FactMetaData::valueTypeDouble)
    , _param4MetaData(FactMetaData::valueTypeDouble)
    , _syncingAltitudeRelativeToHomeAndFrame    (false)
    , _syncingHeadingDegreesAndParam4           (false)
    , _syncingSupportedCommandAndCommand        (false)
{
    _setupMetaData();
    _connectSignals();

    *this = other;
}

const MissionItem& MissionItem::operator=(const MissionItem& other)
{
    setCommand(other.command());
    setFrame(other.frame());
    setRawEdit(other._rawEdit);
    setDirty(other._dirty);
    setSequenceNumber(other._sequenceNumber);
    setAutoContinue(other.autoContinue());
    setIsCurrentItem(other._isCurrentItem);
    setAzimuth(other._distance);
    setDistance(other._distance);
    setHomePositionSpecialCase(other._homePositionSpecialCase);
    setHomePositionValid(other._homePositionValid);

    _syncFrameToAltitudeRelativeToHome();
    _syncCommandToSupportedCommand(QVariant(this->command()));

    _param1Fact.setRawValue(other._param1Fact.rawValue());
    _param2Fact.setRawValue(other._param2Fact.rawValue());
    _param3Fact.setRawValue(other._param3Fact.rawValue());
    _param4Fact.setRawValue(other._param4Fact.rawValue());
    _param5Fact.setRawValue(other._param5Fact.rawValue());
    _param6Fact.setRawValue(other._param6Fact.rawValue());
    _param7Fact.setRawValue(other._param7Fact.rawValue());

    return *this;
}

void MissionItem::_connectSignals(void)
{
    // Connect to change signals to track dirty state
    connect(&_param1Fact,   &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(&_param2Fact,   &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(&_param3Fact,   &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(&_param4Fact,   &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(&_param5Fact,   &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(&_param6Fact,   &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(&_param7Fact,   &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(&_frameFact,    &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(&_commandFact,  &Fact::valueChanged,                    this, &MissionItem::_setDirtyFromSignal);
    connect(this,           &MissionItem::sequenceNumberChanged,    this, &MissionItem::_setDirtyFromSignal);

    // Values from these facts must propogate back and forth between the real object storage
    connect(&_supportedCommandFact,         &Fact::valueChanged,        this, &MissionItem::_syncSupportedCommandToCommand);
    connect(&_commandFact,                  &Fact::valueChanged,        this, &MissionItem::_syncCommandToSupportedCommand);
    connect(&_altitudeRelativeToHomeFact,   &Fact::valueChanged,        this, &MissionItem::_syncAltitudeRelativeToHomeToFrame);
    connect(this,                           &MissionItem::frameChanged, this, &MissionItem::_syncFrameToAltitudeRelativeToHome);

    // These are parameter coordinates, they must emit coordinateChanged signal
    connect(&_param5Fact, &Fact::valueChanged, this, &MissionItem::_sendCoordinateChanged);
    connect(&_param6Fact, &Fact::valueChanged, this, &MissionItem::_sendCoordinateChanged);
    connect(&_param7Fact, &Fact::valueChanged, this, &MissionItem::_sendCoordinateChanged);

    // The following changes may also change friendlyEditAllowed
    connect(&_autoContinueFact, &Fact::valueChanged,        this, &MissionItem::_sendFriendlyEditAllowedChanged);
    connect(&_commandFact,      &Fact::valueChanged,        this, &MissionItem::_sendFriendlyEditAllowedChanged);
    connect(&_frameFact,        &Fact::valueChanged,        this, &MissionItem::_sendFriendlyEditAllowedChanged);

    // When the command changes we need to set defaults. This must go out before the signals below so it must be registered first.
    connect(&_commandFact,  &Fact::valueChanged, this, &MissionItem::setDefaultsForCommand);

    // Whenever these properties change the ui model changes as well
    connect(this, &MissionItem::commandChanged, this, &MissionItem::_sendUiModelChanged);
    connect(this, &MissionItem::rawEditChanged, this, &MissionItem::_sendUiModelChanged);

    // These fact signals must alway signal out through MissionItem signals
    connect(&_commandFact,  &Fact::valueChanged, this, &MissionItem::_sendCommandChanged);
    connect(&_frameFact,    &Fact::valueChanged, this, &MissionItem::_sendFrameChanged);

}

bool MissionItem::_validateKeyTypes(QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types)
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

bool MissionItem::_loadMavCmdInfoJson(void)
{
    QFile jsonFile(":/json/MavCmdInfo.json");
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open MavCmdInfo.json" << jsonFile.errorString();
        return false;
    }

    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() <<  "Unable to open json document" << jsonParseError.errorString();
        return false;
    }

    QJsonObject json = doc.object();

    int version = json.value(_versionJsonKey).toInt();
    if (version != 1) {
        qWarning() << "Invalid version" << version;
        return false;
    }

    QJsonValue jsonValue = json.value(_mavCmdInfoJsonKey);
    if (!jsonValue.isArray()) {
        qWarning() << "mavCmdInfo not array";
        return false;
    }

    QJsonArray jsonArray = jsonValue.toArray();
    foreach(QJsonValue info, jsonArray) {
        if (!info.isObject()) {
            qWarning() << "mavCmdArray should contain objects";
            return false;
        }
        QJsonObject jsonObject = info.toObject();

        // Make sure we have the required keys
        QStringList requiredKeys;
        requiredKeys << _idJsonKey << _rawNameJsonKey;
        foreach (QString key, requiredKeys) {
            if (!jsonObject.contains(key)) {
                qWarning() << "Mission required key" << key;
                return false;
            }
        }

        // Validate key types

        QStringList             keys;
        QList<QJsonValue::Type> types;
        keys << _idJsonKey << _rawNameJsonKey << _friendlyNameJsonKey << _descriptionJsonKey << _specifiesCoordinateJsonKey << _friendlyEditJsonKey
             << _param1JsonKey << _param2JsonKey << _param3JsonKey << _param4JsonKey;
        types << QJsonValue::Double << QJsonValue::String << QJsonValue::String<< QJsonValue::String << QJsonValue::Bool << QJsonValue::Bool
              << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object << QJsonValue::Object;
        if (!_validateKeyTypes(jsonObject, keys, types)) {
            return false;
        }

        MavCmdInfo_t mavCmdInfo;

        mavCmdInfo.command = (MAV_CMD)      jsonObject.value(_idJsonKey).toInt();
        mavCmdInfo.rawName =                jsonObject.value(_rawNameJsonKey).toString();
        mavCmdInfo.friendlyName =           jsonObject.value(_friendlyNameJsonKey).toString(QString());
        mavCmdInfo.description =            jsonObject.value(_descriptionJsonKey).toString(QString());
        mavCmdInfo.specifiesCoordinate =    jsonObject.value(_specifiesCoordinateJsonKey).toBool(false);
        mavCmdInfo.friendlyEdit =           jsonObject.value(_friendlyEditJsonKey).toBool(false);

        if (_mavCmdInfoMap.contains(mavCmdInfo.command)) {
            qWarning() << "Duplicate command" << mavCmdInfo.command;
            return false;
        }

        _mavCmdInfoMap[mavCmdInfo.command] = mavCmdInfo;

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
                    return false;
                }

                if (paramObject.contains(_labelJsonKey)) {
                    _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].label = paramObject.value(_labelJsonKey).toString();
                } else {
                    qWarning() << "param object missing label key" << mavCmdInfo.rawName << paramKey;
                    return false;
                }

                _mavCmdInfoMap[mavCmdInfo.command].friendlyEdit = true; // Assume friendly edit if we have params

                _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].defaultValue =   paramObject.value(_defaultJsonKey).toDouble(0.0);
                _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].decimalPlaces =  paramObject.value(_decimalPlacesJsonKey).toInt(FactMetaData::defaultDecimalPlaces);
                _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].enumStrings =    paramObject.value(_enumStringsJsonKey).toString().split(",", QString::SkipEmptyParts);
                _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].param =          i;
                _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].units =          paramObject.value(_unitsJsonKey).toString();

                QStringList enumValues = paramObject.value(_enumValuesJsonKey).toString().split(",", QString::SkipEmptyParts);
                foreach (QString enumValue, enumValues) {
                    bool    convertOk;
                    double  value = enumValue.toDouble(&convertOk);

                    if (!convertOk) {
                        qWarning() << "Bad enumValue" << enumValue;
                        return false;
                    }

                    _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].enumValues << QVariant(value);
                }
                if (_mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].enumStrings.count() != _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].enumStrings.count()) {
                    qWarning() << "enum strings/values count mismatch" << _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].enumStrings.count() << _mavCmdInfoMap[mavCmdInfo.command].paramInfoMap[i].enumStrings.count();
                    return false;
                }
            }
        }

        if (mavCmdInfo.friendlyEdit) {
            if (mavCmdInfo.description.isEmpty()) {
                qWarning() << "Missing description" << mavCmdInfo.rawName;
                return false;
            }
            if (mavCmdInfo.rawName ==  mavCmdInfo.friendlyName) {
                qWarning() << "Missing friendly name" << mavCmdInfo.rawName << mavCmdInfo.friendlyName;
                return false;
            }
        }
    }

    return true;
}

void MissionItem::_setupMetaData(void)
{
    QStringList enumStrings;
    QVariantList enumValues;

    if (!_altitudeMetaData) {
        _loadMavCmdInfoJson();

        _altitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble);
        _altitudeMetaData->setUnits("meters");
        _altitudeMetaData->setDecimalPlaces(3);

        enumStrings.clear();
        enumValues.clear();
        foreach (MavCmdInfo_t mavCmdInfo, _mavCmdInfoMap) {
            enumStrings.append(mavCmdInfo.rawName);
            enumValues.append(QVariant(mavCmdInfo.command));
        }
        _commandMetaData = new FactMetaData(FactMetaData::valueTypeUint32);
        _commandMetaData->setEnumInfo(enumStrings, enumValues);

        _defaultParamMetaData = new FactMetaData(FactMetaData::valueTypeDouble);
        _defaultParamMetaData->setDecimalPlaces(7);

        enumStrings.clear();
        enumValues.clear();
        for (size_t i=0; i<sizeof(_rgMavFrameInfo)/sizeof(_rgMavFrameInfo[0]); i++) {
            const struct EnumInfo_s* mavFrameInfo = &_rgMavFrameInfo[i];

            enumStrings.append(mavFrameInfo->label);
            enumValues.append(QVariant(mavFrameInfo->frame));
        }
        _frameMetaData = new FactMetaData(FactMetaData::valueTypeUint32);
        _frameMetaData->setEnumInfo(enumStrings, enumValues);

        _latitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble);
        _latitudeMetaData->setUnits("deg");
        _latitudeMetaData->setDecimalPlaces(7);

        _longitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble);
        _longitudeMetaData->setUnits("deg");
        _longitudeMetaData->setDecimalPlaces(7);

        enumStrings.clear();
        enumValues.clear();
        // FIXME: Hack hardcode tp PX4
        QList<MAV_CMD> supportedCommands = qgcApp()->toolbox()->firmwarePluginManager()->firmwarePluginForAutopilot(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR)->supportedMissionCommands();
        if (supportedCommands.count()) {
            foreach (MAV_CMD command, supportedCommands) {
                enumStrings.append(_mavCmdInfoMap[command].friendlyName);
                enumValues.append(QVariant(command));
            }
        } else {
            foreach (MavCmdInfo_t mavCmdInfo, _mavCmdInfoMap) {
                enumStrings.append(mavCmdInfo.friendlyName);
                enumValues.append(QVariant(mavCmdInfo.command));
            }
        }
        _supportedCommandMetaData = new FactMetaData(FactMetaData::valueTypeUint32);
        _supportedCommandMetaData->setEnumInfo(enumStrings, enumValues);
    }

    _commandFact.setMetaData(_commandMetaData);
    _frameFact.setMetaData(_frameMetaData);
    _supportedCommandFact.setMetaData(_supportedCommandMetaData);
}

MissionItem::~MissionItem()
{    
}

void MissionItem::save(QTextStream &saveStream)
{
    // FORMAT: <INDEX> <CURRENT WP> <COORD FRAME> <COMMAND> <PARAM1> <PARAM2> <PARAM3> <PARAM4> <PARAM5/X/LONGITUDE> <PARAM6/Y/LATITUDE> <PARAM7/Z/ALTITUDE> <autoContinue> <DESCRIPTION>
    // as documented here: http://qgroundcontrol.org/waypoint_protocol
    saveStream << sequenceNumber() << "\t"
               << isCurrentItem() << "\t"
               << frame() << "\t"
               << command() << "\t"
               << QString("%1").arg(param1(), 0, 'g', 18) << "\t"
               << QString("%1").arg(param2(), 0, 'g', 18) << "\t"
               << QString("%1").arg(param3(), 0, 'g', 18) << "\t"
               << QString("%1").arg(param4(), 0, 'g', 18) << "\t"
               << QString("%1").arg(param5(), 0, 'g', 18) << "\t"
               << QString("%1").arg(param6(), 0, 'g', 18) << "\t"
               << QString("%1").arg(param7(), 0, 'g', 18) << "\t"
               << this->autoContinue() << "\r\n";
}

bool MissionItem::load(QTextStream &loadStream)
{
    const QStringList &wpParams = loadStream.readLine().split("\t");
    if (wpParams.size() == 12) {
        setSequenceNumber(wpParams[0].toInt());
        setIsCurrentItem(wpParams[1].toInt() == 1 ? true : false);
        setFrame((MAV_FRAME)wpParams[2].toInt());
        setCommand((MAV_CMD)wpParams[3].toInt());
        setParam1(wpParams[4].toDouble());
        setParam2(wpParams[5].toDouble());
        setParam3(wpParams[6].toDouble());
        setParam4(wpParams[7].toDouble());
        setParam5(wpParams[8].toDouble());
        setParam6(wpParams[9].toDouble());
        setParam7(wpParams[10].toDouble());
        setAutoContinue(wpParams[11].toInt() == 1 ? true : false);
        return true;
    }
    return false;
}


void MissionItem::setSequenceNumber(int sequenceNumber)
{
    _sequenceNumber = sequenceNumber;
    emit sequenceNumberChanged(_sequenceNumber);
}

void MissionItem::setCommand(MAV_CMD command)
{
    if ((MAV_CMD)this->command() != command) {
        _commandFact.setRawValue(command);
        setDefaultsForCommand();
        emit commandChanged(this->command());
    }
}

void MissionItem::setCommand(MavlinkQmlSingleton::Qml_MAV_CMD command)
{
    setCommand((MAV_CMD)command);
}


void MissionItem::setFrame(MAV_FRAME frame)
{
    if (this->frame() != frame) {
        _frameFact.setRawValue(frame);
        frameChanged(frame);
    }
}

void MissionItem::setAutoContinue(bool autoContinue)
{
    if (this->autoContinue() != autoContinue) {
        _autoContinueFact.setRawValue(autoContinue);
    }
}

void MissionItem::setIsCurrentItem(bool isCurrentItem)
{
    if (_isCurrentItem != isCurrentItem) {
        _isCurrentItem = isCurrentItem;
        emit isCurrentItemChanged(isCurrentItem);
    }
}

void MissionItem::setParam1(double param)
{
    if (param1() != param) {
        _param1Fact.setRawValue(param);
    }
}

void MissionItem::setParam2(double param)
{
    if (param2() != param) {
        _param2Fact.setRawValue(param);
    }
}

void MissionItem::setParam3(double param)
{
    if (param3() != param) {
        _param3Fact.setRawValue(param);
    }
}

void MissionItem::setParam4(double param)
{
    if (param4() != param) {
        _param4Fact.setRawValue(param);
    }
}

void MissionItem::setParam5(double param)
{
    if (param5() != param) {
        _param5Fact.setRawValue(param);
    }
}

void MissionItem::setParam6(double param)
{
    if (param6() != param) {
        _param6Fact.setRawValue(param);
    }
}

void MissionItem::setParam7(double param)
{
    if (param7() != param) {
        _param7Fact.setRawValue(param);
    }
}

bool MissionItem::specifiesCoordinate(void) const
{
    return _mavCmdInfoMap[(MAV_CMD)command()].specifiesCoordinate;
}

QString MissionItem::commandDescription(void) const
{
    return _mavCmdInfoMap[(MAV_CMD)command()].description;
}

void MissionItem::_clearParamMetaData(void)
{
    _param1MetaData.setUnits("");
    _param1MetaData.setDecimalPlaces(FactMetaData::defaultDecimalPlaces);
    _param1MetaData.setTranslators(FactMetaData::defaultTranslator, FactMetaData::defaultTranslator);
    _param2MetaData.setUnits("");
    _param2MetaData.setDecimalPlaces(FactMetaData::defaultDecimalPlaces);
    _param2MetaData.setTranslators(FactMetaData::defaultTranslator, FactMetaData::defaultTranslator);
    _param3MetaData.setUnits("");
    _param3MetaData.setDecimalPlaces(FactMetaData::defaultDecimalPlaces);
    _param3MetaData.setTranslators(FactMetaData::defaultTranslator, FactMetaData::defaultTranslator);
    _param4MetaData.setUnits("");
    _param4MetaData.setDecimalPlaces(FactMetaData::defaultDecimalPlaces);
    _param4MetaData.setTranslators(FactMetaData::defaultTranslator, FactMetaData::defaultTranslator);
}

QmlObjectListModel* MissionItem::textFieldFacts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);
    
    if (rawEdit()) {
        _param1Fact._setName("Param1:");
        _param1Fact.setMetaData(_defaultParamMetaData);
        model->append(&_param1Fact);
        _param2Fact._setName("Param2:");
        _param2Fact.setMetaData(_defaultParamMetaData);
        model->append(&_param2Fact);
        _param3Fact._setName("Param3:");
        _param3Fact.setMetaData(_defaultParamMetaData);
        model->append(&_param3Fact);
        _param4Fact._setName("Param4:");
        _param4Fact.setMetaData(_defaultParamMetaData);
        model->append(&_param4Fact);
        _param5Fact._setName("Lat/X:");
        _param5Fact.setMetaData(_defaultParamMetaData);
        model->append(&_param5Fact);
        _param6Fact._setName("Lon/Y:");
        _param6Fact.setMetaData(_defaultParamMetaData);
        model->append(&_param6Fact);
        _param7Fact._setName("Alt/Z:");
        _param7Fact.setMetaData(_defaultParamMetaData);
        model->append(&_param7Fact);
    } else {
        _clearParamMetaData();

        MAV_CMD command = (MAV_CMD)this->command();

        Fact*           rgParamFacts[7] =       { &_param1Fact, &_param2Fact, &_param3Fact, &_param4Fact, &_param5Fact, &_param6Fact, &_param7Fact };
        FactMetaData*   rgParamMetaData[7] =    { &_param1MetaData, &_param2MetaData, &_param3MetaData, &_param4MetaData, &_param5MetaData, &_param6MetaData, &_param7MetaData };

        for (int i=1; i<=7; i++) {
            if (_mavCmdInfoMap[command].paramInfoMap.contains(i) && _mavCmdInfoMap[command].paramInfoMap[i].enumStrings.count() == 0) {
                Fact*           paramFact =     rgParamFacts[i-1];
                FactMetaData*   paramMetaData = rgParamMetaData[i-1];

                paramFact->_setName(_mavCmdInfoMap[command].paramInfoMap[i].label);
                paramMetaData->setDecimalPlaces(_mavCmdInfoMap[command].paramInfoMap[i].decimalPlaces);
                paramMetaData->setEnumInfo(_mavCmdInfoMap[command].paramInfoMap[i].enumStrings, _mavCmdInfoMap[command].paramInfoMap[i].enumValues);
                if (_mavCmdInfoMap[command].paramInfoMap[i].units == _degreesConvertUnits) {
                    paramMetaData->setTranslators(_radiansToDegrees, _degreesToRadians);
                    paramMetaData->setUnits(_degreesUnits);
                } else {
                    paramMetaData->setUnits(_mavCmdInfoMap[command].paramInfoMap[i].units);
                }
                paramFact->setMetaData(paramMetaData);
                model->append(paramFact);
            }
        }

        if (specifiesCoordinate()) {
            _param7Fact._setName("Altitude:");
            _param7Fact.setMetaData(_altitudeMetaData);
            model->append(&_param7Fact);
        }
    }
    
    return model;
}

QmlObjectListModel* MissionItem::checkboxFacts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);
    

    if (rawEdit()) {
        model->append(&_autoContinueFact);
    } else if (specifiesCoordinate()) {
        model->append(&_altitudeRelativeToHomeFact);
    }

    return model;
}

QmlObjectListModel* MissionItem::comboboxFacts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);

    if (rawEdit()) {
        model->append(&_commandFact);
        model->append(&_frameFact);
    } else {
        Fact*           rgParamFacts[7] =       { &_param1Fact, &_param2Fact, &_param3Fact, &_param4Fact, &_param5Fact, &_param6Fact, &_param7Fact };
        FactMetaData*   rgParamMetaData[7] =    { &_param1MetaData, &_param2MetaData, &_param3MetaData, &_param4MetaData, &_param5MetaData, &_param6MetaData, &_param7MetaData };

        MAV_CMD command = (MAV_CMD)this->command();

        for (int i=1; i<=7; i++) {
            if (_mavCmdInfoMap[command].paramInfoMap.contains(i) && _mavCmdInfoMap[command].paramInfoMap[i].enumStrings.count()) {
                Fact*           paramFact =     rgParamFacts[i-1];
                FactMetaData*   paramMetaData = rgParamMetaData[i-1];

                paramFact->_setName(_mavCmdInfoMap[command].paramInfoMap[i].label);
                paramMetaData->setDecimalPlaces(_mavCmdInfoMap[command].paramInfoMap[i].decimalPlaces);
                paramMetaData->setEnumInfo(_mavCmdInfoMap[command].paramInfoMap[i].enumStrings, _mavCmdInfoMap[command].paramInfoMap[i].enumValues);
                if (_mavCmdInfoMap[command].paramInfoMap[i].units == _degreesConvertUnits) {
                    paramMetaData->setTranslators(_radiansToDegrees, _degreesToRadians);
                    paramMetaData->setUnits(_degreesUnits);
                } else {
                    paramMetaData->setUnits(_mavCmdInfoMap[command].paramInfoMap[i].units);
                }
                paramFact->setMetaData(paramMetaData);
                model->append(paramFact);
            }
        }
    }

    return model;
}

QGeoCoordinate MissionItem::coordinate(void) const
{
    return QGeoCoordinate(_param5Fact.rawValue().toDouble(), _param6Fact.rawValue().toDouble(), _param7Fact.rawValue().toDouble());
}

void MissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    setParam5(coordinate.latitude());
    setParam6(coordinate.longitude());
    setParam7(coordinate.altitude());
}

bool MissionItem::friendlyEditAllowed(void) const
{
    if (_mavCmdInfoMap[(MAV_CMD)command()].friendlyEdit) {
        if (!autoContinue()) {
            return false;
        }

        if (specifiesCoordinate()) {
            return frame() == MAV_FRAME_GLOBAL || frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT;
        } else {
            return frame() == MAV_FRAME_MISSION;
        }
    }

    return false;
}

bool MissionItem::rawEdit(void) const
{
    return _rawEdit || !friendlyEditAllowed();
}

void MissionItem::setRawEdit(bool rawEdit)
{
    if (this->rawEdit() != rawEdit) {
        _rawEdit = rawEdit;
        emit rawEditChanged(this->rawEdit());
    }
}

void MissionItem::setDirty(bool dirty)
{
    if (!_homePositionSpecialCase || !dirty) {
        // Home position never affects dirty bit

        _dirty = dirty;
        // We want to emit dirtyChanged even if _dirty didn't change. This can be handy signal for
        // any value within the item changing.
        emit dirtyChanged(_dirty);
    }
}

void MissionItem::_setDirtyFromSignal(void)
{
    setDirty(true);
}

void MissionItem::setHomePositionValid(bool homePositionValid)
{
    _homePositionValid = homePositionValid;
    emit homePositionValidChanged(_homePositionValid);
}

void MissionItem::setDistance(double distance)
{
    _distance = distance;
    emit distanceChanged(_distance);
}

void MissionItem::setAzimuth(double azimuth)
{
    _azimuth = azimuth;
    emit azimuthChanged(_azimuth);
}

void MissionItem::_sendCoordinateChanged(void)
{
    emit coordinateChanged(coordinate());
}

void MissionItem::_syncAltitudeRelativeToHomeToFrame(const QVariant& value)
{
    if (!_syncingAltitudeRelativeToHomeAndFrame) {
        _syncingAltitudeRelativeToHomeAndFrame = true;
        setFrame(value.toBool() ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL);
        _syncingAltitudeRelativeToHomeAndFrame = false;
    }
}

void MissionItem::_syncFrameToAltitudeRelativeToHome(void)
{
    if (!_syncingAltitudeRelativeToHomeAndFrame) {
        _syncingAltitudeRelativeToHomeAndFrame = true;
        _altitudeRelativeToHomeFact.setRawValue(relativeAltitude());
        _syncingAltitudeRelativeToHomeAndFrame = false;
    }
}

void MissionItem::_syncSupportedCommandToCommand(const QVariant& value)
{
    if (!_syncingSupportedCommandAndCommand) {
        _syncingSupportedCommandAndCommand = true;
        _commandFact.setRawValue(value.toInt());
        _syncingSupportedCommandAndCommand = false;
    }
}

void MissionItem::_syncCommandToSupportedCommand(const QVariant& value)
{
    if (!_syncingSupportedCommandAndCommand) {
        _syncingSupportedCommandAndCommand = true;
        _supportedCommandFact.setRawValue(value.toInt());
        _syncingSupportedCommandAndCommand = false;
    }
}

void MissionItem::setDefaultsForCommand(void)
{
    // We set these global default first, then if there are param defaults they will get reset
    setParam2(defaultAcceptanceRadius);
    setParam7(defaultAltitude);

    foreach (ParamInfo_t paramInfo, _mavCmdInfoMap[(MAV_CMD)command()].paramInfoMap) {
        Fact* rgParamFacts[7] = { &_param1Fact, &_param2Fact, &_param3Fact, &_param4Fact, &_param5Fact, &_param6Fact, &_param7Fact };

        rgParamFacts[paramInfo.param-1]->setRawValue(paramInfo.defaultValue);
    }

    setAutoContinue(true);
    setFrame(specifiesCoordinate() ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_MISSION);
    setRawEdit(false);
}

void MissionItem::_sendUiModelChanged(void)
{
    emit uiModelChanged();
}

void MissionItem::_sendFrameChanged(void)
{
    emit frameChanged(frame());
}

void MissionItem::_sendCommandChanged(void)
{
    emit commandChanged(command());
}

QString MissionItem::commandName(void) const
{
    MavCmdInfo_t& mavCmdInfo = _mavCmdInfoMap[(MAV_CMD)command()];

    return mavCmdInfo.friendlyName.isEmpty() ? mavCmdInfo.rawName : mavCmdInfo.friendlyName;
}

QVariant MissionItem::_degreesToRadians(const QVariant& degrees)
{
    return QVariant(degrees.toDouble() * (M_PI / 180.0));
}

QVariant MissionItem::_radiansToDegrees(const QVariant& radians)
{
    return QVariant(radians.toDouble() * (180 / M_PI));
}

void MissionItem::_sendFriendlyEditAllowedChanged(void)
{
    emit friendlyEditAllowedChanged(friendlyEditAllowed());
}
