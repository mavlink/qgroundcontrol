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

NMEAConnectionAttempt::NMEAConnectionAttempt(const GPSReceiverProfile& profile, QObject* parent, quint64 generation)
    : QObject(parent)
    , _attempt{generation,
               std::make_shared<const GPSReceiverProfile>(profile.normalized()),
               GPSReceiverAttempt::Phase::Idle,
               GPSConnectionError::None,
               {}}
    , _receiver(this)
    , _connectTimer(this)
{
    qCDebug(NMEAConnectionAttemptLog) << this;
    _connectTimer.setSingleShot(true);
    _connectTimer.setInterval(10000);
    connect(&_connectTimer, &QTimer::timeout, this, [this]() { _fail(tr("Connection timed out")); });
    connect(&_receiver, &GPSReceiverSession::configurationStarted, this, [this]() {
        if (_transition(GPSReceiverAttempt::Phase::Configuring)) {
            emit configuring();
        }
    });
    connect(&_receiver, &GPSReceiverSession::receiverReady, this, [this]() {
        if (!_stopping && !_attempt.terminal()) {
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
    if (_attempt.phase != GPSReceiverAttempt::Phase::Idle) {
        return;
    }
    _receiver.setRecordingBuffer(buffer);
    if (!buffer || _attempt.profile->configurationPolicy == GPSReceiverProfile::ConfigurationPolicy::Configure) {
        return;
    }
    const auto metadata = GPSRecordingMetadata::fromProfile(*_attempt.profile);
    _recording = std::make_shared<GPSRecordingStream>(buffer, metadata);
}

bool NMEAConnectionAttempt::_transition(GPSReceiverAttempt::Phase phase, GPSConnectionError error,
                                        const QString& detail)
{
    if (_attempt.terminal() || _attempt.phase == phase) {
        return false;
    }
    const QPointer<NMEAConnectionAttempt> guard(this);
    _attempt.phase = phase;
    _attempt.error = error;
    _attempt.errorDetail = detail;
    const auto snapshot = _attempt;
    emit attemptChanged(snapshot);
    return guard && _attempt.phase == phase;
}

void NMEAConnectionAttempt::_publishDeviceReady()
{
    if (_recording && !_recordingDevice) {
        _recording->opened(true, _openStartedAtUs);
        _recordingDevice = std::make_unique<GPSRecordingDevice>(device(), _recording);
    }
    if (_transition(GPSReceiverAttempt::Phase::Ready)) {
        emit deviceReady();
    }
}

void NMEAConnectionAttempt::start(GPSProvider::TransportFactory receiverFactory)
{
    if (_attempt.phase != GPSReceiverAttempt::Phase::Idle || _stopping) {
        return;
    }
    if (!_transition(GPSReceiverAttempt::Phase::Connecting)) {
        return;
    }
    _openStartedAtUs = _recording ? _recording->nowUs() : 0;
    if (const QString error = _attempt.profile->validationError(); !error.isEmpty()) {
        _fail(error);
        return;
    }
    if (_attempt.profile->receiver.outputProtocol != GPSReceiverConfig::OutputProtocol::NMEA) {
        _fail(tr("NMEA input requires NMEA output"));
        return;
    }
    if (_attempt.profile->configurationPolicy == GPSReceiverProfile::ConfigurationPolicy::Configure) {
        _startConfigured(std::move(receiverFactory));
        return;
    }
    if (receiverFactory) {
        _fail(tr("Passive input cannot configure a receiver"));
        return;
    }
    switch (_attempt.profile->endpoint.kind) {
        case GPSReceiverProfile::Endpoint::Kind::UdpListener: {
            _udp = std::make_unique<UdpIODevice>();
            if (!_udp->bind(QHostAddress::AnyIPv4, static_cast<quint16>(_attempt.profile->endpoint.port))) {
                _fail(tr("Cannot listen on UDP port %1: %2")
                          .arg(_attempt.profile->endpoint.port)
                          .arg(_udp->errorString()));
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
                if (!_stopping && !_attempt.terminal()) {
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
            _tcp->connectToHost(_attempt.profile->networkHost(), static_cast<quint16>(_attempt.profile->endpoint.port));
            return;
        }
        case GPSReceiverProfile::Endpoint::Kind::Serial:
#ifndef QGC_NO_SERIAL_LINK
            if (!_reserveSerial()) {
                return;
            }
            _serial = std::make_unique<QSerialPort>();
            _serial->setPortName(_attempt.profile->endpoint.device);
            if (!_serial->setBaudRate(_attempt.profile->endpoint.baud) || !_serial->open(QIODevice::ReadOnly)) {
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
        if (_attempt.profile->endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial) {
            if (!_reserveSerial()) {
                return;
            }
#ifndef QGC_NO_SERIAL_LINK
            receiverFactory = [device = _attempt.profile->endpoint.device,
                               reservation = std::move(_reservation)](const std::atomic_bool& stop) {
                return std::make_unique<SerialGPSTransport>(device, stop);
            };
#endif
        } else {
            receiverFactory = GPSReceiverTransportFactory::network(*_attempt.profile);
        }
    }
    if (!receiverFactory) {
        _fail(tr("Cannot create receiver connection"));
        return;
    }
    _receiver.start(*_attempt.profile, std::move(receiverFactory));
}

bool NMEAConnectionAttempt::_reserveSerial()
{
#ifndef QGC_NO_SERIAL_LINK
    if (!_serialPorts) {
        _fail(tr("Serial discovery is unavailable"));
        return false;
    }
    _reservation = _serialPorts->reservePort(_attempt.profile->endpoint.device);
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
    if (_stopping || _attempt.terminal()) {
        return;
    }

    if (_recording) {
        if (_recording->isOpen()) {
            _recording->record(
                disconnected ? GPSRecordingBuffer::Kind::Disconnect : GPSRecordingBuffer::Kind::ReadError, {}, -1);
        } else {
            _recording->opened(false, _openStartedAtUs);
        }
    }
    _connectTimer.stop();
    const auto error = _receiver.attempt().error != GPSConnectionError::None
                           ? _receiver.attempt().error
                           : (_attempt.ready() ? GPSConnectionError::DeviceError : GPSConnectionError::OpenFailed);
    if (_transition(GPSReceiverAttempt::Phase::Failed, error, detail)) {
        emit failed(detail);
    }
}

void NMEAConnectionAttempt::stop()
{
    if (_stopping) {
        return;
    }
    const QPointer<NMEAConnectionAttempt> guard(this);
    _stopping = true;
    _transition(GPSReceiverAttempt::Phase::Cancelled);
    if (!guard) {
        return;
    }
    _connectTimer.stop();
    _recordingDevice.reset();
    if (!guard) {
        return;
    }
    _udp.reset();
    if (!guard) {
        return;
    }
    _tcp.reset();
    if (!guard) {
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    _serial.reset();
    if (!guard) {
        return;
    }
    _reservation.reset();
#endif
    _receiver.stop();
    if (guard && !_receiver.stopping()) {
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
    const QPointer<NMEAConnectionAttempt> guard(this);
    stop();
    if (guard) {
        _receiver.shutdown();
    }
}

#ifndef QGC_NO_SERIAL_LINK
void NMEAConnectionAttempt::setSerialDiscovery(SerialPortManager* serialPorts)
{
    if (_attempt.phase == GPSReceiverAttempt::Phase::Idle) {
        _serialPorts = serialPorts;
    }
}
#endif
