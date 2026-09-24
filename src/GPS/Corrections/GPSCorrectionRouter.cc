#include "GPSCorrectionRouter.h"

#include <algorithm>
#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>

#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"

QGC_LOGGING_CATEGORY(GPSCorrectionRouterLog, "GPS.Corrections.GPSCorrectionRouter")
QGC_LOGGING_CATEGORY(GPSCorrectionSelectorLog, "GPS.Corrections.GPSCorrectionSelector")

namespace {
QVariantMap admissionDiagnostics(const GPSCorrectionLedger::AdmissionCounters& counters, QVariantMap details)
{
    details.insert({
        {QStringLiteral("queuedFrames"), QVariant::fromValue(counters.queuedFrames)},
        {QStringLiteral("queuedBytes"), QVariant::fromValue(counters.queuedBytes)},
        {QStringLiteral("droppedFrames"), QVariant::fromValue(counters.droppedFrames)},
        {QStringLiteral("droppedBytes"), QVariant::fromValue(counters.droppedBytes)},
    });
    return details;
}
}  // namespace

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
    if (_lastSubmittedStream && _lastSubmittedStream->source.category == source) {
        _lastSubmittedStream.reset();
        emit sourceInvalidated();
    }
}

QVariantList GPSCorrectionRouter::sourceDiagnostics() const
{
    const qint64 nowMs = _clock();
    QVariantList result;
    const auto& statistics = _ledger.statistics();
    for (int index = 0; index < static_cast<int>(statistics.size()); ++index) {
        const auto& stats = statistics[index];
        const qint64 age = GPSCorrectionFrame::ageMs(stats.lastValidMs, nowMs);
        // Freshness is published as a state rather than an age so unchanged diagnostics stay equal.
        result.append(admissionDiagnostics(
            stats, {{QStringLiteral("source"), index},
                    {QStringLiteral("session"), QVariant::fromValue(stats.session)},
                    {QStringLiteral("active"), stats.active},
                    {QStringLiteral("receivedBytes"), QVariant::fromValue(stats.receivedBytes)},
                    {QStringLiteral("receivedFrames"), QVariant::fromValue(stats.receivedFrames)},
                    {QStringLiteral("validatedBytes"), QVariant::fromValue(stats.validatedBytes)},
                    {QStringLiteral("selectedFrames"), QVariant::fromValue(stats.selectedFrames)},
                    {QStringLiteral("selectedBytes"), QVariant::fromValue(stats.selectedBytes)},
                    {QStringLiteral("validatedFrames"), QVariant::fromValue(stats.validatedFrames)},
                    {QStringLiteral("filteredFrames"), QVariant::fromValue(stats.filteredFrames)},
                    {QStringLiteral("routedFrames"), QVariant::fromValue(stats.selectedFrames)},
                    {QStringLiteral("submittedBytes"), QVariant::fromValue(stats.submittedBytes)},
                    {QStringLiteral("usable"),
                     stats.active && age >= 0 && age < GPSCorrectionSelector::FRESHNESS_TIMEOUT_MS}}));
    }
    return result;
}

QVariantList GPSCorrectionRouter::sourceInstanceDiagnostics() const
{
    const qint64 nowMs = _clock();
    QVariantList result;
    const auto active = _selector.activeIdentity(nowMs);
    for (const auto& source : _selector.sources()) {
        const qint64 age = GPSCorrectionFrame::ageMs(source.lastRoutableMs, nowMs);
        const bool usable = age >= 0 && age < GPSCorrectionSelector::FRESHNESS_TIMEOUT_MS;
        const bool selected = usable && (_selector.configuration().policy == GPSCorrectionSelector::Policy::All ||
                                         source.identity == active);
        result.append(QVariantMap{{QStringLiteral("source"), static_cast<int>(source.identity.category)},
                                  {QStringLiteral("instanceId"), source.identity.instance},
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
        result.append(admissionDiagnostics(
            destination, {{QStringLiteral("destinationId"), destination.id},
                          {QStringLiteral("destinationSession"), QVariant::fromValue(destination.session)}}));
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
    if (_lastSubmittedStream) {
        _lastSubmittedStream.reset();
        emit sourceInvalidated();
    }
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
        (frame.validated && !RTCMFramer::isValidFrame(frame.data))) {
        frame.validated = false;
        recordRejectedFrame(frame, GPSCorrectionReason::InvalidFrame);
        return false;
    }
    return acceptFrame(frame);
}

GPSCorrectionRouter::Output GPSCorrectionRouter::admissionOnlyOutput(const QString& id, GPSCorrectionSource scope,
                                                                     Sink sink)
{
    if (!sink) {
        return {};
    }
    return {scope, [id, sink = std::move(sink)](const GPSCorrectionFrame& frame) {
                const quint64 queued = sink(frame);
                return QList<Admission>{
                    {id,
                     {queued, 0, queued ? GPSCorrectionReason::None : GPSCorrectionReason::DestinationUnavailable},
                     true}};
            }};
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
    _ledger.registerOutput(id);
}

void GPSCorrectionRouter::removeSink(const QString& id)
{
    ++_revision;
    _sinks.remove(id);
    _ledger.removeOutput(id);
}

void GPSCorrectionRouter::recordRejectedFrame(GPSCorrectionFrame frame, GPSCorrectionReason reason)
{
    if (_shutdown || frame.data.isEmpty()) {
        return;
    }
    _ledger.received(frame);
    _ledger.filtered(frame);
    _ledger.recordDrop(frame, reason, frame.data.size());
}

bool GPSCorrectionRouter::acceptFrame(GPSCorrectionFrame frame)
{
    if (_shutdown || _submitting || frame.data.isEmpty()) {
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
    const bool routable = !frame.filtered && age < FRESHNESS_TIMEOUT_MS;
    _selector.observe(frame, routable, now);
    if (!routable) {
        _ledger.filtered(frame);
        _ledger.recordDrop(frame, frame.filtered ? GPSCorrectionReason::MessageFiltered : GPSCorrectionReason::Expired,
                           frame.data.size());
        return false;
    }
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
    const QPointer<GPSCorrectionRouter> guard(this);
    const quint64 revision = _revision;
    const auto sinks = _sinks;
    _submitting = true;
    auto finishSubmitting = qScopeGuard([guard] {
        if (guard) {
            guard->_submitting = false;
        }
    });
    const StreamIdentity stream{{frame.source, frame.sourceInstance}, frame.session};
    if (selected && _lastSubmittedStream != stream) {
        _lastSubmittedStream = stream;
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
        const QList<Admission> admissions = it->admit(frame);
        if (!guard) {
            return false;
        }
        if (!_shutdown && revision == _revision) {
            QSet<QString> destinations;
            for (const auto& admission : admissions) {
                if (!admission.destination.isEmpty()) {
                    destinations.insert(admission.destination);
                }
            }
            _ledger.updateOutputDestinations(it.key(), destinations);
        }
        // Callback retirement must not discard returned admission evidence.
        for (const auto& admission : admissions) {
            if (admission.destination.isEmpty()) {
                continue;
            }
            const auto& submitted = admission.submission;
            const quint64 bytes = (std::min) (submitted.queuedBytes, static_cast<quint64>(frame.data.size()));
            const bool complete = admission.complete && bytes == static_cast<quint64>(frame.data.size());
            if (!_ledger.admitted(frame, admission.destination, submitted.destinationSession, bytes, complete)) {
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
    _ledger.shutdown();
}

GPSCorrectionSelector::GPSCorrectionSelector()
{
    qCDebug(GPSCorrectionSelectorLog) << this;
}

GPSCorrectionSelector::~GPSCorrectionSelector()
{
    qCDebug(GPSCorrectionSelectorLog) << this;
}

int GPSCorrectionSelector::_priority(GPSCorrectionSource source)
{
    return source == GPSCorrectionSource::Unknown ? 4 : static_cast<int>(source);
}

bool GPSCorrectionSelector::_eligible(const Source& source, qint64 now) const
{
    const qint64 age = GPSCorrectionFrame::ageMs(source.lastRoutableMs, now);
    if (age < 0 || age >= FRESHNESS_TIMEOUT_MS) {
        return false;
    }
    return _configuration.policy != Policy::Manual ||
           (source.identity.category == _configuration.source &&
            (_configuration.instance.isEmpty() || source.identity.instance == _configuration.instance));
}

void GPSCorrectionSelector::_select(qint64 now)
{
    auto best = _sources.cend();
    for (auto it = _sources.cbegin(); it != _sources.cend(); ++it) {
        if (_eligible(it.value(), now) &&
            (best == _sources.cend() || _priority(it->identity.category) < _priority(best->identity.category))) {
            best = it;
        }
    }
    const auto active = _active ? _sources.constFind(*_active) : _sources.cend();
    if (active == _sources.cend() || !_eligible(active.value(), now)) {
        _active = best == _sources.cend() ? std::nullopt : std::optional(best.key());
        _candidate.reset();
        return;
    }
    if (best == _sources.cend() || _priority(best->identity.category) >= _priority(active->identity.category) ||
        _configuration.policy != Policy::Automatic) {
        _candidate.reset();
        return;
    }
    if (_candidate != best.key()) {
        _candidate = best.key();
        _candidateSinceMs = now;
    } else if (GPSCorrectionFrame::ageMs(_candidateSinceMs, now) >= SWITCH_HOLD_DOWN_MS) {
        _active = _candidate;
        _candidate.reset();
    }
}

GPSCorrectionSource GPSCorrectionSelector::activeSource(qint64 now) const
{
    const auto active = activeIdentity(now);
    return active ? active->category : GPSCorrectionSource::Unknown;
}

std::optional<GPSCorrectionSelector::SourceIdentity> GPSCorrectionSelector::activeIdentity(qint64 now) const
{
    const auto active = _active ? _sources.constFind(*_active) : _sources.cend();
    return active != _sources.cend() && _eligible(active.value(), now) ? _active : std::nullopt;
}

void GPSCorrectionSelector::configure(const Configuration& configuration, qint64 now)
{
    _configuration = configuration;
    _active.reset();
    _candidate.reset();
    _select(now);
}

void GPSCorrectionSelector::clear()
{
    _sources.clear();
    _active.reset();
    _candidate.reset();
}

void GPSCorrectionSelector::retire(GPSCorrectionSource source, qint64 now)
{
    _sources.removeIf([source](auto it) { return it->identity.category == source; });
    _select(now);
}

void GPSCorrectionSelector::observe(const GPSCorrectionFrame& frame, bool routable, qint64 now)
{
    const SourceIdentity id{frame.source, frame.sourceInstance};
    if (!_sources.contains(id) && _sources.size() >= MAX_SOURCE_INSTANCES) {
        auto oldest = _sources.end();
        for (auto it = _sources.begin(); it != _sources.end(); ++it) {
            if (it.key() != _active && (oldest == _sources.end() || it->lastReceivedMs < oldest->lastReceivedMs)) {
                oldest = it;
            }
        }
        if (oldest != _sources.end()) {
            _sources.erase(oldest);
        }
    }
    auto& source = _sources[id];
    source.identity = id;
    source.session = frame.session;
    source.lastReceivedMs = (std::max) (source.lastReceivedMs, frame.receivedAtMs);
    if (routable) {
        source.lastRoutableMs = (std::max) (source.lastRoutableMs, frame.receivedAtMs);
        _select(now);
    }
}

bool GPSCorrectionSelector::selected(const GPSCorrectionFrame& frame, qint64 now) const
{
    if (_configuration.policy == Policy::All) {
        return true;
    }
    return activeIdentity(now) == SourceIdentity{frame.source, frame.sourceInstance};
}
