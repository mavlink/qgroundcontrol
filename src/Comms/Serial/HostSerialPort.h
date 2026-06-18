#pragma once

// QGCSerialPort backed by QSerialPort (host ports + Android "/dev/tty*" direct-UART path).

#include <QtCore/QBasicTimer>
#include <QtSerialPort/QSerialPort>

#include "QGCSerialPort.h"

class HostSerialPort final : public QGCSerialPort
{
    Q_OBJECT

public:
    explicit HostSerialPort(const QString& portName, QObject* parent = nullptr);

    // Returns a configured read-only port for a kernel TTY NMEA source, or nullptr on failure.
    static QIODevice* openHostNmeaSource(const QString& name, qint32 baud, QObject* parent);

    void setPortName(const QString& name) override { _port.setPortName(name); }

    QString portName() const override { return _port.portName(); }

    bool open(QIODevice::OpenMode mode) override;
    void close() override;

    qint64 bytesAvailable() const override { return _port.bytesAvailable(); }

    qint64 bytesToWrite() const override { return _port.bytesToWrite(); }

    bool waitForReadyRead(int msecs) override { return _port.waitForReadyRead(msecs); }

    bool waitForBytesWritten(int msecs) override { return _port.waitForBytesWritten(msecs); }

    bool reconfigure(const SerialPortConfig& cfg) override;

    bool setDataTerminalReady(bool on) override { return _port.setDataTerminalReady(on); }

    bool flush() override { return _port.flush(); }

    void setWriteBufferSize(qint64 size) override { _port.setWriteBufferSize(size); }

    qint64 writeBufferSize() const override { return _port.writeBufferSize(); }

    QGCSerialPortError error() const override;

    void clearError() override;

protected:
    qint64 readData(char* data, qint64 maxSize) override { return _port.read(data, maxSize); }

    qint64 writeData(const char* data, qint64 size) override { return _port.write(data, size); }

#ifndef Q_OS_ANDROID
    void timerEvent(QTimerEvent* event) override;
#endif

private:
#ifndef Q_OS_ANDROID
    // Backstop poll: QSerialPort doesn't reliably emit ResourceError on unplug for some adapters/macOS ttys, so while
    // open we watch the enumerated port list and synthesize errorOccurred(ResourceUnavailable) when ours disappears.
    // Android needs none of this — USB-host disconnects arrive event-driven via the JNI layer.
    void _checkPresence();
    QBasicTimer _presenceTimer;
    int _presenceMissCount = 0;
#endif
    void _setError(QGCSerialPortError error, const QString& errorString);
    // QSerialPort emits errorOccurred (→ _setError) synchronously on failure; only synthesize one if it stayed silent.
    void _setErrorIfPortSilent(QGCSerialPortError error, const QString& errorString);

    QSerialPort _port;
    QGCSerialPortError _error = QGCSerialPortError::NoError;
};
