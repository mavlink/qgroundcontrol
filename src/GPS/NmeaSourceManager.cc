#include "NmeaSourceManager.h"

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

QGC_LOGGING_CATEGORY(NmeaSourceManagerLog, "GPS.NmeaSourceManager")

NmeaSourceManager::NmeaSourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager,
                                     QObject* parent)
    : QObject(parent), _settings(settings), _positionManager(positionManager)
{
#ifndef QGC_NO_SERIAL_LINK
    if (_settings) {
        connect(_settings->nmeaSource(), &Fact::rawValueChanged, this, &NmeaSourceManager::_updateSerialRouting);
        connect(_settings->autoConnectNmeaPort(), &Fact::rawValueChanged, this,
                &NmeaSourceManager::_updateSerialRouting);
        _updateSerialRouting();
    }
#endif
}

#ifndef QGC_NO_SERIAL_LINK
void NmeaSourceManager::_updateSerialRouting()
{
    const QString port = _settings->nmeaSource()->rawValue().toInt() == AutoConnectSettings::NmeaSourceSerial
                             ? _settings->autoConnectNmeaPort()->rawValue().toString().trimmed()
                             : QString();
    _autoConnectExclusion = SerialPortManager::instance()->excludeFromAutoConnect(port);
}
#endif

NmeaSourceManager::~NmeaSourceManager()
{
    stop();
}

void NmeaSourceManager::stop()
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

void NmeaSourceManager::update()
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
        if (_udp && _udp->state() == QAbstractSocket::BoundState && _udp->localPort() == port) {
            return;
        }
        stop();
        _source = source;
        auto socket = std::make_unique<UdpIODevice>();
        if (!socket->bind(QHostAddress::AnyIPv4, port)) {
            qCDebug(NmeaSourceManagerLog) << "Cannot bind NMEA UDP port" << port << socket->errorString();
            return;
        }
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
            qCDebug(NmeaSourceManagerLog) << "Cannot open NMEA serial port" << device << serial->errorString();
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
