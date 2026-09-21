#include "NMEASourceManager.h"

#include <utility>

#include "AutoConnectSettings.h"
#include "GPSSourceHealth.h"
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
                           _settings->autoConnectNmeaBaud()}) {
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
    _stop("shutdown");
}

void NMEASourceManager::stop()
{
    _stop("stop requested");
}

void NMEASourceManager::_stop(const char* reason, bool resetStatus)
{
    const quint64 revision = ++_revision;
    const QPointer<NMEASourceManager> guard(this);
    const auto positionManager = _positionManager;
    const bool installed = std::exchange(_sourceInstalled, false);
    _source = -1;
    auto udp = std::move(_udp);
    QIODevice* device = udp.get();
#ifndef QGC_NO_SERIAL_LINK
    auto reservation = std::move(_reservation);
    auto serial = std::move(_serial);
    if (serial) {
        device = serial.get();
    }
    const auto serialDevice = std::exchange(_serialDevice, {});
    const auto serialBaud = std::exchange(_serialBaud, 0);
#endif
    if (udp) {
        qCDebug(NMEASourceManagerLog) << "NMEA input retired:"
                                      << "reason:" << reason << "source: UDP"
                                      << "port:" << udp->localPort() << "peer:" << udp->selectedPeer();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (serial) {
        qCDebug(NMEASourceManagerLog) << "NMEA input retired:"
                                      << "reason:" << reason << "source: serial"
                                      << "port:" << serialDevice << "baud:" << serialBaud;
    }
#endif
    // Keep only the retiring devices alive while the decoder notifies observers.
    if (installed && positionManager) {
        positionManager->resetNmeaSourceDevice(device);
    }
    if (guard && _revision == revision && resetStatus) {
        _setConnectionState(ConnectionState::Disabled);
    }
}

QString NMEASourceManager::connectionStatusText() const
{
    switch (_connectionState) {
        case ConnectionState::Disabled:
            return tr("NMEA input is disabled");
        case ConnectionState::WaitingForDevice:
            return tr("Waiting for the selected NMEA serial device");
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
    if (!_destroying) {
        emit connectionStateChanged();
    }
}

void NMEASourceManager::update()
{
    if (_destroying) {
        return;
    }
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
    if (_source != source) {
        if (!retire(source == AutoConnectSettings::NmeaSourceDisabled ? "source disabled" : "source setting changed")) {
            return;
        }
        _source = source;
    }
    if (source == AutoConnectSettings::NmeaSourceDisabled) {
        _setConnectionState(ConnectionState::Disabled);
        return;
    }
    if (source == AutoConnectSettings::NmeaSourceUdp) {
        const quint16 port = _settings->nmeaUdpPort()->rawValue().toUInt();
        if (_udp && _udp->isOpen() && _udp->localPort() == port) {
            return;
        }
        if (!retire(_udp && !_udp->isOpen() ? "UDP device closed" : "UDP port setting changed")) {
            return;
        }
        _source = source;
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
                if (current && _udp.get() == current && _positionManager &&
                    _positionManager->nmeaSourceDevice() == current) {
                    qCDebug(NMEASourceManagerLog)
                        << "NMEA session restart:"
                        << "reason: UDP peer replaced"
                        << "port:" << current->localPort() << "previousPeer:" << previousPeer << "peer:" << peer;
                    // Retire partial sentences, Qt epoch state, and satellite assembly together.
                    ++_revision;
                    _positionManager->setNmeaSourceDevice(current);
                }
            });
        socket->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        _udp = std::move(socket);
        qCDebug(NMEASourceManagerLog) << "NMEA input started:"
                                      << "source: UDP"
                                      << "port:" << _udp->localPort();
        _sourceInstalled = true;
        _positionManager->setNmeaSourceDevice(_udp.get());
        if (current()) {
            _sourceInstalled = _positionManager->nmeaSourceDevice() == _udp.get();
            _setConnectionState(_sourceInstalled ? ConnectionState::Connected : ConnectionState::Error,
                                _sourceInstalled ? QString() : tr("The NMEA decoder could not be started."));
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
        if (!present || device != _serialDevice || baud != _serialBaud) {
            const char* reason = "serial port missing";
            if (device != _serialDevice) {
                reason = "serial port setting changed";
            } else if (baud != _serialBaud) {
                reason = "serial baud setting changed";
            }
            if (!retire(reason)) {
                return;
            }
            _source = source;
        }
        if (!present) {
            _setConnectionState(ConnectionState::WaitingForDevice);
            return;
        }
        if (_serial) {
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
        _serialDevice = device;
        _serialBaud = baud;
        _reservation = std::move(reservation);
        _serial = std::move(serial);
        connect(
            _serial.get(), &QSerialPort::errorOccurred, this,
            [this, current = QPointer<QSerialPort>(_serial.get())](QSerialPort::SerialPortError error) {
                if (current && _serial.get() == current && error != QSerialPort::NoError &&
                    error != QSerialPort::TimeoutError) {
                    qCDebug(NMEASourceManagerLog) << "NMEA serial error:"
                                                  << "port:" << _serialDevice << "baud:" << _serialBaud
                                                  << "error:" << error << current->errorString();
                    const QString message =
                        tr("NMEA serial device %1 failed: %2").arg(_serialDevice, current->errorString());
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
                                      << "port:" << _serialDevice << "baud:" << _serialBaud;
        _sourceInstalled = true;
        _positionManager->setNmeaSourceDevice(_serial.get());
        if (current()) {
            _sourceInstalled = _positionManager->nmeaSourceDevice() == _serial.get();
            _setConnectionState(_sourceInstalled ? ConnectionState::Connected : ConnectionState::Error,
                                _sourceInstalled ? QString() : tr("The NMEA decoder could not be started."));
        }
    }
#else
    if (source == AutoConnectSettings::NmeaSourceSerial) {
        _setConnectionState(ConnectionState::Error, tr("Serial NMEA input is unavailable in this build."));
    }
#endif
}
