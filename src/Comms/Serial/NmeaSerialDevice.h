#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include <memory>

class QGCSerialPort;

// Reads a serial NMEA GPS on a worker thread, exposed as a sequential read-only QIODevice that QNmeaPositionInfoSource drives on the GUI thread.
// Needed on Android, whose USB-serial backend refuses GUI-thread open() while QtPositioning opens its source there (mirrors UdpIODevice for NMEA-over-UDP).
// Worker moved onto a QThread (not a QThread subclass, per the project rule); reads run off the thread's event loop via readyRead, and stop() tears it down from the owner thread — no polling.
class NmeaSerialReader : public QObject
{
    Q_OBJECT

public:
    NmeaSerialReader(QString portName, qint32 baud, QObject *parent = nullptr);
    ~NmeaSerialReader() override;

public slots:
    void process();  // opens the port then returns; the thread's event loop drives reads via readyRead
    void stop();     // closes the port and emits finished(); invoke via a queued connection from the owner thread

signals:
    void dataReceived(const QByteArray &chunk);
    void finished();       // worker exited (open failure or stop) — quit the thread; does not close the device
    void sourceLost();     // port closed under us after a successful open (USB unplug) — close the device

private slots:
    void _onReadyRead();
    void _onPortAboutToClose();

private:
    const QString _portName;
    const qint32 _baud;
    std::unique_ptr<QGCSerialPort> _serial;
    bool _finished = false;
};

class NmeaSerialDevicePrivate;

class NmeaSerialDevice : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(NmeaSerialDevice)

public:
    NmeaSerialDevice(QString portName, qint32 baud, QObject *parent = nullptr);
    ~NmeaSerialDevice() override;

    bool isSequential() const override { return true; }
    bool open(OpenMode mode) override;
    void close() override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private slots:
    void _appendFromReader(const QByteArray &chunk);
};
