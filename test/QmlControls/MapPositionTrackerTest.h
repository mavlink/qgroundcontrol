#pragma once

#include "UnitTest.h"

class MapPositionTrackerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _vehicleOneShot();
    void _gcsOneShotNoVehicle();
    void _gcsOneShotSuppressedAfterVehicle();
    void _gcsOneShotWithValidVehicle();
    void _externalFirstReceivedSuppressesOneShot();
    void _activeGatesAndReevaluates();
    void _activeReevaluatesGcsOneShot();
    void _allowEnabledAfterPositionReevaluates();
    void _hardFollow();
    void _interactionPausesAndResumes();
    void _insetFollow();
    void _insetFollowGates();
};
