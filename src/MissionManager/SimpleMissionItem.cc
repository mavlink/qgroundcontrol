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
#include <QDebug>

#include "SimpleMissionItem.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "JsonHelper.h"

const double SimpleMissionItem::defaultAltitude =             25.0;

FactMetaData* SimpleMissionItem::_altitudeMetaData =          NULL;
FactMetaData* SimpleMissionItem::_commandMetaData =           NULL;
FactMetaData* SimpleMissionItem::_defaultParamMetaData =      NULL;
FactMetaData* SimpleMissionItem::_frameMetaData =             NULL;
FactMetaData* SimpleMissionItem::_latitudeMetaData =          NULL;
FactMetaData* SimpleMissionItem::_longitudeMetaData =         NULL;

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
    , _homePositionSpecialCase(false)
    , _showHomePosition(false)
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
    , _missionCommands(qgcApp()->toolbox()->missionCommands())
{
    _altitudeRelativeToHomeFact.setRawValue(true);

    _setupMetaData();
    _connectSignals();

    setDefaultsForCommand();
}

SimpleMissionItem::SimpleMissionItem(Vehicle* vehicle, const MissionItem& missionItem, QObject* parent)
    : VisualMissionItem(vehicle, parent)
    , _missionItem(missionItem)
    , _rawEdit(false)
    , _dirty(false)
    , _homePositionSpecialCase(false)
    , _showHomePosition(false)
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
    , _missionCommands(qgcApp()->toolbox()->missionCommands())
{
    _altitudeRelativeToHomeFact.setRawValue(true);

    _setupMetaData();
    _connectSignals();

    _syncFrameToAltitudeRelativeToHome();
}

SimpleMissionItem::SimpleMissionItem(const SimpleMissionItem& other, QObject* parent)
    : VisualMissionItem(other, parent)
    , _missionItem(other._vehicle)
    , _rawEdit(false)
    , _dirty(false)
    , _homePositionSpecialCase(false)
    , _showHomePosition(false)
    , _altitudeRelativeToHomeFact   (0, "Altitude is relative to home", FactMetaData::valueTypeUint32)
    , _supportedCommandFact         (0, "Command:",                     FactMetaData::valueTypeUint32)
    , _param1MetaData(FactMetaData::valueTypeDouble)
    , _param2MetaData(FactMetaData::valueTypeDouble)
    , _param3MetaData(FactMetaData::valueTypeDouble)
    , _param4MetaData(FactMetaData::valueTypeDouble)
    , _syncingAltitudeRelativeToHomeAndFrame    (false)
    , _syncingHeadingDegreesAndParam4           (false)
    , _missionCommands(qgcApp()->toolbox()->missionCommands())
{
    _setupMetaData();
    _connectSignals();

    *this = other;
}

const SimpleMissionItem& SimpleMissionItem::operator=(const SimpleMissionItem& other)
{
    static_cast<VisualMissionItem&>(*this) = other;

    setRawEdit(other._rawEdit);
    setDirty(other._dirty);
    setHomePositionSpecialCase(other._homePositionSpecialCase);
    setShowHomePosition(other._showHomePosition);

    _syncFrameToAltitudeRelativeToHome();

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

    // Values from these facts must propogate back and forth between the real object storage
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
    connect(&_missionItem._commandFact, &Fact::valueChanged, this, &SimpleMissionItem::isStandaloneCoordinateChanged);

    // Whenever these properties change the ui model changes as well
    connect(this, &SimpleMissionItem::commandChanged, this, &SimpleMissionItem::_sendUiModelChanged);
    connect(this, &SimpleMissionItem::rawEditChanged, this, &SimpleMissionItem::_sendUiModelChanged);

    // These fact signals must alway signal out through SimpleMissionItem signals
    connect(&_missionItem._commandFact,     &Fact::valueChanged, this, &SimpleMissionItem::_sendCommandChanged);
    connect(&_missionItem._frameFact,       &Fact::valueChanged, this, &SimpleMissionItem::_sendFrameChanged);

    // Sequence number is kept in mission iteem, so we need to propogate signal up as well
    connect(&_missionItem, &MissionItem::sequenceNumberChanged, this, &SimpleMissionItem::sequenceNumberChanged);
}

void SimpleMissionItem::_setupMetaData(void)
{
    QStringList enumStrings;
    QVariantList enumValues;

    if (!_altitudeMetaData) {
        _altitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble);
        _altitudeMetaData->setRawUnits("meters");
        _altitudeMetaData->setDecimalPlaces(2);
        _altitudeMetaData->setAppSettingsTranslators();

        enumStrings.clear();
        enumValues.clear();
        foreach (const MAV_CMD command, _missionCommands->commandsIds()) {
            const MavCmdInfo* mavCmdInfo = _missionCommands->getMavCmdInfo(command, _vehicle);
            enumStrings.append(mavCmdInfo->rawName());
            enumValues.append(QVariant(mavCmdInfo->command()));
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

void SimpleMissionItem::save(QJsonObject& saveObject) const
{
    _missionItem.save(saveObject);
}

bool SimpleMissionItem::load(QTextStream &loadStream)
{
    return _missionItem.load(loadStream);
}

bool SimpleMissionItem::load(const QJsonObject& json, QString& errorString)
{
    return _missionItem.load(json, errorString);
}

bool SimpleMissionItem::isStandaloneCoordinate(void) const
{
    if (_missionCommands->contains((MAV_CMD)command())) {
        return _missionCommands->getMavCmdInfo((MAV_CMD)command(), _vehicle)->isStandaloneCoordinate();
    } else {
        return false;
    }
}

bool SimpleMissionItem::specifiesCoordinate(void) const
{
    if (_missionCommands->contains((MAV_CMD)command())) {
        return _missionCommands->getMavCmdInfo((MAV_CMD)command(), _vehicle)->specifiesCoordinate();
    } else {
        return false;
    }
}

QString SimpleMissionItem::commandDescription(void) const
{
    if (_missionCommands->contains((MAV_CMD)command())) {
        return _missionCommands->getMavCmdInfo((MAV_CMD)command(), _vehicle)->description();
    } else {
        qWarning() << "Should not ask for command description on unknown command";
        return commandName();
    }
}

QString SimpleMissionItem::commandName(void) const
{
    MAV_CMD command = (MAV_CMD)this->command();
    if (_missionCommands->contains(command)) {
        const MavCmdInfo* mavCmdInfo = _missionCommands->getMavCmdInfo(command, _vehicle);
        return mavCmdInfo->friendlyName().isEmpty() ? mavCmdInfo->rawName() : mavCmdInfo->friendlyName();
    } else {
        qWarning() << "Request for command name on unknown command";
        return QString("Unknown: %1").arg(command);
    }
}

QString SimpleMissionItem::abbreviation() const
{
    if (homePosition())
        return QStringLiteral("H");

    switch(command()) {
    default:
        return QString::number(sequenceNumber());
    case MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF:
        return QStringLiteral("T");
    case MavlinkQmlSingleton::MAV_CMD_NAV_LAND:
        return QStringLiteral("L");
    }
}

void SimpleMissionItem::_clearParamMetaData(void)
{
    _param1MetaData.setRawUnits("");
    _param1MetaData.setDecimalPlaces(FactMetaData::unknownDecimalPlaces);
    _param1MetaData.setBuiltInTranslator();
    _param2MetaData.setRawUnits("");
    _param2MetaData.setDecimalPlaces(FactMetaData::unknownDecimalPlaces);
    _param2MetaData.setBuiltInTranslator();
    _param3MetaData.setRawUnits("");
    _param3MetaData.setDecimalPlaces(FactMetaData::unknownDecimalPlaces);
    _param3MetaData.setBuiltInTranslator();
    _param4MetaData.setRawUnits("");
    _param4MetaData.setDecimalPlaces(FactMetaData::unknownDecimalPlaces);
    _param4MetaData.setBuiltInTranslator();
}

QmlObjectListModel* SimpleMissionItem::textFieldFacts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);
    
    if (rawEdit()) {
        _missionItem._param1Fact._setName("Param1:");
        _missionItem._param1Fact.setMetaData(_defaultParamMetaData);
        model->append(&_missionItem._param1Fact);
        _missionItem._param2Fact._setName("Param2:");
        _missionItem._param2Fact.setMetaData(_defaultParamMetaData);
        model->append(&_missionItem._param2Fact);
        _missionItem._param3Fact._setName("Param3:");
        _missionItem._param3Fact.setMetaData(_defaultParamMetaData);
        model->append(&_missionItem._param3Fact);
        _missionItem._param4Fact._setName("Param4:");
        _missionItem._param4Fact.setMetaData(_defaultParamMetaData);
        model->append(&_missionItem._param4Fact);
        _missionItem._param5Fact._setName("Lat/X:");
        _missionItem._param5Fact.setMetaData(_defaultParamMetaData);
        model->append(&_missionItem._param5Fact);
        _missionItem._param6Fact._setName("Lon/Y:");
        _missionItem._param6Fact.setMetaData(_defaultParamMetaData);
        model->append(&_missionItem._param6Fact);
        _missionItem._param7Fact._setName("Alt/Z:");
        _missionItem._param7Fact.setMetaData(_defaultParamMetaData);
        model->append(&_missionItem._param7Fact);
    } else {
        _clearParamMetaData();

        MAV_CMD command;
        if (_homePositionSpecialCase) {
            command = MAV_CMD_NAV_LAST;
        } else {
            command = _missionItem.command();
        }

        Fact*           rgParamFacts[7] =       { &_missionItem._param1Fact, &_missionItem._param2Fact, &_missionItem._param3Fact, &_missionItem._param4Fact, &_missionItem._param5Fact, &_missionItem._param6Fact, &_missionItem._param7Fact };
        FactMetaData*   rgParamMetaData[7] =    { &_param1MetaData, &_param2MetaData, &_param3MetaData, &_param4MetaData, &_param5MetaData, &_param6MetaData, &_param7MetaData };

        bool altitudeAdded = false;
        for (int i=1; i<=7; i++) {
            const QMap<int, MavCmdParamInfo*>& paramInfoMap = _missionCommands->getMavCmdInfo(command, _vehicle)->paramInfoMap();

            if (paramInfoMap.contains(i) && paramInfoMap[i]->enumStrings().count() == 0) {
                Fact*               paramFact =     rgParamFacts[i-1];
                FactMetaData*       paramMetaData = rgParamMetaData[i-1];
                MavCmdParamInfo*    paramInfo =     paramInfoMap[i];

                paramFact->_setName(paramInfo->label());
                paramMetaData->setDecimalPlaces(paramInfo->decimalPlaces());
                paramMetaData->setEnumInfo(paramInfo->enumStrings(), paramInfo->enumValues());
                paramMetaData->setRawUnits(paramInfo->units());
                paramMetaData->setAppSettingsTranslators();
                paramFact->setMetaData(paramMetaData);
                model->append(paramFact);

                if (i == 7) {
                    altitudeAdded = true;
                }
            }
        }

        if (specifiesCoordinate() && !altitudeAdded) {
            _missionItem._param7Fact._setName("Altitude:");
            _missionItem._param7Fact.setMetaData(_altitudeMetaData);
            model->append(&_missionItem._param7Fact);
        }
    }
    
    return model;
}

QmlObjectListModel* SimpleMissionItem::checkboxFacts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);
    

    if (rawEdit()) {
        model->append(&_missionItem._autoContinueFact);
    } else if (specifiesCoordinate() && !_homePositionSpecialCase) {
        model->append(&_altitudeRelativeToHomeFact);
    }

    return model;
}

QmlObjectListModel* SimpleMissionItem::comboboxFacts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);

    if (rawEdit()) {
        model->append(&_missionItem._commandFact);
        model->append(&_missionItem._frameFact);
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
            const QMap<int, MavCmdParamInfo*>& paramInfoMap = _missionCommands->getMavCmdInfo(command, _vehicle)->paramInfoMap();

            if (paramInfoMap.contains(i) && paramInfoMap[i]->enumStrings().count() != 0) {
                Fact*               paramFact =     rgParamFacts[i-1];
                FactMetaData*       paramMetaData = rgParamMetaData[i-1];
                MavCmdParamInfo*    paramInfo =     paramInfoMap[i];

                paramFact->_setName(paramInfo->label());
                paramMetaData->setDecimalPlaces(paramInfo->decimalPlaces());
                paramMetaData->setEnumInfo(paramInfo->enumStrings(), paramInfo->enumValues());
                paramMetaData->setRawUnits(paramInfo->units());
                paramFact->setMetaData(paramMetaData);
                model->append(paramFact);
            }
        }
    }

    return model;
}

bool SimpleMissionItem::friendlyEditAllowed(void) const
{
    if (_missionCommands->contains((MAV_CMD)command()) && _missionCommands->getMavCmdInfo((MAV_CMD)command(), _vehicle)->friendlyEdit()) {
        if (!_missionItem.autoContinue()) {
            return false;
        }

        if (specifiesCoordinate()) {
            return _missionItem.frame() == MAV_FRAME_GLOBAL || _missionItem.frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT;
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
    if (!_homePositionSpecialCase || !dirty) {
        // Home position never affects dirty bit

        _dirty = dirty;
        // We want to emit dirtyChanged even if _dirty didn't change. This can be handy signal for
        // any value within the item changing.
        emit dirtyChanged(_dirty);
    }
}

void SimpleMissionItem::_setDirtyFromSignal(void)
{
    setDirty(true);
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
    _missionItem.setParam7(defaultAltitude);

    MAV_CMD command = (MAV_CMD)this->command();
    if (_missionCommands->contains(command)) {
        MavCmdInfo* mavCmdInfo = _missionCommands->getMavCmdInfo(command, _vehicle);
        foreach (const MavCmdParamInfo* paramInfo, mavCmdInfo->paramInfoMap()) {
            Fact* rgParamFacts[7] = { &_missionItem._param1Fact, &_missionItem._param2Fact, &_missionItem._param3Fact, &_missionItem._param4Fact, &_missionItem._param5Fact, &_missionItem._param6Fact, &_missionItem._param7Fact };

            rgParamFacts[paramInfo->param()-1]->setRawValue(paramInfo->defaultValue());
        }
    }

    if (command == MAV_CMD_NAV_WAYPOINT) {
        // We default all acceptance radius to 0. This allows flight controller to be in control of
        // accept radius.
        _missionItem.setParam2(0);
    }

    _missionItem.setAutoContinue(true);
    _missionItem.setFrame(specifiesCoordinate() ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_MISSION);
    setRawEdit(false);
}

void SimpleMissionItem::_sendUiModelChanged(void)
{
    emit uiModelChanged();
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
    return qgcApp()->toolbox()->missionCommands()->categoryFromCommand(command());
}

void SimpleMissionItem::setShowHomePosition(bool showHomePosition)
{
    if (showHomePosition != _showHomePosition) {
        _showHomePosition = showHomePosition;
        emit showHomePositionChanged(_showHomePosition);
    }
}

void SimpleMissionItem::setCommand(MavlinkQmlSingleton::Qml_MAV_CMD command)
{
    if ((MAV_CMD)command != _missionItem.command()) {
        _missionItem.setCommand((MAV_CMD)command);
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
    _missionItem.setSequenceNumber(sequenceNumber);
}
