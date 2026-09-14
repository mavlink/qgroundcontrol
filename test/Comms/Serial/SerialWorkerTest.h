#pragma once

#include "UnitTest.h"

class SerialWorkerTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _settingsSnapshotAndAvailability();
    void _configurationFailure_data();
    void _configurationFailure();
    void _reservationLifetime();
    void _missingPort_data();
    void _missingPort();
    void _occupiedPort_data();
    void _occupiedPort();
};
