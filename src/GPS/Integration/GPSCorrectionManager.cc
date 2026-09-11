#include "GPSCorrectionManager.h"

#include "GPSCorrectionSettings.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionManagerLog, "GPS.Corrections.GPSCorrectionManager")

GPSCorrectionManager::GPSCorrectionManager(QObject* parent)
    : QObject(parent)
    , _router(this)
    , _eventModel(this)
    , _rtcmMavlink(this)
    , _udpInput(0, this)
{
    qCDebug(GPSCorrectionManagerLog) << this;
    connect(&_router, &GPSCorrectionRouter::sourceSelected, this, &GPSCorrectionManager::selectedSourceChanged);
    connect(&_router, &GPSCorrectionRouter::sourceInvalidated, this, &GPSCorrectionManager::selectedSourceChanged);
    connect(&_router, &GPSCorrectionRouter::frameRouted, this, &GPSCorrectionManager::correctionRouted);
    _router.setFanoutSink(QStringLiteral("mavlink"), [this](const GPSCorrectionFrame& frame) {
        QList<GPSCorrectionRouter::Admission> results;
        for (const auto& admission : _rtcmMavlink.submitToOutputs(frame.data)) {
            results.append(
                {admission.id,
                 {admission.queuedBytes, admission.session,
                  admission.complete ? GPSCorrectionReason::None : GPSCorrectionReason::DestinationUnavailable},
                 admission.complete});
        }
        if (results.isEmpty()) {
            results.append({QStringLiteral("mavlink"), {}, false});
        }
        return results;
    });
    _diagnosticsTimer.setSingleShot(true);
    _diagnosticsTimer.setInterval(100);
    connect(&_diagnosticsTimer, &QTimer::timeout, this, &GPSCorrectionManager::_refreshDiagnostics);
    _healthTimer.setInterval(1000);
    connect(&_healthTimer, &QTimer::timeout, this, &GPSCorrectionManager::_refreshDiagnostics);
    _healthTimer.start();
}

GPSCorrectionManager::~GPSCorrectionManager()
{
    qCDebug(GPSCorrectionManagerLog) << this;
    shutdown();
}

void GPSCorrectionManager::configureNtripUdpOutput(bool enabled, const QString& address, quint16 port)
{
    const QString id = QStringLiteral("ntripUdp");
    if (_shutdown) {
        return;
    }
    if (enabled && _ntripUdpOutput.isEnabled() && _ntripUdpOutput.address() == address &&
        _ntripUdpOutput.port() == port) {
        return;
    }
    _router.removeSink(id);
    _ntripUdpOutput.stop();
    if (enabled && _ntripUdpOutput.configure(address, port)) {
        _router.setSourceSink(id, GPSCorrectionSource::Ntrip, [this](const GPSCorrectionFrame& frame) {
            return static_cast<quint64>(_ntripUdpOutput.forward(frame.data));
        });
    }
    _scheduleSourcesChanged();
}

void GPSCorrectionManager::init(GPSCorrectionSettings* settings)
{
    if (_settings || !settings || _shutdown) {
        return;
    }
    _settings = settings;
    for (const Fact* fact :
         {settings->rtcmUdpInputEnabled(), settings->rtcmUdpInputPort(), settings->rtcmUdpValidate()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSCorrectionManager::_applyUdpInputSettings);
    }
    _applyUdpInputSettings();
}

void GPSCorrectionManager::_applyUdpInputSettings()
{
    if (!_settings || _shutdown) {
        return;
    }
    const QPointer<GPSCorrectionManager> guard(this);
    const quint64 revision = ++_udpConfigurationRevision;
    const bool enabled = _settings->rtcmUdpInputEnabled()->rawValue().toBool();
    const bool validate = _settings->rtcmUdpValidate()->rawValue().toBool();
    const quint16 port = static_cast<quint16>(_settings->rtcmUdpInputPort()->rawValue().toUInt());
    const auto current = [this, guard, revision]() {
        return guard && !_shutdown && _udpConfigurationRevision == revision;
    };
    _udpRegistration.reset();
    if (!current()) {
        return;
    }
    _udpInput.stop();
    if (!current()) {
        return;
    }
    disconnect(&_udpInput, nullptr, this, nullptr);
    _udpInput.setValidation(validate);
    _udpInput.setPort(port);
    if (!current() || !enabled) {
        return;
    }
    auto registration = registerSource(GPSCorrectionSource::Udp);
    if (!current()) {
        return;
    }
    _udpRegistration = std::move(registration);
    const auto token = _udpRegistration.token();
    connect(&_udpInput, &RTCMUdpInput::frameReceived, this, [this, token](const GPSCorrectionFrame& frame) {
        acceptIngress(token.event(frame.data, frame.receivedAtMs, frame.messageId, frame.validated, frame.filtered,
                                  GPSCorrectionReason::None, frame.sourceInstance));
    });
    connect(&_udpInput, &RTCMUdpInput::frameRejected, this,
            [this, token](const GPSCorrectionFrame& frame, GPSCorrectionReason reason) {
                acceptIngress(token.event(frame.data, frame.receivedAtMs, frame.messageId, false, frame.filtered,
                                          reason, frame.sourceInstance));
            });
    const bool started = _udpInput.start();
    if (current() && !started) {
        _udpRegistration.reset();
    }
}

void GPSCorrectionManager::applyRoutingConfiguration(const RoutingConfiguration& configuration)
{
    const QPointer<GPSCorrectionManager> guard(this);
    _router.applyConfiguration(
        {static_cast<GPSCorrectionRouter::Policy>(configuration.policy), configuration.source, configuration.instance});
    if (guard) {
        _scheduleSourcesChanged();
    }
}

GPSCorrectionSourceRegistration GPSCorrectionManager::registerSource(GPSCorrectionSource source,
                                                                     const QString& instance)
{
    const QPointer<GPSCorrectionManager> guard(this);
    auto registration = _router.registerSource(source, instance);
    if (guard) {
        _scheduleSourcesChanged();
    }
    return registration;
}

void GPSCorrectionManager::acceptIngress(const GPSCorrectionIngress& ingress)
{
    const QPointer<GPSCorrectionManager> guard(this);
    _router.acceptIngress(ingress);
    if (guard) {
        _scheduleSourcesChanged();
    }
}

void GPSCorrectionManager::setSelectedSource(GPSCorrectionSource source)
{
    applyRoutingConfiguration({source == GPSCorrectionSource::Unknown ? RoutingPolicy::All : RoutingPolicy::Manual,
                               source, _router.selectedInstance()});
}

void GPSCorrectionManager::setSelectedInstance(const QString& instance)
{
    applyRoutingConfiguration({routingPolicy(), _router.selectedSource(), instance});
}

void GPSCorrectionManager::setRoutingPolicy(RoutingPolicy policy)
{
    applyRoutingConfiguration({policy, _router.selectedSource(), _router.selectedInstance()});
}

GPSCorrectionManager::RoutingPolicy GPSCorrectionManager::routingPolicy() const
{
    return static_cast<RoutingPolicy>(_router.policy());
}

void GPSCorrectionManager::addSink(const QString& id, GPSCorrectionRouter::Sink sink)
{
    _router.setSink(id, std::move(sink));
}

void GPSCorrectionManager::removeSink(const QString& id)
{
    _router.removeSink(id);
}

void GPSCorrectionManager::addDetailedSink(const QString& id, GPSCorrectionRouter::DetailedSink sink,
                                           bool reportsWrites)
{
    _router.setDetailedSink(id, std::move(sink), reportsWrites);
    _scheduleSourcesChanged();
}

void GPSCorrectionManager::recordDeliveries(const QList<GPSCorrectionDelivery>& deliveries)
{
    for (const auto& delivery : deliveries) {
        _router.recordDelivery(delivery);
    }
    _scheduleSourcesChanged();
}

void GPSCorrectionManager::invalidateDestination(const QString& id, quint64 destinationSession)
{
    _router.invalidateDestination(id, destinationSession);
    _scheduleSourcesChanged();
}

void GPSCorrectionManager::_refreshDiagnostics()
{
    const QPointer<GPSCorrectionManager> guard(this);
    _eventModel.setEvents(_router.events());
    if (guard) {
        _refreshSourceInstances();
    }
    if (guard) {
        emit sourcesChanged();
    }
}

void GPSCorrectionManager::_scheduleSourcesChanged()
{
    if (!_shutdown && !_diagnosticsTimer.isActive()) {
        _diagnosticsTimer.start();
    }
}

QVariantList GPSCorrectionManager::sources() const
{
    QVariantList result;
    const qint64 now = _router.nowMs();
    const auto& statsBySource = _router.statistics();
    for (int index = 0; index < static_cast<int>(statsBySource.size()); ++index) {
        const auto& stats = statsBySource[index];
        result.append(QVariantMap{
            {QStringLiteral("source"), index},
            {QStringLiteral("session"), QVariant::fromValue(stats.session)},
            {QStringLiteral("active"), stats.active},
            {QStringLiteral("receivedBytes"), QVariant::fromValue(stats.receivedBytes)},
            {QStringLiteral("receivedFrames"), QVariant::fromValue(stats.receivedFrames)},
            {QStringLiteral("validatedBytes"), QVariant::fromValue(stats.validatedBytes)},
            {QStringLiteral("selectedFrames"), QVariant::fromValue(stats.selectedFrames)},
            {QStringLiteral("selectedBytes"), QVariant::fromValue(stats.selectedBytes)},
            {QStringLiteral("queuedFrames"), QVariant::fromValue(stats.queuedFrames)},
            {QStringLiteral("queuedBytes"), QVariant::fromValue(stats.queuedBytes)},
            {QStringLiteral("writtenFrames"), QVariant::fromValue(stats.writtenFrames)},
            {QStringLiteral("writtenBytes"), QVariant::fromValue(stats.writtenBytes)},
            {QStringLiteral("transportAcceptedBytes"), QVariant::fromValue(stats.transportAcceptedBytes)},
            {QStringLiteral("droppedFrames"), QVariant::fromValue(stats.droppedFrames)},
            {QStringLiteral("droppedBytes"), QVariant::fromValue(stats.droppedBytes)},
            {QStringLiteral("unconfirmedFrames"), QVariant::fromValue(stats.unconfirmedFrames)},
            {QStringLiteral("unconfirmedBytes"), QVariant::fromValue(stats.unconfirmedBytes)},
            {QStringLiteral("validatedFrames"), QVariant::fromValue(stats.validatedFrames)},
            {QStringLiteral("filteredFrames"), QVariant::fromValue(stats.filteredFrames)},
            {QStringLiteral("routedFrames"), QVariant::fromValue(stats.routedFrames)},
            {QStringLiteral("submittedBytes"), QVariant::fromValue(stats.submittedBytes)},
            {QStringLiteral("ageMs"), stats.lastValidMs > 0 ? now - stats.lastValidMs : qint64(-1)},
            {QStringLiteral("usable"), stats.active && stats.lastValidMs > 0 &&
                                           now - stats.lastValidMs < GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS}});
    }
    return result;
}

void GPSCorrectionManager::_refreshSourceInstances()
{
    const auto instances = sourceInstances();
    if (instances != _lastSourceInstances) {
        _lastSourceInstances = instances;
        emit sourceInstancesChanged();
    }
}

QVariantList GPSCorrectionManager::sourceInstances() const
{
    QVariantList result;
    const qint64 now = _router.nowMs();
    for (const auto& source : _router.sources()) {
        const bool usable = source.lastRoutableMs > 0 && now >= source.lastRoutableMs &&
                            now - source.lastRoutableMs < GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
        const bool selected =
            usable && (_router.policy() == GPSCorrectionRouter::Policy::All ||
                       (source.category == _router.activeSource() && source.instance == activeInstance()));
        result.append(QVariantMap{{QStringLiteral("source"), static_cast<int>(source.category)},
                                  {QStringLiteral("instanceId"), source.instance},
                                  {QStringLiteral("session"), QVariant::fromValue(source.session)},
                                  {QStringLiteral("active"), true},
                                  {QStringLiteral("usable"), usable},
                                  {QStringLiteral("selected"), selected}});
    }
    return result;
}

QVariantList GPSCorrectionManager::destinations() const
{
    QVariantList result;
    for (const auto& destination : _router.destinations()) {
        result.append(QVariantMap{
            {QStringLiteral("destinationId"), destination.id},
            {QStringLiteral("destinationSession"), QVariant::fromValue(destination.session)},
            {QStringLiteral("reportsWrites"), destination.reportsWrites},
            {QStringLiteral("queuedFrames"), QVariant::fromValue(destination.queuedFrames)},
            {QStringLiteral("queuedBytes"), QVariant::fromValue(destination.queuedBytes)},
            {QStringLiteral("writtenFrames"), QVariant::fromValue(destination.writtenFrames)},
            {QStringLiteral("writtenBytes"), QVariant::fromValue(destination.writtenBytes)},
            {QStringLiteral("transportAcceptedBytes"), QVariant::fromValue(destination.transportAcceptedBytes)},
            {QStringLiteral("droppedFrames"), QVariant::fromValue(destination.droppedFrames)},
            {QStringLiteral("droppedBytes"), QVariant::fromValue(destination.droppedBytes)},
            {QStringLiteral("unconfirmedFrames"), QVariant::fromValue(destination.unconfirmedFrames)},
            {QStringLiteral("unconfirmedBytes"), QVariant::fromValue(destination.unconfirmedBytes)},
            {QStringLiteral("pendingFrames"), QVariant::fromValue(destination.pendingFrames)},
            {QStringLiteral("pendingBytes"), QVariant::fromValue(destination.pendingBytes)}});
    }
    return result;
}

void GPSCorrectionManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    _healthTimer.stop();
    _diagnosticsTimer.stop();
    _ntripUdpOutput.stop();
    _router.shutdown();
    _udpInput.stop();
    _refreshDiagnostics();
}
