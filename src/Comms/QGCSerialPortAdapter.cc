// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "QGCSerialPortAdapter.h"

#include "QGCAndroidSerialPort.h"

#ifndef Q_OS_ANDROID
#include <QtSerialPort/QSerialPort>
#endif

// On host, the adapter normally wraps QSerialPort. Tests can call
// setForceAndroidBackendForTests(true) to make it wrap QGCAndroidSerialPort instead
// (paired with QGCAndroidSerialPortFactory::setEngineFactory(mock)) — that's how
// SerialLink integration tests exercise the Android code path without a device.
namespace {
bool& forceAndroidBackend()
{
    static bool flag = false;
    return flag;
}
}  // namespace

void QGCSerialPortAdapter::setForceAndroidBackendForTests(bool force)
{
    forceAndroidBackend() = force;
}

bool QGCSerialPortAdapter::forceAndroidBackendForTests()
{
    return forceAndroidBackend();
}

// ============================================================================
// Strategy interface + per-backend implementations.
//
// Replaces the prior Pimpl-with-ifdef-per-method pattern. The adapter now calls
// exactly one virtual per operation; both concrete types (HostSerialBackend on
// !Q_OS_ANDROID, AndroidSerialBackend everywhere) translate to their underlying
// port. The only platform branching that remains is in makeBackend() — the
// factory that picks the concrete type at construction.
//
// Modeled on Qt's QNetworkAccessBackend / QNetworkAccessBackendFactory split
// (see qtbase/src/network/access/qnetworkaccessbackend_p.h).
// ============================================================================

class QGCSerialBackend
{
public:
    QGCSerialBackend() = default;
    virtual ~QGCSerialBackend() = default;
    Q_DISABLE_COPY_MOVE(QGCSerialBackend)

    virtual QIODevice *device() = 0;
    virtual void setPortName(const QString &name) = 0;
    virtual QString portName() const = 0;
    virtual bool flush() = 0;
    virtual void setWriteBufferSize(qint64 size) = 0;
    virtual void clearError() = 0;
    virtual QGCSerialPortAdapter::Error error() const = 0;
    virtual bool setBaudRate(qint32 baud) = 0;
    virtual bool setDataBits(int bits) = 0;
    virtual bool setParity(int parity) = 0;
    virtual bool setStopBits(int bits) = 0;
    virtual bool setFlowControl(int flow) = 0;
    virtual bool setSerialParameters(qint32 baud, int dataBits, int stopBits, int parity) = 0;
    virtual bool setDataTerminalReady(bool on) = 0;

    // Wires the platform-specific errorOccurred signal so the adapter emits
    // errorOccurred(Error) with a mapped value. Lives on the backend because
    // the source-signal type differs per backend.
    virtual void connectErrorSignal(QGCSerialPortAdapter *adapter) = 0;
};

namespace {

QGCDataBits intToDataBits(int v)
{
    switch (v) {
        case 5: return QGCDataBits::Data5;
        case 6: return QGCDataBits::Data6;
        case 7: return QGCDataBits::Data7;
        default: return QGCDataBits::Data8;
    }
}

QGCParity intToParity(int v)
{
    // QSerialPort::Parity values: NoParity=0, EvenParity=2, OddParity=3,
    // SpaceParity=4, MarkParity=5. (Not 0..4 — gap at 1.)
    switch (v) {
        case 0: return QGCParity::None;
        case 2: return QGCParity::Even;
        case 3: return QGCParity::Odd;
        case 4: return QGCParity::Space;
        case 5: return QGCParity::Mark;
        default: return QGCParity::None;
    }
}

QGCStopBits intToStopBits(int v)
{
    switch (v) {
        case 2: return QGCStopBits::Two;
        case 3: return QGCStopBits::OneAndHalf;
        default: return QGCStopBits::One;
    }
}

QGCFlowControl intToFlowControl(int v)
{
    switch (v) {
        case 1: return QGCFlowControl::HardwareRtsCts;
        case 2: return QGCFlowControl::SoftwareXonXoff;
        default: return QGCFlowControl::None;
    }
}

QGCSerialPortAdapter::Error mapAndroidError(QGCSerialPortError e)
{
    switch (e) {
        case QGCSerialPortError::NoError:             return QGCSerialPortAdapter::NoError;
        case QGCSerialPortError::ResourceUnavailable: return QGCSerialPortAdapter::ResourceError;
        case QGCSerialPortError::PermissionDenied:    return QGCSerialPortAdapter::PermissionError;
        case QGCSerialPortError::Timeout:             return QGCSerialPortAdapter::TimeoutError;
        default:                                       return QGCSerialPortAdapter::OtherError;
    }
}

#ifndef Q_OS_ANDROID
QGCSerialPortAdapter::Error mapHostError(QSerialPort::SerialPortError e)
{
    switch (e) {
        case QSerialPort::NoError:         return QGCSerialPortAdapter::NoError;
        case QSerialPort::ResourceError:   return QGCSerialPortAdapter::ResourceError;
        case QSerialPort::PermissionError: return QGCSerialPortAdapter::PermissionError;
        case QSerialPort::TimeoutError:    return QGCSerialPortAdapter::TimeoutError;
        default:                           return QGCSerialPortAdapter::OtherError;
    }
}
#endif

class AndroidSerialBackend final : public QGCSerialBackend
{
public:
    explicit AndroidSerialBackend(QObject *parent) : _port(new QGCAndroidSerialPort(parent)) {}

    QIODevice *device() override { return _port; }
    void setPortName(const QString &name) override { _port->setSystemLocation(name); }
    QString portName() const override { return _port->portName(); }
    bool flush() override { return _port->flush(); }
    void setWriteBufferSize(qint64 size) override { _port->setWriteBufferSize(size); }
    void clearError() override { _port->clearError(); }
    QGCSerialPortAdapter::Error error() const override { return mapAndroidError(_port->error()); }
    bool setBaudRate(qint32 baud) override { return _port->setBaudRate(baud); }
    bool setDataBits(int bits) override { return _port->setDataBits(intToDataBits(bits)); }
    bool setParity(int parity) override { return _port->setParity(intToParity(parity)); }
    bool setStopBits(int bits) override { return _port->setStopBits(intToStopBits(bits)); }
    bool setFlowControl(int flow) override { return _port->setFlowControl(intToFlowControl(flow)); }
    bool setSerialParameters(qint32 baud, int dataBits, int stopBits, int parity) override
    {
        return _port->setSerialParameters(baud, intToDataBits(dataBits),
                                          intToStopBits(stopBits), intToParity(parity));
    }
    bool setDataTerminalReady(bool on) override { return _port->setDataTerminalReady(on); }

    void connectErrorSignal(QGCSerialPortAdapter *adapter) override
    {
        QObject::connect(_port, &QGCAndroidSerialPort::errorOccurred, adapter,
            [adapter](QGCSerialPortError e) { emit adapter->errorOccurred(mapAndroidError(e)); });
    }

private:
    QGCAndroidSerialPort *const _port;
};

#ifndef Q_OS_ANDROID
class HostSerialBackend final : public QGCSerialBackend
{
public:
    explicit HostSerialBackend(QObject *parent) : _port(new QSerialPort(parent)) {}

    QIODevice *device() override { return _port; }
    void setPortName(const QString &name) override { _port->setPortName(name); }
    QString portName() const override { return _port->portName(); }
    bool flush() override { return _port->flush(); }
    void setWriteBufferSize(qint64 size) override { _port->setWriteBufferSize(size); }
    void clearError() override { _port->clearError(); }
    QGCSerialPortAdapter::Error error() const override { return mapHostError(_port->error()); }
    bool setBaudRate(qint32 baud) override { return _port->setBaudRate(baud); }
    bool setDataBits(int bits) override { return _port->setDataBits(static_cast<QSerialPort::DataBits>(bits)); }
    bool setParity(int parity) override { return _port->setParity(static_cast<QSerialPort::Parity>(parity)); }
    bool setStopBits(int bits) override { return _port->setStopBits(static_cast<QSerialPort::StopBits>(bits)); }
    bool setFlowControl(int flow) override { return _port->setFlowControl(static_cast<QSerialPort::FlowControl>(flow)); }
    // QSerialPort has no batched setter — call the four in sequence. Each goes
    // straight to termios on Linux/macOS / SetCommState on Windows; no JNI hop.
    bool setSerialParameters(qint32 baud, int dataBits, int stopBits, int parity) override
    {
        const bool ok1 = _port->setBaudRate(baud);
        const bool ok2 = _port->setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
        const bool ok3 = _port->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
        const bool ok4 = _port->setParity(static_cast<QSerialPort::Parity>(parity));
        return ok1 && ok2 && ok3 && ok4;
    }
    bool setDataTerminalReady(bool on) override { return _port->setDataTerminalReady(on); }

    void connectErrorSignal(QGCSerialPortAdapter *adapter) override
    {
        QObject::connect(_port, &QSerialPort::errorOccurred, adapter,
            [adapter](QSerialPort::SerialPortError e) { emit adapter->errorOccurred(mapHostError(e)); });
    }

private:
    QSerialPort *const _port;
};
#endif

// Only platform-conditional path that remains: picking the concrete backend.
std::unique_ptr<QGCSerialBackend> makeBackend(QObject *parent)
{
#ifdef Q_OS_ANDROID
    return std::make_unique<AndroidSerialBackend>(parent);
#else
    if (QGCSerialPortAdapter::forceAndroidBackendForTests()) {
        return std::make_unique<AndroidSerialBackend>(parent);
    }
    return std::make_unique<HostSerialBackend>(parent);
#endif
}

}  // namespace

QGCSerialPortAdapter::QGCSerialPortAdapter(QObject *parent)
    : QIODevice(parent)
    , _backend(makeBackend(this))
{
    QIODevice *const backend = _backend->device();
    // Forward readyRead/aboutToClose from the inner device. readyRead drives
    // consumers' read() calls which route through readData() → backend->read().
    connect(backend, &QIODevice::readyRead,    this, &QIODevice::readyRead);
    connect(backend, &QIODevice::bytesWritten, this, &QIODevice::bytesWritten);
    // Inner can self-close on resource error; mirror that into the adapter's
    // QIODevice state so isOpen() reflects reality. aboutToClose fires BEFORE
    // the inner tears down, so calling QIODevice::close() here keeps the
    // forwarded signal observable to consumers.
    connect(backend, &QIODevice::aboutToClose, this, [this]() {
        if (QIODevice::isOpen()) {
            QIODevice::close();
        }
        emit aboutToClose();
    });
    _backend->connectErrorSignal(this);
}

QGCSerialPortAdapter::~QGCSerialPortAdapter() = default;

void QGCSerialPortAdapter::setPortName(const QString &name) { _backend->setPortName(name); }
QString QGCSerialPortAdapter::portName() const              { return _backend->portName(); }

bool QGCSerialPortAdapter::open(QIODevice::OpenMode mode)
{
    if (!_backend->device()->open(mode)) {
        setErrorString(_backend->device()->errorString());
        return false;
    }
    // Mark the adapter open so QIODevice's read/write plumbing accepts calls.
    QIODevice::open(mode);
    return true;
}

void QGCSerialPortAdapter::close()
{
    _backend->device()->close();
    QIODevice::close();
}

// QIODevice's bytesAvailable returns its own buffer size; we keep adapter buffer
// empty (we delegate readData directly) so forward to the inner backend.
qint64 QGCSerialPortAdapter::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + _backend->device()->bytesAvailable();
}

qint64 QGCSerialPortAdapter::bytesToWrite() const    { return _backend->device()->bytesToWrite(); }
bool QGCSerialPortAdapter::waitForReadyRead(int m)   { return _backend->device()->waitForReadyRead(m); }
bool QGCSerialPortAdapter::waitForBytesWritten(int m){ return _backend->device()->waitForBytesWritten(m); }
bool QGCSerialPortAdapter::flush()                   { return _backend->flush(); }

qint64 QGCSerialPortAdapter::readData(char *data, qint64 maxSize)
{
    return _backend->device()->read(data, maxSize);
}

qint64 QGCSerialPortAdapter::writeData(const char *data, qint64 size)
{
    return _backend->device()->write(data, size);
}

void QGCSerialPortAdapter::setWriteBufferSize(qint64 size) { _backend->setWriteBufferSize(size); }
void QGCSerialPortAdapter::clearError()                    { _backend->clearError(); }
QGCSerialPortAdapter::Error QGCSerialPortAdapter::error() const { return _backend->error(); }

bool QGCSerialPortAdapter::setBaudRate(qint32 baud)        { return _backend->setBaudRate(baud); }
bool QGCSerialPortAdapter::setDataBits(int bits)           { return _backend->setDataBits(bits); }
bool QGCSerialPortAdapter::setParity(int parity)           { return _backend->setParity(parity); }
bool QGCSerialPortAdapter::setStopBits(int bits)           { return _backend->setStopBits(bits); }
bool QGCSerialPortAdapter::setFlowControl(int flow)        { return _backend->setFlowControl(flow); }

bool QGCSerialPortAdapter::setSerialParameters(qint32 baud, int dataBits, int stopBits, int parity)
{
    return _backend->setSerialParameters(baud, dataBits, stopBits, parity);
}

bool QGCSerialPortAdapter::setDataTerminalReady(bool on)
{
    return _backend->device()->isOpen() && _backend->setDataTerminalReady(on);
}
