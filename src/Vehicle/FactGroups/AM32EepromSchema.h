/****************************************************************************
 *
 * (c) 2025 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactMetaData.h"

#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <functional>

/// Schema-based field definition loaded from JSON
struct AM32FieldDef {
    QString name;
    QString displayName;
    QString description;
    QString unit;
    int offset = 0;
    int size = 1;

    // Type info
    FactMetaData::ValueType_t valueType = FactMetaData::valueTypeUint8;
    bool isReadOnly = false;
    bool isBool = false;
    bool isEnum = false;

    // Version gating
    int minEepromVersion = 0;
    int maxEepromVersion = 999;
    QString minFirmwareVersion;
    QString maxFirmwareVersion;

    // Raw value constraints
    QVariant rawMin;
    QVariant rawMax;

    // Display conversion: display = raw * factor + offset
    double displayFactor = 1.0;
    double displayOffset = 0.0;
    int displayDecimals = 0;
    QVariant displayMin;
    QVariant displayMax;

    // Enum values (for enum types)
    QList<QPair<int, QString>> enumValues;

    // Disabled value indicator
    QVariant disabledRawValue;
    QString disabledDisplayText;

    // Version-specific overrides (eepromVersion -> override data)
    QMap<int, QJsonObject> versionOverrides;

    // Conversion functions (generated from factor/offset)
    std::function<QVariant(uint8_t)> fromRaw;
    std::function<uint8_t(QVariant)> toRaw;

    /// Check if this field is available for the given EEPROM version
    bool isAvailableForEepromVersion(int version) const {
        return version >= minEepromVersion && version <= maxEepromVersion;
    }

    /// Check if this field is available for the given firmware version
    bool isAvailableForFirmwareVersion(const QString& version) const;

    /// Get the effective display values for a specific EEPROM version
    /// (applies version-specific overrides if present)
    void applyVersionOverrides(int eepromVersion);
};

/// Field group definition for UI organization
struct AM32FieldGroup {
    QString id;
    QString name;
    QString description;
    QStringList fieldNames;
    int minEepromVersion = 0;
};

/// Singleton class that loads and manages the AM32 EEPROM schema
class AM32EepromSchema : public QObject
{
    Q_OBJECT

public:
    static AM32EepromSchema* instance();

    /// Load schema from a JSON file (resource or filesystem)
    bool loadFromFile(const QString& path);

    /// Load schema from JSON data
    bool loadFromJson(const QByteArray& jsonData);

    /// Get all field definitions
    const QMap<QString, AM32FieldDef>& fields() const { return _fields; }

    /// Get a specific field definition by name
    const AM32FieldDef* field(const QString& name) const;

    /// Get fields available for a specific EEPROM version
    QList<const AM32FieldDef*> fieldsForEepromVersion(int version) const;

    /// Get editable (non-readonly) fields for a specific EEPROM version
    QList<const AM32FieldDef*> editableFieldsForEepromVersion(int version) const;

    /// Get field groups
    const QList<AM32FieldGroup>& groups() const { return _groups; }

    /// Get groups available for a specific EEPROM version
    QList<AM32FieldGroup> groupsForEepromVersion(int version) const;

    /// Get schema version
    QString schemaVersion() const { return _schemaVersion; }

    /// Check if schema is loaded
    bool isLoaded() const { return _loaded; }

signals:
    void schemaLoaded();
    void schemaLoadError(const QString& error);

private:
    AM32EepromSchema(QObject* parent = nullptr);

    bool parseSchema(const QJsonObject& root);
    AM32FieldDef parseField(const QString& name, const QJsonObject& fieldObj);
    void setupConversionFunctions(AM32FieldDef& field);
    FactMetaData::ValueType_t determineValueType(const AM32FieldDef& field);

    QMap<QString, AM32FieldDef> _fields;
    QList<AM32FieldGroup> _groups;
    QString _schemaVersion;
    bool _loaded = false;

    static AM32EepromSchema* _instance;
};
