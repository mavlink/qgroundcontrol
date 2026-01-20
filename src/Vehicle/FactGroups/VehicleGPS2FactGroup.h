#pragma once

#include "VehicleGPSFactGroup.h"

class VehicleGPS2FactGroup : public VehicleGPSFactGroup
{
    Q_OBJECT

public:
    explicit VehicleGPS2FactGroup(QObject *parent = nullptr)
        : VehicleGPSFactGroup(parent)
    {
        _gnssIntegrityId = 1;
    }

    // Overrides from VehicleGPSFactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleGps2Raw(const mavlink_message_t &message);
};
