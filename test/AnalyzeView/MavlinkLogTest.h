#pragma once

#include "BaseClasses/VehicleTest.h"

class MavlinkLogTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void _disconnectNotArmed_test_data();
    void _disconnectNotArmed_test();
    void _armedLogSavedWhileConnected_test();
    void _saveTurnedOnWhileConnected_test();
    void _saveTurnedOffWhileConnected_test();
    void _replayArmedStateNotInherited_test();
    void _foreignArmedHeartbeatIgnored_test();
    void _saveDirectoryMissing_test();
    void _savedNameCollision_test();
    void _removeOrphanedLogFiles_test();

private:
    void _connectAndArm();
};
