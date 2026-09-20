#pragma once

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
class QSerialPort;
#endif

class AutoConnectSettings;
class QGCPositionManager;
class UdpIODevice;

/// Owns the NMEA input device; PositionManager owns decoding and GCS fix state.
/// Lifecycle notifications may replace or delete this owner; retiring devices survive decoder detachment.
class NMEASourceManager : public QObject
{
    Q_OBJECT
    friend class NMEASourceManagerTest;

public:
    NMEASourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager, QObject* parent = nullptr);
    ~NMEASourceManager() override;
    void update();
    void stop();

private:
    void _stop(const char* reason);

    QPointer<AutoConnectSettings> _settings;
    QPointer<QGCPositionManager> _positionManager;
    std::unique_ptr<UdpIODevice> _udp;
    int _source = -1;
    bool _sourceInstalled = false;
    quint64 _revision = 0;
    bool _destroying = false;
#ifndef QGC_NO_SERIAL_LINK
    void _updateSerialRouting();
    std::unique_ptr<QSerialPort> _serial;
    SerialPortManager::ReservationPtr _reservation;
    SerialPortManager::ReservationPtr _autoConnectExclusion;
    QString _serialDevice;
    qint32 _serialBaud = 0;
#endif
};
