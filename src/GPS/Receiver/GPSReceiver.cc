#include "GPSReceiver.h"

#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QScopedValueRollback>

#include "GPSCorrectionManager.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverMessages.h"
#include "GPSReceiverSession.h"
#include "GPSRecordingTransport.h"
#include "GPSSourceHealth.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"
#include "TCPGPSTransport.h"
#include "UDPGPSTransport.h"

QGC_LOGGING_CATEGORY(GPSReceiverLog, "GPS.Receiver.GPSReceiver")

namespace {

void logConnectionError(GPSConnectionError error, const QString& detail)
{
    switch (error) {
        case GPSConnectionError::OpenFailed:
            qCWarning(GPSReceiverLog) << "Failed to open GPS receiver transport:" << detail;
            break;
        case GPSConnectionError::ConfigFailed:
        case GPSConnectionError::ConsentRequired:
            qCWarning(GPSReceiverLog) << "GPS receiver did not accept configuration";
            break;
        case GPSConnectionError::ProtocolError:
            qCWarning(GPSReceiverLog) << "GPS receiver protocol error, connection lost:" << detail;
            break;
        case GPSConnectionError::DeviceError:
            qCWarning(GPSReceiverLog) << "GPS device error, connection lost:" << detail;
            break;
        case GPSConnectionError::None:
            break;
    }
}

GPSReceiverSerialPorts* createSerialSide(const GPSReceiver::Dependencies& dependencies, QObject* parent)
{
    GPSReceiverSerialPorts* const serial = dependencies.serial ? dependencies.serial(parent) : nullptr;
    return serial ? serial : GPSReceiverSerialPorts::create(nullptr, parent);
}

}  // namespace

GPSReceiver::GPSReceiver(QObject* parent, RuntimeScheduler* scheduler, const Dependencies& dependencies)
    : QObject(parent)
    , _serial(createSerialSide(dependencies, this))
    , _connectability(GPSReceiverConnectionPolicy::connectability(_configuration, _serial->supported()))
    , _correctionManager(dependencies.corrections)
    , _positionManager(dependencies.positions)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _positionHealth(new GPSSourceHealth(this, _scheduler))
    , _outputOverflowTask(_scheduler, this)
    , _pollTask(_scheduler, this)
    , _recordingDirectory(dependencies.recordingDirectory)
{
    qCDebug(GPSReceiverLog) << this;
    // A silent receiver stays connected, so its last solution must not remain on display.
    connect(_positionHealth, &GPSSourceHealth::positionChanged, this, [this]() {
        if (_positionHealth->state() == GPSSourceHealth::State::Stale) {
            _clearStaleSolution();
        }
    });
    _connection = std::make_unique<GPSReceiverConnectionPolicy>(static_cast<GPSReceiverConnector&>(*this),
                                                                _configuration, _connectability, *_serial, _scheduler);
    _facts = new GPSReceiverFactGroup(this);
    _notified.receiver = _receiverState();
    connect(_serial, &GPSReceiverSerialPorts::scanned, this, &GPSReceiver::_onSerialPortsScanned);
    connect(_serial, &GPSReceiverSerialPorts::entriesChanged, this, [this]() {
        if (!_destroying) {
            emit serialPortEntriesChanged();
        }
    });
}

void GPSReceiver::setConfiguration(const Configuration& configuration)
{
    if (_configuration == configuration) {
        return;
    }
    if (!_configuration.sameConnection(configuration)) {
        _setPersistentChangesAllowed(false);
    }
    const Configuration previous = std::exchange(_configuration, configuration);
    _connectability = GPSReceiverConnectionPolicy::connectability(_configuration, _serial->supported());
    _connection->configurationChanged(previous);
    ++_configurationRevision;
    _emitChanges();
}

void GPSReceiver::setPersistentChangesAllowed(bool allowed)
{
    _setPersistentChangesAllowed(allowed);
    _emitChanges();
}

void GPSReceiver::_setPersistentChangesAllowed(bool allowed)
{
    if (!_destroying) {
        _persistentChangesAllowed = allowed;
    }
}

bool GPSReceiver::canConnect() const
{
    // Connect stays available for what it reports when pressed, such as a missing address or device.
    return _connectability.type && _connectability.baseModeSupported && _connectability.connectionSupported;
}

void GPSReceiver::setWorkerFactory(WorkerFactory factory)
{
    if (_destroying) {
        return;
    }
    if (hasReceiver()) {
        qCWarning(GPSReceiverLog) << "Inject the worker factory before connecting a receiver";
        return;
    }
    _workerFactory = std::move(factory);
}

GPSReceiver::~GPSReceiver()
{
    _destroying = true;
    // The policy must not run against a partially destroyed receiver.
    _connection.reset();
    _retireSession();

    qCDebug(GPSReceiverLog) << this;
}

GPSReceiver::ReceiverState GPSReceiver::_receiverState() const
{
    return {.state = state(),
            .baseMode = _activeBaseMode(),
            .endpoint = activeEndpoint(),
            .identity = receiverIdentity(),
            .detected = detectedReceiver(),
            .presentation = activePresentation()};
}

void GPSReceiver::_emitChanges()
{
    if (_destroying) {
        return;
    }
    // Starting or stopping to reconnect the saved receiver is a session change.
    if (reconnecting() != (_notified.receiver.state == State::Reconnecting)) {
        _setPersistentChangesAllowed(false);
    }
    if (std::exchange(_notified.configurationRevision, _configurationRevision) != _configurationRevision) {
        emit configurationChanged();
    }
    if (std::exchange(_notified.persistentChangesAllowed, _persistentChangesAllowed) != _persistentChangesAllowed) {
        emit persistentChangesAllowedChanged();
    }
    const QString error = errorMessage();
    if (std::exchange(_notified.errorMessage, error) != error) {
        emit errorMessageChanged();
    }
    const QList<GPSConstellationSatellites> satellites = constellations();
    if (std::exchange(_notifiedConstellations, satellites) != satellites) {
        emit constellationsChanged();
    }
    const ReceiverState receiver = _receiverState();
    if (std::exchange(_notified.receiver, receiver) != receiver) {
        emit receiverChanged();
    }
    const QString status = statusText();
    if (std::exchange(_notified.statusText, status) != status) {
        emit statusTextChanged();
    }
    const QString label = summaryLabel();
    if (std::exchange(_notified.summaryLabel, label) != label) {
        emit summaryLabelChanged();
    }
    const QString recording = recordingFile();
    if (std::exchange(_notifiedRecordingFile, recording) != recording) {
        emit recordingFileChanged();
    }
}

void GPSReceiver::_resetStatus()
{
    _facts->_reset();
    _outputOverflowUs = 0;
    _outputOverflowTask.cancel();
    _outputOverflowWarning.clear();
}

void GPSReceiver::_clearStaleSolution()
{
    _facts->_clearSolution();
    _emitChanges();
}

QString GPSReceiver::errorMessage() const
{
    return _errorMessage.isEmpty() ? _outputOverflowWarning : _errorMessage;
}

QList<GPSConstellationSatellites> GPSReceiver::constellations() const
{
    return _facts->constellations();
}

void GPSReceiver::_updateOutputOverflow(uint64_t receiptUs)
{
    if (receiptUs == 0 || std::exchange(_outputOverflowUs, receiptUs) == receiptUs) {
        return;
    }
    _outputOverflowWarning =
        GPSReceiverMessages::outputOverflow(activeRole(), _session && _session->compactObservations());
    _outputOverflowTask.schedule(OUTPUT_OVERFLOW_WARNING_DURATION, [this]() {
        _outputOverflowWarning.clear();
        _emitChanges();
    });
}

void GPSReceiver::_onGPSConnect()
{
    _setError();
    _connection->receiverReady();
    GPSReceiverSession* const session = _session;
    _facts->_setConfigured(true);
    _setPersistentChangesAllowed(false);
    // Marking the status configured notifies its observers, one of which may have replaced the session that is ready.
    if (session && session == _session) {
        session->registerPosition(_positionManager, _positionHealth);
    }
    _emitChanges();
}

void GPSReceiver::_onReceiverDetected()
{
    _setPersistentChangesAllowed(false);
    _emitChanges();
}

void GPSReceiver::_onInputProblem(GPSInputProblem problem)
{
    _setError(GPSReceiverMessages::inputProblem(problem, _session ? _session->detectedType() : std::nullopt,
                                                activeEndpoint(), forwardingCorrections()));
    _emitChanges();
}

void GPSReceiver::_endSession(GPSConnectionError error, const QString& detail, GPSReceiverSessionEnd end)
{
    // The family an Automatic session detected, else the selected one.
    const auto* family = _session ? gpsReceiverDescriptorForManufacturer(_session->manufacturer()) : nullptr;
    GPSReceiverSession* const ended = _detachSession();
    _setPersistentChangesAllowed(false);
    const GPSReceiverSessionOutcome outcome = _connection->sessionEnded(end);
    const bool portRemoved = end == GPSReceiverSessionEnd::Unplugged;
    if (portRemoved) {
        qCDebug(GPSReceiverLog) << "Receiver serial device removed";
    }
    // Automatic connections never pass the flash-save consent this receiver asked for.
    if (outcome == GPSReceiverSessionOutcome::AutoRetrying && error == GPSConnectionError::ConsentRequired &&
        !detail.isEmpty() && family) {
        qCWarning(GPSReceiverLog) << "GPS receiver did not accept configuration";
        _setError(GPSReceiverMessages::consentRefused(detail, family->type));
    } else {
        if (outcome == GPSReceiverSessionOutcome::AutoRetrying || outcome == GPSReceiverSessionOutcome::Retrying) {
            qCWarning(GPSReceiverLog) << "GPS receiver session ended:" << error << detail;
        } else if (outcome == GPSReceiverSessionOutcome::None && !portRemoved) {
            logConnectionError(error, detail);
        }
        _setError(GPSReceiverMessages::sessionEnded(outcome, error, detail, end));
    }
    // Last, because the session's registrations notify as they end and the status Facts as they reset, and an
    // observer may connect or disconnect from there.
    _retire(ended);
    _resetStatus();
}

void GPSReceiver::_onBaseStatusUpdated(const GPSBaseStatus& status)
{
    _facts->_setBaseStatus(status);
    _emitChanges();
}

void GPSReceiver::_onSerialPortsScanned(const QStringList& devices)
{
    if (_destroying) {
        return;
    }
    if (_session && !_session->serialDevice().isEmpty() && !devices.contains(_session->serialDevice())) {
        _endSession(GPSConnectionError::DeviceError, {}, GPSReceiverSessionEnd::Unplugged);
        _emitChanges();
    }
}

bool GPSReceiver::connectSerial(const QString& device, GPSType type, GPSReceiverConfig request, GPSSerialClaim claim)
{
    const QString endpoint = device.trimmed();
    GPSReceiverWorker::TransportFactory transportFactory = _serial->transportFactory(endpoint, std::move(claim));
    if (!transportFactory) {
        return false;
    }
    return _connectReceiver(type, std::move(transportFactory), QStringLiteral("serial:%1").arg(endpoint),
                            std::move(request), endpoint, endpoint);
}

bool GPSReceiver::connectTcp(const QString& host, quint16 port, GPSType type, GPSReceiverConfig request)
{
    // One pass, so the host's text is not searched for place markers: an IPv6 zone reads like one.
    const QString endpoint = QStringLiteral("%1:%2").arg(host, QString::number(port));
    return _connectReceiver(
        type,
        [host, port](GPSCancelToken cancelToken) {
            return std::make_unique<TCPGPSTransport>(host, port, std::move(cancelToken));
        },
        QStringLiteral("tcp:%1").arg(endpoint), std::move(request), {}, endpoint);
}

bool GPSReceiver::connectUdp(quint16 port, GPSType type, GPSReceiverConfig request)
{
    //: %1 is the local UDP port the receiver's data arrives on
    const QString endpoint = tr("UDP port %1").arg(port);
    return _connectReceiver(
        type,
        [port](GPSCancelToken cancelToken) { return std::make_unique<UDPGPSTransport>(port, std::move(cancelToken)); },
        QStringLiteral("udp:%1").arg(port), std::move(request), {}, endpoint);
}

GPSReceiverPresentation GPSReceiver::configuredPresentation() const
{
    return gpsReceiverPresentation(_configuration.receiverRole == RTKSettings::ConfiguredBase
                                       ? _configuration.baseReceiverManufacturer
                                       : gpsReceiverManufacturerForType(GPSType::Passive));
}

GPSReceiver::State GPSReceiver::state() const
{
    if (!hasReceiver()) {
        return reconnecting() ? State::Reconnecting : State::Disconnected;
    }
    return _facts->telemetryAvailable() ? State::Connected : State::Connecting;
}

bool GPSReceiver::hasReceiver() const
{
    return _session && _session->hasWorker();
}

int GPSReceiver::_activeManufacturer() const
{
    return _session ? _session->manufacturer() : GPS_AUTOMATIC_MANUFACTURER;
}

std::optional<BaseModeDefinition::Mode> GPSReceiver::_activeBaseMode() const
{
    return _session ? _session->baseMode() : std::nullopt;
}

GPSBaseState GPSReceiver::_activeBaseState() const
{
    return _session ? _session->baseState() : GPSBaseState::Inactive;
}

int GPSReceiver::activeBaseMode() const
{
    const std::optional<BaseModeDefinition::Mode> mode = _activeBaseMode();
    return mode ? static_cast<int>(*mode) : -1;
}

QString GPSReceiver::activeEndpoint() const
{
    return _session ? _session->endpoint() : QString();
}

QString GPSReceiver::receiverIdentity() const
{
    return _session ? _session->identity() : QString();
}

QString GPSReceiver::detectedReceiver() const
{
    const std::optional<GPSType> detected = _session ? _session->detectedType() : std::nullopt;
    if (!detected) {
        return {};
    }
    return activeRole() == RTKSettings::Passive ? gpsInputProtocolName(*detected) : gpsReceiverName(*detected);
}

RTKSettings::ReceiverRole GPSReceiver::activeRole() const
{
    return _session ? _session->role() : RTKSettings::ConfiguredBase;
}

bool GPSReceiver::forwardingCorrections() const
{
    return _session && _session->forwardsCorrections();
}

GPSReceiverPresentation GPSReceiver::activePresentation() const
{
    return gpsReceiverPresentation(_activeManufacturer());
}

void GPSReceiver::_scanSerialPorts()
{
    // The policy asks for the serial ports only without a receiver, and a scan that is due notifies observers, who
    // may connect, disconnect or change the configuration. Scanned first, they act before the policy decides, and
    // its questions reuse the scan.
    if (!hasReceiver()) {
        _serial->scanIfDue();
    }
}

bool GPSReceiver::connectReceiver()
{
    if (_destroying || _shuttingDown) {
        return false;
    }
    _scanSerialPorts();
    const bool allowed = _persistentChangesAllowed && configuredPresentation().persistentConfiguration;
    _setPersistentChangesAllowed(false);
    const bool connected = _connection->connectConfigured(allowed);
    _emitChanges();
    return connected;
}

void GPSReceiver::disconnectReceiver()
{
    if (_destroying) {
        return;
    }
    _setPersistentChangesAllowed(false);
    _connection->disconnectConfigured();
    _emitChanges();
}

void GPSReceiver::startConnectionPolling(std::function<bool()> suspended)
{
    if (_destroying) {
        return;
    }
    (void) _pollTask.scheduleRepeating(POLL_INTERVAL, [this, suspended = std::move(suspended)]() {
        if (!suspended || !suspended()) {
            tick();
        }
    });
}

void GPSReceiver::tick()
{
    if (_destroying) {
        return;
    }
    _scanSerialPorts();
    // Other connections claim and release their ports without a scan. Their marks are renewed before the policy
    // decides, as the scan is: observers of the entries may act too.
    _serial->refreshEntries();
    _connection->update();
    _emitChanges();
}

bool GPSReceiver::reconnecting() const
{
    return _connection && _connection->reconnecting();
}

std::optional<GPSObservation> GPSReceiver::acceptedPositionObservation(GPSObservation::PositionUse use) const
{
    return _positionHealth->acceptedObservation(use);
}

bool GPSReceiver::_connectReceiver(GPSType type, GPSReceiverWorker::TransportFactory transportFactory,
                                   const QString& sourceInstance, GPSReceiverConfig request,
                                   const QString& serialDevice, const QString& endpoint)
{
    _retireSession();
    _resetStatus();
    _setError();
    transportFactory = _recorded(std::move(transportFactory), type);
    const RTKSettings::ReceiverRole role =
        type == GPSType::Passive ? RTKSettings::Passive : RTKSettings::ConfiguredBase;
    auto* const session = new GPSReceiverSession(
        ++_sessionCount,
        {.type = type,
         .role = role,
         .forwardsCorrections = role == RTKSettings::ConfiguredBase || _configuration.forwardReceiverRtcm,
         .config = std::move(request),
         .serialDevice = serialDevice,
         .endpoint = endpoint},
        this);
    session->registerCorrections(_correctionManager, sourceInstance);
    _session = session;
    connect(session, &GPSReceiverSession::satelliteInfoUpdated, this, &GPSReceiver::_onSatelliteInfoUpdated);
    connect(session, &GPSReceiverSession::positionUpdated, this, &GPSReceiver::_onPositionUpdated);
    connect(session, &GPSReceiverSession::baseStatusUpdated, this, &GPSReceiver::_onBaseStatusUpdated);
    connect(session, &GPSReceiverSession::receiverReady, this, &GPSReceiver::_onGPSConnect);
    connect(session, &GPSReceiverSession::receiverDetected, this, &GPSReceiver::_onReceiverDetected);
    connect(session, &GPSReceiverSession::inputProblem, this, &GPSReceiver::_onInputProblem);
    connect(session, &GPSReceiverSession::ended, this, [this](GPSConnectionError error, const QString& detail) {
        _endSession(error, detail, GPSReceiverSessionEnd::Reported);
        _emitChanges();
    });
    if (!session->start(_workerFactory, std::move(transportFactory))) {
        _retireSession();
        _setError(tr("Failed to create the receiver session."));
        return false;
    }
    _setPersistentChangesAllowed(false);
    return true;
}

void GPSReceiver::_retireSession()
{
    _retire(_detachSession());
}

GPSReceiverSession* GPSReceiver::_detachSession()
{
    GPSReceiverSession* const detached = std::exchange(_session, nullptr);
    if (detached) {
        _retiredWorkers.removeIf([](const QPointer<GPSReceiverWorker>& worker) { return worker.isNull(); });
        if (QPointer<GPSReceiverWorker> worker = detached->worker()) {
            _retiredWorkers.append(std::move(worker));
        }
        detached->deleteLater();
    }
    return detached;
}

void GPSReceiver::_retire(GPSReceiverSession* session)
{
    if (session) {
        session->retire();
    }
    _positionHealth->reset();
}

void GPSReceiver::shutdown()
{
    if (_destroying) {
        return;
    }
    // Ending the connection notifies observers; a session one of them started would outlive the shutdown.
    const QScopedValueRollback shuttingDown(_shuttingDown, true);
    _pollTask.cancel();
    _connection->reset();
    endConnection(ShownError::Keep);
    _joinRetiredWorkers();
    _emitChanges();
}

void GPSReceiver::_joinRetiredWorkers()
{
    // At application exit the event loop that deletes finished workers has stopped, so they are joined here.
    const QDeadlineTimer deadline(SHUTDOWN_TIMEOUT);
    for (const QPointer<GPSReceiverWorker>& worker : std::exchange(_retiredWorkers, {})) {
        if (!worker) {
            continue;
        }
        if (worker->wait(deadline)) {
            delete worker.data();
        } else {
            qCWarning(GPSReceiverLog) << "GPS receiver worker did not stop within" << SHUTDOWN_TIMEOUT.count() << "s";
        }
    }
}

void GPSReceiver::endConnection(ShownError error)
{
    _retireSession();
    _resetStatus();
    _setPersistentChangesAllowed(false);
    if (error == ShownError::Clear) {
        _setError();
    }
}

void GPSReceiver::_onSatelliteInfoUpdated(const GPSSatelliteReport& msg)
{
    const int inView = msg.inView.value_or(-1);
    const int used = msg.used.value_or(-1);
    qCDebug(GPSReceiverLog) << QStringLiteral("%1 in view, %2 used")
                                   .arg(inView)
                                   .arg(msg.used ? QString::number(used) : QStringLiteral("unknown"));
    _facts->_setSatellites(msg);
    _emitChanges();
}

void GPSReceiver::_onPositionUpdated(const GPSPositionReport& report)
{
    if (gpsFixQualityFromValue(_facts->fixType()->rawValue().toInt()) != report.navigation.fixType) {
        qCDebug(GPSReceiverLog) << "Receiver fix changed:" << report.navigation.fixType;
    }
    GPSReceiverSession* const session = _session;
    _updateOutputOverflow(report.integrity.outputOverflowUs);
    // Drivers project integrity to Unknown once it is older than its freshness window.
    _facts->_setSolution(report.navigation.fixType, report.integrity);
    // The status Facts notify as they are written, and an observer of one may have ended or replaced the session that
    // reported.
    if (session && session == _session) {
        _positionHealth->updateObservation(session->observation(report, _scheduler->nowUs()));
    }
    _emitChanges();
}

QString GPSReceiver::statusText() const
{
    return GPSReceiverMessages::receiverStatus({.state = state(),
                                                .identifying = activePresentation().automatic,
                                                .role = activeRole(),
                                                .forwardingCorrections = forwardingCorrections(),
                                                .baseMode = _activeBaseMode(),
                                                .baseState = _activeBaseState(),
                                                .correctionsWithheld = _session && _session->correctionsWithheld()});
}

QString GPSReceiver::summaryLabel() const
{
    if (hasReceiver() && activeRole() == RTKSettings::ConfiguredBase) {
        if (_activeBaseState() == GPSBaseState::Active) {
            return tr("Survey", "Base survey-in in progress");
        }
        return _session->correctionsWithheld()
                   ? tr("Waiting", "Base without a valid position, which sends no corrections")
                   : tr("Base");
    }
    return GPSReceiverMessages::fixLabel(gpsFixQualityFromValue(_facts->fixType()->rawValue().toInt()));
}

void GPSReceiver::setRecordSessions(bool record)
{
    if (_recordSessions != record) {
        _recordSessions = record;
        emit recordSessionsChanged();
    }
}

QString GPSReceiver::recordingFile() const
{
    const bool stopped = _recordingStopped && _recordingStopped->load();
    return _session && !stopped ? _recordingFile : QString();
}

GPSReceiverWorker::TransportFactory GPSReceiver::_recorded(GPSReceiverWorker::TransportFactory transportFactory,
                                                           GPSType type)
{
    _recordingFile.clear();
    _recordingStopped.reset();
    if (!_recordSessions) {
        return transportFactory;
    }
    const QString directory = _recordingDirectory ? _recordingDirectory() : QString();
    if (directory.isEmpty()) {
        qCWarning(GPSReceiverLog) << "No folder to record the receiver session to";
        return transportFactory;
    }
    _recordingFile =
        GPSRecordingTransport::sessionFile(directory, type, _recordingStartTimes.next(QDateTime::currentDateTimeUtc()));
    _recordingStopped = std::make_shared<std::atomic_bool>(false);
    return [factory = std::move(transportFactory), file = _recordingFile,
            stopped = _recordingStopped](GPSCancelToken cancelToken) -> std::unique_ptr<GPSTransport> {
        std::unique_ptr<GPSTransport> transport = factory(std::move(cancelToken));
        if (!transport) {
            return transport;
        }
        return std::make_unique<GPSRecordingTransport>(
            std::move(transport), file, GPSRecordingTransport::MAX_FILE_BYTES, [stopped]() { stopped->store(true); });
    };
}
