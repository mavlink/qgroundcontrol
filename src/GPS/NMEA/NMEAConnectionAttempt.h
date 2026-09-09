#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <memory>

#include "GPSReceiverProfile.h"
#include "GPSReceiverSession.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

class QIODevice;
class QTcpSocket;
class UdpIODevice;
class QSerialPort;
class GPSRecordingBuffer;
class GPSRecordingStream;
class GPSRecordingDevice;

/// Resources for one attempt. Connection intent and retry policy belong to the caller.
class NMEAConnectionAttempt : public QObject
{
    Q_OBJECT

public:
    explicit NMEAConnectionAttempt(const GPSReceiverProfile& profile, QObject* parent = nullptr,
                                   quint64 generation = 1);
    ~NMEAConnectionAttempt() override;

    void start(GPSProvider::TransportFactory receiverFactory = {});
    void setRecordingBuffer(const std::shared_ptr<GPSRecordingBuffer>& buffer);
#ifndef QGC_NO_SERIAL_LINK
    void setSerialDiscovery(SerialPortManager* serialPorts);
#endif
    void stop();
    void shutdown();
    QIODevice* device() const;

    bool stopping() const { return _stopping; }

    const GPSReceiverAttempt& attempt() const { return _attempt; }

    quint16 localPort() const;

signals:
    void attemptChanged(const GPSReceiverAttempt& attempt);
    void deviceReady();
    void configuring();
    void dataReceived();
    void failed(const QString& detail);
    void stopped();

private:
    bool _transition(GPSReceiverAttempt::Phase phase, GPSConnectionError error = GPSConnectionError::None,
                     const QString& detail = {});
    void _publishDeviceReady();
    void _startConfigured(GPSProvider::TransportFactory receiverFactory);
    bool _reserveSerial();
    void _fail(const QString& detail, bool disconnected = false);
    void _finishStop();

    std::shared_ptr<GPSRecordingStream> _recording;
    std::unique_ptr<GPSRecordingDevice> _recordingDevice;
    GPSReceiverAttempt _attempt;
    GPSReceiverSession _receiver;
    QTimer _connectTimer;
    std::unique_ptr<UdpIODevice> _udp;
    std::unique_ptr<QTcpSocket> _tcp;
#ifndef QGC_NO_SERIAL_LINK
    QPointer<SerialPortManager> _serialPorts;
    std::unique_ptr<QSerialPort> _serial;
    SerialPortManager::ReservationPtr _reservation;
#endif
    quint64 _openStartedAtUs = 0;
    bool _stopping = false;
    bool _stopped = false;
};
