/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "EscStatusFactGroupListModel.h"
#include "QGCMAVLink.h"

EscStatusFactGroupListModel::EscStatusFactGroupListModel(QObject* parent)
    : FactGroupListModel("escStatus", parent)
{

}

bool EscStatusFactGroupListModel::_shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const
{
    bool shouldHandle = false;
    uint32_t firstIndex;

    ids.clear();

    switch (message.msgid) {
    case MAVLINK_MSG_ID_ESC_INFO:
    {
        mavlink_esc_info_t escInfo{};
        mavlink_msg_esc_info_decode(&message, &escInfo);
        firstIndex = escInfo.index;
        // ESC_INFO should only be handled if index is multiple of 4
        shouldHandle = (firstIndex % 4 == 0);
    }
        break;

    case MAVLINK_MSG_ID_ESC_STATUS:
    {
        mavlink_esc_status_t escStatus{};
        mavlink_msg_esc_status_decode(&message, &escStatus);
        firstIndex = escStatus.index;
        // ESC_STATUS should only be handled if index is multiple of 4
        shouldHandle = (firstIndex % 4 == 0);
    }
        break;

    case MAVLINK_MSG_ID_AM32_EEPROM:
    {
        mavlink_am32_eeprom_t eeprom{};
        mavlink_msg_am32_eeprom_decode(&message, &eeprom);
        // AM32_EEPROM uses individual ESC index, not first-of-4
        firstIndex = eeprom.index;
        shouldHandle = true;
    }
        break;

    default:
        shouldHandle = false; // Not a message we care about
        break;
    }

    if (shouldHandle) {
        if (message.msgid == MAVLINK_MSG_ID_AM32_EEPROM) {
            // AM32 messages refer to individual ESCs
            ids.append(firstIndex);
        } else {
            // ESC_STATUS and ESC_INFO cover 4 consecutive ESCs
            for (uint32_t index = firstIndex; index <= firstIndex + 3; index++) {
                ids.append(index);
            }
        }
    }

    return shouldHandle;
}

FactGroupWithId *EscStatusFactGroupListModel::_createFactGroupWithId(uint32_t id)
{
    return new EscStatusFactGroup(id, this);
}

EscStatusFactGroup::EscStatusFactGroup(uint32_t escIndex, QObject *parent)
    : FactGroupWithId(1000, QStringLiteral(":/json/Vehicle/EscStatusFactGroup.json"), parent)
{
    _addFact(&_rpmFact);
    _addFact(&_currentFact);
    _addFact(&_voltageFact);
    _addFact(&_countFact);
    _addFact(&_connectionTypeFact);
    _addFact(&_infoFact);
    _addFact(&_failureFlagsFact);
    _addFact(&_errorCountFact);
    _addFact(&_temperatureFact);

    _idFact.setRawValue(escIndex);
    _rpmFact.setRawValue(0);
    _currentFact.setRawValue(0);
    _voltageFact.setRawValue(0);
    _countFact.setRawValue(0);
    _connectionTypeFact.setRawValue(0);
    _infoFact.setRawValue(0);
    _failureFlagsFact.setRawValue(0);
    _errorCountFact.setRawValue(0);
    _temperatureFact.setRawValue(0);

    // Create the AM32EepromFactGroup if necessary
    qDebug() << "Creating AM32EepromFactGroup for ESC" << escIndex;
    _am32EepromFactGroup = new AM32EepromFactGroup(this, escIndex);
}

void EscStatusFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_ESC_INFO:
        _handleEscInfo(vehicle, message);
        break;
    case MAVLINK_MSG_ID_ESC_STATUS:
        _handleEscStatus(vehicle, message);
        break;
    case MAVLINK_MSG_ID_AM32_EEPROM:
        _handleAm32Eeprom(vehicle, message);
        break;
    default:
        break;
    }
}

void EscStatusFactGroup::_handleAm32Eeprom(Vehicle *vehicle, const mavlink_message_t &message)
{
    mavlink_am32_eeprom_t eeprom{};
    mavlink_msg_am32_eeprom_decode(&message, &eeprom);

    if (eeprom.index != _idFact.rawValue().toUInt()) {
        // Only handle messages for our ESC index
        return;
    }

    // TODO: fix this

    // // Create the AM32EepromFactGroup if necessary
    // if (_am32EepromFactGroup == nullptr) {
    //     qDebug() << "Creating AM32EepromFactGroup for ESC" << eeprom.index;
    //     _am32EepromFactGroup = new AM32EepromFactGroup(this, eeprom.index);
    // }

    if (eeprom.mode == 0) {
        // Read response - parse the EEPROM data
        _am32EepromFactGroup->handleEepromData(eeprom.data, eeprom.length);

        // qDebug() << "AM32 EEPROM data received for ESC" << eeprom.index;
        // qDebug() << "Firmware:" << _am32EepromFactGroup->firmwareMajor()->rawValue().toUInt()
        //          << "." << _am32EepromFactGroup->firmwareMinor()->rawValue().toUInt();
    }
    else if (eeprom.mode == 1) {
        // Write acknowledgment
        emit _am32EepromFactGroup->writeComplete(true);
        qDebug() << "AM32 EEPROM write acknowledged for ESC" << eeprom.index;
    }
}

void EscStatusFactGroup::_handleEscInfo(Vehicle *vehicle, const mavlink_message_t &message)
{
    mavlink_esc_info_t escInfo{};
    mavlink_msg_esc_info_decode(&message, &escInfo);

    uint8_t index = _idFact.rawValue().toUInt();

    if (index < escInfo.index || index >= escInfo.index + 4) {
        // Disregard ESC info messages which are not targeted at this ESC index
        return;
    }

    index %= 4; // Convert to 0-based index for the arrays in escInfo

    _countFact.setRawValue(escInfo.count);
    _connectionTypeFact.setRawValue(escInfo.connection_type);
    _infoFact.setRawValue(escInfo.info);
    _failureFlagsFact.setRawValue(escInfo.failure_flags[index]);
    _errorCountFact.setRawValue(escInfo.error_count[index]);
    _temperatureFact.setRawValue(escInfo.temperature[index]);

    _setTelemetryAvailable(true);
}

void EscStatusFactGroup::_handleEscStatus(Vehicle *vehicle, const mavlink_message_t &message)
{
    mavlink_esc_status_t escStatus{};
    mavlink_msg_esc_status_decode(&message, &escStatus);

    uint8_t index = _idFact.rawValue().toUInt();

    if (index < escStatus.index || index >= escStatus.index + 4) {
        // Disregard ESC info messages which are not targeted at this ESC index
        return;
    }

    index %= 4; // Convert to 0-based index for the arrays in escInfo
    _rpmFact.setRawValue(escStatus.rpm[index]);
    _currentFact.setRawValue(escStatus.current[index]);
    _voltageFact.setRawValue(escStatus.voltage[index]);

    _setTelemetryAvailable(true);
}
