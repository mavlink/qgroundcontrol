#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The receiver simulator on a TCP port: the receiver worker configures its u-blox model as a base over the worker's
/// own TCP transport and receives its navigation, base status and corrections, the model refuses a configuration
/// message it does not take as a receiver does, and a capture is played to every client that connects.
class GPSReceiverSimulatorTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _workerConfiguresBase_data();
    void _workerConfiguresBase();
    void _refusesUnknownConfiguration_data();
    void _refusesUnknownConfiguration();
    void _playsCaptureToEachClient_data();
    void _playsCaptureToEachClient();
};
