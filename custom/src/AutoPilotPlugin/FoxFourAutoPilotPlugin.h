#pragma once

#include "APM/APMAutoPilotPlugin.h"
#include "OnboardComputersManager.h"
class Vehicle;

class FoxFourAutoPilotPlugin : public APMAutoPilotPlugin
{
    Q_OBJECT
    Q_PROPERTY(OnboardComputersManager* onboardComputersManager MEMBER _onboardComputersMngr)
public:
    explicit FoxFourAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);
    ~FoxFourAutoPilotPlugin();
    /// This allows us to hide Vehicle Setup pages if needed
    const QVariantList &vehicleComponents() final;

    /// Reboot all onboard computers
    Q_INVOKABLE void rebootOnboardComputers();

private:
    QVariantList _components;
    OnboardComputersManager *_onboardComputersMngr;

};
