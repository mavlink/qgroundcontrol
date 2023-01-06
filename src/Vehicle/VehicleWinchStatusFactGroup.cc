/****************************************************************************
 *
 * (c) TODO
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleWinchStatusFactGroup.h"
#include "Vehicle.h"

const char* VehicleWinchStatusFactGroup::_lineLengthFactName =     "lineLength";
const char* VehicleWinchStatusFactGroup::_speedFactName =          "speed";
const char* VehicleWinchStatusFactGroup::_tensionFactName =        "tension";
const char* VehicleWinchStatusFactGroup::_voltageFactName =        "voltage";
const char* VehicleWinchStatusFactGroup::_currentFactName =        "current";
const char* VehicleWinchStatusFactGroup::_temperatureFactName =    "temperature";
const char* VehicleWinchStatusFactGroup::_healthyFactName =        "healthy";
const char* VehicleWinchStatusFactGroup::_fullyRetractedFactName = "fullyRetracted";
const char* VehicleWinchStatusFactGroup::_movingFactName =         "moving";
const char* VehicleWinchStatusFactGroup::_clutchEngagedFactName =  "clutchEngaged";
const char* VehicleWinchStatusFactGroup::_lockedFactName =         "locked";
const char* VehicleWinchStatusFactGroup::_droppingFactName =       "dropping";
const char* VehicleWinchStatusFactGroup::_arrestingFactName =      "arresting";
const char* VehicleWinchStatusFactGroup::_groundSenseFactName =    "groundSense";
const char* VehicleWinchStatusFactGroup::_retractingFactName =     "retracting";
const char* VehicleWinchStatusFactGroup::_redeliverFactName =      "redeliver";
const char* VehicleWinchStatusFactGroup::_abandonLineFactName =    "abandonLine";
const char* VehicleWinchStatusFactGroup::_lockingFactName =        "locking";
const char* VehicleWinchStatusFactGroup::_loadLineFactName =       "loadLine";
const char* VehicleWinchStatusFactGroup::_loadPayloadFactName =    "loadPayload";
const char* VehicleWinchStatusFactGroup::_dropAllowedFactName =    "dropAllowed";
const char* VehicleWinchStatusFactGroup::_dropBlockerFactName =    "dropBlocker";
const char* VehicleWinchStatusFactGroup::_staleFactName =          "stale";

static const int kStaleTimeout = 6;  // * update rate = 6 * 500 = 3 seconds

VehicleWinchStatusFactGroup::VehicleWinchStatusFactGroup(QObject* parent)
    : FactGroup            (500, ":/json/Vehicle/WinchStatusFactGroup.json", parent)
    , _lineLengthFact      (0, _lineLengthFactName,      FactMetaData::valueTypeFloat)
    , _speedFact           (0, _speedFactName,           FactMetaData::valueTypeFloat)
    , _tensionFact         (0, _tensionFactName,         FactMetaData::valueTypeFloat)
    , _voltageFact         (0, _voltageFactName,         FactMetaData::valueTypeFloat)
    , _currentFact         (0, _currentFactName,         FactMetaData::valueTypeFloat)
    , _temperatureFact     (0, _temperatureFactName,     FactMetaData::valueTypeFloat)
    , _healthyFact         (0, _healthyFactName,         FactMetaData::valueTypeBool)
    , _fullyRetractedFact  (0, _fullyRetractedFactName,  FactMetaData::valueTypeBool)
    , _movingFact          (0, _movingFactName,          FactMetaData::valueTypeBool)
    , _clutchEngagedFact   (0, _clutchEngagedFactName,   FactMetaData::valueTypeBool)
    , _lockedFact          (0, _lockedFactName,          FactMetaData::valueTypeBool)
    , _droppingFact        (0, _droppingFactName,        FactMetaData::valueTypeBool)
    , _arrestingFact       (0, _arrestingFactName,       FactMetaData::valueTypeBool)
    , _groundSenseFact     (0, _groundSenseFactName,     FactMetaData::valueTypeBool)
    , _retractingFact      (0, _retractingFactName,      FactMetaData::valueTypeBool)
    , _redeliverFact       (0, _redeliverFactName,       FactMetaData::valueTypeBool)
    , _abandonLineFact     (0, _abandonLineFactName,     FactMetaData::valueTypeBool)
    , _lockingFact         (0, _lockingFactName,         FactMetaData::valueTypeBool)
    , _loadLineFact        (0, _loadLineFactName,        FactMetaData::valueTypeBool)
    , _loadPayloadFact     (0, _loadPayloadFactName,     FactMetaData::valueTypeBool)
    , _dropAllowedFact     (0, _dropAllowedFactName,     FactMetaData::valueTypeUint8)
    , _dropBlockerFact     (0, _dropBlockerFactName,     FactMetaData::valueTypeUint8)
    , _staleFact           (0, _staleFactName,           FactMetaData::valueTypeBool)
    , _staleCounter        (0)
{
    _addFact(&_lineLengthFact,     _lineLengthFactName);
    _addFact(&_speedFact,          _speedFactName);
    _addFact(&_tensionFact,        _tensionFactName);
    _addFact(&_voltageFact,        _voltageFactName);
    _addFact(&_currentFact,        _currentFactName);
    _addFact(&_temperatureFact,    _temperatureFactName);
    _addFact(&_healthyFact,        _healthyFactName);
    _addFact(&_fullyRetractedFact, _fullyRetractedFactName);
    _addFact(&_movingFact,         _movingFactName);
    _addFact(&_clutchEngagedFact,  _clutchEngagedFactName);
    _addFact(&_lockedFact,         _lockedFactName);
    _addFact(&_droppingFact,       _droppingFactName);
    _addFact(&_arrestingFact,      _arrestingFactName);
    _addFact(&_groundSenseFact,    _groundSenseFactName);
    _addFact(&_retractingFact,     _retractingFactName);
    _addFact(&_redeliverFact,      _redeliverFactName);
    _addFact(&_abandonLineFact,    _abandonLineFactName);
    _addFact(&_lockingFact,        _lockingFactName);
    _addFact(&_loadLineFact,       _loadLineFactName);
    _addFact(&_loadPayloadFact,    _loadPayloadFactName);
    _addFact(&_dropAllowedFact,    _dropAllowedFactName);
    _addFact(&_dropBlockerFact,    _dropBlockerFactName);
    _addFact(&_staleFact,          _staleFactName);
}

enum MAV_WINCH_STATUS_FLAG_EXT
{
    MAV_WINCH_STATUS_LOCKED       = 16,
    MAV_WINCH_STATUS_DROPPING     = 32,
    MAV_WINCH_STATUS_ARRESTING    = 64,
    MAV_WINCH_STATUS_GROUND_SENSE = 128,
    MAV_WINCH_STATUS_RETRACTING   = 256,
    MAV_WINCH_STATUS_REDELIVER    = 512,
    MAV_WINCH_STATUS_ABANDON_LINE = 1024,
    MAV_WINCH_STATUS_LOCKING      = 2048,
    MAV_WINCH_STATUS_LOAD_LINE    = 4096,
    MAV_WINCH_STATUS_LOAD_PAYLOAD = 8192
};

#define CUSTOM_AVIANT_STATUS_SHIFT         24
#define CUSTOM_AVIANT_STATUS_MASK          0xFF  // After shift, max 8 bit
#define CUSTOM_AVIANT_STATUS_DROP_ALLOWED  1     // After shift

void VehicleWinchStatusFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_WINCH_STATUS) {
        return;
    }

    mavlink_winch_status_t winchStatus;
    mavlink_msg_winch_status_decode(&message, &winchStatus);

    lineLength()->setRawValue      (winchStatus.line_length);
    speed()->setRawValue           (winchStatus.speed);
    tension()->setRawValue         (winchStatus.tension);
    voltage()->setRawValue         (winchStatus.voltage);
    current()->setRawValue         (winchStatus.current);
    temperature()->setRawValue     (winchStatus.temperature);
    healthy()->setRawValue         (!!(winchStatus.status & MAV_WINCH_STATUS_HEALTHY));
    fullyRetracted()->setRawValue  (!!(winchStatus.status & MAV_WINCH_STATUS_FULLY_RETRACTED));
    moving()->setRawValue          (!!(winchStatus.status & MAV_WINCH_STATUS_MOVING));
    clutchEngaged()->setRawValue   (!!(winchStatus.status & MAV_WINCH_STATUS_CLUTCH_ENGAGED));
    locked()->setRawValue          (!!(winchStatus.status & MAV_WINCH_STATUS_LOCKED));
    dropping()->setRawValue        (!!(winchStatus.status & MAV_WINCH_STATUS_DROPPING));
    arresting()->setRawValue       (!!(winchStatus.status & MAV_WINCH_STATUS_ARRESTING));
    groundSense()->setRawValue     (!!(winchStatus.status & MAV_WINCH_STATUS_GROUND_SENSE));
    retracting()->setRawValue      (!!(winchStatus.status & MAV_WINCH_STATUS_RETRACTING));
    redeliver()->setRawValue       (!!(winchStatus.status & MAV_WINCH_STATUS_REDELIVER));
    abandonLine()->setRawValue     (!!(winchStatus.status & MAV_WINCH_STATUS_ABANDON_LINE));
    locking()->setRawValue         (!!(winchStatus.status & MAV_WINCH_STATUS_LOCKING));
    loadLine()->setRawValue        (!!(winchStatus.status & MAV_WINCH_STATUS_LOAD_LINE));
    loadPayload()->setRawValue     (!!(winchStatus.status & MAV_WINCH_STATUS_LOAD_PAYLOAD));
    uint8_t custom_aviant_status = static_cast<uint8_t>(winchStatus.status >> CUSTOM_AVIANT_STATUS_SHIFT & CUSTOM_AVIANT_STATUS_MASK);
    dropAllowed()->setRawValue     (!!(custom_aviant_status & CUSTOM_AVIANT_STATUS_DROP_ALLOWED));
    dropBlocker()->setRawValue     (custom_aviant_status);
    stale()->setRawValue           (false);
    _staleCounter = 0;

    _setTelemetryAvailable(true);
}

void VehicleWinchStatusFactGroup::_updateAllValues()
{
    if (++_staleCounter > kStaleTimeout) {
        lineLength()->setRawValue      (lineLength()->rawDefaultValue());
        speed()->setRawValue           (speed()->rawDefaultValue());
        tension()->setRawValue         (tension()->rawDefaultValue());
        voltage()->setRawValue         (voltage()->rawDefaultValue());
        current()->setRawValue         (current()->rawDefaultValue());
        temperature()->setRawValue     (temperature()->rawDefaultValue());
        healthy()->setRawValue         (healthy()->rawDefaultValue());
        fullyRetracted()->setRawValue  (fullyRetracted()->rawDefaultValue());
        moving()->setRawValue          (moving()->rawDefaultValue());
        clutchEngaged()->setRawValue   (clutchEngaged()->rawDefaultValue());
        locked()->setRawValue          (locked()->rawDefaultValue());
        dropping()->setRawValue        (dropping()->rawDefaultValue());
        arresting()->setRawValue       (arresting()->rawDefaultValue());
        groundSense()->setRawValue     (groundSense()->rawDefaultValue());
        retracting()->setRawValue      (retracting()->rawDefaultValue());
        redeliver()->setRawValue       (redeliver()->rawDefaultValue());
        abandonLine()->setRawValue     (abandonLine()->rawDefaultValue());
        locking()->setRawValue         (locking()->rawDefaultValue());
        loadLine()->setRawValue        (loadLine()->rawDefaultValue());
        loadPayload()->setRawValue     (loadPayload()->rawDefaultValue());
        dropAllowed()->setRawValue     (dropAllowed()->rawDefaultValue());
        dropBlocker()->setRawValue     (dropBlocker()->rawDefaultValue());
        stale()->setRawValue           (true);
    }

    FactGroup::_updateAllValues();
}

