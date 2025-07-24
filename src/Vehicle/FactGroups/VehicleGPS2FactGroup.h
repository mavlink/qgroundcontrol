/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "VehicleGPSFactGroup.h"

class VehicleGPS2FactGroup : public VehicleGPSFactGroup
{
    Q_OBJECT

public:
    explicit VehicleGPS2FactGroup(QObject *parent = nullptr)
        : VehicleGPSFactGroup(parent) {}

    // Overrides from VehicleGPSFactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleGps2Raw(const mavlink_message_t &message);
};
