#include "GPSSourceBindings.h"

#include <utility>

#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSPositionSettings.h"
#include "GPSReceiverState.h"
#include "NMEASourceManager.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(GPSSourceBindingsLog, "GPS.Integration.GPSSourceBindings")

GPSSourceBindings::GPSSourceBindings(SettingsManager& settings, GPSPositionService* positions,
                                     GPSReceiverState& receiver, NMEASourceManager& nmea,
                                     GPSCorrectionManager& corrections, QObject* parent)
    : QObject(parent)
    , _settings(settings)
    , _positionManager(positions)
    , _receiver(receiver)
    , _receiverSession(receiver.session())
    , _nmeaSources(&nmea)
    , _corrections(corrections)
{
    qCDebug(GPSSourceBindingsLog) << this;
    connect(&_receiverSession, &GPSReceiverSession::configurationStarted, this,
            &GPSSourceBindings::_startReceiverCorrections);
    connect(&_receiverSession, &GPSReceiverSession::stateChanged, this, [this]() {
        if (!_receiverSession.hasReceiver()) {
            _stopReceiverCorrections();
        }
    });
    connect(&_receiverSession, &GPSReceiverSession::connectionError, this,
            &GPSSourceBindings::_stopReceiverCorrections);
    connect(&_receiverSession, &GPSReceiverSession::rtcmFrameReceived, this,
            [this](const QByteArray& data, qint64 receivedAtMs, quint64 sessionId) {
                if (_shutdown || !sessionId || sessionId != _receiverCorrectionAttemptId ||
                    sessionId != _receiverSession.sessionId() || !_receiverSession.hasReceiver()) {
                    return;
                }
                const auto token = _receiverCorrectionSource.token();
                _corrections.acceptIngress(token.event(data, receivedAtMs, 0, true));
            });
    connect(&nmea, &NMEASourceManager::positionSourceChanged, this, &GPSSourceBindings::_updateNmeaPositionSource);
    connect(&receiver.session(), &GPSReceiverSession::attemptChanged, this, &GPSSourceBindings::_updatePositionSource);
    connect(settings.rtkSettings()->useReceiverPosition(), &Fact::rawValueChanged, this,
            &GPSSourceBindings::_updatePositionSource);
    connect(settings.gpsPositionSettings()->sourceMode(), &Fact::rawValueChanged, this,
            &GPSSourceBindings::_updatePositionSourceMode);
    const QPointer<GPSSourceBindings> guard(this);
    _updatePositionSourceMode();
    if (!guard) {
        return;
    }
    _updatePositionSource();
    if (!guard) {
        return;
    }
    _updateNmeaPositionSource();
    if (guard) {
        _startReceiverCorrections();
    }
}

GPSSourceBindings::~GPSSourceBindings()
{
    qCDebug(GPSSourceBindingsLog) << this;
    shutdown();
}

void GPSSourceBindings::init(NTRIPManager* ntrip)
{
    if (_initialized || _shutdown) {
        return;
    }
    const QPointer<GPSSourceBindings> guard(this);
    _initialized = true;
    auto* correctionSettings = _settings.gpsCorrectionSettings();
    _corrections.init(correctionSettings);
    if (!guard || _shutdown) {
        return;
    }
    for (Fact* fact : {correctionSettings->correctionSource(), correctionSettings->correctionSourceInstance(),
                       correctionSettings->injectLocalReceiver()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSSourceBindings::_updateCorrectionSettings);
    }
    _routingChanges = connect(&_corrections, &GPSCorrectionManager::selectedSourceChanged, &_receiverSession,
                              &GPSReceiverSession::clearPendingCorrections);
    _updateCorrectionSettings();
    if (!guard || _shutdown) {
        return;
    }
    _deliveryReports = connect(&_receiverSession, &GPSReceiverSession::correctionDeliveriesReady, &_corrections,
                               &GPSCorrectionManager::recordDeliveries);
    const auto invalidateDestination = [this]() {
        if (_correctionDestinationSession != 0) {
            const quint64 retiredSession = std::exchange(_correctionDestinationSession, 0);
            _corrections.invalidateDestination(QStringLiteral("localReceiver"), retiredSession);
        }
    };
    connect(&_receiverSession, &GPSReceiverSession::disconnected, this, invalidateDestination);
    connect(&_receiverSession, &GPSReceiverSession::stateChanged, this, [this, invalidateDestination]() {
        if (!_receiverSession.hasReceiver() || _correctionDestinationSession != _receiverSession.sessionId()) {
            invalidateDestination();
        }
    });
    _ntrip = ntrip;
    if (_ntrip) {
        auto* ntripSettings = _settings.ntripSettings();
        for (Fact* fact : {ntripSettings->ntripServerConnectEnabled(), ntripSettings->ntripUdpForwardEnabled(),
                           ntripSettings->ntripUdpTargetAddress(), ntripSettings->ntripUdpTargetPort()}) {
            connect(fact, &Fact::rawValueChanged, this, &GPSSourceBindings::_updateNtripUdpOutput);
        }
        _updateNtripUdpOutput();
        if (!guard || _shutdown) {
            return;
        }
        connect(_ntrip, &NTRIPManager::correctionSessionStarted, this, &GPSSourceBindings::_startNtripCorrections);
        connect(_ntrip, &NTRIPManager::correctionSessionEnded, this, &GPSSourceBindings::_stopNtripCorrections);
        connect(_ntrip, &NTRIPManager::correctionRejectedAt, this,
                [this](const QByteArray& data, int messageId, qint64 receivedAtMs, quint64 attemptId) {
                    if (!_shutdown && _ntrip && attemptId != 0 && attemptId == _ntripAttemptId &&
                        attemptId == _ntrip->correctionAttemptId()) {
                        const auto token = _ntripCorrectionSource.token();
                        _corrections.acceptIngress(
                            token.event(data, receivedAtMs, messageId, false, true, GPSCorrectionReason::InvalidFrame));
                    }
                });
        connect(_ntrip, &NTRIPManager::correctionReceivedAt, this,
                [this](const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs, quint64 attemptId) {
                    if (!_shutdown && _ntrip && attemptId != 0 && attemptId == _ntripAttemptId &&
                        attemptId == _ntrip->correctionAttemptId()) {
                        const auto token = _ntripCorrectionSource.token();
                        _corrections.acceptIngress(token.event(data, receivedAtMs, messageId, true, filtered));
                    }
                });
        _startNtripCorrections(_ntrip->correctionAttemptId(), _ntrip->correctionSourceId());
    }
}

void GPSSourceBindings::_startReceiverCorrections()
{
    if (_shutdown || !_receiverSession.hasReceiver()) {
        return;
    }
    const quint64 attempt = _receiverSession.sessionId();
    const quint64 revision = ++_receiverCorrectionRevision;
    const QPointer<GPSSourceBindings> guard(this);
    _receiverCorrectionAttemptId = attempt;
    _receiverCorrectionSource.reset();
    if (!guard || _shutdown || revision != _receiverCorrectionRevision || attempt != _receiverSession.sessionId() ||
        !_receiverSession.hasReceiver()) {
        return;
    }
    auto registration = _corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    if (guard && !_shutdown && revision == _receiverCorrectionRevision && attempt == _receiverSession.sessionId()) {
        _receiverCorrectionSource = std::move(registration);
    }
}

void GPSSourceBindings::_stopReceiverCorrections()
{
    ++_receiverCorrectionRevision;
    _receiverCorrectionAttemptId = 0;
    _receiverCorrectionSource.reset();
}

void GPSSourceBindings::_startNtripCorrections(quint64 attemptId, const QString& sourceId)
{
    if (_shutdown || !_ntrip || !attemptId || attemptId != _ntrip->correctionAttemptId()) {
        return;
    }
    const quint64 revision = ++_ntripCorrectionRevision;
    const QPointer<GPSSourceBindings> guard(this);
    _ntripAttemptId = attemptId;
    _ntripCorrectionSource.reset();
    if (!guard || _shutdown || revision != _ntripCorrectionRevision || !_ntrip ||
        attemptId != _ntrip->correctionAttemptId()) {
        return;
    }
    auto registration = _corrections.registerSource(GPSCorrectionSource::Ntrip, sourceId);
    if (guard && !_shutdown && revision == _ntripCorrectionRevision && _ntrip &&
        attemptId == _ntrip->correctionAttemptId()) {
        _ntripCorrectionSource = std::move(registration);
    }
}

void GPSSourceBindings::_stopNtripCorrections(quint64 attemptId)
{
    if (attemptId == _ntripAttemptId) {
        ++_ntripCorrectionRevision;
        _ntripAttemptId = 0;
        _ntripCorrectionSource.reset();
    }
}

void GPSSourceBindings::_updatePositionSource()
{
    const QPointer<QObject> source = !_shutdown && _positionManager && _receiverSession.attempt().ready() &&
                                             _settings.rtkSettings()->useReceiverPosition()->rawValue().toBool()
                                         ? &_receiver
                                         : nullptr;
    const quint64 session = source ? _receiverSession.sessionId() : 0;
    _updatePositionBinding(_receiverBinding, GPSPositionService::SelectedSource::Receiver, source,
                           source ? _receiver.health() : nullptr, session);
}

void GPSSourceBindings::_updateNmeaPositionSource()
{
    const QPointer<QObject> source =
        !_shutdown && _positionManager && _nmeaSources->positionSource() ? _nmeaSources : nullptr;
    const quint64 session = source ? _nmeaSources->sessionId() : 0;
    _updatePositionBinding(_nmeaBinding, GPSPositionService::SelectedSource::Nmea, source,
                           source ? _nmeaSources->health() : nullptr, session);
}

void GPSSourceBindings::_updatePositionBinding(PositionBinding& binding, GPSPositionService::SelectedSource kind,
                                               QObject* producer, GPSSourceHealth* health, quint64 session)
{
    const QPointer<QObject> source(producer);
    const QPointer<GPSSourceHealth> guardedHealth(health);
    if (binding.source == source && binding.session == session) {
        return;
    }
    const quint64 revision = ++binding.revision;
    const QPointer<GPSSourceBindings> guard(this);
    binding.source = source;
    binding.session = session;
    binding.registration.reset();
    if (!guard || revision != binding.revision || !source || !_positionManager || _shutdown) {
        return;
    }
    auto registration = _positionManager->registerPositionSource(kind, source, guardedHealth, session);
    if (guard && revision == binding.revision && !_shutdown) {
        if (!registration) {
            binding.source = nullptr;
            binding.session = 0;
        }
        binding.registration = std::move(registration);
    }
}

void GPSSourceBindings::_updatePositionSourceMode()
{
    if (!_shutdown && _positionManager) {
        _positionManager->setSourceMode(static_cast<GPSPositionService::SourceMode>(
            _settings.gpsPositionSettings()->sourceMode()->rawValue().toInt()));
    }
}

void GPSSourceBindings::_updateCorrectionSettings()
{
    if (_shutdown) {
        return;
    }
    const QPointer<GPSSourceBindings> guard(this);
    const quint64 revision = ++_correctionSettingsRevision;
    auto* settings = _settings.gpsCorrectionSettings();
    const bool injectReceiver = settings->injectLocalReceiver()->rawValue().toBool();
    const int source = settings->correctionSource()->rawValue().toInt();
    GPSCorrectionManager::RoutingConfiguration configuration;
    configuration.instance = settings->correctionSourceInstance()->rawValue().toString();
    if (source >= GPSCorrectionSettings::LocalReceiver && source <= GPSCorrectionSettings::Udp) {
        configuration.source = source == GPSCorrectionSettings::LocalReceiver ? GPSCorrectionSource::LocalReceiver
                               : source == GPSCorrectionSettings::Ntrip       ? GPSCorrectionSource::Ntrip
                                                                              : GPSCorrectionSource::Udp;
        configuration.policy = GPSCorrectionManager::RoutingPolicy::Manual;
    } else {
        configuration.policy = source == GPSCorrectionSettings::All ? GPSCorrectionManager::RoutingPolicy::All
                                                                    : GPSCorrectionManager::RoutingPolicy::Automatic;
    }
    _receiverSession.clearPendingCorrections();
    if (!guard || _shutdown || revision != _correctionSettingsRevision) {
        return;
    }
    _corrections.applyRoutingConfiguration(configuration);
    if (!guard || _shutdown || revision != _correctionSettingsRevision) {
        return;
    }
    if (injectReceiver) {
        _corrections.addDetailedSink(QStringLiteral("localReceiver"), [this](const GPSCorrectionFrame& frame) {
            const quint64 session = _receiverSession.sessionId();
            if (_shutdown || frame.source == GPSCorrectionSource::LocalReceiver || !frame.validated ||
                !_settings.gpsCorrectionSettings()->injectLocalReceiver()->rawValue().toBool() ||
                !_receiverSession.readyForCorrections()) {
                return GPSCorrectionRouter::Submission{0, session, GPSCorrectionReason::DestinationUnavailable};
            }
            _correctionDestinationSession = session;
            const auto result = _receiverSession.submitCorrections(frame, session);
            return GPSCorrectionRouter::Submission{
                result.accepted ? static_cast<quint64>(frame.data.size()) : 0, session,
                result.accepted ? GPSCorrectionReason::None : gpsCorrectionReason(result.outcome)};
        });
    } else {
        _corrections.removeSink(QStringLiteral("localReceiver"));
    }
}

void GPSSourceBindings::_updateNtripUdpOutput()
{
    if (_shutdown) {
        return;
    }
    auto* settings = _settings.ntripSettings();
    const bool enabled = _ntrip && settings->ntripServerConnectEnabled()->rawValue().toBool() &&
                         settings->ntripUdpForwardEnabled()->rawValue().toBool();
    _corrections.configureNtripUdpOutput(enabled, settings->ntripUdpTargetAddress()->rawValue().toString(),
                                         settings->ntripUdpTargetPort()->rawValue().toUInt());
}

void GPSSourceBindings::stop()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    const QPointer<GPSSourceBindings> guard(this);
    _stopReceiverCorrections();
    if (!guard) {
        return;
    }
    _stopNtripCorrections(_ntripAttemptId);
    if (!guard) {
        return;
    }
    ++_receiverBinding.revision;
    ++_nmeaBinding.revision;
    _receiverBinding.source = nullptr;
    _nmeaBinding.source = nullptr;
    _receiverBinding.registration.reset();
    if (!guard) {
        return;
    }
    _nmeaBinding.registration.reset();
    if (!guard) {
        return;
    }
    if (_ntrip) {
        _ntrip->disconnect(this);
    }
    _ntrip = nullptr;
}

void GPSSourceBindings::shutdown()
{
    if (_sinkRetired) {
        return;
    }
    const QPointer<GPSSourceBindings> guard(this);
    stop();
    if (!guard) {
        return;
    }
    _sinkRetired = true;
    disconnect(_routingChanges);
    disconnect(_deliveryReports);
    _corrections.removeSink(QStringLiteral("localReceiver"));
}
