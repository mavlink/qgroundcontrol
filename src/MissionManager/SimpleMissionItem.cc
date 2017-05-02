/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

FactMetaData* SimpleMissionItem::_altitudeMetaData =        NULL;
FactMetaData* SimpleMissionItem::_commandMetaData =         NULL;
FactMetaData* SimpleMissionItem::_defaultParamMetaData =    NULL;
FactMetaData* SimpleMissionItem::_frameMetaData =           NULL;
FactMetaData* SimpleMissionItem::_latitudeMetaData =        NULL;
FactMetaData* SimpleMissionItem::_longitudeMetaData =       NULL;

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

SimpleMissionItem::SimpleMissionItem(Vehicle* vehicle, QObject* parent)
    : VisualMissionItem(vehicle, parent)
    , _rawEdit(false)
    , _dirty(false)
    , _ignoreDirtyChangeSignals(false)
    , _speedSection(NULL)
    , _cameraSection(NULL)
    , _commandTree(qgcApp()->toolbox()->missionCommandTree())
    , _altitudeRelativeToHomeFact   (0, "Altitude is relative to home", FactMetaData::valueTypeUint32)
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
{
    _editorQml = QStringLiteral("qrc:/qml/SimpleItemEditor.qml");

    _altitudeRelativeToHomeFact.setRawValue(true);

    _setupMetaData();
    _connectSignals();
    _updateOptionalSections();

    setDefaultsForCommand();
    _rebuildFacts();

    connect(&_missionItem, &MissionItem::specifiedFlightSpeedChanged, this, &SimpleMissionItem::specifiedFlightSpeedChanged);

    connect(this, &SimpleMissionItem::sequenceNumberChanged,        this, &SimpleMissionItem::lastSequenceNumberChanged);
    connect(this, &SimpleMissionItem::cameraSectionChanged,         this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(this, &SimpleMissionItem::cameraSectionChanged,         this, &SimpleMissionItem::_updateLastSequenceNumber);
}

SimpleMissionItem::SimpleMissionItem(Vehicle* vehicle, const MissionItem& missionItem, QObject* parent)
    : VisualMissionItem(vehicle, parent)
    , _missionItem(missionItem)
    , _rawEdit(false)
    , _dirty(false)
    , _ignoreDirtyChangeSignals(false)
    , _speedSection(NULL)
    , _cameraSection(NULL)
    , _commandTree(qgcApp()->toolbox()->missionCommandTree())
    , _altitudeRelativeToHomeFact   (0, "Altitude is relative to home", FactMetaData::valueTypeUint32)
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
{
    _editorQml = QStringLiteral("qrc:/qml/SimpleItemEditor.qml");

    _altitudeRelativeToHomeFact.setRawValue(true);
    _isCurrentItem = missionItem.isCurrentItem();

    _setupMetaData();
    _connectSignals();
    _updateOptionalSections();
    _syncFrameToAltitudeRelativeToHome();
    _rebuildFacts();
}

SimpleMissionItem::SimpleMissionItem(const SimpleMissionItem& other, QObject* parent)
    : VisualMissionItem(other, parent)
    , _missionItem(other._vehicle)
    , _rawEdit(false)
    , _dirty(false)
    , _ignoreDirtyChangeSignals(false)
    , _speedSection(NULL)
    , _cameraSection(NULL)
    , _commandTree(qgcApp()->toolbox()->missionCommandTree())
    , _altitudeRelativeToHomeFact   (0, "Altitude is relative to home", FactMetaData::valueTypeUint32)
    , _supportedCommandFact         (0, "Command:",                     FactMetaData::valueTypeUint32)
    , _param1MetaData(FactMetaData::valueTypeDouble)
    , _param2MetaData(FactMetaData::valueTypeDouble)
    , _param3MetaData(FactMetaData::valueTypeDouble)
    , _param4MetaData(FactMetaData::valueTypeDouble)
    , _syncingAltitudeRelativeToHomeAndFrame    (false)
    , _syncingHeadingDegreesAndParam4           (false)
{
    _editorQml = QStringLiteral("qrc:/qml/SimpleItemEditor.qml");

    _setupMetaData();
    _connectSignals();
    _updateOptionalSections();

    *this = other;

    _rebuildFacts();
}

const SimpleMissionItem& SimpleMissionItem::operator=(const SimpleMissionItem& other)
{
    VisualMissionItem::operator=(other);

    setRawEdit(other._rawEdit);
    setDirty(other._dirty);
    setHomePositionSpecialCase(other._homePositionSpecialCase);
    _syncFrameToAltitudeRelativeToHome();
    _rebuildFacts();

    return *this;
}

void SimpleMissionItem::_connectSignals(void)
{
    // Connect to change signals to track dirty state
    connect(&_missionItem._param1Fact,  &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem._param2Fact,  &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem._param3Fact,  &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem._param4Fact,  &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem._param5Fact,  &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem._param6Fact,  &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem._param7Fact,  &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem._frameFact,   &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem._commandFact, &Fact::valueChanged,                    this, &SimpleMissionItem::_setDirtyFromSignal);
    connect(&_missionItem,              &MissionItem::sequenceNumberChanged,    this, &SimpleMissionItem::_setDirtyFromSignal);

    // Values from these facts must propagate back and forth between the real object storage
    connect(&_altitudeRelativeToHomeFact,   &Fact::valueChanged,    this, &SimpleMissionItem::_syncAltitudeRelativeToHomeToFrame);
    connect(&_missionItem._frameFact,       &Fact::valueChanged,    this, &SimpleMissionItem::_syncFrameToAltitudeRelativeToHome);

    // These are coordinate parameters, they must emit coordinateChanged signal
    connect(&_missionItem._param5Fact, &Fact::valueChanged, this, &SimpleMissionItem::_sendCoordinateChanged);
    connect(&_missionItem._param6Fact, &Fact::valueChanged, this, &SimpleMissionItem::_sendCoordinateChanged);
    connect(&_missionItem._param7Fact, &Fact::valueChanged, this, &SimpleMissionItem::_sendCoordinateChanged);

    // The following changes may also change friendlyEditAllowed
    connect(&_missionItem._autoContinueFact,    &Fact::valueChanged, this, &SimpleMissionItem::_sendFriendlyEditAllowedChanged);
    connect(&_missionItem._commandFact,         &Fact::valueChanged, this, &SimpleMissionItem::_sendFriendlyEditAllowedChanged);
    connect(&_missionItem._frameFact,           &Fact::valueChanged, this, &SimpleMissionItem::_sendFriendlyEditAllowedChanged);

    // A command change triggers a number of other changes as well.
    connect(&_missionItem._commandFact, &Fact::valueChanged, this, &SimpleMissionItem::setDefaultsForCommand);
    connect(&_missionItem._commandFact, &Fact::valueChanged, this, &SimpleMissionItem::commandNameChanged);
    connect(&_missionItem._commandFact, &Fact::valueChanged, this, &SimpleMissionItem::commandDescriptionChanged);
    connect(&_missionItem._commandFact, &Fact::valueChanged, this, &SimpleMissionItem::abbreviationChanged);
    connect(&_missionItem._commandFact, &Fact::valueChanged, this, &SimpleMissionItem::specifiesCoordinateChanged);
    connect(&_missionItem._commandFact, &Fact::valueChanged, this, &SimpleMissionItem::specifiesAltitudeOnlyChanged);
    connect(&_missionItem._commandFact, &Fact::valueChanged, this, &SimpleMissionItem::isStandaloneCoordinateChanged);

    // Whenever these properties change the ui model changes as well
    connect(this, &SimpleMissionItem::commandChanged, this, &SimpleMissionItem::_rebuildFacts);
    connect(this, &SimpleMissionItem::rawEditChanged, this, &SimpleMissionItem::_rebuildFacts);

    // These fact signals must alway signal out through SimpleMissionItem signals
    connect(&_missionItem._commandFact,     &Fact::valueChanged, this, &SimpleMissionItem::_sendCommandChanged);
    connect(&_missionItem._frameFact,       &Fact::valueChanged, this, &SimpleMissionItem::_sendFrameChanged);

    // Sequence number is kept in mission iteem, so we need to propagate signal up as well
    connect(&_missionItem, &MissionItem::sequenceNumberChanged, this, &SimpleMissionItem::sequenceNumberChanged);
}

void SimpleMissionItem::_setupMetaData(void)
{
    QStringList enumStrings;
    QVariantList enumValues;

    if (!_altitudeMetaData) {
        _altitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble);
        _altitudeMetaData->setRawUnits("m");
        _altitudeMetaData->setDecimalPlaces(2);

        enumStrings.clear();
        enumValues.clear();
        MissionCommandTree* commandTree = qgcApp()->toolbox()->missionCommandTree();
        foreach (const MAV_CMD command, commandTree->allCommandIds()) {
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
        missionItems.append(saveObject);
        item->deleteLater();
    }
}

bool SimpleMissionItem::load(QTextStream &loadStream)
{
    return _missionItem.load(loadStream);
}

bool SimpleMissionItem::load(const QJsonObject& json, int sequenceNumber, QString& errorString)
{
    return _missionItem.load(json, sequenceNumber, errorString);
}

bool SimpleMissionItem::isStandaloneCoordinate(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->isStandaloneCoordinate();
    } else {
        return false;
    }
}

bool SimpleMissionItem::specifiesCoordinate(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->specifiesCoordinate();
    } else {
        return false;
    }
}

bool SimpleMissionItem::specifiesAltitudeOnly(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->specifiesAltitudeOnly();
    } else {
        return false;
    }
}

QString SimpleMissionItem::commandDescription(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, (MAV_CMD)command());
    if (uiInfo) {
        return uiInfo->description();
    } else {
        qWarning() << "Should not ask for command description on unknown command";
        return commandName();
    }
}

QString SimpleMissionItem::commandName(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, (MAV_CMD)command());
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
        return QStringLiteral("H");

    switch(command()) {
    default:
        return QString();
    case MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF:
        return QStringLiteral("Takeoff");
    case MavlinkQmlSingleton::MAV_CMD_NAV_LAND:
        return QStringLiteral("Land");
    case MavlinkQmlSingleton::MAV_CMD_NAV_VTOL_TAKEOFF:
        return QStringLiteral("VTOL Takeoff");
    case MavlinkQmlSingleton::MAV_CMD_NAV_VTOL_LAND:
        return QStringLiteral("VTOL Land");
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

        const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, command);

        for (int i=1; i<=7; i++) {
            const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(i);

            if (paramInfo && paramInfo->enumStrings().count() == 0 && !paramInfo->nanUnchanged()) {
                Fact*               paramFact =     rgParamFacts[i-1];
                FactMetaData*       paramMetaData = rgParamMetaData[i-1];

                paramFact->_setName(paramInfo->label());
                paramMetaData->setDecimalPlaces(paramInfo->decimalPlaces());
                paramMetaData->setRawUnits(paramInfo->units());
                paramFact->setMetaData(paramMetaData);
                _textFieldFacts.append(paramFact);
            }
        }

        if (uiInfo->specifiesCoordinate() || uiInfo->specifiesAltitudeOnly()) {
            _missionItem._param7Fact._setName("Altitude");
            _missionItem._param7Fact.setMetaData(_altitudeMetaData);
            _textFieldFacts.append(&_missionItem._param7Fact);
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

        const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, command);

        for (int i=1; i<=7; i++) {
            const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(i);

            // Show hide Heading field on waypoint based on vehicle yaw to next waypoint setting. This needs to come from the actual vehicle if it exists
            // and not _vehicle which is always offline.
            Vehicle* firmwareVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
            if (!firmwareVehicle) {
                firmwareVehicle = _vehicle;
            }
            bool hideWaypointHeading = command == MAV_CMD_NAV_WAYPOINT && firmwareVehicle->firmwarePlugin()->vehicleYawsToNextWaypointInMission(firmwareVehicle);

            if (paramInfo && paramInfo->nanUnchanged() && !hideWaypointHeading) {
                Fact*               paramFact =     rgParamFacts[i-1];
                FactMetaData*       paramMetaData = rgParamMetaData[i-1];

                paramFact->_setName(paramInfo->label());
                paramMetaData->setDecimalPlaces(paramInfo->decimalPlaces());
                paramMetaData->setRawUnits(paramInfo->units());
                paramFact->setMetaData(paramMetaData);
                _nanFacts.append(paramFact);
            }
        }

        _ignoreDirtyChangeSignals = false;
    }
}

void SimpleMissionItem::_rebuildCheckboxFacts(void)
{
    _checkboxFacts.clear();

    if (rawEdit()) {
        _checkboxFacts.append(&_missionItem._autoContinueFact);
    } else if ((specifiesCoordinate() || specifiesAltitudeOnly()) && !_homePositionSpecialCase) {
        _checkboxFacts.append(&_altitudeRelativeToHomeFact);
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
            const MissionCmdParamInfo* paramInfo = _commandTree->getUIInfo(_vehicle, command)->getParamInfo(i);

            if (paramInfo && paramInfo->enumStrings().count() != 0) {
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
    _rebuildCheckboxFacts();
    _rebuildComboBoxFacts();
}

bool SimpleMissionItem::friendlyEditAllowed(void) const
{
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, (MAV_CMD)command());
    if (uiInfo && uiInfo->friendlyEdit()) {
        if (!_missionItem.autoContinue()) {
            return false;
        }

        if (specifiesCoordinate() || specifiesAltitudeOnly()) {
            MAV_FRAME frame = _missionItem.frame();
            switch (frame) {
            case MAV_FRAME_GLOBAL:
            case MAV_FRAME_GLOBAL_RELATIVE_ALT:
                return true;
                break;

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

void SimpleMissionItem::_setDirtyFromSignal(void)
{
    if (!_ignoreDirtyChangeSignals) {
        setDirty(true);
    }
}

void SimpleMissionItem::_sendCoordinateChanged(void)
{
    emit coordinateChanged(coordinate());
}

void SimpleMissionItem::_syncAltitudeRelativeToHomeToFrame(const QVariant& value)
{
    if (!_syncingAltitudeRelativeToHomeAndFrame) {
        _syncingAltitudeRelativeToHomeAndFrame = true;
        _missionItem.setFrame(value.toBool() ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL);
        _syncingAltitudeRelativeToHomeAndFrame = false;
    }
}

void SimpleMissionItem::_syncFrameToAltitudeRelativeToHome(void)
{
    if (!_syncingAltitudeRelativeToHomeAndFrame) {
        _syncingAltitudeRelativeToHomeAndFrame = true;
        _altitudeRelativeToHomeFact.setRawValue(relativeAltitude());
        _syncingAltitudeRelativeToHomeAndFrame = false;
    }
}

void SimpleMissionItem::setDefaultsForCommand(void)
{
    // We set these global defaults first, then if there are param defaults they will get reset
    _missionItem.setParam7(qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue().toDouble());

    MAV_CMD command = (MAV_CMD)this->command();
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, command);
    if (uiInfo) {
        for (int i=1; i<=7; i++) {
            const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(i);
            if (paramInfo) {
                Fact* rgParamFacts[7] = { &_missionItem._param1Fact, &_missionItem._param2Fact, &_missionItem._param3Fact, &_missionItem._param4Fact, &_missionItem._param5Fact, &_missionItem._param6Fact, &_missionItem._param7Fact };
                rgParamFacts[paramInfo->param()-1]->setRawValue(paramInfo->defaultValue());
            }
        }
    }

    switch (command) {
    case MAV_CMD_NAV_WAYPOINT:
        // We default all acceptance radius to 0. This allows flight controller to be in control of
        // accept radius.
        _missionItem.setParam2(0);
        break;

    case MAV_CMD_NAV_LAND:
        _missionItem.setParam7(0);
        break;
    default:
        break;
    }

    _missionItem.setAutoContinue(true);
    _missionItem.setFrame((specifiesCoordinate() || specifiesAltitudeOnly()) ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_MISSION);
    setRawEdit(false);
}

void SimpleMissionItem::_sendFrameChanged(void)
{
    emit frameChanged(_missionItem.frame());
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
    return _commandTree->getUIInfo(_vehicle, (MAV_CMD)command())->category();
}

void SimpleMissionItem::setCommand(MavlinkQmlSingleton::Qml_MAV_CMD command)
{
    if ((MAV_CMD)command != _missionItem.command()) {
        _missionItem.setCommand((MAV_CMD)command);
        _updateOptionalSections();
    }
}

void SimpleMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_missionItem.coordinate() != coordinate) {
        _missionItem.setCoordinate(coordinate);
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

bool SimpleMissionItem::scanForSections(QmlObjectListModel* visualItems, int scanIndex, Vehicle* vehicle)
{
    bool sectionFound = false;

    Q_UNUSED(vehicle);

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
        _cameraSection = NULL;
    }
    if (_speedSection) {
        _speedSection->deleteLater();
        _speedSection = NULL;
    }

    // Add new sections

    _cameraSection = new CameraSection(_vehicle, this);
    _speedSection = new SpeedSection(_vehicle, this);
    if ((MAV_CMD)command() == MAV_CMD_NAV_WAYPOINT) {
        _cameraSection->setAvailable(true);
        _speedSection->setAvailable(true);
    }

    connect(_cameraSection, &CameraSection::dirtyChanged,               this, &SimpleMissionItem::_sectionDirtyChanged);
    connect(_cameraSection, &CameraSection::itemCountChanged,           this, &SimpleMissionItem::_updateLastSequenceNumber);
    connect(_cameraSection, &CameraSection::availableChanged,           this, &SimpleMissionItem::specifiedGimbalYawChanged);
    connect(_cameraSection, &CameraSection::specifyGimbalChanged,       this, &SimpleMissionItem::specifiedGimbalYawChanged);
    connect(_cameraSection, &CameraSection::specifiedGimbalYawChanged,  this, &SimpleMissionItem::specifiedGimbalYawChanged);

    connect(_speedSection,                  &SpeedSection::dirtyChanged,                this, &SimpleMissionItem::_sectionDirtyChanged);
    connect(_speedSection,                  &SpeedSection::itemCountChanged,            this, &SimpleMissionItem::_updateLastSequenceNumber);
    connect(_speedSection,                  &SpeedSection::specifyFlightSpeedChanged,   this, &SimpleMissionItem::specifiedFlightSpeedChanged);
    connect(_speedSection->flightSpeed(),   &Fact::rawValueChanged,                     this, &SimpleMissionItem::specifiedFlightSpeedChanged);

    emit cameraSectionChanged(_cameraSection);
    emit speedSectionChanged(_speedSection);
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
    MAV_CMD command = (MAV_CMD)this->command();
    const MissionCommandUIInfo* uiInfo = _commandTree->getUIInfo(_vehicle, command);

    if (uiInfo->specifiesCoordinate() || uiInfo->specifiesAltitudeOnly()) {
        switch ((MAV_CMD)this->command()) {
        case MAV_CMD_NAV_LAND:
        case MAV_CMD_NAV_VTOL_LAND:
            // Leave alone
            break;
        default:
            _missionItem.setParam7(newAltitude);
            break;
        }
    }
}
