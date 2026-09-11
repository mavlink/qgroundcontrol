#include "NMEASourceManager.h"

#include <QtCore/QIODevice>
#include <QtPositioning/QGeoPositionInfoSource>

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEASourceManagerLog, "GPS.NMEA.NMEASourceManager")

NMEASourceManager::NMEASourceManager(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _control(GPSConnectionControl::NotificationPolicy::AfterCommands, this, scheduler)
    , _decoder(this, _control.scheduler())
    , _udpActivity(_control.scheduler(), this)
{
    qCDebug(NMEASourceManagerLog) << this;
    connect(&_control, &GPSConnectionControl::changed, this, &NMEASourceManager::stateChanged);
    connect(&_decoder, &NMEADecoderSession::satellitesChanged, this, &NMEASourceManager::satellitesChanged);
    connect(&_decoder, &NMEADecoderSession::satellitesReceived, this, &NMEASourceManager::satellitesReceived);
    _status = tr("Disconnected");
    connect(&_control, &GPSConnectionControl::commandsDrained, this, &NMEASourceManager::_scheduleUpdate);
}

NMEASourceManager::~NMEASourceManager()
{
    qCDebug(NMEASourceManagerLog) << this;
    blockSignals(true);
    _control.setShutdown();
    _control.cancelUpdate();
    _control.disconnect(this);
    _decoder.disconnect(this);
    _udpActivity.cancel();
    _uninstallSource();
    if (_attempt) {
        _attempt->disconnect(this);
        _attempt->shutdown();
    }
}

void NMEASourceManager::_dispatch(std::function<void()> command)
{
    _control.dispatch(std::move(command));
}

void NMEASourceManager::_notifyState()
{
    _control.notifyChanged();
}

QGeoPositionInfoSource* NMEASourceManager::positionSource() const
{
    return _sourceAvailable ? _decoder.positionSource() : nullptr;
}

bool NMEASourceManager::_shouldConnect() const
{
    return _control.shouldConnect();
}

void NMEASourceManager::_updateSerialRouting()
{
#ifndef QGC_NO_SERIAL_LINK
    const QString port =
        _shouldConnect() && _control.profile().endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial
            ? _control.profile().endpoint.device
            : QString();
    _autoConnectExclusion = _serialPorts ? _serialPorts->excludeFromAutoConnect(port) : nullptr;
#endif
}

void NMEASourceManager::setProfile(const GPSReceiverProfile& profile)
{
    _dispatch([this, profile = profile.normalized()]() {
        if (!_control.changeProfile(profile)) {
            return;
        }
        const QPointer<NMEASourceManager> guard(this);
        _control.setStopped(false);
        _receiverFactory = {};
        _closeDevice();
        if (!guard) {
            return;
        }
        _control.connection().resetRetry();
        _updateSerialRouting();
        if (!_shouldConnect()) {
            _stop();
        }
    });
}

void NMEASourceManager::setAutoConnect(bool enabled)
{
    _dispatch([this, enabled]() {
        if (!_control.changeAutomatic(enabled)) {
            return;
        }
        const QPointer<NMEASourceManager> guard(this);
        _control.setStopped(false);
        _closeDevice();
        if (!guard) {
            return;
        }
        _control.connection().resetIntent();
        _updateSerialRouting();
        if (!_shouldConnect()) {
            _stop();
        }
    });
}

void NMEASourceManager::setSuspended(bool suspended)
{
    _dispatch([this, suspended]() { _control.changeSuspended(suspended); });
}

void NMEASourceManager::_scheduleUpdate()
{
    qint64 discoveryPollMs = -1;
#ifndef QGC_NO_SERIAL_LINK
    if (_control.profile().endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial && !_receiverFactory) {
        discoveryPollMs = 1000;
    }
#endif
    _control.scheduleUpdate(!_attempt, discoveryPollMs, true, [this]() { update(); });
}

bool NMEASourceManager::connectSource()
{
    if (_control.shutdown() || _control.suspended() ||
        _control.profile().endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Disabled) {
        return false;
    }
    const QPointer<NMEASourceManager> guard(this);
    _dispatch([this]() {
        _control.setStopped(false);
        _control.connection().requestConnect();
        _updateSerialRouting();
        _update();
    });
    return guard && active();
}

void NMEASourceManager::disconnectSource()
{
    _dispatch([this]() {
        const QPointer<NMEASourceManager> guard(this);
        _control.connection().pause();
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
        _control.setShutdown();
        _stop();
        if (!guard) {
            return;
        }
        if (_attempt) {
            _attempt->shutdown();
        }
        if (guard) {
            _updateSerialRouting();
        }
    });
}

void NMEASourceManager::stop()
{
    _dispatch([this]() {
        _control.setStopped(true);
        _stop();
    });
}

void NMEASourceManager::_stop()
{
    const QPointer<NMEASourceManager> guard(this);
    _control.connection().stop();
    _receiverFactory = {};
    _closeDevice();
    if (!guard) {
        return;
    }
    _control.connection().resetRetry();
    _setStatus(_control.connection().paused() && _control.automatic() ? tr("Automatic connection paused")
                                                                      : tr("Disconnected"));
}

void NMEASourceManager::_closeDevice()
{
    const QPointer<NMEASourceManager> guard(this);
    _udpActivity.cancel();
    _uninstallSource();
    if (!guard) {
        return;
    }
    if (_attempt && !_attempt->stopping()) {
        _control.connection().stopping();
        _attempt->stop();
    } else if (!_attempt) {
        _control.connection().stopped();
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
    const auto failedConfig = _control.profile();
    const quint64 failedGeneration = _attemptGeneration;
    const auto disposition =
        _attempt && _attempt->attempt().failure ? _attempt->attempt().failure->retry : GPSRetryDisposition::Retry;
    _closeDevice();
    if (!guard) {
        return;
    }
    // Retry starts after the stopped event, once the attempt has released its endpoint.
    _control.enqueue([this, detail, failedConfig, failedGeneration, disposition]() {
        if (_attemptGeneration == failedGeneration && _control.profile() == failedConfig && _shouldConnect()) {
            _control.connection().stopped();
            _control.connection().failed(disposition);
            _setStatus(detail);
        }
    });
}

void NMEASourceManager::_startAttempt()
{
    _control.startAttempt([this]() {
        _openAttempt();
        return true;
    });
}

void NMEASourceManager::_openAttempt()
{
    _attempt =
        std::make_unique<NMEAConnectionAttempt>(_control.profile(), this, ++_attemptGeneration, _control.scheduler());
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
            _control.connection().configuring();
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
            _control.connection().ready();
            _setStatus(_control.profile().endpoint.kind == GPSReceiverProfile::Endpoint::Kind::UdpListener
                           ? tr("Listening on UDP port %1").arg(_attempt->localPort())
                           : tr("Connected"));
        });
    });
    connect(current, &NMEAConnectionAttempt::dataReceived, this, [this, dispatchCurrent]() {
        dispatchCurrent([this]() {
            _udpActivity.schedule(std::chrono::seconds(5), [this]() {
                _dispatch([this]() {
                    if (_attempt && !_attempt->stopping() &&
                        _control.profile().endpoint.kind == GPSReceiverProfile::Endpoint::Kind::UdpListener) {
                        _setStatus(tr("Listening on UDP port %1").arg(_attempt->localPort()));
                    }
                });
            });
            _setStatus(tr("Receiving UDP data on port %1").arg(_attempt->localPort()));
        });
    });
    connect(current, &NMEAConnectionAttempt::failed, this, [this, dispatchCurrent](const QString& detail) {
        dispatchCurrent([this, detail]() { _attemptFailed(detail); });
    });
    connect(current, &NMEAConnectionAttempt::stopped, this, [this, current]() {
        _dispatch([this, current]() {
            if (current && _attempt.get() == current) {
                if (_control.shutdown()) {
                    _attempt.reset();
                } else {
                    _attempt.release()->deleteLater();
                }
                if (_control.connection().state() == GPSConnectionState::Stopping) {
                    _control.connection().stopped();
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
        _control.setStopped(false);
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
    if (const QString error = _control.profile().validationError(); !error.isEmpty()) {
        _control.connection().pause();
        _stop();
        if (guard) {
            _setStatus(error);
        }
        return;
    }
    _control.connection().updateIntent(_control.automatic());
    if (_control.suspended()) {
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_control.profile().endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial && !_receiverFactory) {
        bool present = false;
        if (!_serialPorts) {
            _closeDevice();
            if (guard) {
                _setStatus(tr("Serial discovery is unavailable"));
            }
            return;
        }
        for (const auto& port : _serialPorts->availablePorts()) {
            present = present || port.systemLocation == _control.profile().endpoint.device;
        }
        if (!present) {
            _closeDevice();
            if (!guard) {
                return;
            }
            _control.connection().resetRetry();
            _setStatus(tr("Waiting for serial device"));
            return;
        }
    }
#endif
    if (!_attempt && _control.connection().canAttempt()) {
        _startAttempt();
    }
}

#ifndef QGC_NO_SERIAL_LINK
void NMEASourceManager::setSerialDiscovery(GPSSerialDiscovery* serialPorts)
{
    const QPointer<GPSSerialDiscovery> inventory(serialPorts);
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
            connect(_serialPorts, &GPSSerialDiscovery::serialPortsChanged, this, &NMEASourceManager::update);
        }
        _autoConnectExclusion.reset();
        if (_control.profile().endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial) {
            _closeDevice();
            if (!guard) {
                return;
            }
            _control.connection().resetRetry();
        }
        _updateSerialRouting();
    });
}
#endif
