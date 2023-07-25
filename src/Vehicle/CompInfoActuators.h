/****************************************************************************
 *
 * (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "CompInfo.h"

#include <QObject>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;

class CompInfoActuators : public CompInfo
{
    Q_OBJECT

public:
    CompInfoActuators(uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName, const QString& translationJsonFileName) override;

private:
};
