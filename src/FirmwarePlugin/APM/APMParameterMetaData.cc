#include "APMParameterMetaData.h"
#include "QGCLoggingCategory.h"

#include <algorithm>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>

using namespace Qt::StringLiterals;

QGC_LOGGING_CATEGORY(APMParameterMetaDataLog, "FirmwarePlugin.APMParameterMetaData")
QGC_LOGGING_CATEGORY(APMParameterMetaDataVerboseLog, "FirmwarePlugin.APMParameterMetaData:verbose")

APMParameterMetaData::APMParameterMetaData(QObject *parent)
    : ParameterMetaData(parent)
{
    qCDebug(APMParameterMetaDataLog) << this;
}

APMParameterMetaData::~APMParameterMetaData()
{
    qCDebug(APMParameterMetaDataLog) << this;
}

QString APMParameterMetaData::_groupFromParameterName(const QString &name)
{
    static const QRegularExpression regex(QStringLiteral("[0-9]*$"));
    QString group = name.split('_').first();
    return group.remove(regex);
}

void APMParameterMetaData::parseParameterJson(const QJsonObject &json)
{
    for (auto groupIt = json.constBegin(); groupIt != json.constEnd(); ++groupIt) {
        if (!groupIt->isObject()) {
            continue;
        }

        const QJsonObject params = groupIt->toObject();

        for (auto paramIt = params.constBegin(); paramIt != params.constEnd(); ++paramIt) {
            if (!paramIt->isObject()) {
                continue;
            }

            const QString name = paramIt.key();
            const QString group = _groupFromParameterName(name);

            if (_rawParams.contains(name)) {
                qCWarning(APMParameterMetaDataLog) << "Duplicate parameter found:" << name;
            }

            _rawParams[name] = RawParamData{group, paramIt->toObject()};
        }
    }

    _correctGroupMemberships();
}

void APMParameterMetaData::_correctGroupMemberships()
{
    // Demote groups with only one member to the default group.
    QHash<QString, int> groupCount;
    for (const auto &raw : std::as_const(_rawParams)) {
        groupCount[raw.group]++;
    }
    for (auto &raw : _rawParams) {
        if (groupCount.value(raw.group) == 1) {
            raw.group = FactMetaData::defaultGroup();
        }
    }
}

FactMetaData *APMParameterMetaData::_lookupMetaData(const QString &name, FactMetaData::ValueType_t type)
{
    auto it = _rawParams.constFind(name);
    if (it == _rawParams.constEnd()) {
        return nullptr;
    }

    const RawParamData &raw = *it;
    const QJsonObject &f = raw.fields;

    auto *metaData = new FactMetaData(type, this);
    metaData->setName(name);
    metaData->setGroup(raw.group);

    const QString displayName = f.value(u"DisplayName").toString();
    if (!displayName.isEmpty()) {
        metaData->setShortDescription(displayName);
    }

    const QString description = f.value(u"Description").toString();
    if (!description.isEmpty()) {
        metaData->setLongDescription(description);
    }

    const QString units = f.value(u"Units").toString();
    if (!units.isEmpty()) {
        metaData->setRawUnits(units);
    }

    const QString category = f.value(u"User").toString();
    if (!category.isEmpty()) {
        metaData->setCategory(category);
    }

    if (f.contains(u"ReadOnly")) {
        metaData->setReadOnly(jsonToBool(f.value(u"ReadOnly")));
    }
    if (f.contains(u"RebootRequired")) {
        metaData->setVehicleRebootRequired(jsonToBool(f.value(u"RebootRequired")));
    }

    const QString increment = f.value(u"Increment").toString();
    if (!increment.isEmpty()) {
        bool ok = false;
        const double val = increment.toDouble(&ok);
        if (ok) {
            metaData->setRawIncrement(val);
        }
    }

    const QJsonObject range = f.value(u"Range").toObject();
    if (!range.isEmpty()) {
        const QString lowStr = range.value(u"low").toString();
        const QString highStr = range.value(u"high").toString();
        if (!lowStr.isEmpty()) {
            setRawConvertedValue(metaData, lowStr, &FactMetaData::setRawMin);
            setRawConvertedValue(metaData, lowStr, &FactMetaData::setRawUserMin);
        }
        if (!highStr.isEmpty()) {
            setRawConvertedValue(metaData, highStr, &FactMetaData::setRawMax);
            setRawConvertedValue(metaData, highStr, &FactMetaData::setRawUserMax);
        }
    }

    const QJsonObject valuesObj = f.value(u"Values").toObject();
    if (!valuesObj.isEmpty()) {
        _applyEnumValues(metaData, valuesObj);
    }

    const QJsonObject bitmaskObj = f.value(u"Bitmask").toObject();
    if (!bitmaskObj.isEmpty()) {
        _applyBitmask(metaData, bitmaskObj);
    }

    return metaData;
}

QList<ParameterMetaData::ValueDescPair> APMParameterMetaData::_sortedNumericPairs(const QJsonObject &obj, const QString &paramName)
{
    // APM format: {"0":"Disabled","1":"Enabled"} — sort by numeric value
    // but preserve original string keys to avoid float round-trip issues.
    struct Entry { double sortKey; QString key; QString desc; };
    QList<Entry> entries;
    entries.reserve(obj.size());
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        bool ok = false;
        const double sortKey = it.key().toDouble(&ok);
        if (!ok) {
            qCWarning(APMParameterMetaDataLog) << "Non-numeric key:" << it.key() << "for" << paramName;
            continue;
        }
        entries.append({sortKey, it.key(), it->toString()});
    }
    std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
        return a.sortKey < b.sortKey;
    });

    QList<ValueDescPair> pairs;
    pairs.reserve(entries.size());
    for (const auto &e : std::as_const(entries)) {
        pairs.append({e.key, e.desc});
    }
    return pairs;
}

void APMParameterMetaData::_applyEnumValues(FactMetaData *metaData, const QJsonObject &valuesObj)
{
    setEnumFromPairs(metaData, _sortedNumericPairs(valuesObj, metaData->name()));
}

void APMParameterMetaData::_applyBitmask(FactMetaData *metaData, const QJsonObject &bitmaskObj)
{
    setBitmaskFromPairs(metaData, _sortedNumericPairs(bitmaskObj, metaData->name()));
}

FactMetaData *APMParameterMetaData::_createDefaultMetaData(const QString &name, FactMetaData::ValueType_t type)
{
    auto *metaData = new FactMetaData(type, this);
    metaData->setCategory(QStringLiteral("Advanced"));
    metaData->setGroup(_groupFromParameterName(name));
    return metaData;
}

void APMParameterMetaData::_postProcessMetaData(const QString &name, FactMetaData *metaData)
{
    if ((name.endsWith(u"_P") || name.endsWith(u"_I") || name.endsWith(u"_D")) &&
        (metaData->type() == FactMetaData::valueTypeFloat || metaData->type() == FactMetaData::valueTypeDouble)) {
        metaData->setDecimalPlaces(6);
    }
}
