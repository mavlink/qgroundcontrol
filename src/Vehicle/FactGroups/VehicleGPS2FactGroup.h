#pragma once

#include "VehicleGPSFactGroup.h"

class VehicleGPS2FactGroup : public VehicleGPSFactGroup
{
    Q_OBJECT

public:
    explicit VehicleGPS2FactGroup(QObject* parent = nullptr, VehicleGPSObservationStream* stream = nullptr)
        : VehicleGPSFactGroup(parent, stream, 1)
    {}
};
