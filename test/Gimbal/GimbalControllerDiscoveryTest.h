#pragma once

#include <functional>

#include "BaseClasses/VehicleTestManualConnect.h"
#include "MockLinkGimbal.h"

class Gimbal;
class GimbalController;
class MockConfiguration;

/// Gimbal discovery: addressing variants, rejection of invalid messages, and request retry/fallback behaviour.
class GimbalControllerDiscoveryTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testAutopilotAttachedGimbal();
    void _testInvalidMessagesIgnored();
    void _testManagerInformationUnavailable_data();
    void _testManagerInformationUnavailable();
    void _testStatusBeforeInformation();
    void _testStatusIntervalRetryFallback();

private:
    /// Starts a PX4 MockLink with gimbal, letting the caller tweak the config and the mock gimbal before
    /// the initial connect sequence completes.
    void _startGimbalMockLink(const std::function<void(MockConfiguration*)>& configure,
                              const std::function<void(MockLinkGimbal*)>& prepareGimbal);

    GimbalController* gimbalController() const;
    Gimbal* activeGimbal() const;
    MockLinkGimbal* mockGimbal() const;
};

Q_DECLARE_METATYPE(MockLinkGimbal::InformationResponse)
