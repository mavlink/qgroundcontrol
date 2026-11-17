#include "PositionManager.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QSignalBlocker>

#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(PositionManagerLog, "GPS.Positioning.PositionManager")

PositionManager::PositionManager(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _recoveryTask(_scheduler, this)
    , _deviceHealth(new GPSSourceHealth(this, _scheduler))
{
    qCDebug(PositionManagerLog) << this;
    connect(_deviceHealth, &GPSSourceHealth::positionChanged, this, [this]() { _update(SelectedSource::Internal); });
}

PositionManager::~PositionManager()
{
    qCDebug(PositionManagerLog) << "Position manager shutdown:" << this;
    blockSignals(true);
    // Stop the borrowed backend and drop the receiver binding while the manager is still whole.
    _releaseDevice();
    for (const ReceiverBinding& binding : _receivers) {
        if (binding.health) {
            binding.health->disconnect(this);
        }
    }
}

void PositionManager::setConfiguration(const Configuration& configuration)
{
    const SourceMode mode = configuration.sourceMode;
    if (_shutdown || _configuration == configuration ||
        (mode != SourceMode::Automatic && mode != SourceMode::ReceiverOnly && mode != SourceMode::InternalOnly) ||
        (configuration.receiver != Receiver::Rtk && configuration.receiver != Receiver::Position)) {
        return;
    }
    _configuration = configuration;
    _receiverUsableSinceMs.reset();
    _recoveryTask.cancel();
    _reselect = true;
    _update();
    _setDeviceActive(mode != SourceMode::ReceiverOnly);
}

void PositionManager::setPlatformSourceFactory(SourceFactory factory)
{
    _platformSourceFactory = std::move(factory);
}

void PositionManager::init()
{
    if (_shutdown) {
        return;
    }
    _checkPermission();
}

void PositionManager::shutdown()
{
    if (std::exchange(_shutdown, true)) {
        return;
    }
    qCDebug(PositionManagerLog) << "Releasing position sources";
    setInternalPositionSource(nullptr, SourceStatus::NoSource);
}

void PositionManager::_setupPositionSources()
{
    auto* platformSource = _platformSourceFactory ? _platformSourceFactory(this) : nullptr;
    const bool custom = platformSource != nullptr;
    if (!custom) {
        // Android reports MSL altitude only when asked; _backendAltitudeDatum() relies on its ellipsoid height.
        platformSource = QGeoPositionInfoSource::createDefaultSource({{QStringLiteral("useMslAltitude"), false}}, this);
        if (platformSource) {
            qCDebug(PositionManagerLog) << "Device position backend:" << platformSource->sourceName();
        }
    }
    setInternalPositionSource(
        platformSource, platformSource ? SourceStatus::WaitingForFix : SourceStatus::BackendUnavailable, custom,
        custom || !platformSource ? GPSAltitudeDatum::Unknown : _backendAltitudeDatum(platformSource->sourceName()));
}

GPSAltitudeDatum PositionManager::_backendAltitudeDatum(const QString& sourceName)
{
    // Android's Location.getAltitude(), which the backend reports while its useMslAltitude parameter is false.
    if (sourceName == QLatin1String("android")) {
        return GPSAltitudeDatum::Ellipsoid;
    }
    // Core Location's altitude and the GGA altitude the NMEA backend reads are above mean sea level, and the WinRT
    // backend reports an altitude only when its reference is the geoid.
    if (sourceName == QLatin1String("corelocation") || sourceName == QLatin1String("winrt") ||
        sourceName == QLatin1String("nmea")) {
        return GPSAltitudeDatum::MeanSeaLevel;
    }
    // GeoClue, the browser and OpenHarmony backends pass on an altitude whose reference they do not state.
    return GPSAltitudeDatum::Unknown;
}

void PositionManager::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
{
    if (_shutdown) {
        return;
    }
    if (permissionStatus == Qt::PermissionStatus::Granted) {
        _setupPositionSources();
    } else {
        _setInternalPositionStatus(SourceStatus::PermissionDenied);
    }
}

void PositionManager::_checkPermission()
{
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);
    const auto status = QCoreApplication::instance()->checkPermission(locationPermission);
    if (status == Qt::PermissionStatus::Undetermined) {
        _setInternalPositionStatus(SourceStatus::PermissionRequired);
        QCoreApplication::instance()->requestPermission(
            locationPermission, this,
            [this](const QPermission& permission) { _handlePermissionStatus(permission.status()); });
    } else {
        _handlePermissionStatus(status);
    }
}

void PositionManager::setInternalPositionSource(QGeoPositionInfoSource* source, SourceStatus status, bool custom,
                                                GPSAltitudeDatum altitudeDatum)
{
    _usingPluginSource = custom;
    _deviceAltitudeDatum = altitudeDatum;
    _platformStatus = status;
    if (source == _device) {
        _update();
        return;
    }
    _releaseDevice();
    _device = source;
    if (source) {
        connect(source, &QGeoPositionInfoSource::positionUpdated, this, &PositionManager::_devicePositionUpdated);
        connect(source, &QGeoPositionInfoSource::errorOccurred, this, &PositionManager::_deviceErrorOccurred);
        connect(source, &QObject::destroyed, this, [this]() {
            _releaseDevice();
            _bindingChanged(SelectedSource::Internal);
        });
    }
    _bindingChanged(SelectedSource::Internal);
    _setDeviceActive(_configuration.sourceMode != SourceMode::ReceiverOnly);
}

void PositionManager::_setInternalPositionStatus(SourceStatus status)
{
    _platformStatus = status;
    _update();
}

GPSPositionSourceRegistration PositionManager::registerReceiver(GPSSourceHealth* producer, quint64 sessionId,
                                                                Receiver receiver)
{
    if (!producer || (receiver != Receiver::Rtk && receiver != Receiver::Position)) {
        return {};
    }
    const quint64 token = _setReceiver(receiver, producer, sessionId);
    // The handle points at this manager only so that it tests true; releasing it retires this binding alone.
    return GPSPositionSourceRegistration(this,
                                         [manager = QPointer<PositionManager>(this), receiver, token](const void*) {
                                             if (manager) {
                                                 manager->_retireReceiver(receiver, token);
                                             }
                                         });
}

quint64 PositionManager::_setReceiver(Receiver receiver, GPSSourceHealth* producer, quint64 sessionId)
{
    ReceiverBinding& binding = _binding(receiver);
    if (binding.health) {
        binding.health->disconnect(this);
    }
    binding.health = producer;
    binding.session = sessionId;
    const quint64 token = ++binding.token;
    if (producer) {
        // What the other receiver reports, and its coming and going, change nothing that is published.
        connect(producer, &GPSSourceHealth::positionChanged, this, [this, receiver]() {
            if (receiver == _configuration.receiver) {
                _update(SelectedSource::Receiver);
            }
        });
        connect(producer, &QObject::destroyed, this, [this, receiver]() { _setReceiver(receiver, nullptr, 0); });
    }
    if (receiver == _configuration.receiver) {
        _bindingChanged(SelectedSource::Receiver);
    }
    return token;
}

void PositionManager::_retireReceiver(Receiver receiver, quint64 token)
{
    if (token == _binding(receiver).token) {
        _setReceiver(receiver, nullptr, 0);
    }
}

void PositionManager::_releaseDevice()
{
    _setDeviceActive(false);
    if (_device) {
        _device->disconnect(this);
    }
    _device = nullptr;
}

void PositionManager::_setDeviceActive(bool active)
{
    active = active && _device;
    if (_deviceActive == active) {
        return;
    }
    _deviceActive = active;
    if (active) {
        _device->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
#if !defined(Q_OS_DARWIN)
        _device->setUpdateInterval(_device->minimumUpdateInterval());
#endif
        _device->startUpdates();
        return;
    }
    if (_device) {
        _device->stopUpdates();
    }
    // Reactivation must not publish a fix from before the stop. An inactive device is never consulted, so this
    // reset needs no re-evaluation, and it must not notify while the service is being destroyed.
    const QSignalBlocker silent(_deviceHealth);
    _deviceHealth->reset();
}

void PositionManager::_devicePositionUpdated(const QGeoPositionInfo& position)
{
    // A stopped backend may still deliver a report it queued before stopUpdates().
    if (!_deviceActive) {
        return;
    }
    _platformStatus = SourceStatus::WaitingForFix;
    if (_selectedKind == SelectedSource::Internal) {
        _setPositioningError(QGeoPositionInfoSource::NoError);
    }
    GPSObservation observation;
    observation.position = position;
    observation.monotonicTimestampUs = _scheduler->nowUs();
    // An altitude without a vertical accuracy may be a placeholder, such as Core Location's for a 2D fix.
    if (position.hasAttribute(QGeoPositionInfo::VerticalAccuracy) && qIsFinite(position.coordinate().altitude())) {
        observation.altitudeDatum = _deviceAltitudeDatum;
    }
    _deviceHealth->updateObservation(observation);
}

void PositionManager::_deviceErrorOccurred(QGeoPositionInfoSource::Error error)
{
    if (!_deviceActive) {
        return;
    }
    if (error == QGeoPositionInfoSource::AccessError) {
        _platformStatus = SourceStatus::PermissionDenied;
    } else if (error == QGeoPositionInfoSource::ClosedError || error == QGeoPositionInfoSource::UnknownSourceError) {
        _platformStatus = SourceStatus::BackendUnavailable;
    }
    if (_selectedKind == SelectedSource::Internal) {
        _setPositioningError(error);
    }
    // A timeout means no new fix, which freshness already expires; only real failures invalidate the fix.
    if (error != QGeoPositionInfoSource::NoError && error != QGeoPositionInfoSource::UpdateTimeoutError) {
        _deviceHealth->invalidatePosition();
    }
    _update();
}

void PositionManager::_bindingChanged(SelectedSource kind)
{
    // A rebound role needs a fresh selection even when its health object is unchanged.
    _reselect = _reselect || _selectedKind == kind;
    _update();
}

void PositionManager::_update(SelectedSource reporter)
{
    const SelectedSource chosen = _choose();
    GPSSourceHealth* const health = _sourceHealth(chosen);
    bool republish = reporter == chosen;
    if (_reselect || chosen != _selectedKind || _currentHealth != health) {
        qCDebug(PositionManagerLog) << "Ground-station position source changed"
                                    << "source:" << chosen << "selected:" << health;
        _reselect = false;
        _selectedKind = chosen;
        _currentHealth = health;
        _selectionRevision = health ? health->observationRevision() : 0;
        _authorized = false;
        _published = {};
        _gcsPositioningError = QGeoPositionInfoSource::NoError;
        republish = _configuration.sourceMode == SourceMode::Automatic;
    }
    // Standby deadlines can run before the selected source's deadline.
    if (republish || (_published.position.isValid() && !_accepted(_selectedKind))) {
        _republish();
    }
    _updateStatus();
    _emitChanges();
}

PositionManager::SelectedSource PositionManager::_choose()
{
    if (_configuration.sourceMode == SourceMode::ReceiverOnly) {
        return SelectedSource::Receiver;
    }
    if (_configuration.sourceMode == SourceMode::InternalOnly) {
        return SelectedSource::Internal;
    }
    const bool receiverUsable = _accepted(SelectedSource::Receiver).has_value();
    const bool deviceUsable = _accepted(SelectedSource::Internal).has_value();
    // A receiver that recovers while the device works takes over only after staying usable for RECOVERY_DELAY.
    if (receiverUsable && deviceUsable && _selectedKind == SelectedSource::Internal) {
        const qint64 nowMs = _scheduler->nowMs();
        if (!_receiverUsableSinceMs) {
            _receiverUsableSinceMs = nowMs;
        }
        const auto remaining = RECOVERY_DELAY - std::chrono::milliseconds(nowMs - *_receiverUsableSinceMs);
        if (remaining > std::chrono::milliseconds::zero()) {
            _recoveryTask.schedule(remaining, [this]() { _update(); });
            return SelectedSource::Internal;
        }
    }
    _receiverUsableSinceMs.reset();
    _recoveryTask.cancel();
    if (receiverUsable) {
        return SelectedSource::Receiver;
    }
    if (deviceUsable) {
        return SelectedSource::Internal;
    }
    if (_sourceHealth(_selectedKind)) {
        return _selectedKind;
    }
    return _sourceHealth(SelectedSource::Receiver) ? SelectedSource::Receiver : SelectedSource::Internal;
}

void PositionManager::_republish()
{
    _published = {};
    if (!_currentHealth) {
        return;
    }
    // Pinned selections need a new observation, not a timeout change.
    _authorized = _configuration.sourceMode == SourceMode::Automatic ||
                  _currentHealth->observationRevision() != _selectionRevision;
    const auto accepted = acceptedObservation();
    if (!accepted) {
        if (_currentHealth->state() != GPSSourceHealth::State::NoData &&
            _gcsPositioningError == QGeoPositionInfoSource::NoError) {
            _setPositioningError(QGeoPositionInfoSource::UpdateTimeoutError);
        }
        return;
    }
    _setPositioningError(QGeoPositionInfoSource::NoError);
    _published = {.position = accepted->position.coordinate(),
                  .heading = accepted->heading(),
                  .horizontalAccuracy = accepted->position.attribute(QGeoPositionInfo::HorizontalAccuracy)};
}

void PositionManager::_updateStatus()
{
    _selectedSource = _currentHealth ? _selectedKind : SelectedSource::None;
    SourceStatus status = SourceStatus::NoSource;
    if (_currentHealth) {
        switch (_currentHealth->state()) {
            case GPSSourceHealth::State::NoData:
                status = SourceStatus::WaitingForFix;
                break;
            case GPSSourceHealth::State::Usable:
                status = !_accepted(_selectedKind)       ? SourceStatus::Stale
                         : _published.position.isValid() ? SourceStatus::Active
                                                         : SourceStatus::WaitingForFix;
                break;
            case GPSSourceHealth::State::Stale:
                status = SourceStatus::Stale;
                break;
            case GPSSourceHealth::State::Invalid:
                status = SourceStatus::InvalidFix;
                break;
        }
    } else if (_configuration.sourceMode != SourceMode::ReceiverOnly) {
        status = _platformStatus;
    }
    if (_selectedSource == SelectedSource::Internal &&
        (_platformStatus == SourceStatus::PermissionDenied || _platformStatus == SourceStatus::BackendUnavailable)) {
        status = _platformStatus;
    }
    _sourceStatus = status;
}

void PositionManager::_emitChanges()
{
    // Each notified value is recorded before its signal, so a nested update never repeats an older value.
    const Notified::Selection selection{_selectedSource, _sourceStatus, selectedSourceName()};
    if (std::exchange(_notified.selection, selection) != selection) {
        emit selectionChanged();
    }
    const qreal accuracy = _published.horizontalAccuracy;
    if (std::exchange(_notified.publication.horizontalAccuracy, accuracy) != accuracy) {
        emit gcsPositionHorizontalAccuracyChanged(accuracy);
    }
    const qreal heading = _published.heading;
    const qreal notifiedHeading = std::exchange(_notified.publication.heading, heading);
    if (notifiedHeading != heading && !(qIsNaN(notifiedHeading) && qIsNaN(heading))) {
        emit gcsHeadingChanged(heading);
    }
    const QGeoCoordinate position = _published.position;
    if (std::exchange(_notified.publication.position, position) != position) {
        emit gcsPositionChanged(position);
    }
}

void PositionManager::_setPositioningError(QGeoPositionInfoSource::Error error)
{
    if (_gcsPositioningError == error) {
        return;
    }
    _gcsPositioningError = error;
    if (error != QGeoPositionInfoSource::NoError) {
        qCDebug(PositionManagerLog) << "Positioning error:" << error;
    }
}

std::optional<GPSObservation> PositionManager::acceptedObservation(GPSObservation::PositionUse use) const
{
    return _currentHealth && _authorized ? _accepted(_selectedKind, use) : std::nullopt;
}

std::optional<GPSObservation> PositionManager::_accepted(SelectedSource kind, GPSObservation::PositionUse use) const
{
    const GPSSourceHealth* const health = _sourceHealth(kind);
    auto observation = health ? health->acceptedObservation(use) : std::nullopt;
    if (observation && kind == SelectedSource::Receiver && _receiverSource().session != 0 &&
        observation->sessionId != _receiverSource().session) {
        return std::nullopt;
    }
    return observation;
}

GPSSourceHealth* PositionManager::_sourceHealth(SelectedSource kind) const
{
    switch (kind) {
        case SelectedSource::Receiver:
            return _receiverSource().health.data();
        case SelectedSource::Internal:
            return _device ? _deviceHealth : nullptr;
        case SelectedSource::None:
            break;
    }
    return nullptr;
}

QString PositionManager::selectedSourceName() const
{
    switch (_selectedSource) {
        case SelectedSource::None:
            return tr("None");
        case SelectedSource::Receiver:
            return _configuration.receiver == Receiver::Position ? tr("Position receiver") : tr("GNSS receiver");
        case SelectedSource::Internal:
            return _usingPluginSource ? tr("Plugin positioning") : tr("This device");
    }
    return {};
}

QString PositionManager::sourceStatusText() const
{
    switch (_sourceStatus) {
        case SourceStatus::NoSource:
            return tr("Selected position source is not connected");
        case SourceStatus::PermissionRequired:
            return tr("Waiting for location permission");
        case SourceStatus::PermissionDenied:
            return tr("Location permission denied");
        case SourceStatus::BackendUnavailable:
            return tr("No positioning backend is available on this device");
        case SourceStatus::WaitingForFix:
            return tr("Waiting for a position fix");
        case SourceStatus::Active:
            return tr("Position is usable");
        case SourceStatus::Stale:
            return tr("Position data is stale");
        case SourceStatus::InvalidFix:
            return tr("Position fix does not meet accuracy requirements");
    }
    return {};
}
