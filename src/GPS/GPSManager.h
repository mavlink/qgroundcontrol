#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

struct GPSBluetoothPlatform;
class GPSCorrectionManager;
class GPSReceiver;
class NTRIPConnectedVehicles;
class NTRIPManager;
class NTRIPVehicleGgaSource;
class PositionManager;
class RuntimeScheduler;
class SerialPortManager;

/// Owns the GPS services and wires them to settings and vehicles. Terms used across src/GPS:
/// - service (manager): an application-side owner of one concern: GPSReceiver, NTRIPManager,
///   GPSCorrectionManager and PositionManager, all on one RuntimeScheduler.
/// - source: an input of corrections (NTRIP, UDP, the local receiver) or of positions (receiver, device, vehicle).
/// - receiver / position receiver: the GPSReceiver of the GNSS Receiver settings, which may be an RTK base station
///   and is the local receiver of the corrections and of NTRIP's GGA, and a second GPSReceiver that is never
///   configured, forwards no corrections and only provides the ground station position. While the position receiver
///   is in use, it alone is PositionManager's receiver source.
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
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT FINAL)
    Q_PROPERTY(GPSReceiver* receiver READ receiver CONSTANT FINAL)
    Q_PROPERTY(GPSReceiver* positionReceiver READ positionReceiver CONSTANT FINAL)
    Q_PROPERTY(NTRIPManager* ntrip READ ntrip CONSTANT FINAL)

    friend class GPSManagerTest;

public:
    /// @a scheduler runs the timers of every GPS service; the manager creates one when none is given. @a serialPorts
    /// lists and claims the serial devices of both receivers; they are the application's when none is given.
    /// @a bluetooth is what the Bluetooth connections of both receivers are built on: the build's when none is given.
    GPSManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr,
               SerialPortManager* serialPorts = nullptr, const GPSBluetoothPlatform* bluetooth = nullptr);
    ~GPSManager();

    static GPSManager* instance();

    /// Initializes the ground-station position manager first, then the GPS services that consume it, and starts
    /// polling the connections of both receivers.
    void init();
    /// Stops the GPS services, then the position manager.
    void shutdown();

    GPSReceiver* receiver() const { return _receiver; }

    GPSReceiver* positionReceiver() const { return _positionReceiver; }

    GPSCorrectionManager* corrections() const { return _corrections; }

    NTRIPManager* ntrip() const { return _ntripManager; }

    PositionManager* positionManager() const { return _positionManager; }

    /// Stores the receiver's current position and accuracy as the fixed base position for a later connection, and as
    /// the saved base site @a siteName when a name is given. Returns false, leaving the settings unchanged, when the
    /// receiver's facts cannot save the current position or the settings cannot save the site.
    Q_INVOKABLE bool saveCurrentBasePosition(const QString& siteName = {});

private:
    /// Supplies NTRIP's GGA positions from the active vehicle, the local receiver and the ground station. The local
    /// receiver is receiver(), whichever receiver provides the ground station position: the position receiver's
    /// position reaches a caster as the ground station's. Must precede NTRIPManager::init().
    void _initGgaSources();
    /// The position receiver is in use on the serial @a device, which receiver()'s USB discovery then leaves alone:
    /// discovery configures the receiver it finds as a base station.
    bool _positionReceiverUses(const QString& device) const;
    /// Why receiver() does not connect the Bluetooth device at @a address, as shown to the user: the position
    /// receiver is in use on it, and a device takes one connection. Empty for any other device.
    QString _positionReceiverBluetoothRefusal(const QString& address) const;

    RuntimeScheduler* const _scheduler;
    GPSCorrectionManager* const _corrections;
    PositionManager* const _positionManager;
    GPSReceiver* const _receiver;
    GPSReceiver* const _positionReceiver;
    NTRIPConnectedVehicles* const _connectedVehicles;
    NTRIPManager* const _ntripManager;
    NTRIPVehicleGgaSource* const _vehicleGga;
    bool _initialized = false;
    bool _shutdown = false;
};
