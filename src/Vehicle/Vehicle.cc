/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "FirmwarePluginManager.h"
#include "LinkManager.h"
#include "FirmwarePlugin.h"
#include "AutoPilotPluginManager.h"
#include "UAS.h"
#include "JoystickManager.h"
#include "MissionManager.h"
#include "CoordinateVector.h"
#include "ParameterLoader.h"
#include "QGCApplication.h"
#include "QGCImageProvider.h"
#include "GAudioOutput.h"

QGC_LOGGING_CATEGORY(VehicleLog, "VehicleLog")

#define UPDATE_TIMER 50
#define DEFAULT_LAT  38.965767f
#define DEFAULT_LON -120.083923f

const char* Vehicle::_settingsGroup =               "Vehicle%1";        // %1 replaced with mavlink system id
const char* Vehicle::_joystickModeSettingsKey =     "JoystickMode";
const char* Vehicle::_joystickEnabledSettingsKey =  "JoystickEnabled";

const char* Vehicle::_rollFactName =                "roll";
const char* Vehicle::_pitchFactName =               "pitch";
const char* Vehicle::_headingFactName =             "heading";
const char* Vehicle::_airSpeedFactName =            "airSpeed";
const char* Vehicle::_groundSpeedFactName =         "groundSpeed";
const char* Vehicle::_climbRateFactName =           "climbRate";
const char* Vehicle::_altitudeRelativeFactName =    "altitudeRelative";
const char* Vehicle::_altitudeAMSLFactName =        "altitudeAMSL";

const char* Vehicle::_gpsFactGroupName =        "gps";
const char* Vehicle::_batteryFactGroupName =    "battery";

const char* VehicleGPSFactGroup::_hdopFactName =                "hdop";
const char* VehicleGPSFactGroup::_vdopFactName =                "vdop";
const char* VehicleGPSFactGroup::_courseOverGroundFactName =    "courseOverGround";
const char* VehicleGPSFactGroup::_countFactName =               "count";
const char* VehicleGPSFactGroup::_lockFactName =                "lock";

const char* VehicleBatteryFactGroup::_voltageFactName =             "voltage";
const char* VehicleBatteryFactGroup::_percentRemainingFactName =    "percentRemaining";
const char* VehicleBatteryFactGroup::_mahConsumedFactName =         "mahConsumed";
const char* VehicleBatteryFactGroup::_currentFactName =             "current";
const char* VehicleBatteryFactGroup::_temperatureFactName =         "temperature";
const char* VehicleBatteryFactGroup::_cellCountFactName =           "cellCount";

const double VehicleBatteryFactGroup::_voltageUnavailable =           -1.0;
const int    VehicleBatteryFactGroup::_percentRemainingUnavailable =  -1;
const int    VehicleBatteryFactGroup::_mahConsumedUnavailable =       -1;
const int    VehicleBatteryFactGroup::_currentUnavailable =           -1;
const double VehicleBatteryFactGroup::_temperatureUnavailable =       -1.0;
const int    VehicleBatteryFactGroup::_cellCountUnavailable =         -1.0;

Vehicle::Vehicle(LinkInterface*             link,
                 int                        vehicleId,
                 MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 AutoPilotPluginManager*    autopilotPluginManager,
                 JoystickManager*           joystickManager)
    : FactGroup(_vehicleUIUpdateRateMSecs, ":/json/Vehicle/VehicleFact.json")
    , _id(vehicleId)
    , _active(false)
    , _firmwareType(firmwareType)
    , _vehicleType(vehicleType)
    , _firmwarePlugin(NULL)
    , _autopilotPlugin(NULL)
    , _joystickMode(JoystickModeRC)
    , _joystickEnabled(false)
    , _uas(NULL)
    , _coordinate(37.803784, -122.462276)
    , _coordinateValid(false)
    , _homePositionAvailable(false)
    , _mav(NULL)
    , _currentMessageCount(0)
    , _messageCount(0)
    , _currentErrorCount(0)
    , _currentWarningCount(0)
    , _currentNormalCount(0)
    , _currentMessageType(MessageNone)
    , _navigationAltitudeError(0.0f)
    , _navigationSpeedError(0.0f)
    , _navigationCrosstrackError(0.0f)
    , _navigationTargetBearing(0.0f)
    , _refreshTimer(new QTimer(this))
    , _updateCount(0)
    , _rcRSSI(0)
    , _rcRSSIstore(100.0)
    , _autoDisconnect(false)
    , _connectionLost(false)
    , _connectionLostEnabled(true)
    , _missionManager(NULL)
    , _missionManagerInitialRequestComplete(false)
    , _parameterLoader(NULL)
    , _armed(false)
    , _base_mode(0)
    , _custom_mode(0)
    , _nextSendMessageMultipleIndex(0)
    , _firmwarePluginManager(firmwarePluginManager)
    , _autopilotPluginManager(autopilotPluginManager)
    , _joystickManager(joystickManager)
    , _flowImageIndex(0)
    , _allLinksInactiveSent(false)
    , _messagesReceived(0)
    , _messagesSent(0)
    , _messagesLost(0)
    , _messageSeq(0)
    , _compID(0)
    , _heardFrom(false)
    , _rollFact             (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact            (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact          (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact      (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact         (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _climbRateFact        (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact     (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _gpsFactGroup(this)
    , _batteryFactGroup(this)
{
    _addLink(link);

    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    connect(_mavlink, &MAVLinkProtocol::messageReceived,     this, &Vehicle::_mavlinkMessageReceived);
    connect(this, &Vehicle::_sendMessageOnThread, this, &Vehicle::_sendMessage, Qt::QueuedConnection);
    connect(this, &Vehicle::_sendMessageOnLinkOnThread, this, &Vehicle::_sendMessageOnLink, Qt::QueuedConnection);

    _uas = new UAS(_mavlink, this, _firmwarePluginManager);

    setLatitude(_uas->getLatitude());
    setLongitude(_uas->getLongitude());

    connect(_uas, &UAS::latitudeChanged,                this, &Vehicle::setLatitude);
    connect(_uas, &UAS::longitudeChanged,               this, &Vehicle::setLongitude);
    connect(_uas, &UAS::imageReady,                     this, &Vehicle::_imageReady);
    connect(this, &Vehicle::remoteControlRSSIChanged,   this, &Vehicle::_remoteControlRSSIChanged);

    _firmwarePlugin     = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);
    _autopilotPlugin    = _autopilotPluginManager->newAutopilotPluginForVehicle(this);

    connect(_autopilotPlugin, &AutoPilotPlugin::parametersReadyChanged,     this, &Vehicle::_parametersReady);
    connect(_autopilotPlugin, &AutoPilotPlugin::missingParametersChanged,   this, &Vehicle::missingParametersChanged);

    // Refresh timer
    connect(_refreshTimer, &QTimer::timeout, this, &Vehicle::_checkUpdate);
    _refreshTimer->setInterval(UPDATE_TIMER);
    _refreshTimer->start(UPDATE_TIMER);

    // Connection Lost time
    _connectionLostTimer.setInterval(Vehicle::_connectionLostTimeoutMSecs);
    _connectionLostTimer.setSingleShot(false);
    _connectionLostTimer.start();
    connect(&_connectionLostTimer, &QTimer::timeout, this, &Vehicle::_connectionLostTimeout);

    _mav = uas();

    // Listen for system messages
    connect(qgcApp()->toolbox()->uasMessageHandler(), &UASMessageHandler::textMessageCountChanged,  this, &Vehicle::_handleTextMessage);
    connect(qgcApp()->toolbox()->uasMessageHandler(), &UASMessageHandler::textMessageReceived,      this, &Vehicle::_handletextMessageReceived);
    // Now connect the new UAS
    connect(_mav, SIGNAL(attitudeChanged                    (UASInterface*,double,double,double,quint64)),              this, SLOT(_updateAttitude(UASInterface*, double, double, double, quint64)));
    connect(_mav, SIGNAL(attitudeChanged                    (UASInterface*,int,double,double,double,quint64)),          this, SLOT(_updateAttitude(UASInterface*,int,double, double, double, quint64)));
    connect(_mav, SIGNAL(statusChanged                      (UASInterface*,QString,QString)),                           this, SLOT(_updateState(UASInterface*, QString,QString)));

    connect(_mav, &UASInterface::speedChanged, this, &Vehicle::_updateSpeed);
    connect(_mav, &UASInterface::altitudeChanged, this, &Vehicle::_updateAltitude);
    connect(_mav, &UASInterface::navigationControllerErrorsChanged,this, &Vehicle::_updateNavigationControllerErrors);
    connect(_mav, &UASInterface::NavigationControllerDataChanged,   this, &Vehicle::_updateNavigationControllerData);

    _loadSettings();

    _missionManager = new MissionManager(this);
    connect(_missionManager, &MissionManager::error, this, &Vehicle::_missionManagerError);

    _parameterLoader = new ParameterLoader(_autopilotPlugin, this /* Vehicle */, this /* parent */);
    connect(_parameterLoader, &ParameterLoader::parametersReady, _autopilotPlugin, &AutoPilotPlugin::_parametersReadyPreChecks);
    connect(_parameterLoader, &ParameterLoader::parameterListProgress, _autopilotPlugin, &AutoPilotPlugin::parameterListProgress);

    _firmwarePlugin->initializeVehicle(this);

    _sendMultipleTimer.start(_sendMessageMultipleIntraMessageDelay);
    connect(&_sendMultipleTimer, &QTimer::timeout, this, &Vehicle::_sendMessageMultipleNext);

    _mapTrajectoryTimer.setInterval(_mapTrajectoryMsecsBetweenPoints);
    connect(&_mapTrajectoryTimer, &QTimer::timeout, this, &Vehicle::_addNewMapTrajectoryPoint);

    // Build FactGroup object model

    _addFact(&_rollFact,                _rollFactName);
    _addFact(&_pitchFact,               _pitchFactName);
    _addFact(&_headingFact,             _headingFactName);
    _addFact(&_groundSpeedFact,         _groundSpeedFactName);
    _addFact(&_airSpeedFact,            _airSpeedFactName);
    _addFact(&_climbRateFact,           _climbRateFactName);
    _addFact(&_altitudeRelativeFact,    _altitudeRelativeFactName);
    _addFact(&_altitudeAMSLFact,        _altitudeAMSLFactName);

    _addFactGroup(&_gpsFactGroup,       _gpsFactGroupName);
    _addFactGroup(&_batteryFactGroup,   _batteryFactGroupName);
    _gpsFactGroup.setVehicle(this);
    _batteryFactGroup.setVehicle(this);
}

Vehicle::~Vehicle()
{
    qCDebug(VehicleLog) << "~Vehicle" << this;

    delete _missionManager;
    _missionManager = NULL;

    delete _autopilotPlugin;
    _autopilotPlugin = NULL;

    delete _mav;
    _mav = NULL;

}

void
Vehicle::resetCounters()
{
    _messagesReceived   = 0;
    _messagesSent       = 0;
    _messagesLost       = 0;
    _messageSeq         = 0;
    _heardFrom          = false;
}

void Vehicle::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    if (message.sysid != _id && message.sysid != 0) {
        return;
    }

    if (!_containsLink(link)) {
        _addLink(link);
    }

    //-- Check link status
    _messagesReceived++;
    emit messagesReceivedChanged();
    if(!_heardFrom) {
        if(message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            _heardFrom = true;
            _compID = message.compid;
            _messageSeq = message.seq + 1;
        }
    } else {
        if(_compID == message.compid) {
            uint16_t seq_received = (uint16_t)message.seq;
            uint16_t packet_lost_count = 0;
            //-- Account for overflow during packet loss
            if(seq_received < _messageSeq) {
                packet_lost_count = (seq_received + 255) - _messageSeq;
            } else {
                packet_lost_count = seq_received - _messageSeq;
            }
            _messageSeq = message.seq + 1;
            _messagesLost += packet_lost_count;
            if(packet_lost_count)
                emit messagesLostChanged();
        }
    }

    // Give the plugin a change to adjust the message contents
    _firmwarePlugin->adjustMavlinkMessage(this, &message);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HOME_POSITION:
        _handleHomePosition(message);
        break;
    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartbeat(message);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS:
        _handleRCChannels(message);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
        _handleRCChannelsRaw(message);
        break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
        _handleBatteryStatus(message);
        break;
    case MAVLINK_MSG_ID_SYS_STATUS:
        _handleSysStatus(message);
        break;
    case MAVLINK_MSG_ID_RAW_IMU:
        emit mavlinkRawImu(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU:
        emit mavlinkScaledImu1(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU2:
        emit mavlinkScaledImu2(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU3:
        emit mavlinkScaledImu3(message);
        break;
    }

    emit mavlinkMessageReceived(message);

    _uas->receiveMessage(message);
}

void Vehicle::_handleSysStatus(mavlink_message_t& message)
{
    mavlink_sys_status_t sysStatus;
    mavlink_msg_sys_status_decode(&message, &sysStatus);

    if (sysStatus.current_battery == -1) {
        _batteryFactGroup.current()->setRawValue(VehicleBatteryFactGroup::_currentUnavailable);
    } else {
        _batteryFactGroup.current()->setRawValue((double)sysStatus.current_battery * 10);
    }
    if (sysStatus.voltage_battery == UINT16_MAX) {
        _batteryFactGroup.voltage()->setRawValue(VehicleBatteryFactGroup::_voltageUnavailable);
    } else {
        _batteryFactGroup.voltage()->setRawValue((double)sysStatus.voltage_battery / 1000.0);
    }
    _batteryFactGroup.percentRemaining()->setRawValue(sysStatus.battery_remaining);
}

void Vehicle::_handleBatteryStatus(mavlink_message_t& message)
{
    mavlink_battery_status_t bat_status;
    mavlink_msg_battery_status_decode(&message, &bat_status);

    if (bat_status.temperature == INT16_MAX) {
        _batteryFactGroup.temperature()->setRawValue(VehicleBatteryFactGroup::_temperatureUnavailable);
    } else {
        _batteryFactGroup.temperature()->setRawValue((double)bat_status.temperature / 100.0);
    }
    if (bat_status.current_consumed == -1) {
        _batteryFactGroup.mahConsumed()->setRawValue(VehicleBatteryFactGroup::_mahConsumedUnavailable);
    } else {
        _batteryFactGroup.mahConsumed()->setRawValue(bat_status.current_consumed);
    }

    int cellCount = 0;
    for (int i=0; i<10; i++) {
        if (bat_status.voltages[i] != UINT16_MAX) {
            cellCount++;
        }
    }
    if (cellCount == 0) {
        cellCount = -1;
    }

    _batteryFactGroup.cellCount()->setRawValue(cellCount);
}

void Vehicle::_handleHomePosition(mavlink_message_t& message)
{
    bool emitHomePositionChanged =          false;
    bool emitHomePositionAvailableChanged = false;

    mavlink_home_position_t homePos;

    mavlink_msg_home_position_decode(&message, &homePos);

    QGeoCoordinate newHomePosition (homePos.latitude / 10000000.0,
                                    homePos.longitude / 10000000.0,
                                    homePos.altitude / 1000.0);
    if (!_homePositionAvailable || newHomePosition != _homePosition) {
        emitHomePositionChanged = true;
        _homePosition = newHomePosition;
    }

    if (!_homePositionAvailable) {
        emitHomePositionAvailableChanged = true;
        _homePositionAvailable = true;
    }

    if (emitHomePositionChanged) {
        qCDebug(VehicleLog) << "New home position" << newHomePosition;
        qgcApp()->setLastKnownHomePosition(_homePosition);
        emit homePositionChanged(_homePosition);
    }
    if (emitHomePositionAvailableChanged) {
        emit homePositionAvailableChanged(true);
    }
}

void Vehicle::_handleHeartbeat(mavlink_message_t& message)
{
    _connectionActive();

    mavlink_heartbeat_t heartbeat;

    mavlink_msg_heartbeat_decode(&message, &heartbeat);

    bool newArmed = heartbeat.base_mode & MAV_MODE_FLAG_DECODE_POSITION_SAFETY;

    if (_armed != newArmed) {
        _armed = newArmed;
        emit armedChanged(_armed);

        // We are transitioning to the armed state, begin tracking trajectory points for the map
        if (_armed) {
            _mapTrajectoryStart();
        } else {
            _mapTrajectoryStop();
        }
    }

    if (heartbeat.base_mode != _base_mode || heartbeat.custom_mode != _custom_mode) {
        _base_mode = heartbeat.base_mode;
        _custom_mode = heartbeat.custom_mode;
        emit flightModeChanged(flightMode());
    }
}

void Vehicle::_handleRCChannels(mavlink_message_t& message)
{
    mavlink_rc_channels_t channels;

    mavlink_msg_rc_channels_decode(&message, &channels);

    uint16_t* _rgChannelvalues[cMaxRcChannels] = {
        &channels.chan1_raw,
        &channels.chan2_raw,
        &channels.chan3_raw,
        &channels.chan4_raw,
        &channels.chan5_raw,
        &channels.chan6_raw,
        &channels.chan7_raw,
        &channels.chan8_raw,
        &channels.chan9_raw,
        &channels.chan10_raw,
        &channels.chan11_raw,
        &channels.chan12_raw,
        &channels.chan13_raw,
        &channels.chan14_raw,
        &channels.chan15_raw,
        &channels.chan16_raw,
        &channels.chan17_raw,
        &channels.chan18_raw,
    };
    int pwmValues[cMaxRcChannels];

    for (int i=0; i<cMaxRcChannels; i++) {
        uint16_t channelValue = *_rgChannelvalues[i];

        if (i < channels.chancount) {
            pwmValues[i] = channelValue == UINT16_MAX ? -1 : channelValue;
        } else {
            pwmValues[i] = -1;
        }
    }

    emit remoteControlRSSIChanged(channels.rssi);
    emit rcChannelsChanged(channels.chancount, pwmValues);
}

void Vehicle::_handleRCChannelsRaw(mavlink_message_t& message)
{
    // We handle both RC_CHANNLES and RC_CHANNELS_RAW since different firmware will only
    // send one or the other.

    mavlink_rc_channels_raw_t channels;

    mavlink_msg_rc_channels_raw_decode(&message, &channels);

    uint16_t* _rgChannelvalues[cMaxRcChannels] = {
        &channels.chan1_raw,
        &channels.chan2_raw,
        &channels.chan3_raw,
        &channels.chan4_raw,
        &channels.chan5_raw,
        &channels.chan6_raw,
        &channels.chan7_raw,
        &channels.chan8_raw,
    };

    int pwmValues[cMaxRcChannels];
    int channelCount = 0;

    for (int i=0; i<cMaxRcChannels; i++) {
        pwmValues[i] = -1;
    }

    for (int i=0; i<8; i++) {
        uint16_t channelValue = *_rgChannelvalues[i];

        if (channelValue == UINT16_MAX) {
            pwmValues[i] = -1;
        } else {
            channelCount = i + 1;
            pwmValues[i] = channelValue;
        }
    }
    for (int i=9; i<18; i++) {
        pwmValues[i] = -1;
    }

    emit remoteControlRSSIChanged(channels.rssi);
    emit rcChannelsChanged(channelCount, pwmValues);
}

bool Vehicle::_containsLink(LinkInterface* link)
{
    return _links.contains(link);
}

void Vehicle::_addLink(LinkInterface* link)
{
    if (!_containsLink(link)) {
        _links += link;
        qCDebug(VehicleLog) << "_addLink:" << QString("%1").arg((ulong)link, 0, 16);
        connect(qgcApp()->toolbox()->linkManager(), &LinkManager::linkInactive, this, &Vehicle::_linkInactiveOrDeleted);
        connect(qgcApp()->toolbox()->linkManager(), &LinkManager::linkDeleted, this, &Vehicle::_linkInactiveOrDeleted);
    }
}

void Vehicle::_linkInactiveOrDeleted(LinkInterface* link)
{
    qCDebug(VehicleLog) << "_linkInactiveOrDeleted linkCount" << _links.count();

    _links.removeOne(link);

    if (_links.count() == 0 && !_allLinksInactiveSent) {
        qCDebug(VehicleLog) << "All links inactive";
        // Make sure to not send this more than one time
        _allLinksInactiveSent = true;
        emit allLinksInactive(this);
    }
}

void Vehicle::sendMessage(mavlink_message_t message)
{
    emit _sendMessageOnThread(message);
}

bool Vehicle::sendMessageOnLink(LinkInterface* link, mavlink_message_t message)
{
    if (!link || !_links.contains(link) || !link->isConnected()) {
        return false;
    }

    emit _sendMessageOnLinkOnThread(link, message);

    return true;
}

void Vehicle::_sendMessageOnLink(LinkInterface* link, mavlink_message_t message)
{
    // Make sure this is still a good link
    if (!link || !_links.contains(link) || !link->isConnected()) {
        return;
    }

    // Give the plugin a chance to adjust
    _firmwarePlugin->adjustMavlinkMessage(this, &message);

    static const uint8_t messageKeys[256] = MAVLINK_MESSAGE_CRCS;
    mavlink_finalize_message_chan(&message, _mavlink->getSystemId(), _mavlink->getComponentId(), link->getMavlinkChannel(), message.len, messageKeys[message.msgid]);

    // Write message into buffer, prepending start sign
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buffer, &message);

    link->writeBytes((const char*)buffer, len);
    _messagesSent++;
    emit messagesSentChanged();
}

void Vehicle::_sendMessage(mavlink_message_t message)
{
    // Emit message on all links that are currently connected
    foreach (LinkInterface* link, _links) {
        if (link->isConnected()) {
            _sendMessageOnLink(link, message);
        }
    }
}

/// @return Direct usb connection link to board if one, NULL if none
LinkInterface* Vehicle::priorityLink(void)
{
#ifndef __ios__
    foreach (LinkInterface* link, _links) {
        if (link->isConnected()) {
            SerialLink* pSerialLink = qobject_cast<SerialLink*>(link);
            if (pSerialLink) {
                LinkConfiguration* pLinkConfig = pSerialLink->getLinkConfiguration();
                if (pLinkConfig) {
                    SerialConfiguration* pSerialConfig = qobject_cast<SerialConfiguration*>(pLinkConfig);
                    if (pSerialConfig && pSerialConfig->usbDirect()) {
                        return link;
                    }
                }
            }
        }
    }
#endif
    return _links.count() ? _links[0] : NULL;
}

void Vehicle::setLatitude(double latitude)
{
    _coordinate.setLatitude(latitude);
    emit coordinateChanged(_coordinate);
}

void Vehicle::setLongitude(double longitude){
    _coordinate.setLongitude(longitude);
    emit coordinateChanged(_coordinate);
}

void Vehicle::_updateAttitude(UASInterface*, double roll, double pitch, double yaw, quint64)
{
    if (isinf(roll)) {
        _rollFact.setRawValue(0);
    } else {
        _rollFact.setRawValue(roll * (180.0 / M_PI));
    }
    if (isinf(pitch)) {
        _pitchFact.setRawValue(0);
    } else {
        _pitchFact.setRawValue(pitch * (180.0 / M_PI));
    }
    if (isinf(yaw)) {
        _headingFact.setRawValue(0);
    } else {
        yaw = yaw * (180.0 / M_PI);
        if (yaw < 0) yaw += 360;
        _headingFact.setRawValue(yaw);
    }
}

void Vehicle::_updateAttitude(UASInterface* uas, int, double roll, double pitch, double yaw, quint64 timestamp)
{
    _updateAttitude(uas, roll, pitch, yaw, timestamp);
}

void Vehicle::_updateSpeed(UASInterface*, double groundSpeed, double airSpeed, quint64)
{
    _groundSpeedFact.setRawValue(groundSpeed);
    _airSpeedFact.setRawValue(airSpeed);
}

void Vehicle::_updateAltitude(UASInterface*, double altitudeAMSL, double altitudeRelative, double climbRate, quint64)
{
    _altitudeAMSLFact.setRawValue(altitudeAMSL);
    _altitudeRelativeFact.setRawValue(altitudeRelative);
    _climbRateFact.setRawValue(climbRate);
}

void Vehicle::_updateNavigationControllerErrors(UASInterface*, double altitudeError, double speedError, double xtrackError) {
    _navigationAltitudeError   = altitudeError;
    _navigationSpeedError      = speedError;
    _navigationCrosstrackError = xtrackError;
}

void Vehicle::_updateNavigationControllerData(UASInterface *uas, float, float, float, float targetBearing, float) {
    if (_mav == uas) {
        _navigationTargetBearing = targetBearing;
    }
}

/*
 * Internal
 */

void Vehicle::_checkUpdate()
{
    // Update current location
    if(_mav) {
        if(latitude() != _mav->getLatitude()) {
            setLatitude(_mav->getLatitude());
        }
        if(longitude() != _mav->getLongitude()) {
            setLongitude(_mav->getLongitude());
        }
    }
}

QString Vehicle::getMavIconColor()
{
    // TODO: Not using because not only the colors are ghastly, it doesn't respect dark/light palette
    if(_mav)
        return _mav->getColor().name();
    else
        return QString("black");
}

QString Vehicle::formatedMessages()
{
    QString messages;
    foreach(UASMessage* message, qgcApp()->toolbox()->uasMessageHandler()->messages()) {
        messages += message->getFormatedText();
    }
    return messages;
}

void Vehicle::_handletextMessageReceived(UASMessage* message)
{
    if(message)
    {
        _formatedMessage = message->getFormatedText();
        emit formatedMessageChanged();
    }
}

void Vehicle::_updateState(UASInterface*, QString name, QString)
{
    if (_currentState != name) {
        _currentState = name;
        emit currentStateChanged();
    }
}

void Vehicle::_handleTextMessage(int newCount)
{
    // Reset?
    if(!newCount) {
        _currentMessageCount = 0;
        _currentNormalCount  = 0;
        _currentWarningCount = 0;
        _currentErrorCount   = 0;
        _messageCount        = 0;
        _currentMessageType  = MessageNone;
        emit newMessageCountChanged();
        emit messageTypeChanged();
        emit messageCountChanged();
        return;
    }

    UASMessageHandler* pMh = qgcApp()->toolbox()->uasMessageHandler();
    Q_ASSERT(pMh);
    MessageType_t type = newCount ? _currentMessageType : MessageNone;
    int errorCount     = _currentErrorCount;
    int warnCount      = _currentWarningCount;
    int normalCount    = _currentNormalCount;
    //-- Add current message counts
    errorCount  += pMh->getErrorCount();
    warnCount   += pMh->getWarningCount();
    normalCount += pMh->getNormalCount();
    //-- See if we have a higher level
    if(errorCount != _currentErrorCount) {
        _currentErrorCount = errorCount;
        type = MessageError;
    }
    if(warnCount != _currentWarningCount) {
        _currentWarningCount = warnCount;
        if(_currentMessageType != MessageError) {
            type = MessageWarning;
        }
    }
    if(normalCount != _currentNormalCount) {
        _currentNormalCount = normalCount;
        if(_currentMessageType != MessageError && _currentMessageType != MessageWarning) {
            type = MessageNormal;
        }
    }
    int count = _currentErrorCount + _currentWarningCount + _currentNormalCount;
    if(count != _currentMessageCount) {
        _currentMessageCount = count;
        // Display current total new messages count
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        _currentMessageType = type;
        // Update message level
        emit messageTypeChanged();
    }
    // Update message count (all messages)
    if(newCount != _messageCount) {
        _messageCount = newCount;
        emit messageCountChanged();
    }
    QString errMsg = pMh->getLatestError();
    if(errMsg != _latestError) {
        _latestError = errMsg;
        emit latestErrorChanged();
    }
}

void Vehicle::resetMessages()
{
    // Reset Counts
    int count = _currentMessageCount;
    MessageType_t type = _currentMessageType;
    _currentErrorCount   = 0;
    _currentWarningCount = 0;
    _currentNormalCount  = 0;
    _currentMessageCount = 0;
    _currentMessageType = MessageNone;
    if(count != _currentMessageCount) {
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        emit messageTypeChanged();
    }
}

int Vehicle::manualControlReservedButtonCount(void)
{
    return _firmwarePlugin->manualControlReservedButtonCount();
}

void Vehicle::_loadSettings(void)
{
    QSettings settings;

    settings.beginGroup(QString(_settingsGroup).arg(_id));

    bool convertOk;

    _joystickMode = (JoystickMode_t)settings.value(_joystickModeSettingsKey, JoystickModeRC).toInt(&convertOk);
    if (!convertOk) {
        _joystickMode = JoystickModeRC;
    }

    _joystickEnabled = settings.value(_joystickEnabledSettingsKey, false).toBool();
}

void Vehicle::_saveSettings(void)
{
    QSettings settings;

    settings.beginGroup(QString(_settingsGroup).arg(_id));

    settings.setValue(_joystickModeSettingsKey, _joystickMode);
    settings.setValue(_joystickEnabledSettingsKey, _joystickEnabled);
}

int Vehicle::joystickMode(void)
{
    return _joystickMode;
}

void Vehicle::setJoystickMode(int mode)
{
    if (mode < 0 || mode >= JoystickModeMax) {
        qCWarning(VehicleLog) << "Invalid joystick mode" << mode;
        return;
    }

    _joystickMode = (JoystickMode_t)mode;
    _saveSettings();
    emit joystickModeChanged(mode);
}

QStringList Vehicle::joystickModes(void)
{
    QStringList list;

    list << "Normal" << "Attitude" << "Position" << "Force" << "Velocity";

    return list;
}

bool Vehicle::joystickEnabled(void)
{
    return _joystickEnabled;
}

void Vehicle::setJoystickEnabled(bool enabled)
{
    _joystickEnabled = enabled;
    _startJoystick(_joystickEnabled);
    _saveSettings();
    emit joystickEnabledChanged(_joystickEnabled);
}

void Vehicle::_startJoystick(bool start)
{
#ifndef __mobile__
    Joystick* joystick = _joystickManager->activeJoystick();
    if (joystick) {
        if (start) {
            if (_joystickEnabled) {
                joystick->startPolling(this);
            }
        } else {
            joystick->stopPolling();
        }
    }
#else
    Q_UNUSED(start);
#endif
}

bool Vehicle::active(void)
{
    return _active;
}

void Vehicle::setActive(bool active)
{
    _active = active;

    _startJoystick(_active);
}

QmlObjectListModel* Vehicle::missionItemsModel(void)
{
    return missionManager()->missionItems();
}

bool Vehicle::homePositionAvailable(void)
{
    return _homePositionAvailable;
}

QGeoCoordinate Vehicle::homePosition(void)
{
    return _homePosition;
}

void Vehicle::setArmed(bool armed)
{
    // We specifically use COMMAND_LONG:MAV_CMD_COMPONENT_ARM_DISARM since it is supported by more flight stacks.

    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_COMPONENT_ARM_DISARM;
    cmd.confirmation = 0;
    cmd.param1 = armed ? 1.0f : 0.0f;
    cmd.param2 = 0.0f;
    cmd.param3 = 0.0f;
    cmd.param4 = 0.0f;
    cmd.param5 = 0.0f;
    cmd.param6 = 0.0f;
    cmd.param7 = 0.0f;
    cmd.target_system = id();
    cmd.target_component = 0;

    mavlink_msg_command_long_encode(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, &cmd);

    sendMessage(msg);
}

bool Vehicle::flightModeSetAvailable(void)
{
    return _firmwarePlugin->isCapable(FirmwarePlugin::SetFlightModeCapability);
}

QStringList Vehicle::flightModes(void)
{
    return _firmwarePlugin->flightModes();
}

QString Vehicle::flightMode(void)
{
    return _firmwarePlugin->flightMode(_base_mode, _custom_mode);
}

void Vehicle::setFlightMode(const QString& flightMode)
{
    uint8_t     base_mode;
    uint32_t    custom_mode;

    if (_firmwarePlugin->setFlightMode(flightMode, &base_mode, &custom_mode)) {
        // setFlightMode will only set MAV_MODE_FLAG_CUSTOM_MODE_ENABLED in base_mode, we need to move back in the existing
        // states.
        uint8_t newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_CUSTOM_MODE;
        newBaseMode |= base_mode;

        mavlink_message_t msg;
        mavlink_msg_set_mode_pack(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, id(), newBaseMode, custom_mode);
        sendMessage(msg);
    } else {
        qWarning() << "FirmwarePlugin::setFlightMode failed, flightMode:" << flightMode;
    }
}

bool Vehicle::hilMode(void)
{
    return _base_mode & MAV_MODE_FLAG_HIL_ENABLED;
}

void Vehicle::setHilMode(bool hilMode)
{
    mavlink_message_t msg;

    uint8_t newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_HIL;
    if (hilMode) {
        newBaseMode |= MAV_MODE_FLAG_HIL_ENABLED;
    }

    mavlink_msg_set_mode_pack(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, id(), newBaseMode, _custom_mode);
    sendMessage(msg);
}

bool Vehicle::missingParameters(void)
{
    return _autopilotPlugin->missingParameters();
}

void Vehicle::requestDataStream(MAV_DATA_STREAM stream, uint16_t rate, bool sendMultiple)
{
    mavlink_message_t               msg;
    mavlink_request_data_stream_t   dataStream;

    dataStream.req_stream_id = stream;
    dataStream.req_message_rate = rate;
    dataStream.start_stop = 1;  // start
    dataStream.target_system = id();
    dataStream.target_component = 0;

    mavlink_msg_request_data_stream_encode(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, &dataStream);

    if (sendMultiple) {
        // We use sendMessageMultiple since we really want these to make it to the vehicle
        sendMessageMultiple(msg);
    } else {
        sendMessage(msg);
    }
}

void Vehicle::_sendMessageMultipleNext(void)
{
    if (_nextSendMessageMultipleIndex < _sendMessageMultipleList.count()) {
        qCDebug(VehicleLog) << "_sendMessageMultipleNext:" << _sendMessageMultipleList[_nextSendMessageMultipleIndex].message.msgid;

        sendMessage(_sendMessageMultipleList[_nextSendMessageMultipleIndex].message);

        if (--_sendMessageMultipleList[_nextSendMessageMultipleIndex].retryCount <= 0) {
            _sendMessageMultipleList.removeAt(_nextSendMessageMultipleIndex);
        } else {
            _nextSendMessageMultipleIndex++;
        }
    }

    if (_nextSendMessageMultipleIndex >= _sendMessageMultipleList.count()) {
        _nextSendMessageMultipleIndex = 0;
    }
}

void Vehicle::sendMessageMultiple(mavlink_message_t message)
{
    SendMessageMultipleInfo_t   info;

    info.message =      message;
    info.retryCount =   _sendMessageMultipleRetries;

    _sendMessageMultipleList.append(info);
}

void Vehicle::_missionManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showMessage(QString("Error during Mission communication with Vehicle: %1").arg(errorMsg));
}

void Vehicle::_addNewMapTrajectoryPoint(void)
{
    if (_mapTrajectoryHaveFirstCoordinate) {
        _mapTrajectoryList.append(new CoordinateVector(_mapTrajectoryLastCoordinate, _coordinate, this));
    }
    _mapTrajectoryHaveFirstCoordinate = true;
    _mapTrajectoryLastCoordinate = _coordinate;
}

void Vehicle::_mapTrajectoryStart(void)
{
    _mapTrajectoryHaveFirstCoordinate = false;
    _mapTrajectoryList.clear();
    _mapTrajectoryTimer.start();
}

void Vehicle::_mapTrajectoryStop()
{
    _mapTrajectoryTimer.stop();
}

void Vehicle::_parametersReady(bool parametersReady)
{
    if (parametersReady && !_missionManagerInitialRequestComplete) {
        _missionManagerInitialRequestComplete = true;
        _missionManager->requestMissionItems();
    }

    if (parametersReady) {
        setJoystickEnabled(_joystickEnabled);
    }
}

void Vehicle::disconnectInactiveVehicle(void)
{
    // Vehicle is no longer communicating with us, disconnect all links

    LinkManager* linkMgr = qgcApp()->toolbox()->linkManager();
    for (int i=0; i<_links.count(); i++) {
        linkMgr->disconnectLink(_links[i]);
    }
}

ParameterLoader* Vehicle::getParameterLoader(void)
{
    return _parameterLoader;
}

void Vehicle::_imageReady(UASInterface*)
{
    if(_uas)
    {
        QImage img = _uas->getImage();
        qgcApp()->toolbox()->imageProvider()->setImage(&img, _id);
        _flowImageIndex++;
        emit flowImageIndexChanged();
    }
}

void Vehicle::_remoteControlRSSIChanged(uint8_t rssi)
{
    // Low pass to git rid of jitter
    _rcRSSIstore = (_rcRSSIstore * 0.9f) + ((float)rssi * 0.1);
    uint8_t filteredRSSI = (uint8_t)ceil(_rcRSSIstore);
    if(_rcRSSIstore < 0.1) {
        filteredRSSI = 0;
    }
    if(_rcRSSI != filteredRSSI) {
        _rcRSSI = filteredRSSI;
        emit rcRSSIChanged(_rcRSSI);
    }
}

void Vehicle::virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust)
{
    // The following if statement prevents the virtualTabletJoystick from sending values if the standard joystick is enabled
    if ( !_joystickEnabled ) {
        _uas->setExternalControlSetpoint(roll, pitch, yaw, thrust, 0, JoystickModeRC);
    }
}

void Vehicle::setConnectionLostEnabled(bool connectionLostEnabled)
{
    if (_connectionLostEnabled != connectionLostEnabled) {
        _connectionLostEnabled = connectionLostEnabled;
        emit connectionLostEnabledChanged(_connectionLostEnabled);
    }
}

void Vehicle::_connectionLostTimeout(void)
{
    if (_connectionLostEnabled && !_connectionLost) {
        _connectionLost = true;
        _heardFrom = false;
        emit connectionLostChanged(true);
        _say(QString("connection lost to vehicle %1").arg(id()), GAudioOutput::AUDIO_SEVERITY_NOTICE);
        if (_autoDisconnect) {
            disconnectInactiveVehicle();
        }
    }
}

void Vehicle::_connectionActive(void)
{
    _connectionLostTimer.start();
    if (_connectionLost) {
        _connectionLost = false;
        emit connectionLostChanged(false);
        _say(QString("connection regained to vehicle %1").arg(id()), GAudioOutput::AUDIO_SEVERITY_NOTICE);
    }
}

void Vehicle::_say(const QString& text, int severity)
{
    if (!qgcApp()->runningUnitTests())
        qgcApp()->toolbox()->audioOutput()->say(text.toLower(), severity);
}

bool Vehicle::fixedWing(void) const
{
    return vehicleType() == MAV_TYPE_FIXED_WING;
}

bool Vehicle::multiRotor(void) const
{
    switch (vehicleType()) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
        return true;
    default:
        return false;
    }
}

void Vehicle::_setCoordinateValid(bool coordinateValid)
{
    if (coordinateValid != _coordinateValid) {
        _coordinateValid = coordinateValid;
        emit coordinateValidChanged(_coordinateValid);
    }
}


VehicleGPSFactGroup::VehicleGPSFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/GPSFact.json", parent)
    , _vehicle(NULL)
    , _hdopFact             (0, _hdopFactName,              FactMetaData::valueTypeDouble)
    , _vdopFact             (0, _vdopFactName,              FactMetaData::valueTypeDouble)
    , _courseOverGroundFact (0, _courseOverGroundFactName,  FactMetaData::valueTypeDouble)
    , _countFact            (0, _countFactName,             FactMetaData::valueTypeInt32)
    , _lockFact             (0, _lockFactName,              FactMetaData::valueTypeInt32)
{
    _addFact(&_hdopFact,                _hdopFactName);
    _addFact(&_vdopFact,                _vdopFactName);
    _addFact(&_courseOverGroundFact,    _courseOverGroundFactName);
    _addFact(&_lockFact,                _lockFactName);
    _addFact(&_countFact,               _countFactName);
}

void VehicleGPSFactGroup::setVehicle(Vehicle* vehicle)
{
    _vehicle = vehicle;

    connect(_vehicle->uas(), &UASInterface::localizationChanged, this, &VehicleGPSFactGroup::_setSatLoc);

    UAS* pUas = dynamic_cast<UAS*>(_vehicle->uas());
    connect(pUas, &UAS::satelliteCountChanged,  this, &VehicleGPSFactGroup::_setSatelliteCount);
    connect(pUas, &UAS::satRawHDOPChanged,      this, &VehicleGPSFactGroup::_setSatRawHDOP);
    connect(pUas, &UAS::satRawVDOPChanged,      this, &VehicleGPSFactGroup::_setSatRawVDOP);
    connect(pUas, &UAS::satRawCOGChanged,       this, &VehicleGPSFactGroup::_setSatRawCOG);
}

void VehicleGPSFactGroup::_setSatelliteCount(double val, QString)
{
    // I'm assuming that a negative value or over 99 means there is no GPS
    if(val < 0.0)  val = -1.0;
    if(val > 99.0) val = -1.0;

    _countFact.setRawValue(val);
}

void VehicleGPSFactGroup::_setSatRawHDOP(double val)
{
    _hdopFact.setRawValue(val);
}

void VehicleGPSFactGroup::_setSatRawVDOP(double val)
{
    _vdopFact.setRawValue(val);
}

void VehicleGPSFactGroup::_setSatRawCOG(double val)
{
    _courseOverGroundFact.setRawValue(val);
}

void VehicleGPSFactGroup::_setSatLoc(UASInterface*, int fix)
{
    _lockFact.setRawValue(fix);

    // fix 0: lost, 1: at least one satellite, but no GPS fix, 2: 2D lock, 3: 3D lock
    if (fix > 2) {
        _vehicle->_setCoordinateValid(true);
    }
}

VehicleBatteryFactGroup::VehicleBatteryFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/BatteryFact.json", parent)
    , _vehicle(NULL)
    , _voltageFact          (0, _voltageFactName,           FactMetaData::valueTypeDouble)
    , _percentRemainingFact (0, _percentRemainingFactName,  FactMetaData::valueTypeInt32)
    , _mahConsumedFact      (0, _mahConsumedFactName,       FactMetaData::valueTypeInt32)
    , _currentFact          (0, _currentFactName,           FactMetaData::valueTypeInt32)
    , _temperatureFact      (0, _temperatureFactName,       FactMetaData::valueTypeDouble)
    , _cellCountFact        (0, _cellCountFactName,         FactMetaData::valueTypeInt32)
{
    _addFact(&_voltageFact,             _voltageFactName);
    _addFact(&_percentRemainingFact,    _percentRemainingFactName);
    _addFact(&_mahConsumedFact,         _mahConsumedFactName);
    _addFact(&_currentFact,             _currentFactName);
    _addFact(&_temperatureFact,         _temperatureFactName);
    _addFact(&_cellCountFact,           _cellCountFactName);

    // Start out as not available
    _voltageFact.setRawValue            (_voltageUnavailable);
    _percentRemainingFact.setRawValue   (_percentRemainingUnavailable);
    _mahConsumedFact.setRawValue        (_mahConsumedUnavailable);
    _currentFact.setRawValue            (_currentUnavailable);
    _temperatureFact.setRawValue        (_temperatureUnavailable);
    _cellCountFact.setRawValue          (_cellCountUnavailable);
}

void VehicleBatteryFactGroup::setVehicle(Vehicle* vehicle)
{
    _vehicle = vehicle;
}
