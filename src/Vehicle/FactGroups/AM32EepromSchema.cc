/****************************************************************************
 *
 * (c) 2025 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AM32EepromSchema.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>

AM32EepromSchema* AM32EepromSchema::_instance = nullptr;

AM32EepromSchema* AM32EepromSchema::instance()
{
    if (!_instance) {
        _instance = new AM32EepromSchema();
    }
    return _instance;
}

AM32EepromSchema::AM32EepromSchema(QObject* parent)
    : QObject(parent)
{
}

bool AM32EepromSchema::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit schemaLoadError(QStringLiteral("Failed to open schema file: %1").arg(path));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    return loadFromJson(data);
}

bool AM32EepromSchema::loadFromJson(const QByteArray& jsonData)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);

    if (error.error != QJsonParseError::NoError) {
        emit schemaLoadError(QStringLiteral("JSON parse error: %1").arg(error.errorString()));
        return false;
    }

    if (!doc.isObject()) {
        emit schemaLoadError(QStringLiteral("Schema root must be an object"));
        return false;
    }

    return parseSchema(doc.object());
}

bool AM32EepromSchema::parseSchema(const QJsonObject& root)
{
    _fields.clear();
    _groups.clear();

    _schemaVersion = root.value("version").toString("unknown");

    // Parse fields
    QJsonObject fieldsObj = root.value("fields").toObject();
    for (auto it = fieldsObj.begin(); it != fieldsObj.end(); ++it) {
        QString fieldName = it.key();
        QJsonObject fieldObj = it.value().toObject();

        AM32FieldDef field = parseField(fieldName, fieldObj);
        _fields.insert(fieldName, field);
    }

    // Parse groups
    QJsonObject groupsObj = root.value("groups").toObject();
    for (auto it = groupsObj.begin(); it != groupsObj.end(); ++it) {
        AM32FieldGroup group;
        group.id = it.key();

        QJsonObject groupObj = it.value().toObject();
        group.name = groupObj.value("name").toString();
        group.description = groupObj.value("description").toString();
        group.minEepromVersion = groupObj.value("minEepromVersion").toInt(0);

        QJsonArray fieldsArray = groupObj.value("fields").toArray();
        for (const QJsonValue& v : fieldsArray) {
            group.fieldNames.append(v.toString());
        }

        _groups.append(group);
    }

    _loaded = true;
    emit schemaLoaded();

    qDebug() << "AM32 Schema loaded:" << _fields.count() << "fields," << _groups.count() << "groups";
    return true;
}

AM32FieldDef AM32EepromSchema::parseField(const QString& name, const QJsonObject& fieldObj)
{
    AM32FieldDef field;
    field.name = name;
    field.displayName = fieldObj.value("name").toString(name);
    field.description = fieldObj.value("description").toString();
    field.unit = fieldObj.value("unit").toString();
    field.offset = fieldObj.value("offset").toInt();
    field.size = fieldObj.value("size").toInt(1);
    field.isReadOnly = fieldObj.value("readOnly").toBool(false);

    // Version gating
    field.minEepromVersion = fieldObj.value("minEepromVersion").toInt(0);
    field.maxEepromVersion = fieldObj.value("maxEepromVersion").toInt(999);
    field.minFirmwareVersion = fieldObj.value("minFirmwareVersion").toString();
    field.maxFirmwareVersion = fieldObj.value("maxFirmwareVersion").toString();

    // Determine type
    QString typeStr = fieldObj.value("type").toString("uint8");
    field.isBool = (typeStr == "bool");
    field.isEnum = (typeStr == "enum");

    // Parse raw constraints
    QJsonObject rawObj = fieldObj.value("raw").toObject();
    if (!rawObj.isEmpty()) {
        if (rawObj.contains("min")) {
            field.rawMin = rawObj.value("min").toVariant();
        }
        if (rawObj.contains("max")) {
            field.rawMax = rawObj.value("max").toVariant();
        }
    }

    // Parse display conversion
    QJsonObject displayObj = fieldObj.value("display").toObject();
    if (!displayObj.isEmpty()) {
        field.displayFactor = displayObj.value("factor").toDouble(1.0);
        field.displayOffset = displayObj.value("offset").toDouble(0.0);
        field.displayDecimals = displayObj.value("decimals").toInt(0);

        if (displayObj.contains("min")) {
            field.displayMin = displayObj.value("min").toVariant();
        }
        if (displayObj.contains("max")) {
            field.displayMax = displayObj.value("max").toVariant();
        }
    }

    // Parse enum values
    if (field.isEnum) {
        QJsonArray valuesArray = fieldObj.value("values").toArray();
        for (const QJsonValue& v : valuesArray) {
            QJsonObject enumObj = v.toObject();
            int raw = enumObj.value("raw").toInt();
            QString enumName = enumObj.value("name").toString();
            field.enumValues.append(qMakePair(raw, enumName));
        }
    }

    // Parse disabled value
    QJsonObject disabledObj = fieldObj.value("disabledValue").toObject();
    if (!disabledObj.isEmpty()) {
        field.disabledRawValue = disabledObj.value("raw").toVariant();
        field.disabledDisplayText = disabledObj.value("display").toString("Disabled");
    }

    // Parse version-specific overrides
    QJsonObject versionsObj = fieldObj.value("versions").toObject();
    for (auto it = versionsObj.begin(); it != versionsObj.end(); ++it) {
        QString versionKey = it.key();

        // Parse version keys like "eeprom:3+" or "firmware:2.18+"
        if (versionKey.startsWith("eeprom:")) {
            QString verStr = versionKey.mid(7);
            verStr.remove('+');
            int ver = verStr.toInt();
            field.versionOverrides.insert(ver, it.value().toObject());
        }
        // "default" is stored as version 0
        else if (versionKey == "default") {
            field.versionOverrides.insert(0, it.value().toObject());
        }
    }

    // Determine the FactMetaData value type
    field.valueType = determineValueType(field);

    // Setup conversion functions
    setupConversionFunctions(field);

    return field;
}

FactMetaData::ValueType_t AM32EepromSchema::determineValueType(const AM32FieldDef& field)
{
    if (field.isBool) {
        return FactMetaData::valueTypeBool;
    }

    if (field.isEnum) {
        return FactMetaData::valueTypeUint8;
    }

    // If there's a display conversion
    if (field.displayFactor != 1.0 || field.displayOffset != 0.0) {
        // Use double if we have decimals
        if (field.displayDecimals > 0) {
            return FactMetaData::valueTypeDouble;
        }
        // Use int32 if offset might produce negative values
        if (field.displayOffset < 0 ||
            (field.displayMin.isValid() && field.displayMin.toDouble() < 0)) {
            return FactMetaData::valueTypeInt32;
        }
        return FactMetaData::valueTypeUint32;
    }

    return FactMetaData::valueTypeUint8;
}

void AM32EepromSchema::setupConversionFunctions(AM32FieldDef& field)
{
    double factor = field.displayFactor;
    double offset = field.displayOffset;

    // Identity conversion
    if (factor == 1.0 && offset == 0.0) {
        field.fromRaw = [](uint8_t v) { return QVariant(v); };
        field.toRaw = [](QVariant v) { return static_cast<uint8_t>(v.toUInt()); };
        return;
    }

    // Setup conversion based on value type
    switch (field.valueType) {
    case FactMetaData::valueTypeDouble:
        field.fromRaw = [factor, offset](uint8_t v) {
            return QVariant(v * factor + offset);
        };
        field.toRaw = [factor, offset](QVariant v) {
            return static_cast<uint8_t>((v.toDouble() - offset) / factor);
        };
        break;

    case FactMetaData::valueTypeInt32:
        field.fromRaw = [factor, offset](uint8_t v) {
            return QVariant(static_cast<int>(v * factor + offset));
        };
        field.toRaw = [factor, offset](QVariant v) {
            return static_cast<uint8_t>((v.toInt() - offset) / factor);
        };
        break;

    case FactMetaData::valueTypeUint32:
        field.fromRaw = [factor, offset](uint8_t v) {
            return QVariant(static_cast<uint32_t>(v * factor + offset));
        };
        field.toRaw = [factor, offset](QVariant v) {
            return static_cast<uint8_t>((v.toUInt() - offset) / factor);
        };
        break;

    default:
        field.fromRaw = [](uint8_t v) { return QVariant(v); };
        field.toRaw = [](QVariant v) { return static_cast<uint8_t>(v.toUInt()); };
        break;
    }
}

void AM32FieldDef::applyVersionOverrides(int eepromVersion)
{
    // Find the highest version override that applies
    int bestVersion = 0;
    QJsonObject* bestOverride = nullptr;

    for (auto it = versionOverrides.begin(); it != versionOverrides.end(); ++it) {
        int overrideVersion = it.key();
        if (overrideVersion <= eepromVersion && overrideVersion > bestVersion) {
            bestVersion = overrideVersion;
            bestOverride = &it.value();
        }
    }

    if (!bestOverride) {
        return;
    }

    // Apply overrides
    QJsonObject& override = *bestOverride;

    if (override.contains("raw")) {
        QJsonObject rawObj = override.value("raw").toObject();
        if (rawObj.contains("min")) rawMin = rawObj.value("min").toVariant();
        if (rawObj.contains("max")) rawMax = rawObj.value("max").toVariant();
    }

    if (override.contains("display")) {
        QJsonObject displayObj = override.value("display").toObject();
        if (displayObj.contains("factor")) displayFactor = displayObj.value("factor").toDouble();
        if (displayObj.contains("offset")) displayOffset = displayObj.value("offset").toDouble();
        if (displayObj.contains("decimals")) displayDecimals = displayObj.value("decimals").toInt();
        if (displayObj.contains("min")) displayMin = displayObj.value("min").toVariant();
        if (displayObj.contains("max")) displayMax = displayObj.value("max").toVariant();
    }

    if (override.contains("values")) {
        enumValues.clear();
        QJsonArray valuesArray = override.value("values").toArray();
        for (const QJsonValue& v : valuesArray) {
            QJsonObject enumObj = v.toObject();
            int raw = enumObj.value("raw").toInt();
            QString enumName = enumObj.value("name").toString();
            enumValues.append(qMakePair(raw, enumName));
        }
    }
}

bool AM32FieldDef::isAvailableForFirmwareVersion(const QString& version) const
{
    if (minFirmwareVersion.isEmpty() && maxFirmwareVersion.isEmpty()) {
        return true;
    }

    // Simple version comparison (assumes format like "2.16" or "v2.16")
    auto parseVersion = [](const QString& v) -> QPair<int, int> {
        QString cleaned = v;
        cleaned.remove('v').remove('V');
        QStringList parts = cleaned.split('.');
        int major = parts.value(0).toInt();
        int minor = parts.value(1).toInt();
        return qMakePair(major, minor);
    };

    auto compareVersions = [](QPair<int, int> a, QPair<int, int> b) -> int {
        if (a.first != b.first) return a.first - b.first;
        return a.second - b.second;
    };

    QPair<int, int> current = parseVersion(version);

    if (!minFirmwareVersion.isEmpty()) {
        QPair<int, int> minVer = parseVersion(minFirmwareVersion);
        if (compareVersions(current, minVer) < 0) {
            return false;
        }
    }

    if (!maxFirmwareVersion.isEmpty()) {
        QPair<int, int> maxVer = parseVersion(maxFirmwareVersion);
        if (compareVersions(current, maxVer) > 0) {
            return false;
        }
    }

    return true;
}

const AM32FieldDef* AM32EepromSchema::field(const QString& name) const
{
    auto it = _fields.find(name);
    if (it != _fields.end()) {
        return &it.value();
    }
    return nullptr;
}

QList<const AM32FieldDef*> AM32EepromSchema::fieldsForEepromVersion(int version) const
{
    QList<const AM32FieldDef*> result;

    for (auto it = _fields.begin(); it != _fields.end(); ++it) {
        if (it.value().isAvailableForEepromVersion(version)) {
            result.append(&it.value());
        }
    }

    // Sort by offset
    std::sort(result.begin(), result.end(), [](const AM32FieldDef* a, const AM32FieldDef* b) {
        return a->offset < b->offset;
    });

    return result;
}

QList<const AM32FieldDef*> AM32EepromSchema::editableFieldsForEepromVersion(int version) const
{
    QList<const AM32FieldDef*> result;

    for (auto it = _fields.begin(); it != _fields.end(); ++it) {
        const AM32FieldDef& field = it.value();
        if (field.isAvailableForEepromVersion(version) &&
            !field.isReadOnly &&
            field.size == 1) {  // Skip array fields like melody
            result.append(&field);
        }
    }

    // Sort by offset
    std::sort(result.begin(), result.end(), [](const AM32FieldDef* a, const AM32FieldDef* b) {
        return a->offset < b->offset;
    });

    return result;
}

QList<AM32FieldGroup> AM32EepromSchema::groupsForEepromVersion(int version) const
{
    QList<AM32FieldGroup> result;

    for (const AM32FieldGroup& group : _groups) {
        if (version >= group.minEepromVersion) {
            result.append(group);
        }
    }

    return result;
}
