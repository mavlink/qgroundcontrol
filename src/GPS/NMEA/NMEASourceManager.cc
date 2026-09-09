#include "NMEASourceManager.h"

#include <QtCore/QIODevice>
#include <QtPositioning/QGeoPositionInfoSource>

#include <algorithm>
#include <utility>

#include "GPSQtRuntimeScheduler.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEASourceManagerLog, "GPS.NMEA.NMEASourceManager")

NMEASourceManager::NMEASourceManager(QObject* parent, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
    , _decoder(this, _scheduler)
    , _connection(this, _scheduler)
{
    qCDebug(NMEASourceManagerLog) << this;
#ifndef QGC_NO_SERIAL_LINK
    _serialPorts = SerialPortManager::instance();
#endif
    connect(&_connection, &GPSConnectionState::changed, this, &NMEASourceManager::_notifyState);
    connect(&_decoder, &NMEADecoderSession::satellitesChanged, this, &NMEASourceManager::satellitesChanged);
    connect(&_decoder, &NMEADecoderSession::satellitesReceived, this, &NMEASourceManager::satellitesReceived);
    _status = tr("Disconnected");
    _udpActivityTimer.setSingleShot(true);
    _udpActivityTimer.setInterval(5000);
    connect(&_udpActivityTimer, &QTimer::timeout, this, [this]() {
        _dispatch([this]() {
            if (_attempt && !_attempt->stopping() &&
                _profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::UdpListener) {
                _setStatus(tr("Listening on UDP port %1").arg(_attempt->localPort()));
            }
        });
    });
}

NMEASourceManager::~NMEASourceManager()
{
    qCDebug(NMEASourceManagerLog) << this;
    blockSignals(true);
    _shutdown = true;
    if (_scheduler) {
        _scheduler->cancel(_updateTask);
    }
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
    _scheduleUpdate();
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
    return _sourceAvailable ? _decoder.positionSource() : nullptr;
}

bool NMEASourceManager::_shouldConnect() const
{
    return !_shutdown && _connection.shouldConnect(_automatic) &&
           _profile.endpoint.kind != GPSReceiverProfile::Endpoint::Kind::Disabled;
}

void NMEASourceManager::_updateSerialRouting()
{
#ifndef QGC_NO_SERIAL_LINK
    const QString port = _shouldConnect() && _profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial
                             ? _profile.endpoint.device
                             : QString();
    _autoConnectExclusion = _serialPorts ? _serialPorts->excludeFromAutoConnect(port) : nullptr;
#endif
}

void NMEASourceManager::setProfile(const GPSReceiverProfile& profile)
{
    _dispatch([this, profile = profile.normalized()]() {
        if (_profile == profile) {
            return;
        }
        const QPointer<NMEASourceManager> guard(this);
        _stopped = false;
        _profile = profile;
        _receiverFactory = {};
        _closeDevice();
        if (!guard) {
            return;
        }
        _connection.resetRetry();
        _updateSerialRouting();
        if (!_shouldConnect()) {
            _stop();
        }
    });
}

void NMEASourceManager::setAutoConnect(bool enabled)
{
    _dispatch([this, enabled]() {
        if (_automatic == enabled) {
            return;
        }
        const QPointer<NMEASourceManager> guard(this);
        _stopped = false;
        _automatic = enabled;
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
}

void NMEASourceManager::setSuspended(bool suspended)
{
    _dispatch([this, suspended]() { _suspended = suspended; });
}

void NMEASourceManager::_scheduleUpdate()
{
    if (!_scheduler) {
        return;
    }
    _scheduler->cancel(std::exchange(_updateTask, 0));
    if (_stopped || !_shouldConnect() || _suspended) {
        return;
    }
    qint64 delay = !_attempt ? _connection.retryRemainingMs() : -1;
#ifndef QGC_NO_SERIAL_LINK
    if (_profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial && !_receiverFactory) {
        delay = delay < 0 ? 1000 : std::min<qint64>(1000, delay);
    }
#endif
    if (!_attempt && _connection.state() == GPSConnectionState::Disconnected) {
        delay = delay < 0 ? 0 : delay;
    }
    if (delay >= 0) {
        _updateTask = _scheduler->schedule(this, std::chrono::milliseconds(delay), [this]() {
            _updateTask = 0;
            update();
        });
    }
}

bool NMEASourceManager::connectSource()
{
    if (_shutdown || _suspended || _profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Disabled) {
        return false;
    }
    const QPointer<NMEASourceManager> guard(this);
    _dispatch([this]() {
        _stopped = false;
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
    _dispatch([this]() {
        _stopped = true;
        _stop();
    });
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
    _setStatus(_connection.paused() && _automatic ? tr("Automatic connection paused") : tr("Disconnected"));
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
    if (std::exchange(_sourceAvailable, false)) {
        emit positionSourceChanged();
    }
    if (guard) {
        _decoder.stop();
    }
}

bool NMEASourceManager::_installSource(QIODevice* device)
{
    if (!device || (!device->isOpen() && !device->open(QIODevice::ReadOnly)) || !device->isReadable()) {
        _setStatus(tr("Cannot read NMEA source"));
        return false;
    }
    const QPointer<NMEASourceManager> guard(this);
    device->readAll();
    if (!_decoder.start(device) || !guard) {
        return false;
    }
    _sourceAvailable = true;
    emit positionSourceChanged();
    if (!guard) {
        return false;
    }
    _decoder.positionSource()->setUpdateInterval(0);
    if (!guard) {
        return false;
    }
    _decoder.positionSource()->startUpdates();
    return guard;
}

void NMEASourceManager::_attemptFailed(const QString& detail)
{
    const QPointer<NMEASourceManager> guard(this);
    const auto failedConfig = _profile;
    const quint64 failedGeneration = _attemptGeneration;
    _closeDevice();
    if (!guard) {
        return;
    }
    // Retry starts after the stopped event, once the attempt has released its endpoint.
    _commands.push_back([this, detail, failedConfig, failedGeneration]() {
        if (_attemptGeneration == failedGeneration && _profile == failedConfig && _shouldConnect()) {
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
    _attempt = std::make_unique<NMEAConnectionAttempt>(_profile, this, ++_attemptGeneration);
    _attempt->setRecordingBuffer(_recordingBuffer);
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
            _setStatus(_profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::UdpListener
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
    _dispatch([this]() {
        _stopped = false;
        _update();
    });
}

void NMEASourceManager::_update()
{
    const QPointer<NMEASourceManager> guard(this);
    if (!_shouldConnect()) {
        _stop();
        return;
    }
    if (const QString error = _profile.validationError(); !error.isEmpty()) {
        _connection.pause();
        _stop();
        if (guard) {
            _setStatus(error);
        }
        return;
    }
    _connection.updateIntent(_automatic);
    if (_suspended) {
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial && !_receiverFactory) {
        bool present = false;
        if (!_serialPorts) {
            _closeDevice();
            if (guard) {
                _setStatus(tr("Serial discovery is unavailable"));
            }
            return;
        }
        for (const auto& port : _serialPorts->availablePorts()) {
            present = present || port.systemLocation == _profile.endpoint.device;
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
        if (_serialPorts) {
            _serialPorts->disconnect(this);
        }
        _serialPorts = inventory;
        if (_serialPorts) {
            connect(_serialPorts, &SerialPortManager::serialPortsChanged, this, &NMEASourceManager::update);
        }
        _autoConnectExclusion.reset();
        if (_profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial) {
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
