// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

// In-memory QGCSerialPort for unit tests: no OS device, no hardware. Inject via
// SerialPlatform::setPortFactoryForTest() to drive SerialWorker/SerialLink, or use directly.
//   feedReceived(bytes)  -> appears on the wire, emits readyRead()
//   writtenData()        -> everything the port was asked to send
//   setLoopback(true)    -> writes echo straight back into the RX buffer
//   injectError()/setOpenResult()/setReconfigureResult() -> scripted failure paths

#include "QGCSerialPort.h"

#include <QtCore/QByteArray>

class MockSerialPort final : public QGCSerialPort
{
    Q_OBJECT

public:
    explicit MockSerialPort(const QString &portName = {}, QObject *parent = nullptr);

    void feedReceived(const QByteArray &data);
    QByteArray writtenData() const { return _written; }
    void clearWritten() { _written.clear(); }

    void setLoopback(bool on) { _loopback = on; }

    void setOpenResult(bool ok) { _openResult = ok; }
    void setReconfigureResult(bool ok) { _reconfigureResult = ok; }
    void injectError(QGCSerialPortError error, const QString &errorString = {});

    // Simulate a full TX buffer: writeData() returns 0 (no bytes accepted) until resumeWrites() drains it.
    void setWriteStalled(bool on) { _writeStalled = on; }
    void resumeWrites();
    // Next writeData() returns -1 with a Write error, exercising the worker's write-failure path.
    void setWriteShouldFail(bool on) { _writeShouldFail = on; }

    SerialPortConfig lastConfig() const { return _lastConfig; }
    bool dataTerminalReady() const { return _dtr; }

    // QGCSerialPort
    void setPortName(const QString &name) override { _portName = name; }
    QString portName() const override { return _portName; }
    bool reconfigure(const SerialPortConfig &cfg) override;
    bool setDataTerminalReady(bool on) override { _dtr = on; return true; }
    bool flush() override { return true; }
    void setWriteBufferSize(qint64 size) override { _writeBufferSize = size; }
    qint64 writeBufferSize() const override { return _writeBufferSize; }
    QGCSerialPortError error() const override { return _error; }
    void clearError() override { _error = QGCSerialPortError::NoError; }

    // QIODevice
    bool open(QIODevice::OpenMode mode) override;
    void close() override;
    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return _rxBuffer.size() + QIODevice::bytesAvailable(); }

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 size) override;

private:
    void _setError(QGCSerialPortError error, const QString &errorString);

    QString _portName;
    QByteArray _rxBuffer;
    QByteArray _written;        // cumulative TX history for assertions, never drained
    qint64 _txBuffered = 0;     // in-flight TX occupancy for backpressure; drains as bytesWritten fires
    SerialPortConfig _lastConfig;
    QGCSerialPortError _error = QGCSerialPortError::NoError;
    qint64 _writeBufferSize = kSerialWriteBufferCapBytes;
    bool _loopback = false;
    bool _dtr = false;
    bool _openResult = true;
    bool _reconfigureResult = true;
    bool _writeStalled = false;
    bool _writeShouldFail = false;
};
