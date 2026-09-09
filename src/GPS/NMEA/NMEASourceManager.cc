#include "NMEASourceManager.h"

#include <QtCore/QUrl>
#include <QtNetwork/QTcpSocket>
#include <QtPositioning/QNmeaSatelliteInfoSource>

#include "AutoConnectSettings.h"
#include "GPSNMEAPreparation.h"
#include "NMEAPositionSource.h"
#include "NMEASatelliteAdapter.h"
#include "NMEAStreamSplitter.h"
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
    , _positionManager(positionManager)
    , _satellitePollTimer(this)
    , _health(this)
    , _connection(this)
{
    qCDebug(NMEASourceManagerLog) << this;
    connect(&_connection, &GPSConnectionState::changed, this, &NMEASourceManager::stateChanged);
    _satellitePollTimer.setInterval(1000);
    connect(&_satellitePollTimer, &QTimer::timeout, this, [this]() {
        if (_satelliteSource) {
            // One-shot requests also report unchanged lists, unlike continuous Qt satellite updates.
            _satelliteSource->requestUpdate(5000);
        }
    });
    connect(&_health, &GPSSourceHealth::satellitesChanged, this, [this]() {
        if (_health.satellitesInViewCount() < 0) {
            _satellitesInView.clear();
        }
        if (_health.satellitesInUseCount() < 0) {
            _satellitesInUse.clear();
        }
        emit satellitesChanged();
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
                           _settings->nmeaUdpPort(), _settings->nmeaTcpHost(), _settings->nmeaTcpPort()}) {
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
    return _positionSource.get();
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

void NMEASourceManager::rememberReceiver(const QString& device, GPSType type)
{
    if (!device.isEmpty() && type == GPSType::u_blox) {
        _receiverTypes[device] = type;
    }
}

void NMEASourceManager::_prepareReceiver(const QString& device, GPSNMEAPreparation::TransportFactory factory)
{
    const quint64 generation = _preparationGeneration;
    _preparation = std::make_unique<GPSNMEAPreparation>(std::move(factory), _receiverTypes.value(device));
    connect(_preparation.get(), &QThread::finished, this, [this, generation, device]() {
        const unsigned baud = _preparation->baudrate();
        _preparation.reset();
        if (generation != _preparationGeneration || !_shouldConnect()) {
            return;
        }
        if (baud == 0) {
            _connection.failed();
            _setStatus(tr("Cannot configure receiver for NMEA"));
            return;
        }
        _receiverTypes.remove(device);
        // The driver may change a UART's baud rate while leaving base mode.
        _settings->autoConnectNmeaBaud()->setRawValue(static_cast<int>(baud));
        _connection.stopped();
        update();
    });
    const QPointer<NMEASourceManager> guard(this);
    _connection.configuring();
    if (!guard) {
        return;
    }
    _setStatus(tr("Configuring receiver for NMEA"));
    if (guard && _preparation) {
        _preparation->start();
    }
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
    stop();
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
    ++_preparationGeneration;
    if (_preparation) {
        _preparation->stop();
    }
    if (_sourceInstalled || _tcp) {
        _connection.stopping();
    }
    _udpActivityTimer.stop();
    _satellitePollTimer.stop();
    // Detach the decoder before destroying the device it reads from.
    if (_sourceInstalled && _positionManager) {
        _positionManager->clearNmeaPositionSource(_positionSource.get());
    }
    _sourceInstalled = false;
    _positionSource.reset();
    _satelliteSource.reset();
    _satelliteAdapter.reset();
    _stream.reset();
    _health.reset();
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
    _connection.stopped();
}

bool NMEASourceManager::_installSource(QIODevice* device)
{
    if (!_positionManager || !device || (!device->isOpen() && !device->open(QIODevice::ReadOnly)) ||
        !device->isReadable()) {
        _setStatus(tr("Cannot read NMEA source"));
        return false;
    }
    device->readAll();
    _stream = std::make_unique<NMEAStreamSplitter>(device);
    _satelliteSource = std::make_unique<QNmeaSatelliteInfoSource>(QNmeaSatelliteInfoSource::UpdateMode::RealTimeMode);
    _satelliteAdapter = std::make_unique<NMEASatelliteAdapter>(_stream->satelliteDevice());
    _satelliteSource->setDevice(_satelliteAdapter.get());
    const QPointer<QNmeaSatelliteInfoSource> current = _satelliteSource.get();
    connect(_satelliteSource.get(), &QGeoSatelliteInfoSource::satellitesInViewUpdated, this,
            [this, current](const QList<QGeoSatelliteInfo>& satellites) {
                QElapsedTimer received;
                received.start();
                QMetaObject::invokeMethod(
                    this,
                    [this, current, satellites, received]() {
                        if (!current || _satelliteSource.get() != current) {
                            return;
                        }
                        _satellitesInView = satellites;
                        _health.updateSatellitesInView(satellites.size(), received.elapsed());
                    },
                    Qt::QueuedConnection);
            });
    connect(_satelliteSource.get(), &QGeoSatelliteInfoSource::satellitesInUseUpdated, this,
            [this, current](const QList<QGeoSatelliteInfo>& satellites) {
                QElapsedTimer received;
                received.start();
                QMetaObject::invokeMethod(
                    this,
                    [this, current, satellites, received]() {
                        if (!current || _satelliteSource.get() != current) {
                            return;
                        }
                        _satellitesInUse = satellites;
                        _health.updateSatellitesInUse(satellites.size(), received.elapsed());
                    },
                    Qt::QueuedConnection);
            });
    connect(
        _satelliteSource.get(), &QGeoSatelliteInfoSource::errorOccurred, this,
        [this, current](QGeoSatelliteInfoSource::Error error) {
            if (current && _satelliteSource.get() == current && error != QGeoSatelliteInfoSource::NoError) {
                _clearSatelliteInfo();
            }
        },
        Qt::QueuedConnection);
    _satelliteSource->requestUpdate(5000);
    _satellitePollTimer.start();
    _positionSource = std::make_unique<NMEAPositionSource>(_stream->positionDevice());
    connect(_positionSource.get(), &QGeoPositionInfoSource::positionUpdated, &_health,
            [this](const QGeoPositionInfo& position) {
                _health.updatePosition(position, _positionSource->lastUpdateAgeMs());
            });
    connect(_positionSource.get(), &QGeoPositionInfoSource::errorOccurred, &_health,
            [this](QGeoPositionInfoSource::Error error) {
                if (error != QGeoPositionInfoSource::NoError) {
                    _health.invalidatePosition();
                }
            });
    _positionManager->setNmeaPositionSource(_positionSource.get(), &_health);
    _sourceInstalled = true;
    return true;
}

void NMEASourceManager::_clearSatelliteInfo()
{
    _health.clearSatellites();
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
        if (_serial || _preparation) {
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
        if (!_connection.beginAttempt()) {
            return;
        }
        if (_receiverTypes.contains(device)) {
            _prepareReceiver(device, [device, reservation = std::move(reservation)](const std::atomic_bool& stop) {
                return std::make_unique<SerialGPSTransport>(device, stop);
            });
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
