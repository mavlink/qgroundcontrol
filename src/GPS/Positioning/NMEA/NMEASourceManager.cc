#include "NMEASourceManager.h"

#include <utility>

#include <QtCore/QThread>

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
        connect(_positionManager, &QObject::destroyed, this, [this]() { _stop("position manager shutdown"); });
        if (_positionManager->scheduler()) {
            connect(_positionManager->scheduler(), &QObject::destroyed, this,
                    [this]() { _stop("scheduler destroyed"); });
        }
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
    const quint64 generation = ++_decoderGeneration;
    const QPointer<NMEASourceManager> guard(this);
    auto retired = std::exchange(_input, {});
    QObject::disconnect(retired.binding.closedConnection);
    QObject::disconnect(retired.binding.destroyedConnection);
    if (retired.binding.decoder) {
        retired.binding.decoder->disconnect(this);
        qCDebug(NMEASourceManagerLog) << "NMEA decoder retired:" << reason << "generation:" << generation;
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
    if (retired.binding.decoder && !_destroying) {
        emit sourceChanged();
        if (!guard || _revision != revision || _decoderGeneration != generation) {
            return;
        }
        emit activityChanged();
    }
    if (guard && _revision == revision && _decoderGeneration == generation && resetStatus) {
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
    if (guard && generation == _decoderGeneration && !_destroying) {
        emit sourceChanged();
        if (guard && generation == _decoderGeneration) {
            emit activityChanged();
        }
    }
}

void NMEASourceManager::_startDecoder(QIODevice* device)
{
    if (_destroying) {
        return;
    }
    if (!_positionManager || !_positionManager->scheduler() || QThread::currentThread() != thread() ||
        (device && device->thread() != thread())) {
        qCWarning(NMEASourceManagerLog) << "NMEA device requires matching thread affinity and a live scheduler";
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
        emit sourceChanged();
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
