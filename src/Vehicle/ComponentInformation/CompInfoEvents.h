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
    /// Empty: fileParser() gives the file to the events code, which has no schema check
    QString jsonSchema() const override { return QString(); }

    /// Gives the events metadata, from EventHandler::loadMetadata(), in ParsedJson::data
    FileParser fileParser() const override;

    void setJson(const QString& fileName, const ParsedJson& json) override;

private:
};
