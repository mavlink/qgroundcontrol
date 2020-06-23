/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"

#include <QObject>

Q_DECLARE_LOGGING_CATEGORY(ComponentInformationLog)

class Vehicle;

class ComponentInformation : public QObject
{
    Q_OBJECT

public:
    ComponentInformation(Vehicle* vehicle, QObject* parent = nullptr);

    void requestVersionMetaData         (Vehicle* vehicle);
    bool requestParameterMetaData       (Vehicle* vehicle);
    void componentInformationReceived   (const mavlink_message_t& message);
    bool metaDataTypeSupported          (COMP_METADATA_TYPE type);

private:
    Vehicle*                    _vehicle =                  nullptr;
    bool                        _versionMetaDataAvailable = false;
    bool                        _paramMetaDataAvailable =   false;
    QList<COMP_METADATA_TYPE>   _supportedMetaDataTypes;
};
