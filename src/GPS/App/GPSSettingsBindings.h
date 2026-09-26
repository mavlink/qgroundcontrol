#pragma once

#include <QtCore/QList>

#include "GPSCorrectionManager.h"
#include "GPSReceiver.h"
#include "NTRIPManager.h"
#include "PositionManager.h"

class Fact;
class GPSCorrectionSettings;
class NTRIPSettings;
class RTKSettings;

/// Translates the persisted GPS settings into the plain configuration the GPS services consume, so the services
/// stay independent of the Fact system.
namespace GPSSettingsBindings {

[[nodiscard]] GPSCorrectionManager::RoutingConfiguration routingConfiguration(GPSCorrectionSettings* settings);
[[nodiscard]] GPSCorrectionManager::UdpInputConfiguration udpInputConfiguration(GPSCorrectionSettings* settings);
[[nodiscard]] GPSCorrectionManager::UdpOutputConfiguration udpOutputConfiguration(GPSCorrectionSettings* settings);
[[nodiscard]] NTRIPManager::Configuration ntripConfiguration(NTRIPSettings* settings);
[[nodiscard]] GPSReceiver::Configuration rtkConfiguration(RTKSettings* settings);
[[nodiscard]] PositionManager::Configuration positionConfiguration(RTKSettings* settings);

/// The Facts the group's configurations are built from and watched on, each listed once.
[[nodiscard]] QList<Fact*> boundFacts(GPSCorrectionSettings* settings);
[[nodiscard]] QList<Fact*> boundFacts(NTRIPSettings* settings);
[[nodiscard]] QList<Fact*> boundFacts(RTKSettings* settings);

/// Applies the correction settings to @a corrections now and whenever they change.
void bindCorrections(GPSCorrectionSettings* settings, GPSCorrectionManager* corrections);

/// Supplies the NTRIP settings to @a ntrip now and whenever they change, and stores the mountpoint the user chooses,
/// the connection a retry enables and the certificate pin trusted on first use. Call before NTRIPManager::init().
void bindNtrip(NTRIPSettings* settings, NTRIPManager* ntrip);

/// Supplies the receiver settings to @a rtk now and whenever they change, and turns auto-connect off when a manual
/// connection asks to. Call before GPSReceiver is allowed to connect.
void bindRtk(RTKSettings* settings, GPSReceiver* rtk);

/// Applies the GCS position settings to @a positions now and whenever they change. Call before PositionManager::init().
void bindPosition(RTKSettings* settings, PositionManager* positions);

}  // namespace GPSSettingsBindings
