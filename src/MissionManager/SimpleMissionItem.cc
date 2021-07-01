/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QStringList>
#include <QDebug>

#include "SimpleMissionItem.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "JsonHelper.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"
#include "PlanMasterController.h"

FactMetaData* SimpleMissionItem::_altitudeMetaData =        nullptr;
FactMetaData* SimpleMissionItem::_commandMetaData =         nullptr;
FactMetaData* SimpleMissionItem::_defaultParamMetaData =    nullptr;
FactMetaData* SimpleMissionItem::_frameMetaData =           nullptr;
FactMetaData* SimpleMissionItem::_latitudeMetaData =        nullptr;
FactMetaData* SimpleMissionItem::_longitudeMetaData =       nullptr;

const char* SimpleMissionItem::_jsonAltitudeModeKey =           "AltitudeMode";
const char* SimpleMissionItem::_jsonAltitudeKey =               "Altitude";
const char* SimpleMissionItem::_jsonAMSLAltAboveTerrainKey =    "AMSLAltAboveTerrain";

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

SimpleMissionItem::SimpleMissionItem(PlanMasterController* masterController, bool flyView, bool forLoad)
    : VisualMissionItem                 (masterController, flyView)
    , _commandTree                      (qgcApp()->toolbox()->missionCommandTree())
    , _supportedCommandFact             (0, "Command:",             FactMetaData::valueTypeUint32)
    , _altitudeFact                     (0, "Altitude",             FactMetaData::valueTypeDouble)
    , _amslAltAboveTerrainFact          (0, "Alt above terrain",    FactMetaData::valueTypeDouble)
    , _param1MetaData                   (FactMetaData::valueTypeDouble)
    , _param2MetaData                   (FactMetaData::valueTypeDouble)
    , _param3MetaData                   (FactMetaData::valueTypeDouble)
    , _param4MetaData                   (FactMetaData::valueTypeDouble)
    , _param5MetaData                   (FactMetaData::valueTypeDouble)
    , _param6MetaData                   (FactMetaData::valueTypeDouble)
    , _param7MetaData                   (FactMetaData::valueTypeDouble)
{
    _editorQml = QStringLiteral("qrc:/qml/SimpleItemEditor.qml");

    _setupMetaData();

    if (!forLoad) {
        // We are are going to load the SimpleItem right after this then don't connnect up signalling until after load is done
        _connectSignals();
        _updateOptionalSections();
        _setDefaultsForCommand();
        _rebuildFacts();
        setDirty(false);
    }
}

SimpleMissionItem::SimpleMissionItem(PlanMasterController* masterController, bool flyView, const MissionItem& missionItem)
    : VisualMissionItem         (masterController, flyView)
    , _missionItem              (missionItem)
    , _commandTree              (qgcApp()->toolbox()->missionCommandTree())
    , _supportedCommandFact     (0,         "Command:",             FactMetaData::valueTypeUint32)
    , _altitudeFact             (0,         "Altitude",             FactMetaData::valueTypeDouble)
    , _amslAltAboveTerrainFact  (0,         "Alt above terrain",    FactMetaData::valueTypeDouble)
    , _param1MetaData           (FactMetaData::valueTypeDouble)
    , _param2MetaData           (FactMetaData::valueTypeDouble)
    , _param3MetaData           (FactMetaData::valueTypeDouble)
    , _param4MetaData           (FactMetaData::valueTypeDouble)
    , _param5MetaData           (FactMetaData::valueTypeDouble)
    , _param6MetaData           (FactMetaData::valueTypeDouble)
    , _param7MetaData           (FactMetaData::valueTypeDouble)
{
    _editorQml = QStringLiteral("qrc:/qml/SimpleItemEditor.qml");

    struct MavFrame2AltMode_s {
        MAV_FRAME                               mavFrame;
        QGroundControlQmlGlobal::AltMode   altMode;
    };

    const struct MavFrame2AltMode_s rgMavFrame2AltMode[] = {
        { MAV_FRAME_GLOBAL_TERRAIN_ALT,     QGroundControlQmlGlobal::AltitudeModeTerrainFrame },
        { MAV_FRAME_GLOBAL,                 QGroundControlQmlGlobal::AltitudeModeAbsolute },
        { MAV_FRAME_GLOBAL_RELATIVE_ALT,    QGroundControlQmlGlobal::AltitudeModeRelative },
    };
    _altitudeMode = QGroundControlQmlGlobal::AltitudeModeRelative;
    for (size_t i=0; i<sizeof(rgMavFrame2AltMode)/sizeof(rgMavFrame2AltMode[0]); i++) {
        const MavFrame2AltMode_s& pMavFrame2AltMode = rgMavFrame2AltMode[i];
        if (pMavFrame2AltMode.mavFrame == missionItem.frame()) {
            _altitudeMode = pMavFrame2AltMode.altMode;
            break;
        }
    }

    _isCurrentItem = missionItem.isCurrentItem();
    _altitudeFact.setRawValue(specifiesAltitude() ? _missionItem._param7Fact.rawValue() : qQNaN());
    _amslAltAboveTerrainFact.setRawValue(qQNaN());

    // In flyView we skip some of the intialization to save memory
    if (!_flyView) {
        _setupMetaData();
    }
    _connectSignals();
    _updateOptionalSections();
    if (!_flyView) {
        _rebuildFacts();
    }

    // Signal coordinate changed to kick off terrain query
    emit coordinateChanged(coordinate());

    setDirty(false);
}

void SimpleMissionItem::_connectSignals(void)
{
    // Connect to change signals to track dirty state
    connect(&_missionItem._param1Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem._param2Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem._param3Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem._param4Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem._param5Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem._param6Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem._param7Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem._frameFact,           &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::_setDirty);
    connect(&_missionItem,                      &MissionItem::sequenceNumberChanged,        this, &SimpleMissionItem::_setDirty);
    connect(this,                               &SimpleMissionItem::altitudeModeChanged,    this, &SimpleMissionItem::_setDirty);

    connect(&_altitudeFact,                     &Fact::valueChanged,                        this, &SimpleMissionItem::_altitudeChanged);
    connect(this,                               &SimpleMissionItem::altitudeModeChanged,    this, &SimpleMissionItem::_altitudeModeChanged);
    connect(this,                               &SimpleMissionItem::terrainAltitudeChanged, this, &SimpleMissionItem::_terrainAltChanged);

    connect(this,                               &SimpleMissionItem::sequenceNumberChanged,  this, &SimpleMissionItem::lastSequenceNumberChanged);
    connect(this,                               &SimpleMissionItem::cameraSectionChanged,   this, &SimpleMissionItem::_setDirty);
    connect(this,                               &SimpleMissionItem::cameraSectionChanged,   this, &SimpleMissionItem::_updateLastSequenceNumber);

    connect(&_missionItem._param7Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_amslEntryAltChanged);
    connect(this,                               &SimpleMissionItem::altitudeModeChanged,    this, &SimpleMissionItem::_amslEntryAltChanged);
    connect(this,                               &SimpleMissionItem::terrainAltitudeChanged, this, &SimpleMissionItem::_amslEntryAltChanged);
    connect(this,                               &SimpleMissionItem::amslEntryAltChanged,    this, &SimpleMissionItem::amslExitAltChanged);

    connect(this, &SimpleMissionItem::wizardModeChanged,                                    this, &SimpleMissionItem::readyForSaveStateChanged);

    // These are coordinate lat/lon values, they must emit coordinateChanged signal
    connect(&_missionItem._param5Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_sendCoordinateChanged);
    connect(&_missionItem._param6Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_sendCoordinateChanged);

    connect(&_missionItem._param1Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_possibleAdditionalTimeDelayChanged);
    connect(&_missionItem._param4Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_possibleVehicleYawChanged);

    // For NAV_LOITER_X commands, they must emit a radiusChanged signal
    connect(&_missionItem._param2Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_possibleRadiusChanged);
    connect(&_missionItem._param3Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_possibleRadiusChanged);
    
    // Exit coordinate is the same as entrance coordinate
    connect(this,                               &SimpleMissionItem::coordinateChanged,      this, &SimpleMissionItem::exitCoordinateChanged);

    // The following changes may also change friendlyEditAllowed
    connect(&_missionItem._autoContinueFact,    &Fact::valueChanged,                        this, &SimpleMissionItem::_sendFriendlyEditAllowedChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::_sendFriendlyEditAllowedChanged);
    connect(&_missionItem._frameFact,           &Fact::valueChanged,                        this, &SimpleMissionItem::_sendFriendlyEditAllowedChanged);

    // A command change triggers a number of other changes as well.
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::_setDefaultsForCommand);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::commandNameChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::commandDescriptionChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::abbreviationChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::specifiesCoordinateChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::specifiesAltitudeOnlyChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::isStandaloneCoordinateChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::isLandCommandChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::isLoiterItemChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::showLoiterRadiusChanged);

    // Whenever these properties change the ui model changes as well
    connect(this,                               &SimpleMissionItem::commandChanged,         this, &SimpleMissionItem::_rebuildFacts);
    connect(this,                               &SimpleMissionItem::rawEditChanged,         this, &SimpleMissionItem::_rebuildFacts);
    connect(this,                               &SimpleMissionItem::previousVTOLModeChanged,this, &SimpleMissionItem::_rebuildFacts);

    // The following changes must signal currentVTOLModeChanged to cause a MissionController recalc
    connect(this,                               &SimpleMissionItem::commandChanged,         this, &SimpleMissionItem::_signalIfVTOLTransitionCommand);
    connect(&_missionItem._param1Fact,          &Fact::valueChanged,                        this, &SimpleMissionItem::_signalIfVTOLTransitionCommand);

    // These fact signals must alway signal out through SimpleMissionItem signals
    connect(&_missionItem._commandFact,         &Fact::valueChanged,                        this, &SimpleMissionItem::_sendCommandChanged);

    // Propogate signals from MissionItem up to SimpleMissionItem
    connect(&_missionItem,                      &MissionItem::sequenceNumberChanged,        this, &SimpleMissionItem::sequenceNumberChanged);
    connect(&_missionItem,                      &MissionItem::specifiedFlightSpeedChanged,  this, &SimpleMissionItem::specifiedFlightSpeedChanged);

    connect(_missionController,                 &MissionController::plannedHomePositionChanged, this, &SimpleMissionItem::_amslEntryAltChanged);
    connect(_missionController,                 &MissionController::plannedHomePositionChanged, this, &SimpleMissionItem::_amslExitAltChanged);
}

void SimpleMissionItem::_setupMetaData(void)
{
    QStringList enumStrings;
    QVariantList enumValues;

    if (!_altitudeMetaData) {
        _altitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble);
        _altitudeMetaData->setRawUnits("m");
        _altitudeMetaData->setRawIncrement(1);
        _altitudeMetaData->setDecimalPlaces(2);

        enumStrings.clear();
        enumValues.clear();
        MissionCommandTree* commandTree = qgcApp()->toolbox()->missionCommandTree();
        for (const MAV_CMD command: commandTree->allCommandIds()) {
            enumStrings.append(commandTree->rawName(command));
            enumValues.append(QVariant((int)command));
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
        _latitudeMetaData->setRawUnits("deg");
        _latitudeMetaData->setDecimalPlaces(7);

        _longitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble);
        _longitudeMetaData->setRawUnits("deg");
        _longitudeMetaData->setDecimalPlaces(7);

    }

    _missionItem._commandFact.setMetaData(_commandMetaData);
    _missionItem._frameFact.setMetaData(_frameMetaData);
    _altitudeFact.setMetaData(_altitudeMetaData);
    _amslAltAboveTerrainFact.setMetaData(_altitudeMetaData);
}

SimpleMissionItem::~SimpleMissionItem()
{    
}

void SimpleMissionItem::save(QJsonArray&  missionItems)
{
    QList<MissionItem*> items;

    appendMissionItems(items, this);

    for (int i=0; i<items.count(); i++) {
        MissionItem* item = items[i];
        QJsonObject saveObject;
        item->save(saveObject);
        if (i == 0) {
            // This is the main simple item, save the alt/terrain data
            if (specifiesAltitude()) {
                saveObject[_jsonAltitudeModeKey] =          _altitudeMode;
                saveObject[_jsonAltitudeKey] =              _altitudeFact.rawValue().toDouble();
                saveObject[_jsonAMSLAltAboveTerrainKey] =   _amslAltAboveTerrainFact.rawValue().toDouble();
            }
        }
        missionItems.append(saveObject);
        item->deleteLater();
    }
}

bool SimpleMissionItem::load(QTextStream &loadStream)
{
    bool success;
    if ((success = _missionItem.load(loadStream))) {
        if (specifiesAltitude()) {
            _altitudeMode = _missionItem.relativeAltitude() ? QGroundControlQmlGlobal::AltitudeModeRelative : QGroundControlQmlGlobal::AltitudeModeAbsolute;
            _altitudeFact.setRawValue(_missionItem._param7Fact.rawValue());
            _amslAltAboveTerrainFact.setRawValue(qQNaN());
        }
        _connectSignals();
        _updateOptionalSections();
        _rebuildFacts();
        setDirty(false);
    }

    return success;
}

bool SimpleMissionItem::load(const QJsonObject& json, int sequenceNumber, QString& errorString)
{
    if (!_missionItem.load(json, sequenceNumber, errorString)) {
        return false;
    }

    if (specifiesAltitude()) {
        if (json.contains(_jsonAltitudeModeKey) || json.contains(_jsonAltitudeKey) || json.contains(_jsonAMSLAltAboveTerrainKey)) {
            QList<JsonHelper::KeyValidateInfo> keyInfoList = {
                { _jsonAltitudeModeKey,         QJsonValue::Double, true },
                { _jsonAltitudeKey,             QJsonValue::Double, true },
                { _jsonAMSLAltAboveTerrainKey,  QJsonValue::Double, true },
            };
            if (!JsonHelper::validateKeys(json, keyInfoList, errorString)) {
                return false;
            }

            _altitudeMode = (QGroundControlQmlGlobal::AltMode)(int)json[_jsonAltitudeModeKey].toDouble();
            _altitudeFact.setRawValue(JsonHelper::possibleNaNJsonValue(json[_jsonAltitudeKey]));
            _amslAltAboveTerrainFact.setRawValue(JsonHelper::possibleNaNJsonValue(json[_jsonAltitudeKey]));
        } else {
            _altitudeMode = _missionItem.relativeAltitude() ? QGroundControlQmlGlobal::AltitudeModeRelative : QGroundControlQmlGlobal::AltitudeModeAbsolute;
            _altitudeFact.setRawValue(_missionItem._param7Fact.rawValue());
            _amslAltAboveTerrainFact.setRawValue(qQNaN());
        }
    }

    _connectSignals();
    _updateOptionalSections();
    _rebuildFacts();
    setDirty(false);

    return true;
}

bool SimpleMissionItem::isStandaloneCoordinate(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->isStandaloneCoordinate();
    } else {
        return false;
    }
}

bool SimpleMissionItem::specifiesCoordinate(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->specifiesCoordinate();
    } else {
        return false;
    }
}

bool SimpleMissionItem::specifiesAltitudeOnly(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->specifiesAltitudeOnly();
    } else {
        return false;
    }
}

QString SimpleMissionItem::commandDescription(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->description();
    } else {
        qWarning() << "Should not ask for command description on unknown command";
        return commandName();
    }
}

QString SimpleMissionItem::commandName(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->friendlyName();
    } else {
        qWarning() << "Request for command name on unknown command";
        return tr("Unknown: %1").arg(command());
    }
}

QString SimpleMissionItem::abbreviation() const
{
    if (homePosition())
        return tr("L");

    switch(command()) {
    case MAV_CMD_NAV_TAKEOFF:
        return tr("Takeoff");
    case MAV_CMD_NAV_LAND:
        return tr("Land");
    case MAV_CMD_NAV_VTOL_TAKEOFF:
        return tr("Transition Direction");
    case MAV_CMD_NAV_VTOL_LAND:
        return tr("VTOL Land");
    case MAV_CMD_DO_SET_ROI:
    case MAV_CMD_DO_SET_ROI_LOCATION:
        return tr("ROI");
    case MAV_CMD_NAV_LOITER_TIME:
    case MAV_CMD_NAV_LOITER_TURNS:
    case MAV_CMD_NAV_LOITER_UNLIM:
    case MAV_CMD_NAV_LOITER_TO_ALT:
        return tr("Loiter");
    default:
        return QString();
    }
}

void SimpleMissionItem::_rebuildTextFieldFacts(void)
{
    _textFieldFacts.clear();
    
    if (rawEdit()) {
        _missionItem._param1Fact._setName("Param1");
        _missionItem._param1Fact.setMetaData(_defaultParamMetaData);
        _textFieldFacts.append(&_missionItem._param1Fact);
        _missionItem._param2Fact._setName("Param2");
        _missionItem._param2Fact.setMetaData(_defaultParamMetaData);
        _textFieldFacts.append(&_missionItem._param2Fact);
        _missionItem._param3Fact._setName("Param3");
        _missionItem._param3Fact.setMetaData(_defaultParamMetaData);
        _textFieldFacts.append(&_missionItem._param3Fact);
        _missionItem._param4Fact._setName("Param4");
        _missionItem._param4Fact.setMetaData(_defaultParamMetaData);
        _textFieldFacts.append(&_missionItem._param4Fact);
        _missionItem._param5Fact._setName("Lat/X");
        _missionItem._param5Fact.setMetaData(_defaultParamMetaData);
        _textFieldFacts.append(&_missionItem._param5Fact);
        _missionItem._param6Fact._setName("Lon/Y");
        _missionItem._param6Fact.setMetaData(_defaultParamMetaData);
        _textFieldFacts.append(&_missionItem._param6Fact);
        _missionItem._param7Fact._setName("Alt/Z");
        _missionItem._param7Fact.setMetaData(_defaultParamMetaData);
        _textFieldFacts.append(&_missionItem._param7Fact);
    } else {
        _ignoreDirtyChangeSignals = true;

        MAV_CMD command;
        if (_homePositionSpecialCase) {
            command = MAV_CMD_NAV_LAST;
        } else {
            command = _missionItem.command();
        }

        Fact*           rgParamFacts[7] =       { &_missionItem._param1Fact, &_missionItem._param2Fact, &_missionItem._param3Fact, &_missionItem._param4Fact, &_missionItem._param5Fact, &_missionItem._param6Fact, &_missionItem._param7Fact };
        FactMetaData*   rgParamMetaData[7] =    { &_param1MetaData, &_param2MetaData, &_param3MetaData, &_param4MetaData, &_param5MetaData, &_param6MetaData, &_param7MetaData };

        const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, command);

        if (uiInfo) {
            for (int i=1; i<=7; i++) {
                bool showUI;
                const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(i, showUI);

                if (showUI && paramInfo && paramInfo->enumStrings().count() == 0 && !paramInfo->nanUnchanged()) {
                    Fact*               paramFact =     rgParamFacts[i-1];
                    FactMetaData*       paramMetaData = rgParamMetaData[i-1];

                    paramFact->_setName(paramInfo->label());
                    paramMetaData->setDecimalPlaces(paramInfo->decimalPlaces());
                    paramMetaData->setRawUnits(paramInfo->units());
                    paramFact->setMetaData(paramMetaData);
                    _textFieldFacts.append(paramFact);
                }
            }
        }

        _ignoreDirtyChangeSignals = false;
    }
}

void SimpleMissionItem::_rebuildNaNFacts(void)
{
    _nanFacts.clear();

    if (!rawEdit()) {
        _ignoreDirtyChangeSignals = true;

        MAV_CMD command;
        if (_homePositionSpecialCase) {
            command = MAV_CMD_NAV_LAST;
        } else {
            command = _missionItem.command();
        }

        Fact*           rgParamFacts[7] =       { &_missionItem._param1Fact, &_missionItem._param2Fact, &_missionItem._param3Fact, &_missionItem._param4Fact, &_missionItem._param5Fact, &_missionItem._param6Fact, &_missionItem._param7Fact };
        FactMetaData*   rgParamMetaData[7] =    { &_param1MetaData, &_param2MetaData, &_param3MetaData, &_param4MetaData, &_param5MetaData, &_param6MetaData, &_param7MetaData };

        const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, command);

        if (uiInfo) {
            for (int i=1; i<=7; i++) {
                bool showUI;
                const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(i, showUI);

                if (showUI && paramInfo && paramInfo->nanUnchanged()) {
                    // Show hide Heading field on waypoint based on vehicle yaw to next waypoint setting. This needs to come from the actual vehicle if it exists
                    // and not _controllerVehicle which is always offline.
                    Vehicle* firmwareVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
                    if (!firmwareVehicle) {
                        firmwareVehicle = _controllerVehicle;
                    }

                    Fact*               paramFact =     rgParamFacts[i-1];
                    FactMetaData*       paramMetaData = rgParamMetaData[i-1];

                    paramFact->_setName(paramInfo->label());
                    paramMetaData->setDecimalPlaces(paramInfo->decimalPlaces());
                    paramMetaData->setRawUnits(paramInfo->units());
                    paramFact->setMetaData(paramMetaData);
                    _nanFacts.append(paramFact);
                }
            }
        }

        _ignoreDirtyChangeSignals = false;
    }
}

bool SimpleMissionItem::specifiesAltitude(void) const
{
    return specifiesCoordinate() || specifiesAltitudeOnly();
}

bool SimpleMissionItem::isLoiterItem() const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->isLoiterCommand();
    } else {
        return false;
    }
}

bool SimpleMissionItem::showLoiterRadius() const
{
    return specifiesCoordinate() && (_controllerVehicle->fixedWing() || _controllerVehicle->vtol()) && isLoiterItem();
}

double SimpleMissionItem::loiterRadius() const
{
    if (isLoiterItem()) {
        return command() == MAV_CMD_NAV_LOITER_TO_ALT ? missionItem().param2() : missionItem().param3();
    } else {
        return qQNaN();
    }
}

void SimpleMissionItem::_rebuildComboBoxFacts(void)
{
    _comboboxFacts.clear();

    if (rawEdit()) {
        _comboboxFacts.append(&_missionItem._commandFact);
        _comboboxFacts.append(&_missionItem._frameFact);
    } else {
        Fact*           rgParamFacts[7] =       { &_missionItem._param1Fact, &_missionItem._param2Fact, &_missionItem._param3Fact, &_missionItem._param4Fact, &_missionItem._param5Fact, &_missionItem._param6Fact, &_missionItem._param7Fact };
        FactMetaData*   rgParamMetaData[7] =    { &_param1MetaData, &_param2MetaData, &_param3MetaData, &_param4MetaData, &_param5MetaData, &_param6MetaData, &_param7MetaData };

        MAV_CMD command;
        if (_homePositionSpecialCase) {
            command = MAV_CMD_NAV_LAST;
        } else {
            command = (MAV_CMD)this->command();
        }

        for (int i=1; i<=7; i++) {
            bool showUI;
            const MissionCmdParamInfo* paramInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, command)->getParamInfo(i, showUI);

            if (showUI && paramInfo && paramInfo->enumStrings().count() != 0) {
                Fact*               paramFact =     rgParamFacts[i-1];
                FactMetaData*       paramMetaData = rgParamMetaData[i-1];

                paramFact->_setName(paramInfo->label());
                paramMetaData->setDecimalPlaces(paramInfo->decimalPlaces());
                paramMetaData->setEnumInfo(paramInfo->enumStrings(), paramInfo->enumValues());
                paramMetaData->setRawUnits(paramInfo->units());
                paramFact->setMetaData(paramMetaData);
                _comboboxFacts.append(paramFact);
            }
        }
    }
}

void SimpleMissionItem::_rebuildFacts(void)
{
    _rebuildTextFieldFacts();
    _rebuildNaNFacts();
    _rebuildComboBoxFacts();
}

bool SimpleMissionItem::friendlyEditAllowed(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, static_cast<MAV_CMD>(command()));
    if (uiInfo && uiInfo->friendlyEdit()) {
        if (!_missionItem.autoContinue()) {
            return false;
        }

        if (specifiesAltitude()) {
            MAV_FRAME frame = _missionItem.frame();
            switch (frame) {
            case MAV_FRAME_GLOBAL:
            case MAV_FRAME_GLOBAL_RELATIVE_ALT:
            case MAV_FRAME_GLOBAL_TERRAIN_ALT:
                return true;
            default:
                return false;
            }
        }

        return true;
    }

    return false;
}

bool SimpleMissionItem::rawEdit(void) const
{
    return _rawEdit || !friendlyEditAllowed();
}

void SimpleMissionItem::setRawEdit(bool rawEdit)
{
    if (this->rawEdit() != rawEdit) {
        _rawEdit = rawEdit;
        emit rawEditChanged(this->rawEdit());
    }
}

void SimpleMissionItem::setDirty(bool dirty)
{
    if (!_homePositionSpecialCase || (_dirty != dirty)) {
        _dirty = dirty;
        if (!dirty) {
            _cameraSection->setDirty(false);
            _speedSection->setDirty(false);
        }
        emit dirtyChanged(dirty);
    }
}

void SimpleMissionItem::_setDirty(void)
{
    if (!_ignoreDirtyChangeSignals) {
        setDirty(true);
    }
}

void SimpleMissionItem::_sendCoordinateChanged(void)
{
    emit coordinateChanged(coordinate());
}

void SimpleMissionItem::_altitudeModeChanged(void)
{
    switch (_altitudeMode) {
    case QGroundControlQmlGlobal::AltitudeModeTerrainFrame:
        _missionItem.setFrame(MAV_FRAME_GLOBAL_TERRAIN_ALT);
        break;
    case QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain:
        // Terrain altitudes are Absolute
        _missionItem.setFrame(MAV_FRAME_GLOBAL);
        // Clear any old calculated values
        _missionItem._param7Fact.setRawValue(qQNaN());
        _amslAltAboveTerrainFact.setRawValue(qQNaN());
        break;
    case QGroundControlQmlGlobal::AltitudeModeAbsolute:
        _missionItem.setFrame(MAV_FRAME_GLOBAL);
        break;
    case QGroundControlQmlGlobal::AltitudeModeRelative:
        _missionItem.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
        break;
    case QGroundControlQmlGlobal::AltitudeModeNone:
        qWarning() << "Internal Error SimpleMissionItem::_altitudeModeChanged: Invalid altitudeMode == AltitudeModeNone";
        break;
    case QGroundControlQmlGlobal::AltitudeModeMixed:
        qWarning() << "Internal Error SimpleMissionItem::_altitudeModeChanged: Invalid altitudeMode == AltitudeModeMixed";
        break;
    }

    // We always call _altitudeChanged to make sure that param7 is always setup correctly on mode change
    _altitudeChanged();
}

void SimpleMissionItem::_altitudeChanged(void)
{
    if (!specifiesAltitude()) {
        return;
    }

    if (_altitudeMode == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain || _altitudeMode == QGroundControlQmlGlobal::AltitudeModeTerrainFrame) {
        _amslAltAboveTerrainFact.setRawValue(qQNaN());
        _terrainAltChanged();
    }

    if (_altitudeMode != QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
        _missionItem._param7Fact.setRawValue(_altitudeFact.rawValue());
    }
}

void SimpleMissionItem::_terrainAltChanged(void)
{
    if (!specifiesAltitude()) {
        // We don't need terrain data
        return;
    }

    if (_altitudeMode == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain || _altitudeMode == QGroundControlQmlGlobal::AltitudeModeTerrainFrame) {
        if (qIsNaN(terrainAltitude())) {
            // Set NaNs to signal we are waiting on terrain data
            if (_altitudeMode == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
                _missionItem._param7Fact.setRawValue(qQNaN());
            }
            _amslAltAboveTerrainFact.setRawValue(qQNaN());
        } else {
            double newAboveTerrain = terrainAltitude() + _altitudeFact.rawValue().toDouble();
            double oldAboveTerrain = _amslAltAboveTerrainFact.rawValue().toDouble();
            if (!QGC::fuzzyCompare(newAboveTerrain, oldAboveTerrain)) {
                if (_altitudeMode == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
                    _missionItem._param7Fact.setRawValue(newAboveTerrain);
                }
                _amslAltAboveTerrainFact.setRawValue(newAboveTerrain);
            }
        }
        emit readyForSaveStateChanged();
    }
}

SimpleMissionItem::ReadyForSaveState SimpleMissionItem::readyForSaveState(void) const
{
    if (_wizardMode) {
        return NotReadyForSaveData;
    }

    bool terrainReady =  !specifiesAltitude() || !qIsNaN(_missionItem._param7Fact.rawValue().toDouble());
    return terrainReady ? ReadyForSave : NotReadyForSaveTerrain;
}

void SimpleMissionItem::_setDefaultsForCommand(void)
{
    // First reset params 1-4 to 0, we leave 5-7 alone to preserve any previous location information on command change
    _missionItem._param1Fact.setRawValue(0);
    _missionItem._param2Fact.setRawValue(0);
    _missionItem._param3Fact.setRawValue(0);
    _missionItem._param4Fact.setRawValue(0);

    if (!specifiesCoordinate() && !isStandaloneCoordinate()) {
        // No need to carry across previous lat/lon
        _missionItem._param5Fact.setRawValue(0);
        _missionItem._param6Fact.setRawValue(0);
    } else if ((specifiesCoordinate() || isStandaloneCoordinate()) && _missionItem._param5Fact.rawValue().toDouble() == 0 && _missionItem._param6Fact.rawValue().toDouble() == 0) {
        // We switched from a command without a coordinate to a command with a coordinate. Use the hint.
        _missionItem._param5Fact.setRawValue(_mapCenterHint.latitude());
        _missionItem._param6Fact.setRawValue(_mapCenterHint.longitude());
    }

    // Set global defaults first, then if there are param defaults they will get reset
    _altitudeMode = QGroundControlQmlGlobal::AltitudeModeRelative;
    emit altitudeModeChanged();
    _amslAltAboveTerrainFact.setRawValue(qQNaN());
    if (specifiesAltitude()) {
        double defaultAlt = qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue().toDouble();
        _altitudeFact.setRawValue(defaultAlt);
        _missionItem._param7Fact.setRawValue(defaultAlt);
        // Note that setAltitudeMode will also set MAV_FRAME correctly through signalling
        // Takeoff items always use relative alt since that is the highest quality data to base altitude from
        setAltitudeMode(isTakeoffItem() ? QGroundControlQmlGlobal::AltitudeModeRelative : _missionController->globalAltitudeModeDefault());
    } else {
        _altitudeFact.setRawValue(0);
        _missionItem._param7Fact.setRawValue(0);
        _missionItem.setFrame(MAV_FRAME_MISSION);
    }

    MAV_CMD command = static_cast<MAV_CMD>(this->command());
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, command);
    if (uiInfo) {
        for (int i=1; i<=7; i++) {
            bool showUI;
            const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(i, showUI);
            if (paramInfo) {
                Fact* rgParamFacts[7] = { &_missionItem._param1Fact, &_missionItem._param2Fact, &_missionItem._param3Fact, &_missionItem._param4Fact, &_missionItem._param5Fact, &_missionItem._param6Fact, &_missionItem._param7Fact };
                rgParamFacts[paramInfo->param()-1]->setRawValue(paramInfo->defaultValue());
            }
        }
    }

    if (command == MAV_CMD_NAV_WAYPOINT) {
        // We default all acceptance radius to 0. This allows flight controller to be in control of
        // accept radius.
        _missionItem.setParam2(0);
    } else if ((uiInfo && uiInfo->isLandCommand()) || command == MAV_CMD_DO_SET_ROI_LOCATION) {
        _altitudeFact.setRawValue(0);
        _missionItem.setParam7(0);
    }

    _missionItem.setAutoContinue(true);
    setRawEdit(false);
}

void SimpleMissionItem::_sendCommandChanged(void)
{
    emit commandChanged(command());
}

void SimpleMissionItem::_sendFriendlyEditAllowedChanged(void)
{
    emit friendlyEditAllowedChanged(friendlyEditAllowed());
}

QString SimpleMissionItem::category(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, static_cast<MAV_CMD>(command()));
    return uiInfo ? uiInfo->category() : QString();
}

void SimpleMissionItem::setCommand(int command)
{
    if (static_cast<MAV_CMD>(command) != _missionItem.command()) {
        _missionItem.setCommand(static_cast<MAV_CMD>(command));
        _updateOptionalSections();
    }
}

void SimpleMissionItem::setRadius(double radius)
{
    if (isLoiterItem()) {
        const MAV_CMD command = static_cast<MAV_CMD>(this->command());
        if (command == MAV_CMD_NAV_LOITER_TO_ALT)
            _missionItem.setParam2(radius);
        else
            _missionItem.setParam3(radius);
    }
}

void SimpleMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    // We only use lat/lon from coordinate. This keeps param7 and the altitude value which is kept to the side in sync.
    if (_missionItem.param5() != coordinate.latitude() || _missionItem.param6() != coordinate.longitude()) {
        _missionItem.setParam5(coordinate.latitude());
        _missionItem.setParam6(coordinate.longitude());
    }
}

void SimpleMissionItem::setSequenceNumber(int sequenceNumber)
{
    if (_missionItem.sequenceNumber() != sequenceNumber) {
        _missionItem.setSequenceNumber(sequenceNumber);
        emit sequenceNumberChanged(sequenceNumber);
        // This is too likely to ignore
        emit abbreviationChanged();
    }
}

double SimpleMissionItem::specifiedFlightSpeed(void)
{
    if (_speedSection->specifyFlightSpeed()) {
        return _speedSection->flightSpeed()->rawValue().toDouble();
    } else {
        return missionItem().specifiedFlightSpeed();
    }
}

double SimpleMissionItem::specifiedGimbalYaw(void)
{
    return _cameraSection->available() ? _cameraSection->specifiedGimbalYaw() : missionItem().specifiedGimbalYaw();
}

double SimpleMissionItem::specifiedGimbalPitch(void)
{
    return _cameraSection->available() ? _cameraSection->specifiedGimbalPitch() : missionItem().specifiedGimbalPitch();
}

double SimpleMissionItem::specifiedVehicleYaw(void)
{
    return command() == MAV_CMD_NAV_WAYPOINT ? missionItem().param4() : qQNaN();
}

void SimpleMissionItem::_possibleVehicleYawChanged(void)
{
    if (command() == MAV_CMD_NAV_WAYPOINT) {
        emit specifiedVehicleYawChanged();
    }
}

bool SimpleMissionItem::scanForSections(QmlObjectListModel* visualItems, int scanIndex, PlanMasterController* /*masterController*/)
{
    bool sectionFound = false;

    if (_cameraSection->available()) {
        sectionFound |= _cameraSection->scanForSection(visualItems, scanIndex);
    }
    if (_speedSection->available()) {
        sectionFound |= _speedSection->scanForSection(visualItems, scanIndex);
    }

    return sectionFound;
}

void SimpleMissionItem::_updateOptionalSections(void)
{
    // Remove previous sections
    if (_cameraSection) {
        _cameraSection->deleteLater();
        _cameraSection = nullptr;
    }
    if (_speedSection) {
        _speedSection->deleteLater();
        _speedSection = nullptr;
    }

    // Add new sections

    _cameraSection = new CameraSection(_masterController, this);
    _speedSection = new SpeedSection(_masterController, this);
    if (static_cast<MAV_CMD>(command()) == MAV_CMD_NAV_WAYPOINT) {
        _cameraSection->setAvailable(true);
        _speedSection->setAvailable(true);
    }

    connect(_cameraSection, &CameraSection::dirtyChanged,                   this, &SimpleMissionItem::_sectionDirtyChanged);
    connect(_cameraSection, &CameraSection::itemCountChanged,               this, &SimpleMissionItem::_updateLastSequenceNumber);
    connect(_cameraSection, &CameraSection::availableChanged,               this, &SimpleMissionItem::specifiedGimbalYawChanged);
    connect(_cameraSection, &CameraSection::availableChanged,               this, &SimpleMissionItem::specifiedGimbalPitchChanged);
    connect(_cameraSection, &CameraSection::specifiedGimbalPitchChanged,    this, &SimpleMissionItem::specifiedGimbalPitchChanged);
    connect(_cameraSection, &CameraSection::specifiedGimbalYawChanged,      this, &SimpleMissionItem::specifiedGimbalYawChanged);

    connect(_speedSection,  &SpeedSection::dirtyChanged,                this, &SimpleMissionItem::_sectionDirtyChanged);
    connect(_speedSection,  &SpeedSection::itemCountChanged,            this, &SimpleMissionItem::_updateLastSequenceNumber);
    connect(_speedSection,  &SpeedSection::specifiedFlightSpeedChanged, this, &SimpleMissionItem::specifiedFlightSpeedChanged);

    emit cameraSectionChanged(_cameraSection);
    emit speedSectionChanged(_speedSection);
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

int SimpleMissionItem::lastSequenceNumber(void) const
{
    return sequenceNumber() + (_cameraSection ? _cameraSection->itemCount() : 0) + (_speedSection ? _speedSection->itemCount() : 0);
}

void SimpleMissionItem::_updateLastSequenceNumber(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

void SimpleMissionItem::_sectionDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

void SimpleMissionItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = sequenceNumber();

    items.append(new MissionItem(missionItem(), missionItemParent));
    seqNum++;

    _cameraSection->appendSectionItems(items, missionItemParent, seqNum);
    _speedSection->appendSectionItems(items, missionItemParent, seqNum);
}

void SimpleMissionItem::applyNewAltitude(double newAltitude)
{
    MAV_CMD command = static_cast<MAV_CMD>(this->command());
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_controllerVehicle, _previousVTOLMode, command);

    if (uiInfo && (uiInfo->specifiesCoordinate() || uiInfo->specifiesAltitudeOnly())) {
        switch (static_cast<MAV_CMD>(this->command())) {
        case MAV_CMD_NAV_LAND:
        case MAV_CMD_NAV_VTOL_LAND:
            // Leave alone
            break;
        default:
            _altitudeFact.setRawValue(newAltitude);
            break;
        }
    }
}

void SimpleMissionItem::setMissionFlightStatus(MissionController::MissionFlightStatus_t& missionFlightStatus)
{
    VisualMissionItem::setMissionFlightStatus(missionFlightStatus);

    // If speed and/or gimbal are not specifically set on this item. Then use the flight status values as initial defaults should a user turn them on.
    if (_speedSection->available() && !_speedSection->specifyFlightSpeed() && !QGC::fuzzyCompare(_speedSection->flightSpeed()->rawValue().toDouble(), missionFlightStatus.vehicleSpeed)) {
        _speedSection->flightSpeed()->setRawValue(missionFlightStatus.vehicleSpeed);
    }
    if (_cameraSection->available() && !_cameraSection->specifyGimbal()) {
        if (!qIsNaN(missionFlightStatus.gimbalYaw) && !QGC::fuzzyCompare(_cameraSection->gimbalYaw()->rawValue().toDouble(), missionFlightStatus.gimbalYaw)) {
            _cameraSection->gimbalYaw()->setRawValue(missionFlightStatus.gimbalYaw);
        }
        if (!qIsNaN(missionFlightStatus.gimbalPitch) && !QGC::fuzzyCompare(_cameraSection->gimbalPitch()->rawValue().toDouble(), missionFlightStatus.gimbalPitch)) {
            _cameraSection->gimbalPitch()->setRawValue(missionFlightStatus.gimbalPitch);
        }
    }
}

void SimpleMissionItem::setAltitudeMode(QGroundControlQmlGlobal::AltMode altitudeMode)
{
    if (altitudeMode != _altitudeMode) {
        _altitudeMode = altitudeMode;
        emit altitudeModeChanged();
    }
}

double SimpleMissionItem::additionalTimeDelay(void) const
{
    switch (command()) {
    case MAV_CMD_NAV_WAYPOINT:
    case MAV_CMD_CONDITION_DELAY:
    case MAV_CMD_NAV_DELAY:
        return missionItem().param1();
    default:
        return 0;
    }
}

void SimpleMissionItem::_possibleAdditionalTimeDelayChanged(void)
{
    switch (command()) {
    case MAV_CMD_NAV_WAYPOINT:
    case MAV_CMD_CONDITION_DELAY:
    case MAV_CMD_NAV_DELAY:
        emit additionalTimeDelayChanged();
        break;
    }

    return;
}

bool SimpleMissionItem::isLandCommand(void) const
{
    return _commandTree->isLandCommand(static_cast<MAV_CMD>(this->command()));
}

QGeoCoordinate SimpleMissionItem::coordinate(void) const
{
    if (qIsNaN(_missionItem.param5()) || qIsNaN(_missionItem.param6())) {
        return QGeoCoordinate();
    } else {
        return QGeoCoordinate(_missionItem.param5(), _missionItem.param6());
    }
}

double SimpleMissionItem::amslEntryAlt(void) const
{
    switch (_altitudeMode) {
    case QGroundControlQmlGlobal::AltitudeModeTerrainFrame:
        return _missionItem.param7() + _terrainAltitude;
    case QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain:
    case QGroundControlQmlGlobal::AltitudeModeAbsolute:
        return _missionItem.param7();
    case QGroundControlQmlGlobal::AltitudeModeRelative:
        return _missionItem.param7() + _masterController->missionController()->plannedHomePosition().altitude();
    case QGroundControlQmlGlobal::AltitudeModeNone:
        qWarning() << "Internal Error SimpleMissionItem::amslEntryAlt: Invalid altitudeMode:AltitudeModeNone";
        return qQNaN();
    case QGroundControlQmlGlobal::AltitudeModeMixed:
        qWarning() << "Internal Error SimpleMissionItem::amslEntryAlt: Invalid altitudeMode:AltitudeModeMixed";
        return qQNaN();
    }

    qWarning() << "Internal Error SimpleMissionItem::amslEntryAlt: Invalid altitudeMode:" << _altitudeMode;
    return qQNaN();
}

void SimpleMissionItem::_signalIfVTOLTransitionCommand(void)
{
    if (mavCommand() == MAV_CMD_DO_VTOL_TRANSITION) {
        // This will cause a MissionController recalc
        emit currentVTOLModeChanged();
    }
}

void SimpleMissionItem::_possibleRadiusChanged(void)
{
    if (isLoiterItem()) {
        emit loiterRadiusChanged(loiterRadius());
    }
}
