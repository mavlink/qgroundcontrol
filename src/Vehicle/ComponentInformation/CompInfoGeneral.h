#pragma once

#include "CompInfo.h"

#include <QtCore/QObject>
#include <QtCore/QMap>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;


class CompInfoGeneral : public CompInfo
{
    Q_OBJECT

public:
    CompInfoGeneral(uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);

    bool isMetaDataTypeSupported(COMP_METADATA_TYPE type) { return _supportedTypes.contains(type); }

    void setUris(CompInfo& compInfo) const;

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName) override;

private:
    QMap<COMP_METADATA_TYPE, Uris>   _supportedTypes;

    static constexpr const char* _jsonMetadataTypesKey = "metadataTypes";
};
