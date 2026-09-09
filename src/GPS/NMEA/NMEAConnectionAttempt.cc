#include "NMEAConnectionAttempt.h"

#include <QtNetwork/QTcpSocket>

#include "GPSReceiverTransportFactory.h"
#include "GPSRecordingDevice.h"
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

NMEAConnectionAttempt::NMEAConnectionAttempt(const GPSReceiverProfile& profile, QObject* parent)
    : QObject(parent)
    , _profile(profile.normalized())
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
            _publishDeviceReady();
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
    if (_recordingDevice) {
        return _recordingDevice.get();
    }
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

void NMEAConnectionAttempt::setRecordingBuffer(const std::shared_ptr<GPSRecordingBuffer>& buffer)
{
    if (_started) {
        return;
    }
    _receiver.setRecordingBuffer(buffer);
    if (!buffer || _profile.configurationPolicy == GPSReceiverProfile::ConfigurationPolicy::Configure) {
        return;
    }
    GPSRecordingMetadata metadata;
    metadata.receiver = _profile.receiver;
    metadata.initialBaud = _profile.endpoint.baud;
    switch (_profile.endpoint.kind) {
        case GPSReceiverProfile::Endpoint::Kind::Serial:
            metadata.transport = GPSRecordingMetadata::Transport::Serial;
            break;
        case GPSReceiverProfile::Endpoint::Kind::Tcp:
            metadata.transport = GPSRecordingMetadata::Transport::Tcp;
            break;
        case GPSReceiverProfile::Endpoint::Kind::UdpListener:
        case GPSReceiverProfile::Endpoint::Kind::UdpPeer:
            metadata.transport = GPSRecordingMetadata::Transport::Udp;
            break;
        case GPSReceiverProfile::Endpoint::Kind::Disabled:
            break;
    }
    _recording = std::make_shared<GPSRecordingStream>(buffer, metadata);
}

void NMEAConnectionAttempt::_publishDeviceReady()
{
    if (_recording && !_recordingDevice) {
        _recording->opened(true, _openStartedAtUs);
        _recordingDevice = std::make_unique<GPSRecordingDevice>(device(), _recording);
    }
    emit deviceReady();
}

void NMEAConnectionAttempt::start(GPSProvider::TransportFactory receiverFactory)
{
    if (_started || _stopping) {
        return;
    }
    _started = true;
    _openStartedAtUs = _recording ? _recording->nowUs() : 0;
    if (const QString error = _profile.validationError(); !error.isEmpty()) {
        _fail(error);
        return;
    }
    if (_profile.receiver.outputProtocol != GPSReceiverConfig::OutputProtocol::NMEA) {
        _fail(tr("NMEA input requires NMEA output"));
        return;
    }
    if (_profile.configurationPolicy == GPSReceiverProfile::ConfigurationPolicy::Configure) {
        _startConfigured(std::move(receiverFactory));
        return;
    }
    if (receiverFactory) {
        _fail(tr("Passive input cannot configure a receiver"));
        return;
    }
    switch (_profile.endpoint.kind) {
        case GPSReceiverProfile::Endpoint::Kind::UdpListener: {
            _udp = std::make_unique<UdpIODevice>();
            if (!_udp->bind(QHostAddress::AnyIPv4, static_cast<quint16>(_profile.endpoint.port))) {
                _fail(tr("Cannot listen on UDP port %1: %2").arg(_profile.endpoint.port).arg(_udp->errorString()));
                return;
            }
            connect(_udp.get(), &QIODevice::readyRead, this, &NMEAConnectionAttempt::dataReceived);
            _publishDeviceReady();
            return;
        }
        case GPSReceiverProfile::Endpoint::Kind::Tcp: {
            _tcp = std::make_unique<QTcpSocket>();
            _tcp->setReadBufferSize(64 * 1024);
            connect(_tcp.get(), &QTcpSocket::connected, this, [this]() {
                _connectTimer.stop();
                if (!_stopping && !_failed) {
                    _publishDeviceReady();
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
                [this]() { _fail(tr("Connection closed — reconnecting"), true); }, Qt::QueuedConnection);
            _connectTimer.start();
            _tcp->connectToHost(_profile.networkHost(), static_cast<quint16>(_profile.endpoint.port));
            return;
        }
        case GPSReceiverProfile::Endpoint::Kind::Serial:
#ifndef QGC_NO_SERIAL_LINK
            if (!_reserveSerial()) {
                return;
            }
            _serial = std::make_unique<QSerialPort>();
            _serial->setPortName(_profile.endpoint.device);
            if (!_serial->setBaudRate(_profile.endpoint.baud) || !_serial->open(QIODevice::ReadOnly)) {
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
            _publishDeviceReady();
#else
            _fail(tr("Serial connections are unavailable in this build"));
#endif
            return;
        case GPSReceiverProfile::Endpoint::Kind::Disabled:
            return;
        case GPSReceiverProfile::Endpoint::Kind::UdpPeer:
            _fail(tr("Passive UDP input requires a listener endpoint"));
            return;
    }
}

void NMEAConnectionAttempt::_startConfigured(GPSProvider::TransportFactory receiverFactory)
{
    if (!receiverFactory) {
        if (_profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial) {
            if (!_reserveSerial()) {
                return;
            }
#ifndef QGC_NO_SERIAL_LINK
            receiverFactory = [device = _profile.endpoint.device,
                               reservation = std::move(_reservation)](const std::atomic_bool& stop) {
                return std::make_unique<SerialGPSTransport>(device, stop);
            };
#endif
        } else {
            receiverFactory = GPSReceiverTransportFactory::network(_profile);
        }
    }
    if (!receiverFactory) {
        _fail(tr("Cannot create receiver connection"));
        return;
    }
    _receiver.start(_profile.driverType, std::move(receiverFactory), _profile.receiver);
}

bool NMEAConnectionAttempt::_reserveSerial()
{
#ifndef QGC_NO_SERIAL_LINK
    if (!_serialPorts) {
        _fail(tr("Serial discovery is unavailable"));
        return false;
    }
    _reservation = _serialPorts->reservePort(_profile.endpoint.device);
    if (!_reservation) {
        _fail(tr("Serial device is in use"));
        return false;
    }
    return true;
#else
    _fail(tr("Serial connections are unavailable in this build"));
    return false;
#endif
}

void NMEAConnectionAttempt::_fail(const QString& detail, bool disconnected)
{
    if (_stopping || _failed) {
        return;
    }
    _failed = true;
    if (_recording) {
        if (_recording->isOpen()) {
            _recording->record(
                disconnected ? GPSRecordingBuffer::Kind::Disconnect : GPSRecordingBuffer::Kind::ReadError, {}, -1);
        } else {
            _recording->opened(false, _openStartedAtUs);
        }
    }
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
    _recordingDevice.reset();
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
