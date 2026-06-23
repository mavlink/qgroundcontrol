#pragma once

#include "ParameterMetaData.h"

class PX4ParameterMetaData : public ParameterMetaData
{
    Q_DECLARE_TR_FUNCTIONS(PX4ParameterMetaData)

public:
    explicit PX4ParameterMetaData(QObject *parent = nullptr);
    ~PX4ParameterMetaData() override;

protected:
    void parseParameterJson(const QJsonObject &json) override;
    void _postProcessMetaData(const QString &name, FactMetaData *metaData) override;
};
