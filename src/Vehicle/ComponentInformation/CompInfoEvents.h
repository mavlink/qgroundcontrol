#pragma once

#include "CompInfo.h"

#include <QtCore/QObject>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;

class CompInfoEvents : public CompInfo
{
    Q_OBJECT

public:
    CompInfoEvents(uint8_t compId_, Vehicle* vehicle_, QObject* parent = nullptr);

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName) override;

private:
};
