#include "NMEASourceManager.h"

#include <utility>

#include <QtCore/QThread>
#include <QtNetwork/QTcpSocket>

#include "AutoConnectSettings.h"
#include "GPSSourceHealth.h"
#include "NMEADecoderSession.h"
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

QGC_LOGGING_CATEGORY(NMEASourceManagerLog, "GPS.NMEA.NMEASourceManager")

NMEASourceManager::NMEASourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager,
                                     QObject* parent)
    : QObject(parent)
    , _settings(settings)
    , _positionManager(positionManager)
{
    qCDebug(NMEASourceManagerLog) << this;
    if (_settings) {
        for (auto* fact : {_settings->nmeaSource(), _settings->nmeaUdpPort(), _settings->autoConnectNmeaPort(),
                           _settings->autoConnectNmeaBaud(), _settings->nmeaTcpHost(), _settings->nmeaTcpPort()}) {
            connect(fact, &Fact::rawValueChanged, this, [this]() { ++_revision; });
        }
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_settings) {
        connect(_settings->nmeaSource(), &Fact::rawValueChanged, this, &NMEASourceManager::_updateSerialRouting);
        connect(_settings->autoConnectNmeaPort(), &Fact::rawValueChanged, this,
                &NMEASourceManager::_updateSerialRouting);
        _updateSerialRouting();
    }
#endif
    if (_positionManager) {
        connect(_positionManager, &QObject::destroyed, this, [this]() { _stop("position manager shutdown"); });
        _positionManager->setNmeaInput(this);
    }
}

#ifndef QGC_NO_SERIAL_LINK
void NMEASourceManager::_updateSerialRouting()
{
    if (_destroying || !_settings) {
        return;
    }
    const QString port = _settings->nmeaSource()->rawValue().toInt() == AutoConnectSettings::NmeaSourceSerial
                             ? _settings->autoConnectNmeaPort()->rawValue().toString().trimmed()
                             : QString();
    _autoConnectExclusion = SerialPortManager::instance()->excludeFromAutoConnect(port);
}
#endif

NMEASourceManager::~NMEASourceManager()
{
    qCDebug(NMEASourceManagerLog) << "NMEA source manager shutdown:" << this;
    _destroying = true;
    _notifications.close();
    _stop("shutdown");
}

void NMEASourceManager::stop()
{
    _stop("stop requested");
}

void NMEASourceManager::_stop(const char* reason, bool resetStatus)
{
    const GPSNotificationQueue::Scope publish(_notifications);
    const quint64 revision = ++_revision;
    const quint64 generation = ++_decoderGeneration;
    const QPointer<NMEASourceManager> guard(this);
    auto retired = std::exchange(_input, {});
    QObject::disconnect(retired.binding.closedConnection);
    QObject::disconnect(retired.binding.destroyedConnection);
    if (retired.binding.decoder) {
        retired.binding.decoder->disconnect(this);
        qCDebug(NMEASourceManagerLog) << "NMEA decoder retired:" << reason << "generation:" << generation;
    }
    if (retired.tcp) {
        qCDebug(NMEASourceManagerLog) << "NMEA input retired:"
                                      << "reason:" << reason << "source: TCP"
                                      << "server:" << retired.tcpHost << "port:" << retired.tcpPort;
        retired.tcp->disconnect(this);
        retired.tcp->abort();
    }
    if (retired.udp) {
        qCDebug(NMEASourceManagerLog) << "NMEA input retired:"
                                      << "reason:" << reason << "source: UDP"
                                      << "port:" << retired.udp->localPort() << "peer:" << retired.udp->selectedPeer();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (retired.serial) {
        qCDebug(NMEASourceManagerLog) << "NMEA input retired:"
                                      << "reason:" << reason << "source: serial"
                                      << "port:" << retired.serialDevice << "baud:" << retired.serialBaud;
    }
#endif
    // Registration callbacks may replace or destroy this owner; retired resources remain local.
    retired.binding.registration.reset();
    if (!guard || _revision != revision || _decoderGeneration != generation) {
        return;
    }
    if (retired.binding.decoder) {
        _notifications.emitSignal(this, &NMEASourceManager::sourceChanged);
        _notifications.emitSignal(this, &NMEASourceManager::activityChanged);
    }
    if (resetStatus) {
        _setConnectionState(ConnectionState::Disabled);
    }
}

void NMEASourceManager::_retireDecoder(const char* reason)
{
    const quint64 generation = ++_decoderGeneration;
    auto retired = std::exchange(_input.binding, {});
    QObject::disconnect(retired.closedConnection);
    QObject::disconnect(retired.destroyedConnection);
    if (!retired.decoder) {
        return;
    }
    retired.decoder->disconnect(this);
    qCDebug(NMEASourceManagerLog) << "NMEA decoder retired:" << reason << "generation:" << generation;
    const QPointer<NMEASourceManager> guard(this);
    retired.registration.reset();
    if (guard && generation == _decoderGeneration) {
        _notifications.emitSignal(this, &NMEASourceManager::sourceChanged);
        _notifications.emitSignal(this, &NMEASourceManager::activityChanged);
    }
}

void NMEASourceManager::_startDecoder(QIODevice* device)
{
    if (_destroying) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    if (!_positionManager || QThread::currentThread() != thread() || (device && device->thread() != thread())) {
        qCWarning(NMEASourceManagerLog) << "NMEA device requires matching thread affinity";
        return;
    }
    const QPointer<NMEASourceManager> guard(this);
    const QPointer<QIODevice> deviceGuard(device);
    const quint64 revision = _revision;
    const quint64 generation = _decoderGeneration + 1;
    _retireDecoder(device ? "device replacement" : "device cleared");
    if (!guard || generation != _decoderGeneration || revision != _revision || !deviceGuard) {
        return;
    }
    auto decoder = std::make_unique<NMEADecoderSession>(nullptr, _positionManager->scheduler());
    if (!decoder->start(deviceGuard)) {
        qCWarning(NMEASourceManagerLog) << "NMEA decoder could not be started";
        return;
    }
    _input.binding.device = deviceGuard;
    _input.binding.decoder = std::move(decoder);
    connect(_input.binding.decoder.get(), &NMEADecoderSession::activityChanged, this,
            &NMEASourceManager::activityChanged);
    _input.binding.closedConnection = connect(
        deviceGuard, &QIODevice::aboutToClose, this,
        [this, generation]() {
            if (generation == _decoderGeneration) {
                _stop("device closed");
            }
        },
        Qt::QueuedConnection);
    _input.binding.destroyedConnection = connect(deviceGuard, &QObject::destroyed, this, [this, generation]() {
        if (generation == _decoderGeneration) {
            _stop("device destroyed");
        }
    });
    auto registration = _positionManager->registerPositionSource(GPSPositionService::SelectedSource::Nmea,
                                                                 _input.binding.decoder->positionSource(), health());
    if (guard && generation == _decoderGeneration) {
        _input.binding.registration = std::move(registration);
        qCDebug(NMEASourceManagerLog) << "NMEA decoder installed:" << "generation:" << generation
                                      << "registered:" << bool(_input.binding.registration);
        _notifications.emitSignal(this, &NMEASourceManager::sourceChanged);
    }
}

GPSSourceHealth* NMEASourceManager::health() const
{
    return _input.binding.decoder ? _input.binding.decoder->health() : nullptr;
}

bool NMEASourceManager::receiving() const
{
    return _input.binding.decoder && _input.binding.decoder->receiving();
}

bool NMEASourceManager::hasData() const
{
    return _input.binding.decoder && _input.binding.decoder->hasReceivedData();
}

QString NMEASourceManager::connectionStatusText() const
{
    switch (_connectionState) {
        case ConnectionState::Disabled:
            return tr("NMEA input is disabled");
        case ConnectionState::WaitingForDevice:
            return _input.source == AutoConnectSettings::NmeaSourceTcp
                       ? tr("Connecting to NMEA TCP server %1:%2").arg(_input.tcpHost).arg(_input.tcpPort)
                       : tr("Waiting for the selected NMEA serial device");
        case ConnectionState::Connected:
            return tr("NMEA input is open");
        case ConnectionState::Error:
            return _errorMessage;
    }
    return {};
}

void NMEASourceManager::_setConnectionState(ConnectionState state, const QString& error)
{
    if (_connectionState == state && _errorMessage == error) {
        return;
    }
    _connectionState = state;
    _errorMessage = error;
    _notifications.emitSignal(this, &NMEASourceManager::connectionStateChanged);
}

void NMEASourceManager::update()
{
    if (_destroying) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    const QPointer<NMEASourceManager> guard(this);
    quint64 revision = ++_revision;
    const auto current = [guard, &revision]() {
        return guard && !guard->_destroying && guard->_revision == revision && guard->_settings &&
               guard->_positionManager;
    };
    const auto retire = [this, &revision, &current](const char* reason) {
        ++revision;
        _stop(reason, false);
        return current();
    };
    if (!_settings || !_positionManager) {
        _stop(!_settings ? "settings unavailable" : "position manager unavailable");
        return;
    }
    const int source = _settings->nmeaSource()->rawValue().toInt();
    if (_input.source != source) {
        if (!retire(source == AutoConnectSettings::NmeaSourceDisabled ? "source disabled" : "source setting changed")) {
            return;
        }
        _input.source = source;
    }
    if (source == AutoConnectSettings::NmeaSourceDisabled) {
        _setConnectionState(ConnectionState::Disabled);
        return;
    }
    if (source == AutoConnectSettings::NmeaSourceUdp) {
        const quint16 port = _settings->nmeaUdpPort()->rawValue().toUInt();
        if (_input.udp && _input.udp->isOpen() && _input.udp->localPort() == port) {
            return;
        }
        if (!retire(_input.udp && !_input.udp->isOpen() ? "UDP device closed" : "UDP port setting changed")) {
            return;
        }
        _input.source = source;
        auto socket = std::make_unique<UdpIODevice>();
        if (!socket->bind(QHostAddress::AnyIPv4, port)) {
            qCDebug(NMEASourceManagerLog) << "Cannot bind NMEA UDP port" << port << socket->errorString();
            _setConnectionState(ConnectionState::Error,
                                tr("Cannot listen on NMEA UDP port %1: %2").arg(port).arg(socket->errorString()));
            return;
        }
        socket->setSelectFirstPeer(true);
        socket->setPeerIdleTimeout(std::chrono::milliseconds(GPSSourceHealth::FRESHNESS_TIMEOUT_MS));
        connect(
            socket.get(), &UdpIODevice::peerReplaced, this,
            [this, current = QPointer<UdpIODevice>(socket.get())](const QString& previousPeer, const QString& peer) {
                if (current && _input.udp.get() == current && _positionManager && _input.binding.device == current) {
                    qCDebug(NMEASourceManagerLog)
                        << "NMEA session restart:"
                        << "reason: UDP peer replaced"
                        << "port:" << current->localPort() << "previousPeer:" << previousPeer << "peer:" << peer;
                    // Retire partial sentences, Qt epoch state, and satellite assembly together.
                    ++_revision;
                    _startDecoder(current);
                }
            });
        socket->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        _input.udp = std::move(socket);
        qCDebug(NMEASourceManagerLog) << "NMEA input started:"
                                      << "source: UDP"
                                      << "port:" << _input.udp->localPort();
        _startDecoder(_input.udp.get());
        if (current()) {
            const bool installed = bool(_input.binding.registration);
            _setConnectionState(installed ? ConnectionState::Connected : ConnectionState::Error,
                                installed ? QString() : tr("The NMEA decoder could not be started."));
        }
        return;
    }
    if (source == AutoConnectSettings::NmeaSourceTcp) {
        const QString host = _settings->nmeaTcpHost()->rawValue().toString().trimmed();
        const uint port = _settings->nmeaTcpPort()->rawValue().toUInt();
        if (_input.tcp && (_input.tcpHost != host || _input.tcpPort != port)) {
            if (!retire("TCP server setting changed")) {
                return;
            }
            _input.source = source;
        }
        if (host.isEmpty() || port == 0 || port > 65535) {
            _setConnectionState(ConnectionState::Error, tr("Enter the NMEA TCP server host and port."));
            return;
        }
        if (_input.tcp) {
            const auto state = _input.tcp->state();
            const bool connecting =
                state == QAbstractSocket::HostLookupState || state == QAbstractSocket::ConnectingState;
            if (state == QAbstractSocket::ConnectedState ||
                (connecting && !_input.tcpConnecting.hasExpired(kTcpConnectTimeoutMs))) {
                return;
            }
            const QString message = connecting ? tr("Timed out connecting to NMEA TCP server %1:%2").arg(host).arg(port)
                                               : tr("NMEA TCP connection to %1:%2 closed").arg(host).arg(port);
            if (!retire(connecting ? "TCP connection timed out" : "TCP connection closed")) {
                return;
            }
            _input.source = source;
            // The next update retries.
            _setConnectionState(ConnectionState::Error, message);
            return;
        }
        auto socket = std::make_unique<QTcpSocket>();
        const QPointer<QTcpSocket> socketGuard(socket.get());
        connect(socket.get(), &QTcpSocket::connected, this, [this, socketGuard]() {
            if (!socketGuard || _input.tcp.get() != socketGuard) {
                return;
            }
            qCDebug(NMEASourceManagerLog) << "NMEA input started:"
                                          << "source: TCP"
                                          << "server:" << _input.tcpHost << "port:" << _input.tcpPort;
            const QPointer<NMEASourceManager> connectedGuard(this);
            _startDecoder(socketGuard);
            if (connectedGuard && socketGuard && _input.tcp.get() == socketGuard) {
                const bool installed = bool(_input.binding.registration);
                _setConnectionState(installed ? ConnectionState::Connected : ConnectionState::Error,
                                    installed ? QString() : tr("The NMEA decoder could not be started."));
            }
        });
        connect(
            socket.get(), &QTcpSocket::errorOccurred, this,
            [this, socketGuard](QAbstractSocket::SocketError error) {
                if (!socketGuard || _input.tcp.get() != socketGuard) {
                    return;
                }
                qCDebug(NMEASourceManagerLog) << "NMEA TCP error:"
                                              << "server:" << _input.tcpHost << "port:" << _input.tcpPort
                                              << "error:" << error << socketGuard->errorString();
                const QString message = tr("NMEA TCP connection to %1:%2 failed: %3")
                                            .arg(_input.tcpHost)
                                            .arg(_input.tcpPort)
                                            .arg(socketGuard->errorString());
                const QPointer<NMEASourceManager> errorGuard(this);
                const quint64 errorRevision = _revision + 1;
                _stop("TCP error", false);
                if (errorGuard && _revision == errorRevision) {
                    _setConnectionState(ConnectionState::Error, message);
                }
            },
            Qt::QueuedConnection);
        _input.tcpHost = host;
        _input.tcpPort = static_cast<quint16>(port);
        _input.tcpConnecting.start();
        _input.tcp = std::move(socket);
        _setConnectionState(ConnectionState::WaitingForDevice);
        if (current() && _input.tcp) {
            _input.tcp->connectToHost(host, static_cast<quint16>(port), QIODevice::ReadOnly);
        }
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    if (source == AutoConnectSettings::NmeaSourceSerial) {
        const QString device = _settings->autoConnectNmeaPort()->rawValue().toString().trimmed();
        const qint32 baud = _settings->autoConnectNmeaBaud()->rawValue().toInt();
        auto* ports = SerialPortManager::instance();
        bool present = false;
        const auto availablePorts = ports->availablePorts();
        if (!current()) {
            return;
        }
        for (const auto& port : availablePorts) {
            if (port.systemLocation == device) {
                present = true;
                break;
            }
        }
        if (!present || device != _input.serialDevice || baud != _input.serialBaud) {
            const char* reason = "serial port missing";
            if (device != _input.serialDevice) {
                reason = "serial port setting changed";
            } else if (baud != _input.serialBaud) {
                reason = "serial baud setting changed";
            }
            if (!retire(reason)) {
                return;
            }
            _input.source = source;
        }
        if (!present) {
            _setConnectionState(ConnectionState::WaitingForDevice);
            return;
        }
        if (_input.serial) {
            return;
        }
        auto reservation = ports->reservePort(device);
        if (!reservation) {
            _setConnectionState(ConnectionState::Error, tr("The NMEA serial device %1 is already in use.").arg(device));
            return;
        }
        auto serial = std::make_unique<QSerialPort>();
        serial->setPortName(device);
        if (!serial->setBaudRate(baud) || !serial->open(QIODevice::ReadOnly)) {
            qCDebug(NMEASourceManagerLog)
                << "Cannot open NMEA serial port" << device << "baud:" << baud << serial->errorString();
            _setConnectionState(ConnectionState::Error, tr("Cannot open NMEA serial device %1 at %2 baud: %3")
                                                            .arg(device)
                                                            .arg(baud)
                                                            .arg(serial->errorString()));
            return;
        }
        _input.serialDevice = device;
        _input.serialBaud = baud;
        _input.reservation = std::move(reservation);
        _input.serial = std::move(serial);
        connect(
            _input.serial.get(), &QSerialPort::errorOccurred, this,
            [this, current = QPointer<QSerialPort>(_input.serial.get())](QSerialPort::SerialPortError error) {
                if (current && _input.serial.get() == current && error != QSerialPort::NoError &&
                    error != QSerialPort::TimeoutError) {
                    qCDebug(NMEASourceManagerLog) << "NMEA serial error:"
                                                  << "port:" << _input.serialDevice << "baud:" << _input.serialBaud
                                                  << "error:" << error << current->errorString();
                    const QString message =
                        tr("NMEA serial device %1 failed: %2").arg(_input.serialDevice, current->errorString());
                    const QPointer<NMEASourceManager> errorGuard(this);
                    const quint64 errorRevision = _revision + 1;
                    _stop("serial error", false);
                    if (errorGuard && _revision == errorRevision) {
                        _setConnectionState(ConnectionState::Error, message);
                    }
                }
            },
            Qt::QueuedConnection);
        qCDebug(NMEASourceManagerLog) << "NMEA input started:"
                                      << "source: serial"
                                      << "port:" << _input.serialDevice << "baud:" << _input.serialBaud;
        _startDecoder(_input.serial.get());
        if (current()) {
            const bool installed = bool(_input.binding.registration);
            _setConnectionState(installed ? ConnectionState::Connected : ConnectionState::Error,
                                installed ? QString() : tr("The NMEA decoder could not be started."));
        }
    }
#else
    if (source == AutoConnectSettings::NmeaSourceSerial) {
        _setConnectionState(ConnectionState::Error, tr("Serial NMEA input is unavailable in this build."));
    }
#endif
}
