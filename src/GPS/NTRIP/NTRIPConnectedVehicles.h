#pragma once

#include "NTRIPVehiclePresence.h"

/// Whether the application has a connected vehicle, as MultiVehicleManager lists them. The vehicle kept for planning
/// without a connection is not one.
class NTRIPConnectedVehicles : public NTRIPVehiclePresence
{
    Q_OBJECT

public:
    using NTRIPVehiclePresence::NTRIPVehiclePresence;

    /// Follows MultiVehicleManager's vehicles from now on; until then no vehicle is reported. Call before the NTRIP
    /// connection is started.
    void attach();

private slots:
    void _update();
};
