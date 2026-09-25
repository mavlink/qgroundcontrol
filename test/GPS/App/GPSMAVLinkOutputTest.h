#pragma once

#include "BaseClasses/CommsTest.h"

class GPSMAVLinkOutputTest : public CommsTest
{
    Q_OBJECT

private slots:
    void _admissionFollowsLinkLifetime();
    void _primarySwitchPreservesIdentity();
    void _replayExcludedFromLiveAdmissions();
};
