/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FactGroup.h"
#include "JsonHelper.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>
#include <QFile>
#include <QQmlEngine>

QGC_LOGGING_CATEGORY(FactGroupLog, "FactGroupLog")

const char* FactGroup::_decimalPlacesJsonKey =      "decimalPlaces";
const char* FactGroup::_nameJsonKey =               "name";
const char* FactGroup::_propertiesJsonKey =         "properties";
const char* FactGroup::_versionJsonKey =            "version";
const char* FactGroup::_typeJsonKey =               "type";
const char* FactGroup::_shortDescriptionJsonKey =   "shortDescription";
const char* FactGroup::_unitsJsonKey =              "units";
const char* FactGroup::_defaultValueJsonKey =       "defaultValue";
const char* FactGroup::_minJsonKey =                "min";
const char* FactGroup::_maxJsonKey =                "max";

FactGroup::FactGroup(int updateRateMsecs, const QString& metaDataFile, QObject* parent)
    : QObject(parent)
    , _updateRateMSecs(updateRateMsecs)
{
    if (_updateRateMSecs > 0) {
        connect(&_updateTimer, &QTimer::timeout, this, &FactGroup::_updateAllValues);
        _updateTimer.setSingleShot(false);
        _updateTimer.start(_updateRateMSecs);
    }

    _loadMetaData(metaDataFile);
}

Fact* FactGroup::getFact(const QString& name)
{
    Fact* fact = NULL;

    if (name.contains(".")) {
        QStringList parts = name.split(".");
        if (parts.count() != 2) {
            qWarning() << "Only single level of hierarchy supported";
            return NULL;
        }

        FactGroup * factGroup = getFactGroup(parts[0]);
        if (!factGroup) {
            qWarning() << "Unknown FactGroup" << parts[0];
            return NULL;
        }

        return factGroup->getFact(parts[1]);
    }

    if (_nameToFactMap.contains(name)) {
        fact = _nameToFactMap[name];
        QQmlEngine::setObjectOwnership(fact, QQmlEngine::CppOwnership);
    } else {
        qWarning() << "Unknown Fact" << name;
    }

    return fact;
}

FactGroup* FactGroup::getFactGroup(const QString& name)
{
    FactGroup* factGroup = NULL;

    if (_nameToFactGroupMap.contains(name)) {
        factGroup = _nameToFactGroupMap[name];
        QQmlEngine::setObjectOwnership(factGroup, QQmlEngine::CppOwnership);
    } else {
        qWarning() << "Unknown FactGroup" << name;
    }

    return factGroup;
}

void FactGroup::_addFact(Fact* fact, const QString& name)
{
    if (_nameToFactMap.contains(name)) {
        qWarning() << "Duplicate Fact" << name;
        return;
    }

    fact->setSendValueChangedSignals(_updateRateMSecs == 0);
    if (_nameToFactMetaDataMap.contains(name)) {
        fact->setMetaData(_nameToFactMetaDataMap[name]);
    }
    _nameToFactMap[name] = fact;
}

void FactGroup::_addFactGroup(FactGroup* factGroup, const QString& name)
{
    if (_nameToFactGroupMap.contains(name)) {
        qWarning() << "Duplicate FactGroup" << name;
        return;
    }

    _nameToFactGroupMap[name] = factGroup;
}

void FactGroup::_updateAllValues(void)
{
    foreach(Fact* fact, _nameToFactMap) {
        fact->sendDeferredValueChangedSignal();
    }
}

void FactGroup::_loadMetaData(const QString& jsonFilename)
{
    if (jsonFilename.isEmpty()) {
        return;
    }

    qCDebug(FactGroupLog) << "Loading" << jsonFilename;

    QFile jsonFile(jsonFilename);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file" << jsonFilename << jsonFile.errorString();
        return;
    }

    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() <<  "Unable to open json document" << jsonFilename << jsonParseError.errorString();
        return;
    }

    QJsonObject json = doc.object();

    int version = json.value(_versionJsonKey).toInt();
    if (version != 1) {
        qWarning() << "Invalid version" << version;
        return;
    }

    QJsonValue jsonValue = json.value(_propertiesJsonKey);
    if (!jsonValue.isArray()) {
        qWarning() << "properties object not array";
        return;
    }

    QJsonArray jsonArray = jsonValue.toArray();
    foreach(QJsonValue property, jsonArray) {
        if (!property.isObject()) {
            qWarning() << "properties object should contain only objects";
            return;
        }
        QJsonObject jsonObject = property.toObject();

        // Make sure we have the required keys
        QString errorString;
        QStringList requiredKeys;
        requiredKeys << _nameJsonKey << _typeJsonKey << _shortDescriptionJsonKey;
        if (!JsonHelper::validateRequiredKeys(jsonObject, requiredKeys, errorString)) {
            qWarning() << errorString;
            return;
        }

        // Validate key types

        QStringList             keys;
        QList<QJsonValue::Type> types;
        keys << _nameJsonKey << _decimalPlacesJsonKey << _typeJsonKey << _shortDescriptionJsonKey << _unitsJsonKey << _defaultValueJsonKey << _minJsonKey << _maxJsonKey;
        types << QJsonValue::String << QJsonValue::Double << QJsonValue::String << QJsonValue::String << QJsonValue::String << QJsonValue::Double << QJsonValue::Double << QJsonValue::Double;
        if (!JsonHelper::validateKeyTypes(jsonObject, keys, types, errorString)) {
            qWarning() << errorString;
            return;
        }

        QString name = jsonObject.value(_nameJsonKey).toString();
        if (_nameToFactMetaDataMap.contains(name)) {
            qWarning() << "Duplicate property name" << name;
            continue;
        }

        bool unknownType;
        FactMetaData::ValueType_t type = FactMetaData::stringToType(jsonObject.value(_typeJsonKey).toString(), unknownType);
        if (unknownType) {
            qWarning() << "Unknown type" << jsonObject.value(_typeJsonKey).toString();
            return;
        }

        QStringList enumValues, enumStrings;
        if (!JsonHelper::parseEnum(jsonObject, enumStrings, enumValues, errorString)) {
            qWarning() << errorString;
            return;
        }

        FactMetaData* metaData = new FactMetaData(type, this);

        metaData->setDecimalPlaces(jsonObject.value(_decimalPlacesJsonKey).toInt(0));
        metaData->setShortDescription(jsonObject.value(_shortDescriptionJsonKey).toString());
        metaData->setRawUnits(jsonObject.value(_unitsJsonKey).toString());

        if (jsonObject.contains(_defaultValueJsonKey)) {
            metaData->setRawDefaultValue(jsonObject.value(_defaultValueJsonKey).toDouble());
        }
        if (jsonObject.contains(_minJsonKey)) {
            metaData->setRawMin(jsonObject.value(_minJsonKey).toDouble());
        }
        if (jsonObject.contains(_maxJsonKey)) {
            metaData->setRawMax(jsonObject.value(_maxJsonKey).toDouble());
        }

        for (int i=0; i<enumValues.count(); i++) {
            QVariant    enumVariant;
            QString     errorString;

            if (metaData->convertAndValidateRaw(enumValues[i], false /* validate */, enumVariant, errorString)) {
                metaData->addEnumInfo(enumStrings[i], enumVariant);
            } else {
                qWarning() << "Invalid enum value, name:" << metaData->name()
                            << " type:" << metaData->type() << " value:" << enumValues[i]
                            << " error:" << errorString;
                delete metaData;
                return;
            }
        }

        // All FactGroup Facts use translator based on app settings
        metaData->setAppSettingsTranslators();

        _nameToFactMetaDataMap[name] = metaData;
    }
}
