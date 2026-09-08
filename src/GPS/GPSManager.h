#pragma once

#include <QtCore/QObject>

class GPSRtk;
class NmeaSourceManager;
class RTKAutoConnect;
class QTimer;

class GPSManager : public QObject
{
    Q_OBJECT

public:
    GPSManager(QObject *parent = nullptr);
    ~GPSManager();

    static GPSManager *instance();

    void init();
    void shutdown();

    GPSRtk *gpsRtk() { return _gpsRtk; }

private:
    void _updateConnections();
    QTimer* _connectionTimer = nullptr;
    NmeaSourceManager* _nmeaSources = nullptr;
#ifndef QGC_NO_SERIAL_LINK
    RTKAutoConnect* _rtkAutoConnect = nullptr;
#endif
    GPSRtk *_gpsRtk = nullptr;
};
