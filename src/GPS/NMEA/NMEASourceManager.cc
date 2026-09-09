#include "NMEASourceManager.h"

#include <QtCore/QIODevice>

#include <utility>

#include "AutoConnectSettings.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEASourceManagerLog, "GPS.NMEA.NMEASourceManager")

NMEASourceManager::NMEASourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager,
                                     QObject* parent)
    : QObject(parent)
    , _settings(settings)
    , _positionManager(positionManager)
    , _decoder(this)
    , _connection(this)
{
    qCDebug(NMEASourceManagerLog) << this;
#ifndef QGC_NO_SERIAL_LINK
    _serialPorts = SerialPortManager::instance();
#endif
    connect(&_connection, &GPSConnectionState::changed, this, &NMEASourceManager::_notifyState);
    connect(&_decoder, &NMEADecoderSession::satellitesChanged, this, &NMEASourceManager::satellitesChanged);
    _status = tr("Disconnected");
    _udpActivityTimer.setSingleShot(true);
    _udpActivityTimer.setInterval(5000);
    connect(&_udpActivityTimer, &QTimer::timeout, this, [this]() {
        _dispatch([this]() {
            if (_attempt && !_attempt->stopping() && _config.source == NMEAConnectionConfig::Udp) {
                _setStatus(tr("Listening on UDP port %1").arg(_attempt->localPort()));
            }
        });
    });
    if (_settings) {
        _config = NMEAConnectionConfig::fromSettings(*_settings);
        for (Fact* fact : {_settings->nmeaSource(), _settings->autoConnectNmeaPort(), _settings->autoConnectNmeaBaud(),
                           _settings->nmeaUdpPort(), _settings->nmeaTcpHost(), _settings->nmeaTcpPort(),
                           _settings->nmeaReceiverMode()}) {
            connect(fact, &Fact::rawValueChanged, this, &NMEASourceManager::_settingsChanged);
        }
        connect(_settings->nmeaAutoConnect(), &Fact::rawValueChanged, this, [this]() {
            _dispatch([this]() {
                const QPointer<NMEASourceManager> guard(this);
                _closeDevice();
                if (!guard) {
                    return;
                }
                _connection.resetIntent();
                _updateSerialRouting();
                if (!_shouldConnect()) {
                    _stop();
                }
            });
        });
        _updateSerialRouting();
    }
}

NMEASourceManager::~NMEASourceManager()
{
    qCDebug(NMEASourceManagerLog) << this;
    disconnect(this, nullptr, nullptr, nullptr);
    _shutdown = true;
    _connection.disconnect(this);
    _decoder.disconnect(this);
    _udpActivityTimer.stop();
    _uninstallSource();
    if (_attempt) {
        _attempt->disconnect(this);
        _attempt->shutdown();
    }
}

void NMEASourceManager::_dispatch(std::function<void()> command)
{
    _commands.push_back(std::move(command));
    if (_dispatching) {
        return;
    }
    _dispatching = true;
    const QPointer<NMEASourceManager> guard(this);
    while (!_commands.empty() || _stateNotificationPending) {
        if (!_commands.empty()) {
            auto next = std::move(_commands.front());
            _commands.pop_front();
            next();
            if (!guard) {
                return;
            }
        } else {
            _stateNotificationPending = false;
            emit stateChanged();
            if (!guard) {
                return;
            }
        }
    }
    _dispatching = false;
}

void NMEASourceManager::_notifyState()
{
    _stateNotificationPending = true;
    if (!_dispatching) {
        _dispatch([]() {});
    }
}

QGeoPositionInfoSource* NMEASourceManager::positionSource() const
{
    return _decoder.positionSource();
}

bool NMEASourceManager::_shouldConnect() const
{
    return !_shutdown && _settings && _connection.shouldConnect(_settings->nmeaAutoConnect()->rawValue().toBool()) &&
           _config.source != NMEAConnectionConfig::Disabled;
}

void NMEASourceManager::_updateSerialRouting()
{
#ifndef QGC_NO_SERIAL_LINK
    const QString port =
        _shouldConnect() && _config.source == NMEAConnectionConfig::Serial ? _config.device : QString();
    _autoConnectExclusion = _serialPorts ? _serialPorts->excludeFromAutoConnect(port) : nullptr;
#endif
}

void NMEASourceManager::_settingsChanged()
{
    _dispatch([this]() {
        const QPointer<NMEASourceManager> guard(this);
        const auto config = NMEAConnectionConfig::fromSettings(*_settings);
        if (config != _config) {
            _config = config;
            _receiverFactory = {};
            _closeDevice();
            if (!guard) {
                return;
            }
            _connection.resetRetry();
        }
        _updateSerialRouting();
        if (!_shouldConnect()) {
            _stop();
        }
    });
}

bool NMEASourceManager::connectSource()
{
    if (_shutdown || !_settings || !_positionManager || _config.source == NMEAConnectionConfig::Disabled) {
        return false;
    }
    const QPointer<NMEASourceManager> guard(this);
    _dispatch([this]() {
        _connection.requestConnect();
        _updateSerialRouting();
        _update();
    });
    return guard && active();
}

void NMEASourceManager::disconnectSource()
{
    _dispatch([this]() {
        const QPointer<NMEASourceManager> guard(this);
        _connection.pause();
        _stop();
        if (guard) {
            _updateSerialRouting();
        }
    });
}

void NMEASourceManager::_setStatus(const QString& status)
{
    if (_status != status) {
        _status = status;
        qCDebug(NMEASourceManagerLog) << "Connection status:" << _status;
        _notifyState();
    }
}

void NMEASourceManager::shutdown()
{
    _dispatch([this]() {
        const QPointer<NMEASourceManager> guard(this);
        _shutdown = true;
        _stop();
        if (!guard) {
            return;
        }
        if (_attempt) {
            _attempt->shutdown();
        }
        _updateSerialRouting();
    });
}

void NMEASourceManager::stop()
{
    _dispatch([this]() { _stop(); });
}

void NMEASourceManager::_stop()
{
    const QPointer<NMEASourceManager> guard(this);
    _connection.stop();
    _receiverFactory = {};
    _closeDevice();
    if (!guard) {
        return;
    }
    _connection.resetRetry();
    _setStatus(_connection.paused() && _settings && _settings->nmeaAutoConnect()->rawValue().toBool()
                   ? tr("Automatic connection paused")
                   : tr("Disconnected"));
}

void NMEASourceManager::_closeDevice()
{
    const QPointer<NMEASourceManager> guard(this);
    _udpActivityTimer.stop();
    _uninstallSource();
    if (!guard) {
        return;
    }
    if (_attempt && !_attempt->stopping()) {
        _connection.stopping();
        _attempt->stop();
    } else if (!_attempt) {
        _connection.stopped();
    }
}

void NMEASourceManager::_uninstallSource()
{
    const QPointer<NMEASourceManager> guard(this);
    const bool installed = std::exchange(_sourceInstalled, false);
    if (installed && _positionManager) {
        _positionManager->clearNmeaPositionSource(_decoder.positionSource());
    }
    if (guard) {
        _decoder.stop();
    }
}

bool NMEASourceManager::_installSource(QIODevice* device)
{
    if (!_positionManager || !device || (!device->isOpen() && !device->open(QIODevice::ReadOnly)) ||
        !device->isReadable()) {
        _setStatus(tr("Cannot read NMEA source"));
        return false;
    }
    const QPointer<NMEASourceManager> guard(this);
    device->readAll();
    if (!_decoder.start(device) || !guard) {
        return false;
    }
    _sourceInstalled = true;
    _positionManager->setNmeaPositionSource(_decoder.positionSource(), _decoder.health());
    return guard;
}

void NMEASourceManager::_attemptFailed(const QString& detail)
{
    const QPointer<NMEASourceManager> guard(this);
    const auto failedConfig = _config;
    _closeDevice();
    if (!guard) {
        return;
    }
    // Retry starts after the stopped event, once the attempt has released its endpoint.
    _commands.push_back([this, detail, failedConfig]() {
        if (_config == failedConfig && _shouldConnect()) {
            _connection.stopped();
            _connection.failed();
            _setStatus(detail);
        }
    });
}

void NMEASourceManager::_startAttempt()
{
    if (!_connection.beginAttempt()) {
        return;
    }
    _attempt = std::make_unique<NMEAConnectionAttempt>(_config, this);
#ifndef QGC_NO_SERIAL_LINK
    _attempt->setSerialDiscovery(_serialPorts);
#endif
    const QPointer<NMEAConnectionAttempt> current(_attempt.get());
    const auto dispatchCurrent = [this, current](std::function<void()> command) {
        _dispatch([this, current, command = std::move(command)]() {
            if (current && _attempt.get() == current && !current->stopping()) {
                command();
            }
        });
    };
    connect(current, &NMEAConnectionAttempt::configuring, this, [this, dispatchCurrent]() {
        dispatchCurrent([this]() {
            _connection.configuring();
            _setStatus(tr("Configuring receiver for NMEA"));
        });
    });
    connect(current, &NMEAConnectionAttempt::deviceReady, this, [this, dispatchCurrent]() {
        dispatchCurrent([this]() {
            const QPointer<NMEASourceManager> guard(this);
            if (!_installSource(_attempt->device())) {
                if (guard) {
                    _attemptFailed(tr("Cannot read NMEA source"));
                }
                return;
            }
            _connection.ready();
            _setStatus(_config.source == NMEAConnectionConfig::Udp
                           ? tr("Listening on UDP port %1").arg(_attempt->localPort())
                           : tr("Connected"));
        });
    });
    connect(current, &NMEAConnectionAttempt::dataReceived, this, [this, dispatchCurrent]() {
        dispatchCurrent([this]() {
            _udpActivityTimer.start();
            _setStatus(tr("Receiving UDP data on port %1").arg(_attempt->localPort()));
        });
    });
    connect(current, &NMEAConnectionAttempt::failed, this, [this, dispatchCurrent](const QString& detail) {
        dispatchCurrent([this, detail]() { _attemptFailed(detail); });
    });
    connect(current, &NMEAConnectionAttempt::stopped, this, [this, current]() {
        _dispatch([this, current]() {
            if (current && _attempt.get() == current) {
                if (_shutdown) {
                    _attempt.reset();
                } else {
                    _attempt.release()->deleteLater();
                }
                if (_connection.state() == GPSConnectionState::Stopping) {
                    _connection.stopped();
                }
            }
        });
    });
    _setStatus(tr("Connecting"));
    _attempt->start(_receiverFactory);
}

void NMEASourceManager::update()
{
    _dispatch([this]() { _update(); });
}

void NMEASourceManager::_update()
{
    const QPointer<NMEASourceManager> guard(this);
    if (!_settings || !_positionManager || !_shouldConnect()) {
        _stop();
        return;
    }
    if (const QString error = _config.validationError(); !error.isEmpty()) {
        _connection.pause();
        _stop();
        if (guard) {
            _setStatus(error);
        }
        return;
    }
    _connection.updateIntent(_settings->nmeaAutoConnect()->rawValue().toBool());
#ifndef QGC_NO_SERIAL_LINK
    if (_config.source == NMEAConnectionConfig::Serial && !_receiverFactory) {
        bool present = false;
        if (!_serialPorts) {
            _closeDevice();
            if (guard) {
                _setStatus(tr("Serial discovery is unavailable"));
            }
            return;
        }
        for (const auto& port : _serialPorts->availablePorts()) {
            present = present || port.systemLocation == _config.device;
        }
        if (!present) {
            _closeDevice();
            if (!guard) {
                return;
            }
            _connection.resetRetry();
            _setStatus(tr("Waiting for serial device"));
            return;
        }
    }
#endif
    if (!_attempt && _connection.canAttempt()) {
        _startAttempt();
    }
}

#ifndef QGC_NO_SERIAL_LINK
void NMEASourceManager::setSerialDiscovery(SerialPortManager* serialPorts)
{
    const QPointer<SerialPortManager> inventory(serialPorts);
    _dispatch([this, inventory]() {
        if (_serialPorts == inventory) {
            return;
        }
        const QPointer<NMEASourceManager> guard(this);
        _serialPorts = inventory;
        _autoConnectExclusion.reset();
        if (_config.source == NMEAConnectionConfig::Serial) {
            _closeDevice();
            if (!guard) {
                return;
            }
            _connection.resetRetry();
        }
        _updateSerialRouting();
    });
}
#endif
