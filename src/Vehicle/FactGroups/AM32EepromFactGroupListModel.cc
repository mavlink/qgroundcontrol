/****************************************************************************
 *
 * (c) 2025 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AM32EepromFactGroupListModel.h"
#include "Vehicle.h"
#include <QtCore/QDebug>
// #include <cstring>

//-----------------------------------------------------------------------------
// AM32Setting Implementation
//-----------------------------------------------------------------------------

AM32Setting::AM32Setting(const AM32SettingConfig& config, QObject* parent)
    : QObject(parent)
    , _eepromByteIndex(config.eepromByteIndex)
    , _fromRaw(config.fromRaw)
    , _toRaw(config.toRaw)
{
    _fact = new Fact(0, config.name, config.type, this);
}

void AM32Setting::updateFromEeprom(uint8_t value)
{
    _rawOriginalValue = value;
    _fact->setRawValue(_fromRaw(value));
    emit pendingChangesChanged();
}

uint8_t AM32Setting::getRawValue() const
{
    return _toRaw(_fact->rawValue());
}

void AM32Setting::setPendingValue(const QVariant& value)
{
    _fact->setRawValue(value);
    emit pendingChangesChanged();
}

bool AM32Setting::hasPendingChanges() const
{
    return getRawValue() != _rawOriginalValue;
}

void AM32Setting::discardChanges()
{
    _fact->setRawValue(_fromRaw(_rawOriginalValue));
    emit pendingChangesChanged();
}

//-----------------------------------------------------------------------------
// AM32EepromFactGroupListModel Implementation
//-----------------------------------------------------------------------------

AM32EepromFactGroupListModel::AM32EepromFactGroupListModel(QObject* parent)
    : FactGroupListModel("am32Eeprom", parent)
{}

bool AM32EepromFactGroupListModel::_shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const
{
    if (message.msgid == MAVLINK_MSG_ID_AM32_EEPROM) {
        mavlink_am32_eeprom_t eeprom{};
        mavlink_msg_am32_eeprom_decode(&message, &eeprom);
        ids.append(eeprom.esc_index);
        return true;
    }

    return false;
}

void AM32EepromFactGroupListModel::requestReadAll(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_REQUEST_MESSAGE,
        false,  // showError
        MAVLINK_MSG_ID_AM32_EEPROM,
        255, // param2: ESC index (255 = all)
        0, 0, 0, 0, 0  // unused params
    );
}

bool AM32EepromFactGroupListModel::_allEscsHaveMatchingChanges(const QList<int>& escIndices)
{
    if (escIndices.isEmpty()) {
        return false;
    }

    auto* firstEsc = value<AM32EepromFactGroup*>(escIndices[0]);
    if (!firstEsc || !firstEsc->hasUnsavedChanges()) {
        return false;
    }

    for (int i = 1; i < escIndices.count(); i++) {
        auto* esc = value<AM32EepromFactGroup*>(escIndices[i]);
        if (!esc || !esc->hasUnsavedChanges()) {
            return false;
        }

        if (!esc->settingsMatch(firstEsc)) {
            return false;
        }
    }

    return true;
}

void AM32EepromFactGroupListModel::_sendEepromWrite(Vehicle* vehicle, uint8_t escIndex, const QByteArray& data, const uint32_t writeMask[6])
{
    qDebug() << "Writing AM32 EEPROM to ESC" << (escIndex == 255 ? "broadcast" : QString::number(escIndex + 1));
    qDebug() << "Write mask:" << Qt::hex
             << writeMask[0] << writeMask[1] << writeMask[2]
             << writeMask[3] << writeMask[4] << writeMask[5];

    mavlink_message_t msg;
    mavlink_am32_eeprom_t eeprom;

    eeprom.target_system = vehicle->id();
    eeprom.target_component = vehicle->defaultComponentId();
    eeprom.esc_index = escIndex;
    memcpy(eeprom.write_mask, writeMask, sizeof(eeprom.write_mask));
    eeprom.length = qMin(data.size(), static_cast<qsizetype>(sizeof(eeprom.data)));
    memcpy(eeprom.data, data.data(), eeprom.length);

    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (sharedLink) {
        mavlink_msg_am32_eeprom_encode_chan(
            vehicle->id(),
            vehicle->defaultComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            &eeprom
        );

        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void AM32EepromFactGroupListModel::requestWriteAll(Vehicle* vehicle, const QList<int>& escIndices)
{
    if (!vehicle || escIndices.isEmpty()) {
        return;
    }

    if (_allEscsHaveMatchingChanges(escIndices)) {
        // Broadcast write - all ESCs get the same data
        qDebug() << "All ESCs have matching changes, using broadcast write";

        auto* firstEsc = value<AM32EepromFactGroup*>(escIndices[0]);
        if (!firstEsc) {
            return;
        }

        QByteArray packedData = firstEsc->getEepromData();

        uint32_t writeMask[6] = {0};
        for (int escIndex : escIndices) {
            auto* esc = value<AM32EepromFactGroup*>(escIndex);
            if (esc && esc->hasUnsavedChanges()) {
                uint32_t escWriteMask[6];
                esc->calculateWriteMask(escWriteMask);
                for (int i = 0; i < 6; i++) {
                    writeMask[i] |= escWriteMask[i];
                }
            }
        }

        _sendEepromWrite(vehicle, 255, packedData, writeMask);
    } else {
        // Individual writes - ESCs have different changes
        qDebug() << "ESCs have different changes, writing individually";

        for (int escIndex : escIndices) {
            auto* esc = value<AM32EepromFactGroup*>(escIndex);
            if (esc && esc->hasUnsavedChanges()) {
                QByteArray packedData = esc->getEepromData();
                uint32_t writeMask[6];
                esc->calculateWriteMask(writeMask);
                _sendEepromWrite(vehicle, esc->escIndex(), packedData, writeMask);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// AM32EepromFactGroup Implementation
//-----------------------------------------------------------------------------

void AM32EepromFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_AM32_EEPROM:
        _handleAM32Eeprom(vehicle, message);
        break;
    default:
        break;
    }
}

void AM32EepromFactGroup::_handleAM32Eeprom(Vehicle *vehicle, const mavlink_message_t &message)
{
    mavlink_am32_eeprom_t eeprom{};
    mavlink_msg_am32_eeprom_decode(&message, &eeprom);

    if (eeprom.esc_index != _idFact.rawValue().toUInt()) {
        // Only handle messages for our ESC index
        return;
    }

    if (eeprom.length != 48) {
        qWarning() << "AM32 EEPROM data length mismatch:" << eeprom.length;
        return;
    }

    // Store original data
    _originalEepromData = QByteArray(reinterpret_cast<const char*>(eeprom.data), eeprom.length);

    // Parse read-only info
    _eepromVersionFact.setRawValue(eeprom.data[1]);
    _bootloaderVersionFact.setRawValue(eeprom.data[2]);
    _firmwareMajorFact.setRawValue(eeprom.data[3]);
    _firmwareMinorFact.setRawValue(eeprom.data[4]);

    // Update all settings
    for (AM32Setting* setting : _settings) {
        uint8_t index = setting->byteIndex();
        if (index < eeprom.length) {
            // qDebug() << "Updating " << setting->name() << "(" << index << ")" << "to" << eeprom.data[index];
            setting->updateFromEeprom(eeprom.data[index]);
        }
    }

    _dataLoaded = true;
    emit dataLoadedChanged();

    // Clear any unsaved changes flag since we just loaded fresh data
    updateHasUnsavedChanges();

    qDebug() << "ESC" << (_escIndex + 1) << "received eeprom data";
}

FactGroupWithId *AM32EepromFactGroupListModel::_createFactGroupWithId(uint32_t id)
{
    // qDebug() << "_createFactGroupWithId: " << id;
    return new AM32EepromFactGroup(id, this);
}

AM32EepromFactGroup::AM32EepromFactGroup(uint8_t escIndex, QObject* parent)
    : FactGroupWithId(1000, QStringLiteral(":/json/Vehicle/AM32EepromFact.json"), parent)
    , _settingsMap(new QQmlPropertyMap(this))
    , _escIndex(escIndex)
{
    _idFact.setRawValue(escIndex);

    // Add read-only facts to the group
    _addFact(&_eepromVersionFact);
    _addFact(&_bootloaderVersionFact);
    _addFact(&_firmwareMajorFact);
    _addFact(&_firmwareMinorFact);

    initializeEepromFacts();
}

void AM32EepromFactGroup::initializeEepromFacts()
{
    AM32SettingConfig configuration_array[] = {
        // Byte 5
        { "maxRampSpeed", FactMetaData::valueTypeDouble, 5,
          [](uint8_t v) { return QVariant(v / 10.0); },
          [](QVariant v) { return uint8_t(v.toDouble() * 10); }
        },

        // Byte 6
        { "minDutyCycle", FactMetaData::valueTypeDouble, 6,
          [](uint8_t v) { return QVariant(v / 2.0); },
          [](QVariant v) { return uint8_t(v.toDouble() * 2); }
        },

        // Byte 7
        { "disableStickCalibration", FactMetaData::valueTypeBool, 7 },

        // Byte 8
        { "absoluteVoltageCutoff", FactMetaData::valueTypeDouble, 8,
          [](uint8_t v) { return QVariant(v * 0.5); },
          [](QVariant v) { return uint8_t(v.toDouble() / 0.5); }
        },

        // Byte 9
        { "currentPidP", FactMetaData::valueTypeUint32, 9,
          [](uint8_t v) { return QVariant(uint32_t(v * 2)); },
          [](QVariant v) { return uint8_t(v.toUInt() / 2); }
        },

        // Byte 10
        { "currentPidI", FactMetaData::valueTypeUint32, 10 },

        // Byte 11
        { "currentPidD", FactMetaData::valueTypeUint32, 11,
          [](uint8_t v) { return QVariant(uint32_t(v * 10)); },
          [](QVariant v) { return uint8_t(v.toUInt() / 10); }
        },

        // Byte 12
        { "activeBrakePower", FactMetaData::valueTypeUint8, 12 },

        // Bytes 13-16 are reserved

        // Byte 17
        { "directionReversed", FactMetaData::valueTypeBool, 17 },

        // Byte 18
        { "bidirectionalMode", FactMetaData::valueTypeBool, 18 },

        // Byte 19
        { "sineStartup", FactMetaData::valueTypeBool, 19 },

        // Byte 20
        { "complementaryPwm", FactMetaData::valueTypeBool, 20 },

        // Byte 21
        { "variablePwmFreq", FactMetaData::valueTypeBool, 21 },

        // Byte 22
        { "stuckRotorProtection", FactMetaData::valueTypeBool, 22 },

        // Byte 23
        { "timingAdvance", FactMetaData::valueTypeDouble, 23,
          [](uint8_t v) { return QVariant(v * 0.9375); },
          [](QVariant v) { return uint8_t(v.toDouble() / 0.9375); }
        },

        // Byte 24
        { "pwmFrequency", FactMetaData::valueTypeUint8, 24 },

        // Byte 25
        { "startupPower", FactMetaData::valueTypeUint8, 25 },

        // Byte 26
        { "motorKv", FactMetaData::valueTypeUint32, 26,
          [](uint8_t v) { return QVariant(uint32_t(v * 40)); },
          [](QVariant v) { return uint8_t(v.toUInt() / 40); }
        },

        // Byte 27
        { "motorPoles", FactMetaData::valueTypeUint8, 27 },

        // Byte 28
        { "brakeOnStop", FactMetaData::valueTypeBool, 28 },

        // Byte 29
        { "antiStall", FactMetaData::valueTypeBool, 29 },

        // Byte 30
        { "beepVolume", FactMetaData::valueTypeUint8, 30 },

        // Byte 31
        { "telemetry30ms", FactMetaData::valueTypeBool, 31 },

        // Byte 32
        { "servoLowThreshold", FactMetaData::valueTypeUint32, 32,
          [](uint8_t v) { return QVariant(uint32_t((v * 2) + 750)); },
          [](QVariant v) { return uint8_t((v.toUInt() - 750) / 2); }
        },

        // Byte 33
        { "servoHighThreshold", FactMetaData::valueTypeUint32, 33,
          [](uint8_t v) { return QVariant(uint32_t((v * 2) + 1750)); },
          [](QVariant v) { return uint8_t((v.toUInt() - 1750) / 2); }
        },

        // Byte 34
        { "servoNeutral", FactMetaData::valueTypeUint32, 34,
          [](uint8_t v) { return QVariant(uint32_t(1374 + v)); },
          [](QVariant v) { return uint8_t(v.toUInt() - 1374); }
        },

        // Byte 35
        { "servoDeadband", FactMetaData::valueTypeUint8, 35 },

        // Byte 36
        { "lowVoltageCutoff", FactMetaData::valueTypeBool, 36 },

        // Byte 37
        { "lowVoltageThreshold", FactMetaData::valueTypeDouble, 37,
          [](uint8_t v) { return QVariant((v + 250) / 100.0); },
          [](QVariant v) { return uint8_t((v.toDouble() * 100) - 250); }
        },

        // Byte 38
        { "rcCarReversing", FactMetaData::valueTypeBool, 38 },

        // Byte 39
        { "hallSensors", FactMetaData::valueTypeBool, 39 },

        // Byte 40
        { "sineModeRange", FactMetaData::valueTypeUint8, 40 },

        // Byte 41
        { "dragBrakeStrength", FactMetaData::valueTypeUint8, 41 },

        // Byte 42
        { "runningBrakeLevel", FactMetaData::valueTypeUint8, 42 },

        // Byte 43
        { "temperatureLimit", FactMetaData::valueTypeUint8, 43 },

        // Byte 44
        { "currentLimit", FactMetaData::valueTypeUint8, 44,
          [](uint8_t v) { return QVariant(uint32_t(v * 2)); },
          [](QVariant v) { return uint8_t(v.toUInt() / 2); }
        },

        // Byte 45
        { "sineModePower", FactMetaData::valueTypeUint8, 45 },

        // Byte 46
        { "inputType", FactMetaData::valueTypeUint8, 46 },

        // Byte 47
        { "autoTiming", FactMetaData::valueTypeUint8, 47 }
    };

    for (const auto& config : configuration_array) {
        auto setting = new AM32Setting(config, this);

        _addFact(setting->fact());
        _settings.append(setting);
        _settingsMap->insert(config.name, QVariant::fromValue(setting));

        connect(setting, &AM32Setting::pendingChangesChanged, this, &AM32EepromFactGroup::updateHasUnsavedChanges);
    }
}

AM32Setting* AM32EepromFactGroup::getSetting(const QString& name)
{
    for (auto* setting : _settings) {
        if (setting->name() == name) {
            return setting;
        }
    }
    return nullptr;
}

bool AM32EepromFactGroup::hasUnsavedChanges() const
{
    for (const auto* setting : _settings) {
        if (setting->hasPendingChanges()) {
            return true;
        }
    }
    return false;
}

void AM32EepromFactGroup::updateHasUnsavedChanges()
{
    // qDebug() << "updateHasUnsavedChanges";
    bool hasChanges = hasUnsavedChanges();
    if (_hasUnsavedChanges != hasChanges) {
        _hasUnsavedChanges = hasChanges;
        emit hasUnsavedChangesChanged();
    }
}

bool AM32EepromFactGroup::settingsMatch(AM32EepromFactGroup* other) const
{
    if (!other || !_dataLoaded || !other->_dataLoaded) {
        return false;
    }

    // Compare all settings
    for (const auto* mySetting : _settings) {
        auto* otherSetting = other->getSetting(mySetting->name());

        if (!otherSetting ||
            mySetting->getRawValue() != otherSetting->getRawValue()) {
            return false;
        }
    }

    return true;
}

QByteArray AM32EepromFactGroup::getEepromData() const
{
    QByteArray data = _originalEepromData;

    if (data.isEmpty()) {
        data = QByteArray(48, 0);
        data[0] = 1; // Start byte
    }

    // Write all current setting values - the write mask controls what actually gets written
    for (const auto* setting : _settings) {
        uint8_t index = setting->byteIndex();
        if (index < data.size()) {
            data[index] = setting->getRawValue();
        }
    }

    return data;
}

void AM32EepromFactGroup::calculateWriteMask(uint32_t writeMask[6]) const
{
    // Initialize mask to all zeros
    memset(writeMask, 0, 6 * sizeof(uint32_t));

    // Set bits only for modified bytes
    for (const auto* setting : _settings) {
        if (setting->hasPendingChanges()) {
            uint8_t byteIndex = setting->byteIndex();
            if (byteIndex < 192) {
                int maskIndex = byteIndex / 32;
                int bitIndex = byteIndex % 32;
                writeMask[maskIndex] |= (1U << bitIndex);
            }
        }
    }

    // Never write to read-only bytes 0-4
    writeMask[0] &= 0xFFFFFFE0;  // Clear bits 0-4
}

void AM32EepromFactGroup::discardChanges()
{
    // qDebug() << "discardChanges";
    for (auto* setting : _settings) {
        setting->discardChanges();
    }
    updateHasUnsavedChanges();
}
