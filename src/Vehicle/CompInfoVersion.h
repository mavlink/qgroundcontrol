/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "CompInfo.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "FactMetaData.h"

#include <QObject>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;

Q_DECLARE_LOGGING_CATEGORY(CompInfoVersionLog)

class CompInfoVersion : public CompInfo
{
    Q_OBJECT

public:
    CompInfoVersion(uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);

    bool isMetaDataTypeSupported(COMP_METADATA_TYPE type) { return _supportedTypes.contains(type); }

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName, const QString& translationJsonFileName) override;

private:
    QList<COMP_METADATA_TYPE>   _supportedTypes;

    static const char*          _jsonSupportedCompMetadataTypesKey;
};
