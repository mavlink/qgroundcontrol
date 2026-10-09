#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class GPSCorrectionManager;
class GPSReceiver;
class NTRIPManager;
class NTRIPVehicleGgaSource;
class PositionManager;
class RuntimeScheduler;

/// Owns the GPS services and wires them to settings and vehicles. Terms used across src/GPS:
/// - service (manager): an application-side owner of one concern: GPSReceiver, NTRIPManager,
///   GPSCorrectionManager and PositionManager, all on one RuntimeScheduler.
/// - source: an input of corrections (NTRIP, UDP, the local receiver) or of positions (receiver, device, vehicle).
/// - session: one connection's lifetime: GPSReceiverSession for a receiver, NTRIPHttpSession for a caster.
/// - worker: GPSReceiverWorker, the thread that runs one receiver session's I/O; everything else is on the main thread.
/// - driver / protocol / plan: the receiver wire layer: GPSDriver runs a family's protocol class, which sends the
///   commands of its plan.
class GPSManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by QGroundControl")
    Q_MOC_INCLUDE("GPSCorrectionManager.h")
    Q_MOC_INCLUDE("GPSReceiver.h")
    Q_MOC_INCLUDE("NTRIPManager.h")
    Q_MOC_INCLUDE("PositionManager.h")
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT FINAL)
    Q_PROPERTY(GPSReceiver* receiver READ receiver CONSTANT FINAL)
    Q_PROPERTY(NTRIPManager* ntrip READ ntrip CONSTANT FINAL)
    Q_PROPERTY(PositionManager* positionManager READ positionManager CONSTANT FINAL)

    friend class GPSManagerTest;

public:
    /// @a scheduler runs the timers of every GPS service; the manager creates one when none is given.
    GPSManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSManager();

    static GPSManager* instance();

    /// Initializes the ground-station position manager first, then the GPS services that consume it, and starts
    /// polling the receiver connection.
    void init();
    /// Stops the GPS services, then the position manager.
    void shutdown();

    GPSReceiver* receiver() const { return _receiver; }

    GPSCorrectionManager* corrections() const { return _corrections; }

    NTRIPManager* ntrip() const { return _ntripManager; }

    PositionManager* positionManager() const { return _positionManager; }

    /// Stores the receiver's current position and accuracy as the fixed base position for a later connection.
    /// Returns false, leaving the settings unchanged, when the receiver's facts cannot save the current position.
    Q_INVOKABLE bool saveCurrentBasePosition();

private:
    /// Supplies NTRIP's GGA positions from the active vehicle, the local receiver and the ground station. Must precede
    /// NTRIPManager::init().
    void _initGgaSources();

    RuntimeScheduler* const _scheduler;
    GPSCorrectionManager* const _corrections;
    PositionManager* const _positionManager;
    GPSReceiver* const _receiver;
    NTRIPManager* const _ntripManager;
    NTRIPVehicleGgaSource* const _vehicleGga;
    bool _initialized = false;
    bool _shutdown = false;
};
