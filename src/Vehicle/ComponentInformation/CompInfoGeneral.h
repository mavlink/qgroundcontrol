#pragma once

#include "CompInfo.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QMap>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;

Q_DECLARE_LOGGING_CATEGORY(CompInfoGeneralLog)

class CompInfoGeneral : public CompInfo
{
    Q_OBJECT

public:
    CompInfoGeneral(uint8_t compId_, Vehicle* vehicle_, QObject* parent = nullptr);

    bool isMetaDataTypeSupported(COMP_METADATA_TYPE metadataType) { return _supportedTypes.contains(metadataType); }

    void setUris(CompInfo& compInfo) const;

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName) override;

private:
    QMap<COMP_METADATA_TYPE, Uris>   _supportedTypes;

    static constexpr const char* _jsonMetadataTypesKey = "metadataTypes";
};
