#include "GPSReceiver.h"

#include "GPSCorrectionManager.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverMessages.h"
#include "GPSReceiverSession.h"
#include "GPSSourceHealth.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"
#include "TCPGPSTransport.h"
#include "UDPGPSTransport.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#include "SerialPortManager.h"
#endif

#include <algorithm>
#include <type_traits>
#include <utility>
#include <variant>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QSet>

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

}  // namespace

GPSReceiver::GPSReceiver(QObject* parent, RuntimeScheduler* scheduler, const Dependencies& dependencies)
    : QObject(parent)
    , _correctionManager(dependencies.corrections)
    , _positionManager(dependencies.positions)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _positionHealth(new GPSSourceHealth(this, _scheduler))
    , _outputOverflowTask(_scheduler, this)
    , _pollTask(_scheduler, this)
#ifndef QGC_NO_SERIAL_LINK
    , _serialPorts(dependencies.serialPorts)
#endif
{
    qCDebug(GPSReceiverLog) << this;
    // A silent receiver stays connected, so its last solution must not remain on display.
    connect(_positionHealth, &GPSSourceHealth::positionChanged, this, [this]() {
        if (_positionHealth->state() == GPSSourceHealth::State::Stale) {
            _clearStaleSolution();
        }
    });
    _connection = std::make_unique<GPSReceiverConnectionPolicy>(static_cast<GPSReceiverConnector&>(*this),
                                                                _configuration, _scheduler);
    _facts = new GPSReceiverFactGroup(this);
    for (Fact* fact : {_facts->fixType(), _facts->active()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSReceiver::_updateLabels);
    }
    connect(_facts, &FactGroup::telemetryAvailableChanged, this, &GPSReceiver::_updateLabels);
    connect(this, &GPSReceiver::receiverChanged, this, &GPSReceiver::_updateLabels);
    _updateLabels();
    _notified.receiver = _receiverState();
#ifndef QGC_NO_SERIAL_LINK
    _serialTransportFactory = [](const QString& device, GPSCancelToken cancelToken) {
        return std::make_unique<SerialGPSTransport>(device, std::move(cancelToken));
    };
    if (_serialPorts) {
        connect(_serialPorts, &SerialPortManager::portsEnumerated, this, &GPSReceiver::_onSerialPortsEnumerated);
    }
#endif
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

bool GPSReceiver::connectionSupported() const
{
    return _configuration.receiverRole != RTKSettings::ConfiguredBase || effectiveConnectionType() != RTKSettings::Udp;
}

bool GPSReceiver::baseModeSupported() const
{
    const GPSReceiverPresentation presentation =
        capabilitiesFor(_configuration.receiverRole, _configuration.baseReceiverManufacturer);
    if (presentation.passive) {
        return true;
    }
    return std::visit(
        [&presentation]<typename Mode>(const Mode&) {
            if constexpr (std::is_same_v<Mode, GPSBaseStationConfig::Fixed>) {
                return presentation.rtkBase;
            } else if constexpr (std::is_same_v<Mode, GPSBaseStationConfig::SurveyIn>) {
                return presentation.surveyIn;
            } else {
                return presentation.receiverAveraging;
            }
        },
        _configuration.base.mode);
}

bool GPSReceiver::canConnect() const
{
    const GPSReceiverPresentation presentation =
        capabilitiesFor(_configuration.receiverRole, _configuration.baseReceiverManufacturer);
    return (presentation.specificReceiver || presentation.automatic) && baseModeSupported() && connectionSupported();
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
    return {.hasReceiver = hasReceiver(),
            .baseMode = activeBaseMode(),
            .endpoint = activeEndpoint(),
            .identity = receiverIdentity(),
            .detected = detectedReceiver(),
            .reconnecting = reconnecting(),
            .presentation = activePresentation(),
            .role = activeRole(),
            .forwardingCorrections = forwardingCorrections()};
}

void GPSReceiver::_emitChanges()
{
    if (_destroying) {
        return;
    }
    // Starting or stopping to reconnect the saved receiver is a session change.
    if (reconnecting() != _notified.receiver.reconnecting) {
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
    const ReceiverState receiver = _receiverState();
    if (std::exchange(_notified.receiver, receiver) != receiver) {
        emit receiverChanged();
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
    _facts->_setConfigured(true);
    _setPersistentChangesAllowed(false);
    if (_session) {
        _session->registerPosition(_positionManager, _positionHealth);
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

void GPSReceiver::_endSession(GPSConnectionError error, const QString& detail, bool portRemoved)
{
    // The family an Automatic session detected, else the selected one.
    const auto* family = _session ? gpsReceiverDescriptorForManufacturer(_session->manufacturer()) : nullptr;
    _retireSession();
    _resetStatus();
    _setPersistentChangesAllowed(false);
    const GPSReceiverSessionOutcome outcome = _connection->sessionEnded(portRemoved);
    if (portRemoved) {
        qCDebug(GPSReceiverLog) << "Receiver serial device removed";
    }
    // Automatic connections never pass the flash-save consent this receiver asked for.
    if (outcome == GPSReceiverSessionOutcome::AutoRetrying && error == GPSConnectionError::ConsentRequired &&
        !detail.isEmpty() && family) {
        qCWarning(GPSReceiverLog) << "GPS receiver did not accept configuration";
        _setError(GPSReceiverMessages::consentRefused(detail, family->type));
        return;
    }
    if (outcome == GPSReceiverSessionOutcome::AutoRetrying || outcome == GPSReceiverSessionOutcome::Retrying) {
        qCWarning(GPSReceiverLog) << "GPS receiver session ended:" << error << detail;
    } else if (outcome == GPSReceiverSessionOutcome::None && !portRemoved) {
        logConnectionError(error, detail);
    }
    _setError(GPSReceiverMessages::sessionEnded(outcome, error, detail, portRemoved));
}

void GPSReceiver::_onGPSSurveyReport(const GPSSurveyReport& status)
{
    _facts->_setSurvey(status);
    _emitChanges();
}

#ifndef QGC_NO_SERIAL_LINK
void GPSReceiver::_onSerialPortsEnumerated(const QStringList& devices)
{
    if (_destroying) {
        return;
    }
    if (_session && !_session->serialDevice().isEmpty() && !devices.contains(_session->serialDevice())) {
        _endSession(GPSConnectionError::DeviceError, {}, true);
        _emitChanges();
    }
    _updateSerialPortEntries();
}

bool GPSReceiver::serialPortAvailable(const QString& device) const
{
    if (device.isEmpty() || !_serialPorts) {
        return false;
    }
    const QList<SerialPortManager::Port> ports = _serialPorts->availablePorts();
    return std::any_of(ports.cbegin(), ports.cend(),
                       [&device](const auto& port) { return port.systemLocation == device && !port.bootloader; });
}

QStringList GPSReceiver::rtkReceiverPorts() const
{
    QStringList receivers;
    if (!_serialPorts) {
        return receivers;
    }
    QSet<QString> seenDevices;
    for (const auto& port : _serialPorts->availablePorts()) {
        if (port.boardType != QGCSerialPortInfo::BoardTypeRTKGPS || port.bootloader) {
            continue;
        }
        // A labelled NMEA interface remains a receiver candidate on composite GPS devices.
        const bool primary = SerialPortManager::isPrimaryInterface(port, seenDevices);
        if ((primary || port.description.contains(QStringLiteral("NMEA"))) &&
            _serialPorts->canReservePort(port.systemLocation)) {
            receivers.append(port.systemLocation);
        }
    }
    return receivers;
}

GPSSerialClaim GPSReceiver::claimSerialPort(const QString& device)
{
    GPSSerialClaim claim = _serialPorts ? _serialPorts->reservePort(device) : nullptr;
    if (claim) {
        _serialClaims.insert(device, claim);
    }
    return claim;
}

void GPSReceiver::_updateSerialPortEntries()
{
    // Just after a scan the manager returns that inventory without scanning again.
    const QList<SerialPortManager::Port> available = _serialPorts->availablePorts();
    QList<GPSSerialPortEntry> entries;
    entries.reserve(available.size());
    for (const auto& port : available) {
        // The USB product string, else the name of a known board.
        const QString name = port.description.trimmed().isEmpty() ? port.boardName : port.description.trimmed();
        //: %1 is a serial device's name, such as u-blox GNSS receiver; %2 its path, such as /dev/ttyACM0 or COM3
        QString label = name.isEmpty() ? port.systemLocation : tr("%1 – %2").arg(name, port.systemLocation);
        if (_serialPorts->isPortReserved(port.systemLocation) && _serialClaims.value(port.systemLocation).expired()) {
            //: %1 is a serial device that another QGroundControl connection, such as a vehicle link, uses
            label = tr("%1 (in use)").arg(label);
        }
        entries.append({.value = port.systemLocation, .label = label});
    }
    if (entries != _serialPortEntries) {
        _serialPortEntries = std::move(entries);
        emit serialPortEntriesChanged();
    }
}

bool GPSReceiver::connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges,
                                GPSSerialClaim claim)
{
    const QString endpoint = device.trimmed();
    return _connectReceiver(
        type,
        [endpoint, claim = std::move(claim), factory = _serialTransportFactory](GPSCancelToken cancelToken) {
            return factory(endpoint, std::move(cancelToken));
        },
        QStringLiteral("serial:%1").arg(endpoint), baudRate, allowPersistentChanges, endpoint, endpoint);
}
#endif

bool GPSReceiver::connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges)
{
    const QString endpoint = QStringLiteral("%1:%2").arg(host).arg(port);
    return _connectReceiver(
        type,
        [host, port](GPSCancelToken cancelToken) {
            return std::make_unique<TCPGPSTransport>(host, port, std::move(cancelToken));
        },
        QStringLiteral("tcp:%1").arg(endpoint), GPSTransport::BRIDGE_BAUDRATE, allowPersistentChanges, {}, endpoint);
}

bool GPSReceiver::connectUdp(quint16 port, GPSType type)
{
    //: %1 is the local UDP port the receiver's data arrives on
    const QString endpoint = tr("UDP port %1").arg(port);
    return _connectReceiver(
        type,
        [port](GPSCancelToken cancelToken) { return std::make_unique<UDPGPSTransport>(port, std::move(cancelToken)); },
        QStringLiteral("udp:%1").arg(port), GPSTransport::BRIDGE_BAUDRATE, false, {}, endpoint);
}

QList<int> GPSReceiver::serialBaudRates()
{
    QList<int> rates;
#ifndef QGC_NO_SERIAL_LINK
    for (const QString& text : SerialPortManager::supportedBaudRates()) {
        const uint32_t rate = text.toUInt();
        if (gpsValidBaudRate(rate, true) && !rates.contains(static_cast<int>(rate))) {
            rates.append(static_cast<int>(rate));
        }
    }
#endif
    return rates;
}

GPSReceiverPresentation GPSReceiver::capabilitiesFor(int role, int manufacturer) const
{
    return gpsReceiverPresentation(
        role == RTKSettings::ConfiguredBase ? manufacturer : gpsReceiverManufacturerForType(GPSType::passive));
}

bool GPSReceiver::hasReceiver() const
{
    return _session && _session->hasWorker();
}

int GPSReceiver::_activeManufacturer() const
{
    return _session ? _session->manufacturer() : 0;
}

int GPSReceiver::activeBaseMode() const
{
    return _session ? _session->baseMode() : -1;
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

bool GPSReceiver::connectReceiver()
{
    if (_destroying) {
        return false;
    }
    const bool allowed =
        _persistentChangesAllowed &&
        capabilitiesFor(_configuration.receiverRole, _configuration.baseReceiverManufacturer).persistentConfiguration;
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
                                   const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                                   const QString& serialDevice, const QString& endpoint)
{
    QString configError;
    GPSReceiverConfig config =
        gpsReceiverConfigFor(_configuration.base, type, baudRate, allowPersistentChanges, &configError);
    if (!configError.isEmpty()) {
        _setError(configError);
        return false;
    }
    _retireSession();
    _resetStatus();
    _setError();
    const RTKSettings::ReceiverRole role =
        type == GPSType::passive ? RTKSettings::Passive : RTKSettings::ConfiguredBase;
    auto* const session = new GPSReceiverSession(
        ++_sessionCount,
        {.type = type,
         .role = role,
         .forwardsCorrections = role == RTKSettings::ConfiguredBase || _configuration.forwardReceiverRtcm,
         .config = std::move(config),
         .serialDevice = serialDevice,
         .endpoint = endpoint},
        this);
    session->registerCorrections(_correctionManager, sourceInstance);
    _session = session;
    connect(session, &GPSReceiverSession::satelliteInfoUpdated, this, &GPSReceiver::_onSatelliteInfoUpdated);
    connect(session, &GPSReceiverSession::positionUpdated, this, &GPSReceiver::_onPositionUpdated);
    connect(session, &GPSReceiverSession::surveyInStatusUpdated, this, &GPSReceiver::_onGPSSurveyReport);
    connect(session, &GPSReceiverSession::receiverReady, this, &GPSReceiver::_onGPSConnect);
    connect(session, &GPSReceiverSession::receiverDetected, this, &GPSReceiver::_onReceiverDetected);
    connect(session, &GPSReceiverSession::inputProblem, this, &GPSReceiver::_onInputProblem);
    connect(session, &GPSReceiverSession::ended, this, [this](GPSConnectionError error, const QString& detail) {
        _endSession(error, detail, false);
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
    if (auto* const retired = std::exchange(_session, nullptr)) {
        _retiredWorkers.removeIf([](const QPointer<GPSReceiverWorker>& worker) { return worker.isNull(); });
        if (QPointer<GPSReceiverWorker> worker = retired->worker()) {
            _retiredWorkers.append(std::move(worker));
        }
        retired->deleteLater();
        retired->retire();
    }
    _positionHealth->reset();
}

void GPSReceiver::shutdown()
{
    if (_destroying) {
        return;
    }
    _pollTask.cancel();
    _connection->reset();
    endConnection(false);
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

void GPSReceiver::endConnection(bool clearError)
{
    _retireSession();
    _resetStatus();
    _setPersistentChangesAllowed(false);
    if (clearError) {
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
    _facts->_setSatellites(inView, used);
    _emitChanges();
}

void GPSReceiver::_onPositionUpdated(const GPSPositionReport& report)
{
    if (gpsFixQualityFromValue(_facts->fixType()->rawValue().toInt()) != report.navigation.fixType) {
        qCDebug(GPSReceiverLog) << "Receiver fix changed:" << report.navigation.fixType;
    }
    // Drivers project integrity to Unknown once it is older than its freshness window.
    _facts->_setSolution(report.navigation.fixType, report.integrity);
    _updateOutputOverflow(report.integrity.outputOverflowUs);
    if (_session) {
        _positionHealth->updateObservation(_session->observation(report, _scheduler->nowUs()));
    }
    _emitChanges();
}

void GPSReceiver::_updateLabels()
{
    const QString status = GPSReceiverMessages::receiverStatus({.configured = _facts->telemetryAvailable(),
                                                                .hasReceiver = hasReceiver(),
                                                                .identifying = activePresentation().automatic,
                                                                .reconnecting = reconnecting(),
                                                                .role = activeRole(),
                                                                .forwardingCorrections = forwardingCorrections(),
                                                                .baseMode = activeBaseMode(),
                                                                .surveyActive = _facts->active()->rawValue().toBool()});
    if (std::exchange(_statusText, status) != status) {
        emit statusTextChanged();
    }

    QString label;
    if (hasReceiver() && activeRole() == RTKSettings::ConfiguredBase) {
        label = _facts->active()->rawValue().toBool() ? tr("Survey", "Base survey-in in progress") : tr("Base");
    } else {
        label = GPSReceiverMessages::fixLabel(gpsFixQualityFromValue(_facts->fixType()->rawValue().toInt()));
    }
    if (std::exchange(_summaryLabel, label) != label) {
        emit summaryLabelChanged();
    }
}
