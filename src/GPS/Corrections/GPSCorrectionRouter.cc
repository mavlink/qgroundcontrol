#include "GPSCorrectionRouter.h"

#include <algorithm>
#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>

#include "QGCLoggingCategory.h"
#include "RTCMFrame.h"

QGC_LOGGING_CATEGORY(GPSCorrectionRouterLog, "GPS.Corrections.GPSCorrectionRouter")

GPSCorrectionRouter::GPSCorrectionRouter(QObject* parent, Clock clock)
    : QObject(parent)
    , _clock(clock ? std::move(clock) : Clock(GPSCorrectionFrame::monotonicNowMs))
    , _ledger(_clock)
{
    qCDebug(GPSCorrectionRouterLog) << this;
}

GPSCorrectionRouter::~GPSCorrectionRouter()
{
    qCDebug(GPSCorrectionRouterLog) << this;
}

int GPSCorrectionRouter::_sourceIndex(GPSCorrectionSource source)
{
    const int index = static_cast<int>(source);
    return index >= 0 && index < 4 ? index : -1;
}

quint64 GPSCorrectionRouter::beginSourceSession(GPSCorrectionSource source, const QString& instance)
{
    const int index = _sourceIndex(source);
    if (index < 0 || _shutdown) {
        return 0;
    }
    const QPointer<GPSCorrectionRouter> guard(this);
    const quint64 revisionAfterEnd = _revision + 1;
    endSourceSession(source);
    if (!guard || _shutdown || _revision != revisionAfterEnd) {
        return 0;
    }
    const quint64 session = _ledger.beginSource(source);
    _configuredInstances[index] = instance;
    return session;
}

void GPSCorrectionRouter::endSourceSession(GPSCorrectionSource source)
{
    const int index = _sourceIndex(source);
    if (index < 0) {
        return;
    }
    ++_revision;
    _ledger.endSource(source);
    _selector.retire(source, _clock());
    if (_lastSubmittedSource.startsWith(QString::number(index) + QLatin1Char('/'))) {
        _lastSubmittedSource.clear();
        emit sourceInvalidated();
    }
}

quint64 GPSCorrectionRouter::sourceSession(GPSCorrectionSource source) const
{
    const int index = _sourceIndex(source);
    return index < 0 ? 0 : _ledger.statistics()[index].session;
}

QString GPSCorrectionRouter::sourceInstance(GPSCorrectionSource source) const
{
    const int index = _sourceIndex(source);
    return index < 0 ? QString() : _configuredInstances[index];
}

QVariantList GPSCorrectionRouter::sourceDiagnostics() const
{
    const qint64 nowMs = _clock();
    QVariantList result;
    const auto& statistics = _ledger.statistics();
    for (int index = 0; index < static_cast<int>(statistics.size()); ++index) {
        const auto& stats = statistics[index];
        const qint64 age = GPSCorrectionFrame::ageMs(stats.lastValidMs, nowMs);
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
            {QStringLiteral("routedFrames"), QVariant::fromValue(stats.selectedFrames)},
            {QStringLiteral("submittedBytes"), QVariant::fromValue(stats.submittedBytes)},
            {QStringLiteral("ageMs"), age},
            {QStringLiteral("usable"), stats.active && age >= 0 && age < GPSCorrectionSelector::FRESHNESS_TIMEOUT_MS}});
    }
    return result;
}

QVariantList GPSCorrectionRouter::sourceInstanceDiagnostics() const
{
    const qint64 nowMs = _clock();
    QVariantList result;
    const auto activeSource = _selector.activeSource(nowMs);
    const auto activeInstance = _selector.activeInstance(nowMs);
    for (const auto& source : _selector.sources()) {
        const qint64 age = GPSCorrectionFrame::ageMs(source.lastRoutableMs, nowMs);
        const bool usable = age >= 0 && age < GPSCorrectionSelector::FRESHNESS_TIMEOUT_MS;
        const bool selected = usable && (_selector.configuration().policy == GPSCorrectionSelector::Policy::All ||
                                         (source.category == activeSource && source.instance == activeInstance));
        result.append(QVariantMap{{QStringLiteral("source"), static_cast<int>(source.category)},
                                  {QStringLiteral("instanceId"), source.instance},
                                  {QStringLiteral("session"), QVariant::fromValue(source.session)},
                                  {QStringLiteral("active"), true},
                                  {QStringLiteral("usable"), usable},
                                  {QStringLiteral("selected"), selected}});
    }
    return result;
}

QVariantList GPSCorrectionRouter::destinationDiagnostics() const
{
    QVariantList result;
    for (const auto& destination : _ledger.destinations()) {
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

void GPSCorrectionRouter::applyConfiguration(const Configuration& configuration)
{
    if (_shutdown || _sourceIndex(configuration.source) < 0 ||
        (configuration.policy != Policy::Automatic && configuration.policy != Policy::Manual &&
         configuration.policy != Policy::All) ||
        configuration == this->configuration()) {
        return;
    }
    ++_revision;
    _selector.configure(configuration, _clock());
    if (!_lastSubmittedSource.isEmpty()) {
        _lastSubmittedSource.clear();
        emit sourceInvalidated();
    }
}

void GPSCorrectionRouter::setPolicy(Policy policy)
{
    applyConfiguration({policy, selectedSource(), selectedInstance()});
}

void GPSCorrectionRouter::setSelectedSource(GPSCorrectionSource source, const QString& instance)
{
    applyConfiguration({policy(), source, instance});
}

GPSCorrectionSourceRegistration GPSCorrectionRouter::registerSource(GPSCorrectionSource source, const QString& instance)
{
    const QPointer<GPSCorrectionRouter> guard(this);
    const quint64 session = beginSourceSession(source, instance);
    return guard && session ? GPSCorrectionSourceRegistration(GPSCorrectionSourceToken(this, source, session, instance))
                            : GPSCorrectionSourceRegistration();
}

bool GPSCorrectionRouter::isCurrentSource(GPSCorrectionSource source, quint64 session, const QString& instance) const
{
    const int index = _sourceIndex(source);
    return !_shutdown && index > 0 && session && _ledger.statistics()[index].active &&
           _ledger.statistics()[index].session == session && _configuredInstances[index] == instance;
}

bool GPSCorrectionRouter::acceptIngress(const GPSCorrectionIngress& ingress)
{
    const auto& token = ingress.token();
    if (!token.belongsTo(this) || !token.valid() ||
        (!token.instance().isEmpty() && ingress.frame().sourceInstance != token.instance())) {
        return false;
    }
    auto frame = ingress.frame();
    if (ingress.rejection() != GPSCorrectionReason::None) {
        recordRejectedFrame(frame, ingress.rejection());
        return false;
    }
    if ((!frame.validated && frame.source != GPSCorrectionSource::Udp) ||
        (frame.validated && !RTCM::isValidFrame(frame.data))) {
        frame.validated = false;
        recordRejectedFrame(frame, GPSCorrectionReason::InvalidFrame);
        return false;
    }
    return acceptFrame(frame);
}

void GPSCorrectionRouter::setSink(const QString& id, Sink sink)
{
    setOutput(id, admissionOnlyOutput(id, GPSCorrectionSource::Unknown, std::move(sink)));
}

void GPSCorrectionRouter::setSourceSink(const QString& id, GPSCorrectionSource source, Sink sink)
{
    if (_sourceIndex(source) <= 0) {
        return;
    }
    setOutput(id, admissionOnlyOutput(id, source, std::move(sink)));
}

GPSCorrectionRouter::Output GPSCorrectionRouter::admissionOnlyOutput(const QString& id, GPSCorrectionSource scope,
                                                                     Sink sink)
{
    if (!sink) {
        return {};
    }
    return {scope, Completion::AdmissionOnly, [id, sink = std::move(sink)](const GPSCorrectionFrame& frame) {
                const quint64 queued = sink(frame);
                return QList<Admission>{
                    {id,
                     {queued, 0, queued ? GPSCorrectionReason::None : GPSCorrectionReason::DestinationUnavailable},
                     true}};
            }};
}

void GPSCorrectionRouter::setDetailedSink(const QString& id, DetailedSink sink, bool reportsWrites)
{
    if (!sink) {
        removeSink(id);
        return;
    }
    setOutput(id, {{},
                   reportsWrites ? Completion::Reported : Completion::AdmissionOnly,
                   [id, sink = std::move(sink)](const GPSCorrectionFrame& frame) {
                       return QList<Admission>{{id, sink(frame), true}};
                   }});
}

void GPSCorrectionRouter::setFanoutSink(const QString& id, FanoutSink sink)
{
    setOutput(id, {{}, Completion::AdmissionOnly, std::move(sink)});
}

void GPSCorrectionRouter::setOutput(const QString& id, Output output)
{
    if (id.isEmpty()) {
        return;
    }
    if (!output.admit) {
        removeSink(id);
        return;
    }
    ++_revision;
    _sinks.insert(id, std::move(output));
    _ledger.registerOutput(id, _sinks[id].completion == Completion::Reported);
}

void GPSCorrectionRouter::removeSink(const QString& id)
{
    ++_revision;
    _sinks.remove(id);
    _ledger.removeOutput(id, _admission ? _admission->deliveryId : 0);
    _deferRetirement({RetirementKind::Output, id});
}

bool GPSCorrectionRouter::recordDelivery(const GPSCorrectionDelivery& delivery)
{
    if (_admission && delivery.deliveryId == _admission->deliveryId &&
        _admission->deliveries.size() < MAX_PENDING_DELIVERIES) {
        _admission->deliveries.append(delivery);
        return true;
    }
    return _ledger.recordDelivery(delivery);
}

void GPSCorrectionRouter::invalidateDestination(const QString& id, quint64 session)
{
    _ledger.invalidateDestination(id, session, _admission ? _admission->deliveryId : 0);
    _deferRetirement({RetirementKind::Destination, id, session});
}

void GPSCorrectionRouter::_deferRetirement(Retirement retirement)
{
    if (!_admission || retirement.id.isEmpty() || _admission->retireDelivery ||
        _admission->retirements.contains(retirement)) {
        return;
    }
    if (_admission->retirements.size() < MAX_PENDING_DELIVERIES) {
        _admission->retirements.append(std::move(retirement));
    } else {
        _admission->retireDelivery = true;
        qCWarning(GPSCorrectionRouterLog) << "Retirement limit reached; settling in-flight admissions as unconfirmed";
    }
}

void GPSCorrectionRouter::_finishAdmission()
{
    const auto admission = std::exchange(_admission, std::nullopt);
    if (!admission) {
        return;
    }
    for (const auto& delivery : admission->deliveries) {
        _ledger.recordDelivery(delivery);
    }
    for (const auto& retirement : admission->retirements) {
        switch (retirement.kind) {
            case RetirementKind::Destination:
                _ledger.invalidateDestination(retirement.id, retirement.session);
                break;
            case RetirementKind::Output:
                _ledger.invalidateDelivery(admission->deliveryId, retirement.id);
                break;
        }
    }
    if (admission->retireDelivery) {
        _ledger.invalidateDelivery(admission->deliveryId);
    }
}

void GPSCorrectionRouter::recordRejectedFrame(GPSCorrectionFrame frame, GPSCorrectionReason reason)
{
    if (_shutdown || frame.data.isEmpty()) {
        return;
    }
    frame.deliveryId = ++_nextDelivery;
    if (frame.sourceInstance.isEmpty()) {
        frame.sourceInstance = sourceInstance(frame.source);
    }
    _ledger.received(frame);
    _ledger.filtered(frame);
    _ledger.recordDrop(frame, reason, frame.data.size());
}

bool GPSCorrectionRouter::acceptFrame(GPSCorrectionFrame frame)
{
    const int index = _sourceIndex(frame.source);
    if (_shutdown || _submitting || index < 0 || frame.data.isEmpty()) {
        return false;
    }
    frame.deliveryId = ++_nextDelivery;
    const auto& stats = _ledger.statistics()[index];
    if (!stats.active || stats.session != frame.session) {
        _ledger.recordEvent(
            frame, GPSCorrectionStage::Dropped,
            stats.session != frame.session ? GPSCorrectionReason::SessionMismatch : GPSCorrectionReason::InactiveSource,
            frame.data.size());
        return false;
    }
    if (frame.sourceInstance.isEmpty()) {
        frame.sourceInstance = _configuredInstances[index];
    } else if (!_configuredInstances[index].isEmpty() && frame.sourceInstance != _configuredInstances[index]) {
        _ledger.recordEvent(frame, GPSCorrectionStage::Dropped, GPSCorrectionReason::SessionMismatch,
                            frame.data.size());
        return false;
    }
    _ledger.received(frame);
    const qint64 now = _clock();
    const qint64 age = GPSCorrectionFrame::ageMs(frame.receivedAtMs, now);
    if (age < 0) {
        _ledger.filtered(frame);
        _ledger.recordDrop(frame, GPSCorrectionReason::InvalidTimestamp, frame.data.size());
        return false;
    }
    if (frame.validated) {
        _ledger.validated(frame);
    }
    _selector.observe(frame, false, now);
    if (frame.filtered || age >= FRESHNESS_TIMEOUT_MS) {
        _ledger.filtered(frame);
        _ledger.recordDrop(frame, frame.filtered ? GPSCorrectionReason::MessageFiltered : GPSCorrectionReason::Expired,
                           frame.data.size());
        return false;
    }
    _selector.observe(frame, true, now);
    if (frame.validated && frame.messageId == 0 && frame.data.size() >= 8 &&
        static_cast<quint8>(frame.data[0]) == 0xD3) {
        frame.messageId = (static_cast<quint8>(frame.data[3]) << 4) | (static_cast<quint8>(frame.data[4]) >> 4);
    }
    const bool selected = _selector.selected(frame, now);
    if (!selected) {
        _ledger.filtered(frame);
        _ledger.recordDrop(frame, GPSCorrectionReason::NotSelected, frame.data.size());
    } else {
        _ledger.selected(frame);
    }
    return _submit(frame, selected);
}

bool GPSCorrectionRouter::_submit(const GPSCorrectionFrame& frame, bool selected)
{
    const QString key = GPSCorrectionSelector::key(frame.source, frame.sourceInstance);
    const QPointer<GPSCorrectionRouter> guard(this);
    const quint64 revision = _revision;
    const auto sinks = _sinks;
    _submitting = true;
    auto finishSubmitting = qScopeGuard([guard] {
        if (guard) {
            guard->_submitting = false;
        }
    });
    const QString submissionSource = key + QLatin1Char('#') + QString::number(frame.session);
    if (selected && _lastSubmittedSource != submissionSource) {
        _lastSubmittedSource = submissionSource;
        emit sourceSelected(frame.source, frame.sourceInstance);
        if (!guard) {
            return false;
        }
        if (_shutdown || revision != _revision) {
            return false;
        }
    }
    quint64 logicalQueuedBytes = 0;
    bool completeSubmission = false;
    bool attempted = false;
    GPSCorrectionReason submissionFailure = GPSCorrectionReason::DestinationUnavailable;
    for (auto it = sinks.cbegin(); it != sinks.cend(); ++it) {
        if (it->scope == GPSCorrectionSource::Unknown ? !selected : it->scope != frame.source) {
            continue;
        }
        attempted = true;
        if (it->completion == Completion::Reported && !_ledger.admissionAvailable()) {
            submissionFailure = GPSCorrectionReason::DiagnosticsBackpressure;
            _ledger.recordDrop(frame, submissionFailure, frame.data.size(), it.key(), 0, false);
            continue;
        }
        _admission.emplace();
        _admission->deliveryId = frame.deliveryId;
        const auto finishAdmission = qScopeGuard([guard] {
            if (guard) {
                guard->_finishAdmission();
            }
        });
        const QList<Admission> admissions = it->admit(frame);
        if (!guard) {
            return false;
        }
        // Callback retirement must not discard returned admission evidence.
        for (const auto& admission : admissions) {
            if (admission.destination.isEmpty()) {
                continue;
            }
            const auto& submitted = admission.submission;
            const quint64 bytes = (std::min) (submitted.queuedBytes, static_cast<quint64>(frame.data.size()));
            const bool complete = admission.complete && bytes == static_cast<quint64>(frame.data.size());
            if (!_ledger.admitted(frame, it.key(), admission.destination, submitted.destinationSession, bytes, complete,
                                  it->completion == Completion::Reported)) {
                continue;
            }
            logicalQueuedBytes = (std::max) (logicalQueuedBytes, bytes);
            completeSubmission |= complete;
            if (!complete) {
                submissionFailure = submitted.reason == GPSCorrectionReason::None
                                        ? GPSCorrectionReason::DestinationUnavailable
                                        : submitted.reason;
                _ledger.recordDrop(frame, submissionFailure, frame.data.size() - bytes, admission.destination,
                                   submitted.destinationSession, false);
            }
        }
        if (_shutdown || revision != _revision) {
            for (const auto& admission : admissions) {
                _deferRetirement(
                    {RetirementKind::Destination, admission.destination, admission.submission.destinationSession});
            }
            break;
        }
    }
    _ledger.queued(frame, logicalQueuedBytes, completeSubmission);
    if (attempted && !completeSubmission) {
        _ledger.recordDrop(frame, submissionFailure, frame.data.size() - logicalQueuedBytes);
    }
    _ledger.pruneDestinationHistory();
    _submitting = false;
    finishSubmitting.dismiss();
    if (selected && !_shutdown && revision == _revision) {
        emit frameRouted(frame);
    }
    return selected;
}

void GPSCorrectionRouter::shutdown()
{
    ++_revision;
    _shutdown = true;
    _selector.clear();
    _sinks.clear();
    _ledger.shutdown(_admission ? _admission->deliveryId : 0);
    if (_admission) {
        _admission->retireDelivery = true;
    }
}
