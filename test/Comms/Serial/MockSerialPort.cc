// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "MockSerialPort.h"

#include <cstring>

MockSerialPort::MockSerialPort(const QString &portName, QObject *parent)
    : QGCSerialPort(parent), _portName(portName)
{
}

bool MockSerialPort::open(QIODevice::OpenMode mode)
{
    if (!_openResult) {
        _setError(QGCSerialPortError::OpenFailed, QStringLiteral("mock open refused"));
        return false;
    }
    return QIODevice::open(mode);
}

void MockSerialPort::close()
{
    _rxBuffer.clear();
    _txBuffered = 0;
    QIODevice::close();
}

bool MockSerialPort::reconfigure(const SerialPortConfig &cfg)
{
    _lastConfig = cfg;
    if (!cfg.isValid()) {
        _setError(QGCSerialPortError::UnsupportedOperation, QStringLiteral("invalid serial config"));
        return false;
    }
    if (!_reconfigureResult) {
        _setError(QGCSerialPortError::UnsupportedOperation, QStringLiteral("mock reconfigure refused"));
        return false;
    }
    return true;
}

void MockSerialPort::feedReceived(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }
    _rxBuffer.append(data);
    if (isOpen() && (openMode() & QIODevice::ReadOnly)) {
        emit readyRead();
    }
}

void MockSerialPort::injectError(QGCSerialPortError error, const QString &errorString)
{
    _setError(error, errorString);
}

qint64 MockSerialPort::readData(char *data, qint64 maxSize)
{
    const qint64 n = qMin<qint64>(maxSize, _rxBuffer.size());
    if (n > 0) {
        std::memcpy(data, _rxBuffer.constData(), static_cast<size_t>(n));
        _rxBuffer.remove(0, n);
    }
    return n;
}

qint64 MockSerialPort::writeData(const char *data, qint64 size)
{
    if (!isOpen() || !(openMode() & QIODevice::WriteOnly)) {
        _setError(QGCSerialPortError::NotOpen, QStringLiteral("write on closed mock port"));
        return -1;
    }
    if (_writeShouldFail) {
        _setError(QGCSerialPortError::Write, QStringLiteral("mock write failure"));
        return -1;
    }
    if (_writeStalled || ((_txBuffered + size) > _writeBufferSize)) {
        return 0;  // TX buffer full — mirrors HostSerialPort backpressure (write() returns 0).
    }

    _written.append(data, static_cast<int>(size));
    _txBuffered += size;
    // QSerialPort emits bytesWritten asynchronously after the OS drains; emitting it synchronously here would
    // reenter SerialWorker::_flushPendingWrites (connected to bytesWritten) and recurse. Defer to the event loop,
    // releasing the in-flight occupancy as the "OS" drains it.
    QMetaObject::invokeMethod(this, [this, size]() { _txBuffered -= size; emit bytesWritten(size); },
                              Qt::QueuedConnection);

    if (_loopback) {
        feedReceived(QByteArray(data, static_cast<int>(size)));
    }
    return size;
}

void MockSerialPort::resumeWrites()
{
    _writeStalled = false;
    emit bytesWritten(0);  // wakes SerialWorker::_onPortBytesWritten to drain its pending backlog
}

void MockSerialPort::_setError(QGCSerialPortError error, const QString &errorString)
{
    _error = error;
    setErrorString(errorString);
    emit errorOccurred(error);
}
