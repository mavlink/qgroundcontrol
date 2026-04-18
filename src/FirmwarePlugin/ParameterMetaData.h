#pragma once

#include <QtCore/QJsonValue>
#include <QtCore/QObject>
#include <QtCore/QStringView>
#include <QtCore/QVersionNumber>

#include "FactMetaData.h"

class QJsonObject;

class ParameterMetaData : public QObject
{
public:
    explicit ParameterMetaData(QObject *parent = nullptr);
    ~ParameterMetaData() override;

    void loadParameterFactMetaDataFile(const QString &metaDataFile);
    FactMetaData *getMetaDataForFact(const QString &name, FactMetaData::ValueType_t type);

    static QVersionNumber versionFromMetaDataFile(const QString &metaDataFile);
    static QVersionNumber versionFromJsonData(const QByteArray &jsonData);
    static QVersionNumber versionFromJsonData(const QByteArray &jsonData, bool *validJson);
    static QVersionNumber versionFromFileName(const QString &fileName);

    static const FactMetaData::DefineMap_t kEmptyDefines;

protected:
    virtual void parseParameterJson(const QJsonObject &json) = 0;
    virtual FactMetaData *_lookupMetaData(const QString &name, FactMetaData::ValueType_t type);
    virtual FactMetaData *_createDefaultMetaData(const QString &name, FactMetaData::ValueType_t type);
    virtual void _postProcessMetaData(const QString &name, FactMetaData *metaData);

    /// A value/description pair used for enum and bitmask entries.
    struct ValueDescPair {
        QString value;       ///< Numeric value as string (enum code or bitmask index)
        QString description; ///< Human-readable label
    };

    static bool textToBool(QStringView text) { return text.compare(u"true", Qt::CaseInsensitive) == 0; }
    static bool jsonToBool(const QJsonValue &value) { return value.isBool() ? value.toBool() : textToBool(value.toString()); }
    static bool setRawConvertedValue(FactMetaData *metaData, const QString &rawText, void (FactMetaData::*setter)(const QVariant &));
    static void setEnumFromPairs(FactMetaData *metaData, const QList<ValueDescPair> &pairs);
    static void setBitmaskFromPairs(FactMetaData *metaData, const QList<ValueDescPair> &pairs);

    FactMetaData::NameToMetaDataMap_t _cachedMetaData;
    bool _parameterMetaDataLoaded = false;
};
