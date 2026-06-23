#pragma once

#include "CompInfo.h"
#include "FactMetaData.h"

#include <QtCore/QRegularExpression>

class ParameterMetaData;

class CompInfoParam : public CompInfo
{
    Q_OBJECT

public:
    CompInfoParam(uint8_t compId_, Vehicle *vehicle_, QObject *parent = nullptr);

    FactMetaData *factMetaDataForName(const QString &name, FactMetaData::ValueType_t valueType);

    void setJson(const QString &metadataJsonFileName) override;

private:
    ParameterMetaData *_getParameterMetaData();
    FactMetaData *_resolveMetaData(const QString &name, FactMetaData::ValueType_t valueType);
    FactMetaData *_lookupJsonMetaData(const QString &name);

    struct IndexedParamEntry {
        QRegularExpression regex;
        FactMetaData *templateMeta;
    };

    bool _noJsonMetadata = true;
    FactMetaData::NameToMetaDataMap_t _nameToMetaDataMap;
    QList<IndexedParamEntry> _indexedNameMetaDataList;
    ParameterMetaData *_parameterMetaData = nullptr;

    static constexpr const char *kJsonParametersKey = "parameters";
    static constexpr const char *kIndexedNameTag = "{n}";
};
