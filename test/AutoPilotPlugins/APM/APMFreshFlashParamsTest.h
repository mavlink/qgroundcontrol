#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Non-UI coverage for MockConfiguration::OptionAPMStartFreshParams
/// (MockLink::_applyAPMFreshFlashState). Verifies for every ArduPilot vehicle
/// type that a fresh-flash MockLink connects with uncalibrated
/// sensors/radio, no airframe selected (where FRAME_CLASS exists), correct
/// parameter storage types, setup-incomplete vehicle components and the
/// setup-required app message. Also verifies a normal (non-fresh) connect
/// does NOT show the setup-required message.
class APMFreshFlashParamsTest : public VehicleTestManualConnect
{
    Q_OBJECT

public:
    APMFreshFlashParamsTest() = default;

private slots:
    void _testFreshFlashParams_data();
    void _testFreshFlashParams();
    void _testNormalConnectNoSetupRequiredMessage();

private:
    /// Start the APM MockLink for \a vehicleKey (Copter/Plane/Rover/Sub) with
    /// \a options and wait for connect + parameters ready.
    void _connectAPMMockLink(const QString &vehicleKey, MockConfiguration::Options options);
};
