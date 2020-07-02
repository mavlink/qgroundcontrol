/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "FactMetaData.h"

#include <QObject>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;

/// Base class for all CompInfo types
class CompInfo : public QObject
{
    Q_OBJECT

public:
    CompInfo(COMP_METADATA_TYPE type, uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);

    /// Called to pass the COMPONENT_INFORMATION message in
    void setMessage(const mavlink_message_t& message);

    virtual void setJson(const QString& metaDataJsonFileName, const QString& translationJsonFileName) = 0;

    COMP_METADATA_TYPE  type;
    Vehicle*            vehicle            = nullptr;
    uint8_t             compId             = MAV_COMP_ID_ALL;
    bool                available          = false;
    uint32_t            uidMetaData        = 0;
    uint32_t            uidTranslation     = 0;
    QString             uriMetaData;
    QString             uriTranslation;
};
