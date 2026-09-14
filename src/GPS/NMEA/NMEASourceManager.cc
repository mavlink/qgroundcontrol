#include "NMEASourceManager.h"

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
    : QObject(parent), _settings(settings), _positionManager(positionManager)
{
    qCDebug(NMEASourceManagerLog) << this;
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
    const QString port = _settings->nmeaSource()->rawValue().toInt() == AutoConnectSettings::NmeaSourceSerial
                             ? _settings->autoConnectNmeaPort()->rawValue().toString().trimmed()
                             : QString();
    _autoConnectExclusion = SerialPortManager::instance()->excludeFromAutoConnect(port);
}
#endif

NMEASourceManager::~NMEASourceManager()
{
    qCDebug(NMEASourceManagerLog) << this;
    stop();
}

void NMEASourceManager::stop()
{
    // Detach the decoder before destroying the device it reads from.
    if (_sourceInstalled && _positionManager) {
        _positionManager->resetNmeaSourceDevice();
    }
    _sourceInstalled = false;
    _udp.reset();
#ifndef QGC_NO_SERIAL_LINK
    _serial.reset();
    _reservation.reset();
    _serialDevice.clear();
    _serialBaud = 0;
#endif
    _source = -1;
}

void NMEASourceManager::update()
{
    if (!_settings || !_positionManager) {
        stop();
        return;
    }
    const int source = _settings->nmeaSource()->rawValue().toInt();
    if (_source != source) {
        stop();
        _source = source;
    }
    if (source == AutoConnectSettings::NmeaSourceUdp) {
        const quint16 port = _settings->nmeaUdpPort()->rawValue().toUInt();
        if (_udp && _udp->isOpen() && _udp->localPort() == port) {
            return;
        }
        stop();
        _source = source;
        auto socket = std::make_unique<UdpIODevice>();
        if (!socket->bind(QHostAddress::AnyIPv4, port)) {
            qCDebug(NMEASourceManagerLog) << "Cannot bind NMEA UDP port" << port << socket->errorString();
            return;
        }
        socket->setSelectFirstPeer(true);
        socket->setPeerIdleTimeout(std::chrono::milliseconds(GPSSourceHealth::FRESHNESS_TIMEOUT_MS));
        connect(socket.get(), &UdpIODevice::peerReplaced, this,
                [this, current = QPointer<UdpIODevice>(socket.get())]() {
                    if (current && _udp.get() == current && _positionManager) {
                        // Retire partial sentences, Qt epoch state, and satellite assembly together.
                        _positionManager->setNmeaSourceDevice(current);
                    }
                });
        socket->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        _udp = std::move(socket);
        _positionManager->setNmeaSourceDevice(_udp.get());
        _sourceInstalled = true;
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
            stop();
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
            qCDebug(NMEASourceManagerLog) << "Cannot open NMEA serial port" << device << serial->errorString();
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
                    stop();
                }
            },
            Qt::QueuedConnection);
        _positionManager->setNmeaSourceDevice(_serial.get());
        _sourceInstalled = true;
    }
#endif
}
