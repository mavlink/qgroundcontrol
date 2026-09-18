#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

class GPSCorrectionManager;
class GPSRtk;
class NMEASourceManager;
class NTRIPManager;
class RTKAutoConnect;
class QTimer;

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

    GPSRtk* gpsRtk() { return _gpsRtk; }

    GPSCorrectionManager* corrections() const { return _corrections; }

private:
    void _configureGgaProviders();

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
