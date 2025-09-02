/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"
#include <QtCore/QSet>
#include <QtCore/QVariantMap>
#include <QtCore/QHash>
#include "QmlObjectListModel.h"

class Vehicle;

struct AM32SettingConfig {

    AM32SettingConfig(QString name, FactMetaData::ValueType_t type, uint8_t eepromByteIndex)
    {
        this->name = name;
        this->type = type;
        this->eepromByteIndex = eepromByteIndex;
    };

    QString name {};
    uint8_t eepromByteIndex {};
    FactMetaData::ValueType_t type {};   // type is in the Fact metadata json

    // TODO: lambda which converts from _fact.value to _rawValue and vice versa

    // QVariant from {};                    // min is in the Fact metadata json
    // QVariant to {};                      // max is in the Fact metadata json
    // QVariant step {};                    // increment is in the Fact metadata json
    // uint8_t decimals {};                 // decimalPlaces is in the Fact metadata json
};

class AM32Setting : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    // TODO: add Q_PROPERTY and qml bindings for the name/values

public:
    AM32Setting(int escIndex, const AM32SettingConfig& config)
        : _escIndex(escIndex)
        , _eepromByteIndex(config.eepromByteIndex)
    {
        // TODO: shouldn't the Fact be built from the json?? Why do we have to double specify? Why do we need to create the fact
        // with new at all? Shouldn't the FactGroup.cc create the facts?
        _fact = new Fact(0, config.name, config.type, nullptr);
    }

    QString name() { return _fact->name(); };
    Fact* fact() { return _fact; };

    uint8_t byteIndex() { return _eepromByteIndex; };


    void updateFromEeprom(uint8_t value);
    {
        _rawValue = value;

        // TODO: function to convert rawValue to Fact value and vice versa (need to send raw uint8_t bytes but display the converted value)

        // Dismiss pending changes
        _pendingValue = _rawValue;
    }


private:
    // NOTE: these are contained in the Fact metadata json
    // Defines range, step, and decimals
    // QVariant _from {};
    // QVariant _to {};
    // QVariant _step {};
    // int _decimals {};

    // ESC
    uint8_t _escIndex {};
    uint8_t _eepromByteIndex {}; // Memory location in the eeprom where this setting resides (see AM32/Inc/eeprom.h)

    // Values
    uint8_t _rawValue {};       // stores the raw eeprom value
    Fact* _fact {};             // Current value from the eeprom
    QVariant _pendingValue {};  // Pending value, user updated

    // Converts the display value to the uint8_t format expected in the AM32_EEPROM.data structure.
    uint8_t convertDisplayValueToRawValue();

    // If the _pendingValue does not match the _fact.rawValue(), then we have pending changes...
};

/// AM32 ESC EEPROM settings fact group
class AM32EepromFactGroup : public FactGroup
{
    Q_OBJECT

    // Read-only info properties
    Q_PROPERTY(Fact* firmwareMajor         READ firmwareMajor          CONSTANT)
    Q_PROPERTY(Fact* firmwareMinor         READ firmwareMinor          CONSTANT)
    Q_PROPERTY(Fact* bootloaderVersion     READ bootloaderVersion      CONSTANT)
    Q_PROPERTY(Fact* eepromVersion         READ eepromVersion          CONSTANT)

    Q_PROPERTY(bool  dataLoaded            READ dataLoaded             NOTIFY dataLoadedChanged)
    Q_PROPERTY(bool  hasUnsavedChanges     READ hasUnsavedChanges      NOTIFY hasUnsavedChangesChanged)
    Q_PROPERTY(int   escIndex              READ escIndex               WRITE setEscIndex NOTIFY escIndexChanged)

public:
    AM32EepromFactGroup(QObject* parent = nullptr);

    void initializeEepromFacts();

    // Read-only info facts
    Fact* firmwareMajor()       { return &_firmwareMajorFact; }
    Fact* firmwareMinor()       { return &_firmwareMinorFact; }
    Fact* bootloaderVersion()    { return &_bootloaderVersionFact; }
    Fact* eepromVersion()        { return &_eepromVersionFact; }

    // Status
    bool dataLoaded() const { return _dataLoaded; }
    bool hasUnsavedChanges() const { return !_pendingChanges.isEmpty(); }
    int escIndex() const { return _escIndex; }
    void setEscIndex(int index);

    /// Parse EEPROM data from AM32_EEPROM message
    void handleEepromData(const uint8_t* data, int length);

    /// Pack current facts into EEPROM data for writing
    QByteArray packEepromData() const;

    /// Calculate write mask based on modified bytes
    void calculateWriteMask(uint32_t writeMask[6]) const;

    /// Request EEPROM read from ESC
    Q_INVOKABLE void requestReadAll(Vehicle* vehicle);

    /// Write EEPROM data to ESC (only modified bytes)
    Q_INVOKABLE void requestWrite(Vehicle* vehicle);

    /// Apply pending changes to this ESC
    Q_INVOKABLE void applyPendingChanges(const QVariantMap& changes);

    /// Get pending changes
    Q_INVOKABLE QVariantMap getPendingChanges() const { return _pendingChanges; }

    /// Clear pending changes (after write)
    Q_INVOKABLE void clearPendingChanges();

    /// Clear a specific pending change for a fact
    Q_INVOKABLE void clearPendingChange(const QString& factName);

    /// Discard pending changes and revert to fact values
    Q_INVOKABLE void discardChanges();

    /// Check if settings match another ESC
    Q_INVOKABLE bool settingsMatch(AM32EepromFactGroup* other) const;

    /// Get a fact value by name (includes pending changes)
    Q_INVOKABLE QVariant getFactValue(const QString& factName) const;

    /// Get the original loaded value for a fact (before any changes)
    Q_INVOKABLE QVariant getOriginalValue(const QString& factName) const;

    /// Check if a specific fact has pending changes
    Q_INVOKABLE bool hasPendingChange(const QString& factName) const { return _pendingChanges.contains(factName); }

signals:
    void dataLoadedChanged();
    void hasUnsavedChangesChanged();
    void escIndexChanged();
    void writeComplete(bool success);
    void pendingChangesUpdated();

private:
    Fact* _getFactByName(const QString& name) const;


    // Info facts (read-only)
    Fact _firmwareMajorFact         = Fact(0, QStringLiteral("firmwareMajor"), FactMetaData::valueTypeUint8);
    Fact _firmwareMinorFact         = Fact(0, QStringLiteral("firmwareMinor"), FactMetaData::valueTypeUint8);
    Fact _bootloaderVersionFact     = Fact(0, QStringLiteral("bootloaderVersion"), FactMetaData::valueTypeUint8);
    Fact _eepromVersionFact         = Fact(0, QStringLiteral("eepromVersion"), FactMetaData::valueTypeUint8);

    QmlObjectListModel _am32_settings;

    bool _dataLoaded = false;
    int _escIndex = 0;







    // Change tracking
    QSet<int> _modifiedBytes;              // Set of byte indices that have been modified
    QByteArray _originalEepromData;        // Original EEPROM data from last read
    QVariantMap _originalValues;           // Original parsed values from EEPROM (fact name -> value)

    // Maps for efficient lookups
    QHash<QString, Fact*> _factsByName;    // Map fact names to fact pointers
    QHash<Fact*, int> _factToByteIndex;    // Map facts to their EEPROM byte index

    // Pending changes not yet written
    QVariantMap _pendingChanges;           // Map of fact name to pending value
};
