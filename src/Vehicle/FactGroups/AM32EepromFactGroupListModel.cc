/****************************************************************************
 *
 * (c) 2025 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AM32EepromFactGroupListModel.h"
#include "AM32EepromSchema.h"
#include "Vehicle.h"

#include <QtCore/QDebug>

//-----------------------------------------------------------------------------
// AM32Setting Implementation
//-----------------------------------------------------------------------------

AM32Setting::AM32Setting(const AM32FieldDef& fieldDef, QObject* parent)
    : QObject(parent)
    , _eepromByteIndex(fieldDef.offset)
    , _displayName(fieldDef.displayName)
    , _description(fieldDef.description)
    , _unit(fieldDef.unit)
    , _isEnum(fieldDef.isEnum)
    , _isBool(fieldDef.isBool)
    , _fromRaw(fieldDef.fromRaw)
    , _toRaw(fieldDef.toRaw)
{
    _fact = new Fact(0, fieldDef.name, fieldDef.valueType, this);

    // Apply min/max/units to the fact's metadata
    FactMetaData* metaData = _fact->metaData();

    // Use display min/max if available, otherwise raw
    if (fieldDef.displayMin.isValid()) {
        metaData->setRawMin(fieldDef.displayMin);
    } else if (fieldDef.rawMin.isValid()) {
        metaData->setRawMin(fieldDef.rawMin);
    }

    if (fieldDef.displayMax.isValid()) {
        metaData->setRawMax(fieldDef.displayMax);
    } else if (fieldDef.rawMax.isValid()) {
        metaData->setRawMax(fieldDef.rawMax);
    }

    if (!fieldDef.unit.isEmpty()) {
        metaData->setRawUnits(fieldDef.unit);
    }

    // Initialize with a default value (0 in raw units, converted to display units)
    _fact->setRawValue(_fromRaw(0));
}

void AM32Setting::updateFromEeprom(uint8_t value)
{
    _rawOriginalValue = value;
    _fact->setRawValue(_fromRaw(value));
    emit pendingChangesChanged();
}

void AM32Setting::updateConversions(const AM32FieldDef& fieldDef)
{
    _fromRaw = fieldDef.fromRaw;
    _toRaw = fieldDef.toRaw;

    // Update fact metadata with version-specific min/max
    FactMetaData* metaData = _fact->metaData();
    if (fieldDef.displayMin.isValid()) {
        metaData->setRawMin(fieldDef.displayMin);
    }
    if (fieldDef.displayMax.isValid()) {
        metaData->setRawMax(fieldDef.displayMax);
    }
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

void AM32Setting::setMatchesMajority(bool matches)
{
    if (_matchesMajority != matches) {
        _matchesMajority = matches;
        emit matchesMajorityChanged();
    }
}

void AM32Setting::setAllMatch(bool allMatch)
{
    if (_allMatch != allMatch) {
        _allMatch = allMatch;
        emit allMatchChanged();
    }
}

//-----------------------------------------------------------------------------
// AM32EepromFactGroupListModel Implementation
//-----------------------------------------------------------------------------

AM32EepromFactGroupListModel::AM32EepromFactGroupListModel(QObject* parent)
    : FactGroupListModel("am32Eeprom", parent)
{
    // Start fetching schema if not already loaded
    AM32EepromSchema* schema = AM32EepromSchema::instance();
    if (!schema->isLoaded() && !schema->isFetching()) {
        schema->fetchSchema();
    }
}

bool AM32EepromFactGroupListModel::_shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const
{
    if (message.msgid == MAVLINK_MSG_ID_ESC_EEPROM) {
        mavlink_esc_eeprom_t eeprom{};
        mavlink_msg_esc_eeprom_decode(&message, &eeprom);

        // Only handle AM32 firmware messages
        if (eeprom.firmware != ESC_FIRMWARE_AM32) {
            return false;
        }

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
        MAVLINK_MSG_ID_ESC_EEPROM,
        255, // param2: ESC index (255 = all)
        0, 0, 0, 0, 0  // unused params
    );
}

bool AM32EepromFactGroupListModel::_allEscsHaveMatchingChanges(const QList<int>& escIndices)
{
    // Only allow broadcast if ALL ESCs in the system are selected
    if (escIndices.isEmpty() || escIndices.count() != count()) {
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
    qCDebug(AM32EepromLog) << "Writing AM32 EEPROM to ESC" << (escIndex == 255 ? "broadcast" : QString::number(escIndex + 1));
    qCDebug(AM32EepromLog) << "Write mask:" << Qt::hex
             << writeMask[0] << writeMask[1] << writeMask[2]
             << writeMask[3] << writeMask[4] << writeMask[5];

    mavlink_message_t msg;
    mavlink_esc_eeprom_t eeprom{};

    eeprom.target_system = vehicle->id();
    eeprom.target_component = vehicle->defaultComponentId();
    eeprom.firmware = ESC_FIRMWARE_AM32;
    eeprom.msg_index = 0;
    eeprom.msg_count = 1;
    eeprom.esc_index = escIndex;
    memcpy(eeprom.write_mask, writeMask, sizeof(eeprom.write_mask));
    eeprom.length = qMin(data.size(), static_cast<qsizetype>(sizeof(eeprom.data)));
    memcpy(eeprom.data, data.data(), eeprom.length);

    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (sharedLink) {
        mavlink_msg_esc_eeprom_encode_chan(
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
    case MAVLINK_MSG_ID_ESC_EEPROM:
        _handleEscEeprom(vehicle, message);
        break;
    default:
        break;
    }
}

void AM32EepromFactGroup::_handleEscEeprom(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    mavlink_esc_eeprom_t eeprom{};
    mavlink_msg_esc_eeprom_decode(&message, &eeprom);

    // Only handle AM32 firmware messages
    if (eeprom.firmware != ESC_FIRMWARE_AM32) {
        return;
    }

    if (eeprom.esc_index != _idFact.rawValue().toUInt()) {
        // Only handle messages for our ESC index
        return;
    }

    if (eeprom.length != 48) {
        qCWarning(AM32EepromLog) << "AM32 EEPROM data length mismatch:" << eeprom.length;
        return;
    }

    // Store original data
    _originalEepromData = QByteArray(reinterpret_cast<const char*>(eeprom.data), eeprom.length);

    // Parse read-only info
    _eepromVersionFact.setRawValue(eeprom.data[1]);
    _bootloaderVersionFact.setRawValue(eeprom.data[2]);
    _firmwareMajorFact.setRawValue(eeprom.data[3]);
    _firmwareMinorFact.setRawValue(eeprom.data[4]);

    int eepromVer = eeprom.data[1];

    // Apply version-specific overrides to settings
    AM32EepromSchema* schema = AM32EepromSchema::instance();
    for (AM32Setting* setting : _settings) {
        const AM32FieldDef* baseDef = schema->field(setting->name());
        if (baseDef && !baseDef->versionOverrides.isEmpty()) {
            AM32FieldDef updatedDef = *baseDef;
            updatedDef.applyVersionOverrides(eepromVer);
            AM32EepromSchema::setupConversionFunctions(updatedDef);
            setting->updateConversions(updatedDef);
        }
    }

    // Update all settings
    for (AM32Setting* setting : _settings) {
        uint8_t index = setting->byteIndex();
        if (index < eeprom.length) {
            setting->updateFromEeprom(eeprom.data[index]);
        }
    }

    _dataLoaded = true;
    emit dataLoadedChanged();

    // Clear any unsaved changes flag since we just loaded fresh data
    _updateHasUnsavedChanges();

    qCDebug(AM32EepromLog) << "ESC" << (_escIndex + 1) << "received eeprom data, version:" << eepromVersionValue();
}

FactGroupWithId *AM32EepromFactGroupListModel::_createFactGroupWithId(uint32_t id)
{
    auto* esc = new AM32EepromFactGroup(id, this);
    _connectEscSignals(esc);
    return esc;
}

void AM32EepromFactGroupListModel::_connectEscSignals(AM32EepromFactGroup* esc)
{
    // When data is loaded, recalculate majority matches for all ESCs
    connect(esc, &AM32EepromFactGroup::dataLoadedChanged, this, &AM32EepromFactGroupListModel::_updateMajorityMatches);

    // When any setting changes, recalculate majority matches
    connect(esc, &AM32EepromFactGroup::hasUnsavedChangesChanged, this, &AM32EepromFactGroupListModel::_updateMajorityMatches);
}

void AM32EepromFactGroupListModel::_updateMajorityMatches()
{
    // Collect all ESCs
    QList<AM32EepromFactGroup*> escs;
    for (int i = 0; i < count(); i++) {
        auto* esc = value<AM32EepromFactGroup*>(i);
        if (esc && esc->dataLoaded()) {
            escs.append(esc);
        }
    }

    if (escs.isEmpty()) {
        return;
    }

    // Get list of setting names from first ESC
    auto* firstEsc = escs.first();
    QStringList settingNames = firstEsc->settings()->keys();

    // For each setting, find majority value and update all ESCs
    for (const QString& settingName : settingNames) {
        // Count occurrences of each value
        QMap<uint8_t, int> valueCounts;
        for (auto* esc : escs) {
            auto* setting = esc->getSetting(settingName);
            if (setting) {
                uint8_t rawValue = setting->getRawValue();
                valueCounts[rawValue] = valueCounts.value(rawValue, 0) + 1;
            }
        }

        // Find majority value
        uint8_t majorityValue = 0;
        int maxCount = 0;
        for (auto it = valueCounts.begin(); it != valueCounts.end(); ++it) {
            if (it.value() > maxCount) {
                maxCount = it.value();
                majorityValue = it.key();
            }
        }

        // All ESCs match if there's only one unique value
        bool allMatch = (valueCounts.size() == 1);

        // Update each ESC's setting with whether it matches majority and whether all match
        for (auto* esc : escs) {
            auto* setting = esc->getSetting(settingName);
            if (setting) {
                setting->setMatchesMajority(setting->getRawValue() == majorityValue);
                setting->setAllMatch(allMatch);
            }
        }
    }
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

    AM32EepromSchema* schema = AM32EepromSchema::instance();
    if (schema->isLoaded()) {
        _initializeSettingsFromSchema();
    } else {
        // Schema not loaded yet - connect to signal to initialize when ready
        connect(schema, &AM32EepromSchema::schemaLoaded, this, &AM32EepromFactGroup::_initializeSettingsFromSchema);
    }
}

void AM32EepromFactGroup::_initializeSettingsFromSchema()
{
    // Avoid re-initialization
    if (!_settings.isEmpty()) {
        return;
    }

    AM32EepromSchema* schema = AM32EepromSchema::instance();

    // Disconnect from signal now that we're initializing
    disconnect(schema, &AM32EepromSchema::schemaLoaded, this, &AM32EepromFactGroup::_initializeSettingsFromSchema);

    if (!schema->isLoaded()) {
        qCWarning(AM32EepromLog) << "AM32 schema not loaded, ESC settings unavailable";
        return;
    }

    // Get all editable fields from schema
    // Note: We create settings for ALL fields regardless of EEPROM version.
    // The UI will filter based on version. This allows settings to be updated
    // when EEPROM data arrives and we know the actual version.
    QList<const AM32FieldDef*> fields = schema->editableFieldsForEepromVersion(999);

    for (const AM32FieldDef* fieldDef : fields) {
        // Make a mutable copy so we can apply version overrides later if needed
        AM32FieldDef field = *fieldDef;

        auto* setting = new AM32Setting(field, this);

        _addFact(setting->fact());
        _settings.append(setting);
        _settingsMap->insert(field.name, QVariant::fromValue(setting));

        connect(setting, &AM32Setting::pendingChangesChanged, this, &AM32EepromFactGroup::_updateHasUnsavedChanges);
    }

    qCDebug(AM32EepromLog) << "Initialized" << _settings.count() << "settings from schema for ESC" << (_escIndex + 1);
}

QString AM32EepromFactGroup::firmwareVersionString() const
{
    return QStringLiteral("%1.%2")
        .arg(_firmwareMajorFact.rawValue().toInt())
        .arg(_firmwareMinorFact.rawValue().toInt(), 2, 10, QChar('0'));
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

bool AM32EepromFactGroup::isSettingAvailable(const QString& name) const
{
    AM32EepromSchema* schema = AM32EepromSchema::instance();
    const AM32FieldDef* field = schema->field(name);

    if (!field) {
        return false;
    }

    // Check EEPROM version
    if (!field->isAvailableForEepromVersion(eepromVersionValue())) {
        return false;
    }

    // Check firmware version
    if (!field->isAvailableForFirmwareVersion(firmwareVersionString())) {
        return false;
    }

    return true;
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

void AM32EepromFactGroup::_updateHasUnsavedChanges()
{
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
    for (auto* setting : _settings) {
        setting->discardChanges();
    }
    _updateHasUnsavedChanges();
}
