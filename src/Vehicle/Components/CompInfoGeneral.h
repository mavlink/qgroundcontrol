/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
    CompInfoGeneral(uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);

    bool isMetaDataTypeSupported(COMP_METADATA_TYPE type) { return _supportedTypes.contains(type); }

    void setUris(CompInfo& compInfo) const;

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName) override;

private:
    QMap<COMP_METADATA_TYPE, Uris>   _supportedTypes;

    static constexpr const char* _jsonMetadataTypesKey = "metadataTypes";
};
