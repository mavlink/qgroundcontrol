#include "GPSCorrectionRouter.h"

#include <QtCore/QPointer>

#include <algorithm>
#include <utility>

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
    if (frame.validated && !RTCM::isValidFrame(frame.data)) {
        frame.validated = false;
        recordRejectedFrame(frame, GPSCorrectionReason::InvalidFrame);
        return false;
    }
    return acceptFrame(frame);
}

void GPSCorrectionRouter::setSink(const QString& id, Sink sink)
{
    if (!sink) {
        removeSink(id);
        return;
    }
    setDetailedSink(
        id,
        [sink = std::move(sink)](const GPSCorrectionFrame& frame) {
            const quint64 queued = sink(frame);
            return Submission{queued, 0,
                              queued ? GPSCorrectionReason::None : GPSCorrectionReason::DestinationUnavailable};
        },
        false);
}

void GPSCorrectionRouter::setSourceSink(const QString& id, GPSCorrectionSource source, Sink sink)
{
    if (_sourceIndex(source) <= 0) {
        return;
    }
    setSink(id, std::move(sink));
    if (auto it = _sinks.find(id); it != _sinks.end()) {
        it->scope = source;
    }
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
    if (id.isEmpty())
        return;
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
    _ledger.removeOutput(id);
}

bool GPSCorrectionRouter::recordDelivery(const GPSCorrectionDelivery& delivery)
{
    if (_admittingDelivery && delivery.deliveryId == _admittingDelivery &&
        _deferredDeliveries.size() < MAX_PENDING_DELIVERIES) {
        _deferredDeliveries.append(delivery);
        return true;
    }
    return _ledger.recordDelivery(delivery);
}

void GPSCorrectionRouter::invalidateDestination(const QString& id, quint64 session)
{
    _ledger.invalidateDestination(id, session);
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
    if (frame.receivedAtMs <= 0 || frame.receivedAtMs > now) {
        _ledger.filtered(frame);
        _ledger.recordDrop(frame, GPSCorrectionReason::InvalidTimestamp, frame.data.size());
        return false;
    }
    if (frame.validated)
        _ledger.validated(frame);
    _selector.observe(frame, false, now);
    if (frame.filtered || now - frame.receivedAtMs >= FRESHNESS_TIMEOUT_MS) {
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
    const QString submissionSource = key + QLatin1Char('#') + QString::number(frame.session);
    if (selected && _lastSubmittedSource != submissionSource) {
        _lastSubmittedSource = submissionSource;
        emit sourceSelected(frame.source, frame.sourceInstance);
        if (!guard) {
            return false;
        }
        if (_shutdown || revision != _revision) {
            _submitting = false;
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
        _admittingDelivery = frame.deliveryId;
        const QList<Admission> admissions = it->admit(frame);
        if (!guard) {
            return false;
        }
        // Preserve evidence returned by an output even if its callback retired the source or changed routing.
        for (const auto& admission : admissions) {
            if (admission.destination.isEmpty()) {
                continue;
            }
            const auto& submitted = admission.submission;
            const quint64 bytes = (std::min) (submitted.queuedBytes, static_cast<quint64>(frame.data.size()));
            const bool complete = admission.complete && bytes == static_cast<quint64>(frame.data.size());
            logicalQueuedBytes = (std::max) (logicalQueuedBytes, bytes);
            completeSubmission |= complete;
            _ledger.admitted(frame, it.key(), admission.destination, submitted.destinationSession, bytes, complete,
                             it->completion == Completion::Reported);
            if (!complete) {
                submissionFailure = submitted.reason == GPSCorrectionReason::None
                                        ? GPSCorrectionReason::DestinationUnavailable
                                        : submitted.reason;
                _ledger.recordDrop(frame, submissionFailure, frame.data.size() - bytes, admission.destination,
                                   submitted.destinationSession, false);
            }
        }
        _admittingDelivery = 0;
        const auto deliveries = std::exchange(_deferredDeliveries, {});
        for (const auto& delivery : deliveries)
            _ledger.recordDelivery(delivery);
        if (_shutdown || revision != _revision) {
            for (const auto& admission : admissions)
                _ledger.invalidateDestination(admission.destination, admission.submission.destinationSession);
            break;
        }
    }
    _ledger.queued(frame, logicalQueuedBytes, completeSubmission);
    if (attempted && !completeSubmission) {
        _ledger.recordDrop(frame, submissionFailure, frame.data.size() - logicalQueuedBytes);
    }
    _ledger.pruneDestinationHistory();
    _submitting = false;
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
