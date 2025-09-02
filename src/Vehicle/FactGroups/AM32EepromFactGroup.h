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
#include "QmlObjectListModel.h"
#include <QtCore/QVariantMap>
#include <functional>

class Vehicle;

struct AM32SettingConfig {
    QString name;
    FactMetaData::ValueType_t type;
    uint8_t eepromByteIndex;

    // Conversion functions
    std::function<QVariant(uint8_t)> fromRaw;  // Convert raw byte to display value
    std::function<uint8_t(QVariant)> toRaw;    // Convert display value to raw byte

    // Constructor with default identity conversions
    AM32SettingConfig(const QString& name,
                      FactMetaData::ValueType_t type,
                      uint8_t eepromByteIndex,
                      std::function<QVariant(uint8_t)> fromRaw = nullptr,
                      std::function<uint8_t(QVariant)> toRaw = nullptr)
        : name(name)
        , type(type)
        , eepromByteIndex(eepromByteIndex)
        , fromRaw(fromRaw ? fromRaw : [](uint8_t v) { return QVariant(v); })
        , toRaw(toRaw ? toRaw : [](QVariant v) { return v.toUInt(); })
    {
    }
};

// AM32Setting class with conversion support
class AM32Setting : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(Fact* fact READ fact CONSTANT)
    Q_PROPERTY(bool hasPendingChanges READ hasPendingChanges NOTIFY pendingChangesChanged)

public:
    AM32Setting(int escIndex, const AM32SettingConfig& config);

    QString name() const { return _fact->name(); }
    Fact* fact() { return _fact; }
    uint8_t byteIndex() const { return _eepromByteIndex; }

    bool hasPendingChanges() const;

    Q_INVOKABLE void setPendingValue(const QVariant& value);
    void updateFromEeprom(uint8_t value);
    uint8_t getRawValueForTransmit() const;
    void discardChanges();

signals:
    void pendingChangesChanged();

private:
    int _escIndex;
    uint8_t _eepromByteIndex;
    uint8_t _rawValue = 0;
    Fact* _fact;

    std::function<QVariant(uint8_t)> _fromRaw;
    std::function<uint8_t(QVariant)> _toRaw;
};

/// AM32 ESC EEPROM settings fact group
class AM32EepromFactGroup : public FactGroup
{
    Q_OBJECT

    // Read-only info properties
    Q_PROPERTY(Fact* firmwareMajor READ firmwareMajor CONSTANT)
    Q_PROPERTY(Fact* firmwareMinor READ firmwareMinor CONSTANT)
    Q_PROPERTY(Fact* bootloaderVersion READ bootloaderVersion CONSTANT)
    Q_PROPERTY(Fact* eepromVersion READ eepromVersion CONSTANT)

    Q_PROPERTY(bool dataLoaded READ dataLoaded NOTIFY dataLoadedChanged)
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY hasUnsavedChangesChanged)
    Q_PROPERTY(int escIndex READ escIndex CONSTANT)

public:
    AM32EepromFactGroup(QObject* parent = nullptr, int escIndex = 0);

    // Read-only info facts
    Fact* firmwareMajor() { return &_firmwareMajorFact; }
    Fact* firmwareMinor() { return &_firmwareMinorFact; }
    Fact* bootloaderVersion() { return &_bootloaderVersionFact; }
    Fact* eepromVersion() { return &_eepromVersionFact; }

    // Status
    bool dataLoaded() const { return _dataLoaded; }
    bool hasUnsavedChanges() const;
    int escIndex() const { return _escIndex; }

    /// Get a setting by name
    Q_INVOKABLE AM32Setting* getSetting(const QString& name);

    /// Check if settings match another ESC
    Q_INVOKABLE bool settingsMatch(AM32EepromFactGroup* other) const;

    /// Request EEPROM read from ESC
    Q_INVOKABLE void requestReadAll(Vehicle* vehicle);

    /// Write EEPROM data to ESC (only modified bytes)
    Q_INVOKABLE void requestWrite(Vehicle* vehicle);

    /// Discard all pending changes
    Q_INVOKABLE void discardChanges();

    /// Parse EEPROM data from AM32_EEPROM message
    void handleEepromData(const uint8_t* data, int length);

    /// Get modified EEPROM data for transmission
    QByteArray getModifiedEepromData() const;

signals:
    void dataLoadedChanged();
    void hasUnsavedChangesChanged();
    void writeComplete(bool success);

private:
    void initializeEepromFacts();
    void updateHasUnsavedChanges();
    void calculateWriteMask(uint32_t writeMask[6]) const;

    // Info facts (read-only)
    Fact _eepromVersionFact = Fact(0, QStringLiteral("eepromVersion"), FactMetaData::valueTypeUint8);
    Fact _bootloaderVersionFact = Fact(0, QStringLiteral("bootloaderVersion"), FactMetaData::valueTypeUint8);
    Fact _firmwareMajorFact = Fact(0, QStringLiteral("firmwareMajor"), FactMetaData::valueTypeUint8);
    Fact _firmwareMinorFact = Fact(0, QStringLiteral("firmwareMinor"), FactMetaData::valueTypeUint8);

    // Settings list
    QList<AM32Setting*> _am32_settings;

    // State
    bool _dataLoaded = false;
    bool _hasUnsavedChanges = false;
    int _escIndex = 0;
    QByteArray _originalEepromData;
};
