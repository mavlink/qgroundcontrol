#pragma once

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include "ParameterMetaData.h"

class APMParameterMetaData : public ParameterMetaData
{
public:
    explicit APMParameterMetaData(QObject *parent = nullptr);
    ~APMParameterMetaData() override;

protected:
    void parseParameterJson(const QJsonObject &json) override;
    FactMetaData *_lookupMetaData(const QString &name, FactMetaData::ValueType_t type) override;
    FactMetaData *_createDefaultMetaData(const QString &name, FactMetaData::ValueType_t type) override;
    void _postProcessMetaData(const QString &name, FactMetaData *metaData) override;

private:
    struct RawParamData {
        QString group;
        QJsonObject fields;
    };

    void _correctGroupMemberships();
    static QString _groupFromParameterName(const QString &name);
    static QList<ValueDescPair> _sortedNumericPairs(const QJsonObject &obj, const QString &paramName);
    static void _applyEnumValues(FactMetaData *metaData, const QJsonObject &valuesObj);
    static void _applyBitmask(FactMetaData *metaData, const QJsonObject &bitmaskObj);

    QHash<QString, RawParamData> _rawParams;
};
