#pragma once

#include <QtCore/QList>

class Fact;
class GPSCorrectionManager;
class GPSCorrectionSettings;
class GPSReceiver;
class NTRIPManager;
class NTRIPSettings;
class PositionManager;
class PositionReceiverSettings;
class RTKSettings;

/// Translates the persisted GPS settings into the plain configurations the GPS services consume. Each is a value the
/// service compares with the one it runs and applies whole, and tests build without settings.
class GPSSettingsBindings
{
    friend class GPSManagerTest;
    friend class GPSSettingsBindingsTest;

public:
    GPSSettingsBindings() = delete;

    /// Applies the correction settings to @a corrections now and whenever they change.
    static void bindCorrections(GPSCorrectionSettings* settings, GPSCorrectionManager* corrections);

    /// Supplies the NTRIP settings to @a ntrip now and whenever they change, and stores the connection a retry
    /// enables, the certificate pin trusted on first use and the casters found to take NTRIP version 1 requests only.
    /// Call before NTRIPManager::init().
    static void bindNtrip(NTRIPSettings* settings, NTRIPManager* ntrip);

    /// Supplies the receiver settings to @a receiver now and whenever they change. Call before the receiver is
    /// allowed to connect.
    static void bindReceiver(RTKSettings* settings, GPSReceiver* receiver);

    /// Supplies the position receiver's settings to @a receiver now and whenever they change: a passive receiver
    /// that forwards no RTCM, kept connected while it is in use and disconnected when that is turned off. Call before
    /// the receiver is allowed to connect.
    static void bindPositionReceiver(PositionReceiverSettings* settings, GPSReceiver* receiver);

    /// Applies the GCS position settings to @a positions now and whenever they change: the source mode of
    /// @a settings, and the position receiver as the receiver source while @a positionReceiver has it in use. Call
    /// before PositionManager::init().
    static void bindPosition(RTKSettings* settings, PositionReceiverSettings* positionReceiver,
                             PositionManager* positions);

private:
    /// The Facts the group's configurations read, which their bindings watch. The position receiver's are those of
    /// its receiver configuration, which reads every one; the position configuration watches its enabled Fact too.
    [[nodiscard]] static QList<Fact*> _boundFacts(GPSCorrectionSettings* settings);
    [[nodiscard]] static QList<Fact*> _boundFacts(NTRIPSettings* settings);
    [[nodiscard]] static QList<Fact*> _boundFacts(PositionReceiverSettings* settings);
    [[nodiscard]] static QList<Fact*> _boundFacts(RTKSettings* settings);
};
