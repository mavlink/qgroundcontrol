#pragma once

#include "BaseClasses/CommsTest.h"

class RTCMMAVLinkVehicleTest : public CommsTest
{
    Q_OBJECT

private slots:
    void _admissionFollowsLinkLifetime();
    void _replayExcludedFromLiveAdmissions();
};
