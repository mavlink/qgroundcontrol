#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

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

public:
    GPSManager(QObject* parent = nullptr);
    ~GPSManager();

    static GPSManager* instance();

    void init();
    void shutdown();

    GPSRtk* gpsRtk() const { return _gpsRtk; }

    GPSCorrectionManager* corrections() const { return _corrections; }

    NTRIPManager* ntrip() const { return _ntripManager; }

private:
    void _configureNtripProviders();

    void _updateConnections();
    QTimer* _connectionTimer = nullptr;
    GPSCorrectionManager* _corrections = nullptr;
    GPSRtk* _gpsRtk = nullptr;
    NTRIPManager* _ntripManager = nullptr;
    bool _startupConnectPending = false;
    bool _shutdown = false;
};
