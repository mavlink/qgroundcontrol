#include "NmeaSourceManager.h"

#include <QtCore/QUrl>
#include <QtNetwork/QTcpSocket>

#include <algorithm>

#include "AutoConnectSettings.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "UdpIODevice.h"

#ifndef QGC_NO_SERIAL_LINK
#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif
#endif

QGC_LOGGING_CATEGORY(NmeaSourceManagerLog, "GPS.NMEA.NmeaSourceManager")

NmeaSourceManager::NmeaSourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager,
                                     QObject* parent)
    : QObject(parent)
    , _settings(settings)
    , _positionManager(positionManager)
{
    qCDebug(NmeaSourceManagerLog) << this;
    _status = tr("Disconnected");
    if (_settings) {
        for (Fact* fact : {_settings->nmeaSource(), _settings->autoConnectNmeaPort(), _settings->autoConnectNmeaBaud(),
                           _settings->nmeaUdpPort(), _settings->nmeaTcpHost(), _settings->nmeaTcpPort()}) {
            connect(fact, &Fact::rawValueChanged, this, &NmeaSourceManager::_settingsChanged);
        }
        connect(_settings->nmeaAutoConnect(), &Fact::rawValueChanged, this, [this]() {
            _paused = false;
            _manualRequested = false;
            _settingsChanged();
        });
        _updateSerialRouting();
    }
}

bool NmeaSourceManager::_shouldConnect() const
{
    return _settings && !_paused && (_manualRequested || _settings->nmeaAutoConnect()->rawValue().toBool()) &&
           _settings->nmeaSource()->rawValue().toInt() != AutoConnectSettings::NmeaSourceDisabled;
}

void NmeaSourceManager::_updateSerialRouting()
{
#ifndef QGC_NO_SERIAL_LINK
    const QString port =
        _shouldConnect() && _settings->nmeaSource()->rawValue().toInt() == AutoConnectSettings::NmeaSourceSerial
            ? _settings->autoConnectNmeaPort()->rawValue().toString().trimmed()
            : QString();
    _autoConnectExclusion = SerialPortManager::instance()->excludeFromAutoConnect(port);
#endif
}

void NmeaSourceManager::_settingsChanged()
{
    _closeDevice();
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = 1000;
    _updateSerialRouting();
    if (!_shouldConnect()) {
        stop();
    }
}

bool NmeaSourceManager::connectSource()
{
    if (!_settings || !_positionManager ||
        _settings->nmeaSource()->rawValue().toInt() == AutoConnectSettings::NmeaSourceDisabled) {
        return false;
    }
    _paused = false;
    _manualRequested = true;
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = 1000;
    _updateSerialRouting();
    update();
    return _active;
}

void NmeaSourceManager::disconnectSource()
{
    _paused = true;
    stop();
    _updateSerialRouting();
}

void NmeaSourceManager::_setStatus(const QString& status)
{
    if (_status != status) {
        _status = status;
        emit stateChanged();
    }
}

NmeaSourceManager::~NmeaSourceManager()
{
    qCDebug(NmeaSourceManagerLog) << this;
    stop();
}

void NmeaSourceManager::stop()
{
    _manualRequested = false;
    _closeDevice();
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = 1000;
    if (_active) {
        _active = false;
        emit stateChanged();
    }
    _setStatus(_paused && _settings && _settings->nmeaAutoConnect()->rawValue().toBool()
                   ? tr("Automatic connection paused")
                   : tr("Disconnected"));
}

void NmeaSourceManager::_closeDevice()
{
    // Detach the decoder before destroying the device it reads from.
    if (_sourceInstalled && _positionManager) {
        _positionManager->resetNmeaSourceDevice();
    }
    _sourceInstalled = false;
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
}

void NmeaSourceManager::update()
{
    if (!_settings || !_positionManager || !_shouldConnect()) {
        stop();
        return;
    }
    if (!_active) {
        _active = true;
        emit stateChanged();
    }
    const int source = _settings->nmeaSource()->rawValue().toInt();
    if (_source != source) {
        _closeDevice();
        _source = source;
    }
    if (source == AutoConnectSettings::NmeaSourceTcp) {
        _updateTcp();
        return;
    }
    if (source == AutoConnectSettings::NmeaSourceUdp) {
        const quint16 port = _settings->nmeaUdpPort()->rawValue().toUInt();
        if (_udp && _udp->state() == QAbstractSocket::BoundState && _udp->localPort() == port) {
            return;
        }
        _closeDevice();
        _source = source;
        auto socket = std::make_unique<UdpIODevice>();
        if (!socket->bind(QHostAddress::AnyIPv4, port)) {
            _setStatus(tr("Cannot listen on UDP port %1: %2").arg(port).arg(socket->errorString()));
            return;
        }
        _udp = std::move(socket);
        _positionManager->setNmeaSourceDevice(_udp.get());
        _sourceInstalled = true;
        _setStatus(tr("Listening on UDP port %1").arg(port));
    }
#ifndef QGC_NO_SERIAL_LINK
    if (source == AutoConnectSettings::NmeaSourceSerial) {
        const QString device = _settings->autoConnectNmeaPort()->rawValue().toString().trimmed();
        const qint32 baud = _settings->autoConnectNmeaBaud()->rawValue().toInt();
        auto* ports = SerialPortManager::instance();
        bool present = false;
        for (const auto& port : ports->availablePorts()) {
            if (port.systemLocation == device) {
                present = true;
                break;
            }
        }
        if (!present || device != _serialDevice || baud != _serialBaud) {
            _closeDevice();
            _source = source;
        }
        if (!present) {
            _setStatus(tr("Waiting for serial device"));
            return;
        }
        if (_serial) {
            return;
        }
        auto reservation = ports->reservePort(device);
        if (!reservation) {
            _setStatus(tr("Serial device is in use"));
            return;
        }
        auto serial = std::make_unique<QSerialPort>();
        serial->setPortName(device);
        if (!serial->setBaudRate(baud) || !serial->open(QIODevice::ReadOnly)) {
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
                    _setStatus(tr("Serial connection lost; reconnecting"));
                }
            },
            Qt::QueuedConnection);
        _positionManager->setNmeaSourceDevice(_serial.get());
        _sourceInstalled = true;
        _setStatus(tr("Connected"));
    }
#else
    if (source == AutoConnectSettings::NmeaSourceSerial) {
        _setStatus(tr("Serial connections are unavailable in this build"));
    }
#endif
}

void NmeaSourceManager::_updateTcp()
{
    if (_tcp) {
        if (_tcp->state() != QAbstractSocket::ConnectedState && _connectDeadline.hasExpired()) {
            _tcpFailed(tr("Connection timed out"));
        }
        return;
    }
    if (!_retryDeadline.isForever() && !_retryDeadline.hasExpired()) {
        return;
    }
    const QString host = _settings->nmeaTcpHost()->rawValue().toString().trimmed();
    const int port = _settings->nmeaTcpPort()->rawValue().toInt();
    QUrl endpoint;
    endpoint.setScheme(QStringLiteral("tcp"));
    endpoint.setHost(host);
    if (host.isEmpty() || !endpoint.isValid() || endpoint.host().isEmpty() || port < 1 || port > 65535) {
        disconnectSource();
        _setStatus(tr("Enter a valid TCP host and port"));
        return;
    }
    _tcp = std::make_unique<QTcpSocket>();
    _tcp->setReadBufferSize(64 * 1024);
    const QPointer<QTcpSocket> socket = _tcp.get();
    connect(socket, &QTcpSocket::connected, this, [this, socket]() {
        if (!socket || _tcp.get() != socket || !_shouldConnect() || !_positionManager) {
            return;
        }
        _retryDelayMs = 1000;
        _retryDeadline = QDeadlineTimer::Forever;
        _positionManager->setNmeaSourceDevice(socket);
        _sourceInstalled = true;
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

void NmeaSourceManager::_tcpFailed(const QString& error)
{
    _closeDevice();
    _source = AutoConnectSettings::NmeaSourceTcp;
    _retryDeadline.setRemainingTime(_retryDelayMs);
    _retryDelayMs = (std::min) (_retryDelayMs * 2, 30000);
    _setStatus(tr("%1 — reconnecting").arg(error));
}
