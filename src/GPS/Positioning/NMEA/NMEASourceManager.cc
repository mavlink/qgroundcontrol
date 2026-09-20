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

void NMEASourceManager::_stop(const char* reason)
{
    ++_revision;
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
        _stop(reason);
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
        if (!present || _serial) {
            return;
        }
        auto reservation = ports->reservePort(device);
        if (!reservation) {
            return;
        }
        auto serial = std::make_unique<QSerialPort>();
        serial->setPortName(device);
        if (!serial->setBaudRate(baud) || !serial->open(QIODevice::ReadOnly)) {
            qCDebug(NMEASourceManagerLog)
                << "Cannot open NMEA serial port" << device << "baud:" << baud << serial->errorString();
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
                    _stop("serial error");
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
        }
    }
#endif
}
