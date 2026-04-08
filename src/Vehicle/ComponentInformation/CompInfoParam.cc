#include "CompInfoParam.h"
#include "FirmwarePlugin.h"
#include "JsonHelper.h"
#include "JsonParsing.h"
#include "ParameterMetaData.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QRegularExpression>

QGC_LOGGING_CATEGORY(CompInfoParamLog, "ComponentInformation.CompInfoParam")

CompInfoParam::CompInfoParam(uint8_t compId_, Vehicle *vehicle_, QObject *parent)
    : CompInfo(COMP_METADATA_TYPE_PARAMETER, compId_, vehicle_, parent)
{
}

void CompInfoParam::setJson(const QString &metadataJsonFileName)
{
    if (metadataJsonFileName.isEmpty()) {
        return;
    }

    QString errorString;
    QJsonDocument jsonDoc;

    if (!JsonParsing::isJsonFile(metadataJsonFileName, jsonDoc, errorString)) {
        qCWarning(CompInfoParamLog) << "Metadata json file open failed: compid:" << compId << errorString;
        return;
    }

    const QJsonObject jsonObj = jsonDoc.object();
    const QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        {JsonHelper::jsonVersionKey, QJsonValue::Double, true},
        {kJsonParametersKey,         QJsonValue::Array,  true},
    };
    if (!JsonHelper::validateKeys(jsonObj, keyInfoList, errorString)) {
        qCWarning(CompInfoParamLog) << "Metadata json validation failed: compid:" << compId << errorString;
        return;
    }

    if (jsonObj[JsonHelper::jsonVersionKey].toInt() != 1) {
        qCWarning(CompInfoParamLog) << "Metadata json unsupported version" << jsonObj[JsonHelper::jsonVersionKey].toInt();
        return;
    }

    _noJsonMetadata = false;

    const QJsonArray parameters = jsonObj[kJsonParametersKey].toArray();
    const QString escapedTag = QRegularExpression::escape(kIndexedNameTag);

    for (const QJsonValue &parameterValue : parameters) {
        if (!parameterValue.isObject()) {
            qCWarning(CompInfoParamLog) << "Metadata json read failed: compid:" << compId << "parameters array contains non-object";
            return;
        }

        FactMetaData *newMetaData = FactMetaData::createFromJsonObject(parameterValue.toObject(), ParameterMetaData::kEmptyDefines, this);

        if (newMetaData->name().contains(kIndexedNameTag)) {
            QString regexPattern = QRegularExpression::escape(newMetaData->name());
            regexPattern.replace(escapedTag, QStringLiteral("(\\d+)"));
            _indexedNameMetaDataList.append({QRegularExpression(QStringLiteral("^%1$").arg(regexPattern)), newMetaData});
        } else {
            _nameToMetaDataMap[newMetaData->name()] = newMetaData;
        }
    }
}

FactMetaData *CompInfoParam::factMetaDataForName(const QString &name, FactMetaData::ValueType_t valueType)
{
    if (FactMetaData *cached = _nameToMetaDataMap.value(name)) {
        return cached;
    }

    FactMetaData *factMetaData = _resolveMetaData(name, valueType);
    _nameToMetaDataMap[name] = factMetaData;
    return factMetaData;
}

FactMetaData *CompInfoParam::_resolveMetaData(const QString &name, FactMetaData::ValueType_t valueType)
{
    if (_noJsonMetadata) {
        // No vehicle-provided metadata — use firmware-bundled metadata
        if (ParameterMetaData *fwMeta = _getParameterMetaData()) {
            return fwMeta->getMetaDataForFact(name, valueType);
        }
    } else {
        // Vehicle provided JSON metadata — check exact match then indexed patterns
        if (FactMetaData *found = _lookupJsonMetaData(name)) {
            return found;
        }
    }

    // Generic fallback
    auto *factMetaData = new FactMetaData(valueType, this);
    const int sep = name.indexOf('_');
    if (sep > 0) {
        factMetaData->setGroup(name.left(sep));
    }
    if (compId != MAV_COMP_ID_AUTOPILOT1) {
        factMetaData->setCategory(tr("Component %1").arg(compId));
    }
    return factMetaData;
}

FactMetaData *CompInfoParam::_lookupJsonMetaData(const QString &name)
{
    // Try indexed name patterns (e.g. "CAL_GYRO{n}_ID" matches "CAL_GYRO0_ID")
    for (const auto &[regex, templateMeta] : _indexedNameMetaDataList) {
        const QRegularExpressionMatch match = regex.match(name);
        if (match.hasMatch()) {
            auto *factMetaData = new FactMetaData(*templateMeta, this);
            factMetaData->setName(name);

            const QString index = match.captured(1);
            QString desc = factMetaData->shortDescription();
            desc.replace(kIndexedNameTag, index);
            factMetaData->setShortDescription(desc);

            desc = factMetaData->longDescription();
            desc.replace(kIndexedNameTag, index);
            factMetaData->setLongDescription(desc);
            return factMetaData;
        }
    }

    return nullptr;
}

ParameterMetaData *CompInfoParam::_getParameterMetaData()
{
    if (!_parameterMetaData && compId == MAV_COMP_ID_AUTOPILOT1) {
        _parameterMetaData = vehicle->firmwarePlugin()->loadParameterMetaData(vehicle);
        if (_parameterMetaData) {
            _parameterMetaData->setParent(this);
        }
    }

    return _parameterMetaData;
}
