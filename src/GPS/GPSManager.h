#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "NTRIPGgaProvider.h"

class GPSCorrectionManager;
class GPSRtk;
class NMEASourceManager;
class NTRIPManager;
class NTRIPSettings;
struct NTRIPConfiguration;
class RTKAutoConnect;
class QTimer;
class Vehicle;

class GPSManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("GPSCorrectionManager.h")
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT)

public:
    GPSManager(QObject* parent = nullptr);
    ~GPSManager();

    static GPSManager* instance();

    void init();
    void shutdown();
    static NTRIPConfiguration ntripConfigFromSettings(NTRIPSettings& settings);
    void configureGgaProvider(NTRIPGgaProvider& provider, NTRIPSettings* settings);
    /// nowUs shares the observation's local monotonic clock domain.
    static PositionResult vehicleGgaPosition(Vehicle* vehicle, NTRIPGgaProvider::PositionSource source, quint64 nowUs);

    GPSRtk* gpsRtk() { return _gpsRtk; }

    GPSCorrectionManager* corrections() const { return _corrections; }

private:
    void _updateConnections();
    QTimer* _connectionTimer = nullptr;
    NMEASourceManager* _nmeaSources = nullptr;
#ifndef QGC_NO_SERIAL_LINK
    RTKAutoConnect* _rtkAutoConnect = nullptr;
#endif
    GPSCorrectionManager* _corrections = nullptr;
    GPSRtk* _gpsRtk = nullptr;
    QPointer<NTRIPManager> _ntripManager;
    bool _shutdown = false;
};
