#pragma once

#include "GPSCorrectionManager.h"
#include "GPSRtk.h"
#include "NTRIPManager.h"

class GPSCorrectionSettings;
class NTRIPSettings;
class RTKSettings;

/// Translates the persisted GPS settings into the plain configuration the GPS services consume, so the services
/// stay independent of the Fact system.
namespace GPSSettingsBindings {

GPSCorrectionManager::RoutingConfiguration routingConfiguration(GPSCorrectionSettings* settings);
GPSCorrectionManager::UdpInputConfiguration udpInputConfiguration(GPSCorrectionSettings* settings);
GPSCorrectionManager::UdpOutputConfiguration udpOutputConfiguration(GPSCorrectionSettings* settings);
NTRIPManager::Configuration ntripConfiguration(NTRIPSettings* settings);
GPSRtk::Configuration rtkConfiguration(RTKSettings* settings);

/// Applies the correction settings to @a corrections now and whenever they change.
void bindCorrections(GPSCorrectionSettings* settings, GPSCorrectionManager* corrections);

/// Supplies the NTRIP settings to @a ntrip now and whenever they change, and stores the mountpoint the user chooses
/// and the connection a retry enables. Call before NTRIPManager::init().
void bindNtrip(NTRIPSettings* settings, NTRIPManager* ntrip);

/// Supplies the RTK receiver settings to @a rtk now and whenever they change, and stores receiver-driven
/// write-backs. Call before GPSRtk is allowed to connect.
void bindRtk(RTKSettings* settings, GPSRtk* rtk);

}  // namespace GPSSettingsBindings
