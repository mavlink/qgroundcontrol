#pragma once

#include <QtCore/QHash>
#include <QtCore/QJsonObject>

#include "ParameterMetaData.h"

class PX4ParameterMetaData : public ParameterMetaData
{
    Q_DECLARE_TR_FUNCTIONS(PX4ParameterMetaData)

public:
    explicit PX4ParameterMetaData(QObject *parent = nullptr);
    ~PX4ParameterMetaData() override;

protected:
    void parseParameterJson(const QJsonObject &json) override;
    FactMetaData* _lookupMetaData(const QString& name, FactMetaData::ValueType_t type) override;
    void _postProcessMetaData(const QString &name, FactMetaData *metaData) override;

private:
    /// Parameter json by name, converted to FactMetaData on first lookup: vehicles use a fraction of the catalog
    QHash<QString, QJsonObject> _paramJsonByName;
};
