#include "NMEASourceManager.h"

#include <QtCore/QUrl>
#include <QtNetwork/QTcpSocket>

#include "AutoConnectSettings.h"
#include "PositionManager.h"
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

QGC_LOGGING_CATEGORY(NMEASourceManagerLog, "GPS.NMEA.NMEASourceManager")

NMEASourceManager::NMEASourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager,
                                     QObject* parent)
    : QObject(parent)
    , _settings(settings)
    , _receiver(this)
    , _positionManager(positionManager)
    , _decoder(this)
    , _connection(this)
    , _receiverAutoConnect(&_receiver, _decoder.health(), this, &_connection)
{
    qCDebug(NMEASourceManagerLog) << this;
    connect(&_connection, &GPSConnectionState::changed, this, &NMEASourceManager::stateChanged);
    connect(&_decoder, &NMEADecoderSession::satellitesChanged, this, &NMEASourceManager::satellitesChanged);
    connect(&_receiver, &GPSReceiverSession::configurationStarted, this, [this]() {
        if (_shouldConnect()) {
            _setStatus(tr("Configuring receiver for NMEA"));
        }
    });
    connect(&_receiver, &GPSReceiverSession::receiverReady, this, [this]() {
        if (!_shouldConnect() || !_receiver.ready()) {
            return;
        }
        if (!_installSource(_receiver.nmeaDevice())) {
            _closeDevice();
            return;
        }
        if (_shouldConnect()) {
            _setStatus(tr("Connected"));
        }
    });
    connect(&_receiver, &GPSReceiverSession::connectionError, this, [this](GPSConnectionError error) {
        const QPointer<NMEASourceManager> guard(this);
        _uninstallSource();
        if (!guard) {
            return;
        }
        const QString summary = error == GPSConnectionError::DeviceError ? tr("Serial connection lost; reconnecting")
                                                                         : tr("Cannot configure receiver for NMEA");
        _setStatus(_receiver.errorDetail().isEmpty() ? summary : tr("%1: %2").arg(summary, _receiver.errorDetail()));
    });
    _status = tr("Disconnected");
    _udpActivityTimer.setSingleShot(true);
    _udpActivityTimer.setInterval(5000);
    connect(&_udpActivityTimer, &QTimer::timeout, this, [this]() {
        if (_udp) {
            _setStatus(tr("Listening on UDP port %1").arg(_udp->localPort()));
        }
    });
    if (_settings) {
        _config = NMEAConnectionConfig::fromSettings(*_settings);
        for (Fact* fact : {_settings->nmeaSource(), _settings->autoConnectNmeaPort(), _settings->autoConnectNmeaBaud(),
                           _settings->nmeaUdpPort(), _settings->nmeaTcpHost(), _settings->nmeaTcpPort(),
                           _settings->nmeaReceiverMode()}) {
            connect(fact, &Fact::rawValueChanged, this, &NMEASourceManager::_settingsChanged);
        }
        connect(_settings->nmeaAutoConnect(), &Fact::rawValueChanged, this, [this]() {
            _closeDevice();
            _connection.resetIntent();
            _settingsChanged();
        });
        _updateSerialRouting();
    }
}

QGeoPositionInfoSource* NMEASourceManager::positionSource() const
{
    return _decoder.positionSource();
}

bool NMEASourceManager::_shouldConnect() const
{
    return _settings && _connection.shouldConnect(_settings->nmeaAutoConnect()->rawValue().toBool()) &&
           _config.source != NMEAConnectionConfig::Disabled;
}

void NMEASourceManager::_updateSerialRouting()
{
#ifndef QGC_NO_SERIAL_LINK
    const QString port =
        _shouldConnect() && _config.source == NMEAConnectionConfig::Serial ? _config.device : QString();
    _autoConnectExclusion = SerialPortManager::instance()->excludeFromAutoConnect(port);
#endif
}

void NMEASourceManager::_settingsChanged()
{
    const auto config = NMEAConnectionConfig::fromSettings(*_settings);
    if (config != _config) {
        _config = config;
        _closeDevice();
        _connection.resetRetry();
    }
    _updateSerialRouting();
    if (!_shouldConnect()) {
        stop();
    }
}

bool NMEASourceManager::connectSource()
{
    if (!_settings || !_positionManager || _config.source == NMEAConnectionConfig::Disabled) {
        return false;
    }
    _connection.requestConnect();
    _updateSerialRouting();
    update();
    return active();
}

void NMEASourceManager::disconnectSource()
{
    _connection.pause();
    stop();
    _updateSerialRouting();
}

void NMEASourceManager::_startReceiver(GPSProvider::TransportFactory factory)
{
    GPSConnectionConfig config;
    config.transport = GPSConnectionConfig::Serial;
    config.device = _config.device;
    config.receiverType = GPSType::u_blox;
    config.receiver.role = GPSReceiverConfig::Role::Position;
    config.receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
    _managedReceiver = true;
    _receiverAutoConnect.connectReceiver(config, std::move(factory));
}

void NMEASourceManager::_setStatus(const QString& status)
{
    if (_status != status) {
        _status = status;
        qCDebug(NMEASourceManagerLog) << "Connection status:" << _status;
        emit stateChanged();
    }
}

NMEASourceManager::~NMEASourceManager()
{
    qCDebug(NMEASourceManagerLog) << this;
    shutdown();
}

void NMEASourceManager::shutdown()
{
    stop();
    _receiverAutoConnect.stop();
    _receiver.shutdown();
}

void NMEASourceManager::stop()
{
    _connection.stop();
    _closeDevice();
    _connection.resetRetry();
    _setStatus(_connection.paused() && _settings && _settings->nmeaAutoConnect()->rawValue().toBool()
                   ? tr("Automatic connection paused")
                   : tr("Disconnected"));
}

void NMEASourceManager::_closeDevice()
{
    if (_sourceInstalled || _tcp || _receiver.hasReceiver() || _receiver.stopping()) {
        _connection.stopping();
    }
    _udpActivityTimer.stop();
    _uninstallSource();
    if (_managedReceiver) {
        _managedReceiver = false;
        _receiverAutoConnect.stopAttempt();
    }
    _udp.reset();
    if (_tcp) {
        _tcp->disconnect(this);
        _tcp.reset();
    }
    _connectDeadline = QDeadlineTimer::Forever;
#ifndef QGC_NO_SERIAL_LINK
    _serial.reset();
    _reservation.reset();
    _serialDevice.clear();
    _serialBaud = 0;
#endif
    _source = -1;
    if (!_receiver.stopping()) {
        _connection.stopped();
    }
}

void NMEASourceManager::_uninstallSource()
{
    // Detach the decoder before destroying the device it reads from.
    if (_sourceInstalled && _positionManager) {
        _positionManager->clearNmeaPositionSource(_decoder.positionSource());
    }
    _sourceInstalled = false;
    _decoder.stop();
}

bool NMEASourceManager::_installSource(QIODevice* device)
{
    if (!_positionManager || !device || (!device->isOpen() && !device->open(QIODevice::ReadOnly)) ||
        !device->isReadable()) {
        _setStatus(tr("Cannot read NMEA source"));
        return false;
    }
    device->readAll();
    if (!_decoder.start(device)) {
        return false;
    }
    _positionManager->setNmeaPositionSource(_decoder.positionSource(), _decoder.health());
    _sourceInstalled = true;
    return true;
}

void NMEASourceManager::update()
{
    if (!_settings || !_positionManager || !_shouldConnect()) {
        stop();
        return;
    }
    if (const QString error = _config.validationError(); !error.isEmpty()) {
        disconnectSource();
        _setStatus(error);
        return;
    }
    _connection.updateIntent(_settings->nmeaAutoConnect()->rawValue().toBool());
    const int source = _config.source;
    if (_source != source) {
        _closeDevice();
        _source = source;
    }
    if (source == NMEAConnectionConfig::Tcp) {
        _updateTcp();
        return;
    }
    if (source == NMEAConnectionConfig::Udp) {
        const quint16 port = _config.port;
        if (_udp && _udp->state() == QAbstractSocket::BoundState) {
            return;
        }
        if (_udp) {
            _closeDevice();
        }
        _source = source;
        if (!_connection.beginAttempt()) {
            return;
        }
        auto socket = std::make_unique<UdpIODevice>();
        if (!socket->bind(QHostAddress::AnyIPv4, port)) {
            _connection.failed();
            _setStatus(tr("Cannot listen on UDP port %1: %2").arg(port).arg(socket->errorString()));
            return;
        }
        _udp = std::move(socket);
        connect(_udp.get(), &QIODevice::readyRead, this, [this]() {
            _udpActivityTimer.start();
            _setStatus(tr("Receiving UDP data on port %1").arg(_udp->localPort()));
        });
        if (!_installSource(_udp.get())) {
            _closeDevice();
            return;
        }
        _connection.ready();
        _setStatus(tr("Listening on UDP port %1").arg(port));
    }
#ifndef QGC_NO_SERIAL_LINK
    if (source == NMEAConnectionConfig::Serial) {
        const QString device = _config.device;
        const qint32 baud = _config.baud;
        auto* ports = SerialPortManager::instance();
        bool present = false;
        for (const auto& port : ports->availablePorts()) {
            if (port.systemLocation == device) {
                present = true;
                break;
            }
        }
        if (!present || (_serial && (device != _serialDevice || baud != _serialBaud))) {
            _closeDevice();
            _source = source;
        }
        if (!present) {
            _connection.resetRetry();
            _setStatus(tr("Waiting for serial device"));
            return;
        }
        if (_managedReceiver) {
            _receiverAutoConnect.update();
            return;
        }
        if (_serial || _receiver.hasReceiver() || _receiver.stopping()) {
            return;
        }
        if (!_connection.canAttempt()) {
            return;
        }
        auto reservation = ports->reservePort(device);
        if (!reservation) {
            _setStatus(tr("Serial device is in use"));
            return;
        }
        if (_config.receiverMode == NMEAConnectionConfig::Ublox) {
            _startReceiver([device, reservation = std::move(reservation)](const std::atomic_bool& stop) {
                return std::make_unique<SerialGPSTransport>(device, stop);
            });
            return;
        }
        if (!_connection.beginAttempt()) {
            return;
        }
        auto serial = std::make_unique<QSerialPort>();
        serial->setPortName(device);
        if (!serial->setBaudRate(baud) || !serial->open(QIODevice::ReadOnly)) {
            _connection.failed();
            _setStatus(tr("Cannot open serial device: %1").arg(serial->errorString()));
            return;
        }
        _serialDevice = device;
        _serialBaud = baud;
        _reservation = std::move(reservation);
        _serial = std::move(serial);
        connect(
            _serial.get(), &QSerialPort::errorOccurred, this,
            [this, current = QPointer<QSerialPort>(_serial.get())](QSerialPort::SerialPortError error) {
                if (current && _serial.get() == current && error != QSerialPort::NoError &&
                    error != QSerialPort::TimeoutError) {
                    _closeDevice();
                    _source = NMEAConnectionConfig::Serial;
                    _connection.failed();
                    _setStatus(tr("Serial connection lost; reconnecting"));
                }
            },
            Qt::QueuedConnection);
        if (!_installSource(_serial.get())) {
            _closeDevice();
            return;
        }
        _connection.ready();
        _setStatus(tr("Connected"));
    }
#else
    if (source == NMEAConnectionConfig::Serial) {
        _setStatus(tr("Serial connections are unavailable in this build"));
    }
#endif
}

void NMEASourceManager::_updateTcp()
{
    if (_tcp) {
        if (_tcp->state() != QAbstractSocket::ConnectedState && _connectDeadline.hasExpired()) {
            _tcpFailed(tr("Connection timed out"));
        }
        return;
    }
    if (!_connection.canAttempt()) {
        return;
    }
    QUrl endpoint;
    endpoint.setScheme(QStringLiteral("tcp"));
    endpoint.setHost(_config.host);
    const int port = _config.port;
    if (!_connection.beginAttempt()) {
        return;
    }
    _tcp = std::make_unique<QTcpSocket>();
    _tcp->setReadBufferSize(64 * 1024);
    const QPointer<QTcpSocket> socket = _tcp.get();
    connect(socket, &QTcpSocket::connected, this, [this, socket]() {
        if (!socket || _tcp.get() != socket || !_shouldConnect() || !_positionManager) {
            return;
        }

        if (!_installSource(socket)) {
            _closeDevice();
            return;
        }
        _connection.ready();
        _setStatus(tr("Connected"));
    });
    connect(
        socket, &QTcpSocket::errorOccurred, this,
        [this, socket]() {
            if (socket && _tcp.get() == socket) {
                _tcpFailed(socket->errorString());
            }
        },
        Qt::QueuedConnection);
    connect(
        socket, &QTcpSocket::disconnected, this,
        [this, socket]() {
            if (socket && _tcp.get() == socket) {
                _tcpFailed(tr("Connection closed"));
            }
        },
        Qt::QueuedConnection);
    _connectDeadline.setRemainingTime(10000);
    _setStatus(tr("Connecting"));
    _tcp->connectToHost(endpoint.host(), static_cast<quint16>(port));
}

void NMEASourceManager::_tcpFailed(const QString& error)
{
    _closeDevice();
    _source = NMEAConnectionConfig::Tcp;
    _connection.failed();
    _setStatus(tr("%1 — reconnecting").arg(error));
}
