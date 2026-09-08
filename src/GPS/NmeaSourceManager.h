#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <memory>

#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
class QSerialPort;
#endif

class AutoConnectSettings;
class QGCPositionManager;
class UdpIODevice;

/// Owns the NMEA input device; PositionManager owns decoding and GCS fix state.
class NmeaSourceManager : public QObject
{
    Q_OBJECT
    friend class NmeaSourceManagerTest;

public:
    NmeaSourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager, QObject* parent = nullptr);
    ~NmeaSourceManager() override;
    void update();
    void stop();

private:
    AutoConnectSettings* _settings;
    QPointer<QGCPositionManager> _positionManager;
    std::unique_ptr<UdpIODevice> _udp;
    int _source = -1;
    bool _sourceInstalled = false;
#ifndef QGC_NO_SERIAL_LINK
    void _updateSerialRouting();
    std::unique_ptr<QSerialPort> _serial;
    SerialPortManager::ReservationPtr _reservation;
    SerialPortManager::ReservationPtr _autoConnectExclusion;
    QString _serialDevice;
    qint32 _serialBaud = 0;
#endif
};
