#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class QSignalSpy;

/// Verifies Vehicle::rebootVehicleToRomBootloader: it sends REBOOT_SHUTDOWN_ACTION_REBOOT_TO_ROM_BOOTLOADER, falls back
/// to the ArduPilot DFU sequence only for ArduPilot vehicles which do not support it, and reports refusals.
class RebootToRomBootloaderTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _apmUnsupportedFallsBackToDfuSequence();
    void _apmAcceptedDoesNotFallBack();
    void _apmFailedIsReported();
    void _px4UnsupportedIsReported();
    void _armedSendsNothing();

private:
    void _connect(MAV_AUTOPILOT autopilot, MAV_RESULT romBootloaderResult);
    void _waitForRebootAcked();
    void _flushCommandQueue();
    static void _verifyRebootParams(const QSignalSpy& spy, int index, float param1, float param2 = 0.0f,
                                    float param3 = 0.0f, float param4 = 0.0f);
};
