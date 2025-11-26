/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"
#include "FactGroupListModel.h"
#include <QQmlPropertyMap>
#include <functional>

class Vehicle;
class AM32EepromFactGroup;
struct AM32FieldDef;

/// AM32Setting class with conversion support - created dynamically from schema
class AM32Setting : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(QString unit READ unit CONSTANT)
    Q_PROPERTY(Fact* fact READ fact CONSTANT)
    Q_PROPERTY(bool hasPendingChanges READ hasPendingChanges NOTIFY pendingChangesChanged)
    Q_PROPERTY(bool matchesMajority READ matchesMajority NOTIFY matchesMajorityChanged)
    Q_PROPERTY(bool allMatch READ allMatch NOTIFY allMatchChanged)
    Q_PROPERTY(bool isEnum READ isEnum CONSTANT)
    Q_PROPERTY(bool isBool READ isBool CONSTANT)

public:
    /// Create from schema field definition
    explicit AM32Setting(const AM32FieldDef& fieldDef, QObject* parent = nullptr);

    QString name() const { return _fact->name(); }
    QString displayName() const { return _displayName; }
    QString description() const { return _description; }
    QString unit() const { return _unit; }
    Fact* fact() { return _fact; }
    uint8_t byteIndex() const { return _eepromByteIndex; }
    bool isEnum() const { return _isEnum; }
    bool isBool() const { return _isBool; }

    bool hasPendingChanges() const;
    bool matchesMajority() const { return _matchesMajority; }
    void setMatchesMajority(bool matches);
    bool allMatch() const { return _allMatch; }
    void setAllMatch(bool allMatch);

    Q_INVOKABLE void setPendingValue(const QVariant& value);
    void updateFromEeprom(uint8_t value);
    uint8_t getRawValue() const;
    void discardChanges();

signals:
    void pendingChangesChanged();
    void matchesMajorityChanged();
    void allMatchChanged();

private:
    uint8_t _eepromByteIndex;
    uint8_t _rawOriginalValue = 0;
    Fact* _fact;
    QString _displayName;
    QString _description;
    QString _unit;
    bool _matchesMajority = true;
    bool _allMatch = true;
    bool _isEnum = false;
    bool _isBool = false;

    std::function<QVariant(uint8_t)> _fromRaw;
    std::function<uint8_t(QVariant)> _toRaw;
};

class AM32EepromFactGroupListModel : public FactGroupListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit AM32EepromFactGroupListModel(QObject* parent = nullptr);

    /// Request EEPROM read from ESC
    Q_INVOKABLE void requestReadAll(Vehicle* vehicle);

    /// Write settings to all ESCs at once (broadcast)
    Q_INVOKABLE void requestWriteAll(Vehicle* vehicle, const QList<int>& escIndices);

protected:
    // Overrides from FactGroupListModel
    bool _shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const final;
    FactGroupWithId *_createFactGroupWithId(uint32_t id) final;

private:
    bool _allEscsHaveMatchingChanges(const QList<int>& escIndices);
    void _sendEepromWrite(Vehicle* vehicle, uint8_t escIndex, const QByteArray& data, const uint32_t writeMask[6]);
    void _updateMajorityMatches();
    void _connectEscSignals(AM32EepromFactGroup* esc);
};


/// AM32 ESC EEPROM settings fact group
class AM32EepromFactGroup : public FactGroupWithId
{
    Q_OBJECT

    // Read-only info properties
    Q_PROPERTY(Fact* firmwareMajor READ firmwareMajor CONSTANT)
    Q_PROPERTY(Fact* firmwareMinor READ firmwareMinor CONSTANT)
    Q_PROPERTY(Fact* bootloaderVersion READ bootloaderVersion CONSTANT)
    Q_PROPERTY(Fact* eepromVersion READ eepromVersion CONSTANT)

    // Convenience version accessors for QML
    Q_PROPERTY(int eepromVersionValue READ eepromVersionValue NOTIFY dataLoadedChanged)
    Q_PROPERTY(QString firmwareVersionString READ firmwareVersionString NOTIFY dataLoadedChanged)

    // Dynamic qml binding map for AM32Settings
    Q_PROPERTY(QQmlPropertyMap* settings READ settings CONSTANT)

    Q_PROPERTY(bool dataLoaded READ dataLoaded NOTIFY dataLoadedChanged)
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY hasUnsavedChangesChanged)
    Q_PROPERTY(uint8_t escIndex READ escIndex CONSTANT)

public:
    AM32EepromFactGroup(uint8_t escIndex, QObject* parent = nullptr);

    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

    QQmlPropertyMap* settings() { return _settingsMap; }

    // Read-only info facts
    Fact* firmwareMajor() { return &_firmwareMajorFact; }
    Fact* firmwareMinor() { return &_firmwareMinorFact; }
    Fact* bootloaderVersion() { return &_bootloaderVersionFact; }
    Fact* eepromVersion() { return &_eepromVersionFact; }

    // Convenience accessors
    int eepromVersionValue() const { return _eepromVersionFact.rawValue().toInt(); }
    QString firmwareVersionString() const;

    // Status
    bool dataLoaded() const { return _dataLoaded; }
    bool hasUnsavedChanges() const;
    uint8_t escIndex() const { return _escIndex; }

    /// Get a setting by name
    Q_INVOKABLE AM32Setting* getSetting(const QString& name);

    /// Check if a setting is available for this ESC's EEPROM/firmware version
    Q_INVOKABLE bool isSettingAvailable(const QString& name) const;

    /// Check if settings match another ESC
    Q_INVOKABLE bool settingsMatch(AM32EepromFactGroup* other) const;

    /// Discard all pending changes
    Q_INVOKABLE void discardChanges();

    /// Get current EEPROM data for transmission
    QByteArray getEepromData() const;

    void calculateWriteMask(uint32_t writeMask[6]) const;

signals:
    void dataLoadedChanged();
    void hasUnsavedChangesChanged();

private:
    void _handleAM32Eeprom(Vehicle *vehicle, const mavlink_message_t &message);
    void _initializeSettingsFromSchema();
    void _updateHasUnsavedChanges();

    // Info facts (read-only)
    Fact _eepromVersionFact = Fact(0, QStringLiteral("eepromVersion"), FactMetaData::valueTypeUint8);
    Fact _bootloaderVersionFact = Fact(0, QStringLiteral("bootloaderVersion"), FactMetaData::valueTypeUint8);
    Fact _firmwareMajorFact = Fact(0, QStringLiteral("firmwareMajor"), FactMetaData::valueTypeUint8);
    Fact _firmwareMinorFact = Fact(0, QStringLiteral("firmwareMinor"), FactMetaData::valueTypeUint8);

    // Settings list
    QQmlPropertyMap* _settingsMap;
    QList<AM32Setting*> _settings;

    // State
    bool _dataLoaded = false;
    bool _hasUnsavedChanges = false;
    uint8_t _escIndex = 0;
    QByteArray _originalEepromData;
};
