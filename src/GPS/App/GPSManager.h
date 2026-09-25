#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class GPSCorrectionManager;
class GPSCorrectionStatus;
class GPSGgaSources;
class GPSRTK;
class GPSRTKFactGroup;
class NTRIPManager;
class NTRIPNetworkMonitor;
class PositionManager;
class QTimer;

class GPSManager : public QObject
{
    Q_OBJECT
    friend class RTKConnectionPolicyTest;
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("GPSCorrectionManager.h")
    Q_MOC_INCLUDE("GPSRTK.h")
    Q_MOC_INCLUDE("GPSRTKFactGroup.h")
    Q_MOC_INCLUDE("NTRIPManager.h")
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT FINAL)
    Q_PROPERTY(GPSRTK* gpsRtk READ gpsRtk CONSTANT FINAL)
    Q_PROPERTY(GPSRTKFactGroup* gpsRtkFacts READ gpsRtkFacts CONSTANT FINAL)
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

    GPSRTK* gpsRtk() const { return _gpsRtk; }

    /// The receiver's status as Facts for QML.
    GPSRTKFactGroup* gpsRtkFacts() const { return _gpsRtkFacts; }

    GPSCorrectionManager* corrections() const { return _corrections; }

    NTRIPManager* ntrip() const { return _ntripManager; }

    PositionManager* positionManager() const { return _positionManager; }

    CorrectionState correctionState() const;

    /// Stores the receiver's current position and accuracy as the fixed base position for a later connection.
    /// Returns false, leaving the settings unchanged, when gpsRtkFacts cannot save the current position.
    Q_INVOKABLE bool saveCurrentBasePosition();

signals:
    void correctionStateChanged();

private:
    void _updateConnections();
    QTimer* _connectionTimer = nullptr;
    GPSCorrectionManager* _corrections = nullptr;
    GPSRTK* _gpsRtk = nullptr;
    GPSRTKFactGroup* _gpsRtkFacts = nullptr;
    NTRIPManager* _ntripManager = nullptr;
    NTRIPNetworkMonitor* _ntripNetworkMonitor = nullptr;
    PositionManager* const _positionManager;
    GPSCorrectionStatus* const _correctionStatus;
    GPSGgaSources* const _ggaSources;
    bool _startupConnectPending = false;
    bool _shutdown = false;
};
