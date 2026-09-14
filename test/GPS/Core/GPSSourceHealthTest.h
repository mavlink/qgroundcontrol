#pragma once

#include "UnitTest.h"

class GPSSourceHealthTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _schedulerDestructionClearsAcceptedState();
    void _foreignSchedulerRejected();
    void _invalidatedPositionTimeout();
    void _retainedMeasurementExpires_data();
    void _retainedMeasurementExpires();
    void _normalizesObservation_data();
    void _normalizesObservation();
    void _ageAndRecovery();
    void _resetDuringPositionNotification();
};
