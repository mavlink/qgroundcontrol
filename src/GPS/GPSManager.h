#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class Fact;
class GPSCorrectionManager;
class GPSRtk;
class NTRIPManager;
class QTimer;

class GPSManager : public QObject
{
    Q_OBJECT
    friend class RTKConnectionPolicyTest;
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("GPSCorrectionManager.h")
    Q_MOC_INCLUDE("GPSRtk.h")
    Q_MOC_INCLUDE("NTRIPManager.h")
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT)
    Q_PROPERTY(GPSRtk* gpsRtk READ gpsRtk CONSTANT)
    Q_PROPERTY(NTRIPManager* ntrip READ ntrip CONSTANT)
    Q_PROPERTY(CorrectionState correctionState READ correctionState NOTIFY correctionStateChanged)

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

    void init();
    void shutdown();

    GPSRtk* gpsRtk() const { return _gpsRtk; }

    GPSCorrectionManager* corrections() const { return _corrections; }

    NTRIPManager* ntrip() const { return _ntripManager; }

    CorrectionState correctionState() const { return _correctionState; }

signals:
    void correctionStateChanged();

private:
    void _updateCorrectionState();

    void _configureNtripProviders();

    void _updateConnections();
    QTimer* _connectionTimer = nullptr;
    GPSCorrectionManager* _corrections = nullptr;
    GPSRtk* _gpsRtk = nullptr;
    NTRIPManager* _ntripManager = nullptr;
    Fact* const _udpInputEnabled;
    CorrectionState _correctionState = CorrectionState::Inactive;
    bool _startupConnectPending = false;
    bool _shutdown = false;
};
