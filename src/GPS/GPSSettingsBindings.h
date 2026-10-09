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

/// Translates the persisted GPS settings into the plain configurations the GPS services consume. Each is a value the
/// service compares with the one it runs and applies whole, and tests build without settings.
class GPSSettingsBindings
{
    friend class GPSManagerTest;
    friend class GPSReceiverSettingsBindingTest;

public:
    GPSSettingsBindings() = delete;

    /// Applies the correction settings to @a corrections now and whenever they change.
    static void bindCorrections(GPSCorrectionSettings* settings, GPSCorrectionManager* corrections);

    /// Supplies the NTRIP settings to @a ntrip now and whenever they change, and stores the connection a retry
    /// enables and the certificate pin trusted on first use. Call before NTRIPManager::init().
    static void bindNtrip(NTRIPSettings* settings, NTRIPManager* ntrip);

    /// Supplies the receiver settings to @a receiver now and whenever they change. Call before the receiver is
    /// allowed to connect.
    static void bindReceiver(RTKSettings* settings, GPSReceiver* receiver);

    /// Applies the GCS position settings to @a positions now and whenever they change. Call before
    /// PositionManager::init().
    static void bindPosition(RTKSettings* settings, PositionManager* positions);

private:
    /// The Facts the group's configurations read, which their bindings watch.
    [[nodiscard]] static QList<Fact*> _boundFacts(GPSCorrectionSettings* settings);
    [[nodiscard]] static QList<Fact*> _boundFacts(NTRIPSettings* settings);
    [[nodiscard]] static QList<Fact*> _boundFacts(RTKSettings* settings);
};
