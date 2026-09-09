#include "NMEAConnectionAttempt.h"

#include <QtCore/QUrl>
#include <QtNetwork/QTcpSocket>

#include "QGCLoggingCategory.h"
#include "UdpIODevice.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif
#endif

QGC_LOGGING_CATEGORY(NMEAConnectionAttemptLog, "GPS.NMEA.NMEAConnectionAttempt")

NMEAConnectionAttempt::NMEAConnectionAttempt(const NMEAConnectionConfig& config, QObject* parent)
    : QObject(parent)
    , _config(config)
    , _receiver(this)
    , _connectTimer(this)
{
    qCDebug(NMEAConnectionAttemptLog) << this;
    _connectTimer.setSingleShot(true);
    _connectTimer.setInterval(10000);
    connect(&_connectTimer, &QTimer::timeout, this, [this]() { _fail(tr("Connection timed out")); });
    connect(&_receiver, &GPSReceiverSession::configurationStarted, this, &NMEAConnectionAttempt::configuring);
    connect(&_receiver, &GPSReceiverSession::receiverReady, this, [this]() {
        if (!_stopping && !_failed) {
            emit deviceReady();
        }
    });
    connect(&_receiver, &GPSReceiverSession::connectionError, this, [this](GPSConnectionError error) {
        const QString summary = error == GPSConnectionError::DeviceError ? tr("Serial connection lost; reconnecting")
                                                                         : tr("Cannot configure receiver for NMEA");
        _fail(_receiver.errorDetail().isEmpty() ? summary : tr("%1: %2").arg(summary, _receiver.errorDetail()));
    });
    connect(&_receiver, &GPSReceiverSession::stateChanged, this, [this]() {
        if (_stopping && !_receiver.stopping()) {
            _finishStop();
        }
    });
}

NMEAConnectionAttempt::~NMEAConnectionAttempt()
{
    qCDebug(NMEAConnectionAttemptLog) << this;
    disconnect(this, nullptr, nullptr, nullptr);
    shutdown();
}

QIODevice* NMEAConnectionAttempt::device() const
{
    if (_udp) {
        return _udp.get();
    }
    if (_tcp) {
        return _tcp.get();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_serial) {
        return _serial.get();
    }
#endif
    return _receiver.nmeaDevice();
}

quint16 NMEAConnectionAttempt::localPort() const
{
    return _udp ? _udp->localPort() : 0;
}

void NMEAConnectionAttempt::start(GPSProvider::TransportFactory receiverFactory)
{
    if (_started || _stopping) {
        return;
    }
    _started = true;
    if (receiverFactory) {
        GPSReceiverConfig config;
        config.role = GPSReceiverConfig::Role::Position;
        config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
        _receiver.start(GPSType::u_blox, std::move(receiverFactory), config);
        return;
    }
    switch (_config.source) {
        case NMEAConnectionConfig::Udp: {
            _udp = std::make_unique<UdpIODevice>();
            if (!_udp->bind(QHostAddress::AnyIPv4, static_cast<quint16>(_config.port))) {
                _fail(tr("Cannot listen on UDP port %1: %2").arg(_config.port).arg(_udp->errorString()));
                return;
            }
            connect(_udp.get(), &QIODevice::readyRead, this, &NMEAConnectionAttempt::dataReceived);
            emit deviceReady();
            return;
        }
        case NMEAConnectionConfig::Tcp: {
            _tcp = std::make_unique<QTcpSocket>();
            _tcp->setReadBufferSize(64 * 1024);
            connect(_tcp.get(), &QTcpSocket::connected, this, [this]() {
                _connectTimer.stop();
                if (!_stopping && !_failed) {
                    emit deviceReady();
                }
            });
            connect(
                _tcp.get(), &QTcpSocket::errorOccurred, this,
                [this]() {
                    if (_tcp) {
                        _fail(tr("%1 — reconnecting").arg(_tcp->errorString()));
                    }
                },
                Qt::QueuedConnection);
            connect(
                _tcp.get(), &QTcpSocket::disconnected, this,
                [this]() { _fail(tr("Connection closed — reconnecting")); }, Qt::QueuedConnection);
            QUrl endpoint;
            endpoint.setScheme(QStringLiteral("tcp"));
            endpoint.setHost(_config.host);
            _connectTimer.start();
            _tcp->connectToHost(endpoint.host(), static_cast<quint16>(_config.port));
            return;
        }
        case NMEAConnectionConfig::Serial:
#ifndef QGC_NO_SERIAL_LINK
            if (!_serialPorts) {
                _fail(tr("Serial discovery is unavailable"));
                return;
            }
            _reservation = _serialPorts->reservePort(_config.device);
            if (!_reservation) {
                _fail(tr("Serial device is in use"));
                return;
            }
            if (_config.receiverMode == NMEAConnectionConfig::Ublox) {
                GPSReceiverConfig config;
                config.role = GPSReceiverConfig::Role::Position;
                config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
                _receiver.start(
                    GPSType::u_blox,
                    [device = _config.device, reservation = std::move(_reservation)](const std::atomic_bool& stop) {
                        return std::make_unique<SerialGPSTransport>(device, stop);
                    },
                    config);
                return;
            }
            _serial = std::make_unique<QSerialPort>();
            _serial->setPortName(_config.device);
            if (!_serial->setBaudRate(_config.baud) || !_serial->open(QIODevice::ReadOnly)) {
                _fail(tr("Cannot open serial device: %1").arg(_serial->errorString()));
                return;
            }
            connect(
                _serial.get(), &QSerialPort::errorOccurred, this,
                [this](QSerialPort::SerialPortError error) {
                    if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
                        _fail(tr("Serial connection lost; reconnecting"));
                    }
                },
                Qt::QueuedConnection);
            emit deviceReady();
#else
            _fail(tr("Serial connections are unavailable in this build"));
#endif
            return;
        case NMEAConnectionConfig::Disabled:
            return;
    }
}

void NMEAConnectionAttempt::_fail(const QString& detail)
{
    if (_stopping || _failed) {
        return;
    }
    _failed = true;
    _connectTimer.stop();
    emit failed(detail);
}

void NMEAConnectionAttempt::stop()
{
    if (_stopping) {
        return;
    }
    _stopping = true;
    _connectTimer.stop();
    _udp.reset();
    _tcp.reset();
#ifndef QGC_NO_SERIAL_LINK
    _serial.reset();
    _reservation.reset();
#endif
    _receiver.stop();
    if (!_receiver.stopping()) {
        _finishStop();
    }
}

void NMEAConnectionAttempt::_finishStop()
{
    if (!_stopped) {
        _stopped = true;
        emit stopped();
    }
}

void NMEAConnectionAttempt::shutdown()
{
    stop();
    _receiver.shutdown();
}

#ifndef QGC_NO_SERIAL_LINK
void NMEAConnectionAttempt::setSerialDiscovery(SerialPortManager* serialPorts)
{
    if (!_started) {
        _serialPorts = serialPorts;
    }
}
#endif
