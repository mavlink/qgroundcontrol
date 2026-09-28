#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class GPSCorrectionManager;
class GPSCorrectionStatus;
class GPSGgaSources;
class GPSReceiver;
class GPSReceiverFactGroup;
class MultiVehicleManager;
class NTRIPManager;
class NTRIPNetworkMonitor;
class PositionManager;
class QTimer;
class SimulatedPosition;

class GPSManager : public QObject
{
    Q_OBJECT
    friend class GPSManagerTest;
    friend class GPSReceiverConnectionPolicyTest;
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("GPSCorrectionManager.h")
    Q_MOC_INCLUDE("GPSReceiver.h")
    Q_MOC_INCLUDE("GPSReceiverFactGroup.h")
    Q_MOC_INCLUDE("NTRIPManager.h")
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT FINAL)
    Q_PROPERTY(GPSReceiver* receiver READ receiver CONSTANT FINAL)
    Q_PROPERTY(GPSReceiverFactGroup* receiverFacts READ receiverFacts CONSTANT FINAL)
    Q_PROPERTY(NTRIPManager* ntrip READ ntrip CONSTANT FINAL)
    Q_PROPERTY(CorrectionState correctionState READ correctionState NOTIFY correctionStateChanged FINAL)

public:
    /// Whether vehicles receive RTK corrections, across NTRIP, UDP input, and the local receiver.
    enum class CorrectionState
    {
        /// No correction source is enabled or connected.
        Inactive,
        /// A source is enabled or connected, but no fresh stream is selected for vehicles.
        Waiting,
        /// A fresh stream is selected for vehicles.
        Fresh,
    };
    Q_ENUM(CorrectionState)

    GPSManager(QObject* parent = nullptr);
    ~GPSManager();

    static GPSManager* instance();

    /// Initializes the ground-station position manager first, then the GPS services that consume it.
    void init();
    /// Stops the GPS services, then the position manager.
    void shutdown();

    GPSReceiver* receiver() const { return _receiver; }

    /// The receiver's status as Facts for QML.
    GPSReceiverFactGroup* receiverFacts() const { return _receiverFacts; }

    GPSCorrectionManager* corrections() const { return _corrections; }

    NTRIPManager* ntrip() const { return _ntripManager; }

    PositionManager* positionManager() const { return _positionManager; }

    CorrectionState correctionState() const;

    /// Stores the receiver's current position and accuracy as the fixed base position for a later connection.
    /// Returns false, leaving the settings unchanged, when receiverFacts cannot save the current position.
    Q_INVOKABLE bool saveCurrentBasePosition();

signals:
    void correctionStateChanged();

private:
    /// Moves @a simulated to the home position of the vehicle @a vehicles added last, once that home is known.
    static void _followVehicleHome(MultiVehicleManager* vehicles, SimulatedPosition* simulated);
    void _updateConnections();
    QTimer* _connectionTimer = nullptr;
    GPSCorrectionManager* _corrections = nullptr;
    GPSReceiver* _receiver = nullptr;
    GPSReceiverFactGroup* _receiverFacts = nullptr;
    NTRIPManager* _ntripManager = nullptr;
    NTRIPNetworkMonitor* _ntripNetworkMonitor = nullptr;
    PositionManager* const _positionManager;
    GPSCorrectionStatus* const _correctionStatus;
    GPSGgaSources* const _ggaSources;
    bool _startupConnectPending = false;
    bool _shutdown = false;
};
