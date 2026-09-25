#pragma once

#include "GPSCorrectionManager.h"
#include "NTRIPManager.h"

class GPSCorrectionSettings;
class NTRIPSettings;

/// Translates the persisted GPS settings into the plain configuration the GPS services consume, so the services
/// stay independent of the Fact system.
namespace GPSSettingsBindings {

GPSCorrectionManager::RoutingConfiguration routingConfiguration(GPSCorrectionSettings* settings);
GPSCorrectionManager::UdpInputConfiguration udpInputConfiguration(GPSCorrectionSettings* settings);
GPSCorrectionManager::UdpOutputConfiguration udpOutputConfiguration(GPSCorrectionSettings* settings);
NTRIPManager::Configuration ntripConfiguration(NTRIPSettings* settings);

/// Applies the correction settings to @a corrections now and whenever they change.
void bindCorrections(GPSCorrectionSettings* settings, GPSCorrectionManager* corrections);

/// Supplies the NTRIP settings to @a ntrip now and whenever they change, and stores the mountpoint the user chooses
/// and the connection a retry enables. Call before NTRIPManager::init().
void bindNtrip(NTRIPSettings* settings, NTRIPManager* ntrip);

}  // namespace GPSSettingsBindings
