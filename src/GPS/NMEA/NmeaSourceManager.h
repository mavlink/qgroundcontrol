#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include <memory>

#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
class QSerialPort;
#endif

class AutoConnectSettings;
class QGCPositionManager;
class QTcpSocket;
class UdpIODevice;

/// Owns NMEA connections; PositionManager owns decoding and GCS fix state.
class NmeaSourceManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    friend class NmeaSourceManagerTest;

public:
    NmeaSourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager, QObject* parent = nullptr);
    ~NmeaSourceManager() override;
    void update();
    void stop();
    bool connectSource();
    void disconnectSource();

    bool active() const { return _active; }

    QString status() const { return _status; }

signals:
    void stateChanged();

private:
    bool _shouldConnect() const;
    void _closeDevice();
    void _setStatus(const QString& status);
    void _updateTcp();
    void _tcpFailed(const QString& error);
    void _settingsChanged();
    void _updateSerialRouting();

    AutoConnectSettings* _settings;
    QPointer<QGCPositionManager> _positionManager;
    std::unique_ptr<UdpIODevice> _udp;
    std::unique_ptr<QTcpSocket> _tcp;
    QTimer _udpActivityTimer;
    QDeadlineTimer _connectDeadline = QDeadlineTimer::Forever;
    QDeadlineTimer _retryDeadline = QDeadlineTimer::Forever;
    int _retryDelayMs = 1000;
    int _source = -1;
    bool _sourceInstalled = false;
    bool _active = false;
    bool _manualRequested = false;
    bool _paused = false;
    QString _status;
#ifndef QGC_NO_SERIAL_LINK
    std::unique_ptr<QSerialPort> _serial;
    SerialPortManager::ReservationPtr _reservation;
    SerialPortManager::ReservationPtr _autoConnectExclusion;
    QString _serialDevice;
    qint32 _serialBaud = 0;
#endif
};
