#pragma once

#include "CompInfo.h"

#include <QtCore/QObject>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;

class CompInfoActuators : public CompInfo
{
    Q_OBJECT

public:
    CompInfoActuators(uint8_t compId_, Vehicle* vehicle_, QObject* parent = nullptr);

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName) override;

private:
};
