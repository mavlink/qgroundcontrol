#pragma once

#include "VehicleGPSFactGroup.h"

class VehicleGPS2FactGroup : public VehicleGPSFactGroup
{
    Q_OBJECT

public:
    explicit VehicleGPS2FactGroup(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr)
        : VehicleGPSFactGroup(parent, scheduler)
    {
        _gnssIntegrityId = 1;
    }

    // Overrides from VehicleGPSFactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;
};
