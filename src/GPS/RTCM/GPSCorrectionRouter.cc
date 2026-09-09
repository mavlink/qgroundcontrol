#include "GPSCorrectionRouter.h"

#include <QtCore/QPointer>

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionRouterLog, "GPS.RTCM.GPSCorrectionRouter")

GPSCorrectionRouter::GPSCorrectionRouter(QObject* parent, Clock clock)
    : QObject(parent)
    , _clock(clock ? std::move(clock) : Clock(GPSCorrectionFrame::monotonicNowMs))
{
    qCDebug(GPSCorrectionRouterLog) << this;
    _statistics[0].active = true;
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

int GPSCorrectionRouter::_priority(GPSCorrectionSource source)
{
    return source == GPSCorrectionSource::Unknown ? 4 : static_cast<int>(source);
}

QString GPSCorrectionRouter::_key(GPSCorrectionSource source, const QString& instance)
{
    return QString::number(static_cast<int>(source)) + QLatin1Char('/') + instance;
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
    const quint64 session = _statistics[index].session + 1;
    _statistics[index] = Statistics{};
    _statistics[index].session = session;
    _statistics[index].active = true;
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
    _statistics[index].active = false;
    for (auto it = _sources.begin(); it != _sources.end();) {
        if (it->category == source) {
            it = _sources.erase(it);
        } else {
            ++it;
        }
    }
    _select(_clock());
    if (_lastSubmittedSource.startsWith(QString::number(index) + QLatin1Char('/'))) {
        _lastSubmittedSource.clear();
        emit sourceInvalidated();
    }
}

quint64 GPSCorrectionRouter::sourceSession(GPSCorrectionSource source) const
{
    const int index = _sourceIndex(source);
    return index < 0 ? 0 : _statistics[index].session;
}

QString GPSCorrectionRouter::sourceInstance(GPSCorrectionSource source) const
{
    const int index = _sourceIndex(source);
    return index < 0 ? QString() : _configuredInstances[index];
}

void GPSCorrectionRouter::setPolicy(Policy policy)
{
    if (_policy == policy) {
        return;
    }
    ++_revision;
    _policy = policy;
    _active.clear();
    _candidate.clear();
    _select(_clock());
}

void GPSCorrectionRouter::setSelectedSource(GPSCorrectionSource source, const QString& instance)
{
    if (_sourceIndex(source) < 0 || (_manualSource == source && _manualInstance == instance)) {
        return;
    }
    ++_revision;
    _manualSource = source;
    _manualInstance = instance;
    _active.clear();
    _candidate.clear();
    _select(_clock());
}

QString GPSCorrectionRouter::activeInstance() const
{
    const auto active = _sources.constFind(_active);
    return active != _sources.cend() && _eligible(active.value(), _clock()) ? active->instance : QString();
}

GPSCorrectionSource GPSCorrectionRouter::activeSource() const
{
    const auto active = _sources.constFind(_active);
    return active != _sources.cend() && _eligible(active.value(), _clock()) ? active->category
                                                                            : GPSCorrectionSource::Unknown;
}

void GPSCorrectionRouter::setSink(const QString& id, Sink sink)
{
    ++_revision;
    if (sink) {
        _sinks.insert(id, std::move(sink));
    } else {
        _sinks.remove(id);
    }
}

void GPSCorrectionRouter::removeSink(const QString& id)
{
    ++_revision;
    _sinks.remove(id);
}

bool GPSCorrectionRouter::_eligible(const Source& source, qint64 now) const
{
    if (source.lastRoutableMs <= 0 || now < source.lastRoutableMs ||
        now - source.lastRoutableMs >= FRESHNESS_TIMEOUT_MS) {
        return false;
    }
    return _policy != Policy::Manual ||
           (source.category == _manualSource && (_manualInstance.isEmpty() || source.instance == _manualInstance));
}

void GPSCorrectionRouter::_select(qint64 now)
{
    auto best = _sources.cend();
    for (auto it = _sources.cbegin(); it != _sources.cend(); ++it) {
        if (_eligible(it.value(), now) &&
            (best == _sources.cend() || _priority(it->category) < _priority(best->category))) {
            best = it;
        }
    }
    const auto active = _sources.constFind(_active);
    if (active == _sources.cend() || !_eligible(active.value(), now)) {
        _active = best == _sources.cend() ? QString() : best.key();
        _candidate.clear();
        return;
    }
    if (best == _sources.cend() || _priority(best->category) >= _priority(active->category) ||
        _policy != Policy::Automatic) {
        _candidate.clear();
        return;
    }
    if (_candidate != best.key()) {
        _candidate = best.key();
        _candidateSinceMs = now;
    } else if (now - _candidateSinceMs >= SWITCH_HOLD_DOWN_MS) {
        _active = _candidate;
        _candidate.clear();
    }
}

bool GPSCorrectionRouter::acceptFrame(GPSCorrectionFrame frame)
{
    const int index = _sourceIndex(frame.source);
    if (_shutdown || _submitting || index < 0 || frame.data.isEmpty()) {
        return false;
    }
    auto& stats = _statistics[index];
    if (!stats.active || stats.session != frame.session) {
        return false;
    }
    if (frame.sourceInstance.isEmpty()) {
        frame.sourceInstance = _configuredInstances[index];
    } else if (!_configuredInstances[index].isEmpty() && frame.sourceInstance != _configuredInstances[index]) {
        return false;
    }
    stats.receivedBytes += frame.data.size();
    const qint64 now = _clock();
    if (frame.receivedAtMs <= 0 || frame.receivedAtMs > now) {
        ++stats.filteredFrames;
        return false;
    }
    if (frame.validated) {
        ++stats.validatedFrames;
        stats.lastValidMs = (std::max) (stats.lastValidMs, frame.receivedAtMs);
    }
    const QString key = _key(frame.source, frame.sourceInstance);
    if (!_sources.contains(key) && _sources.size() >= MAX_SOURCE_INSTANCES) {
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
    auto& source = _sources[key];
    source.category = frame.source;
    source.instance = frame.sourceInstance;
    source.session = frame.session;
    source.lastReceivedMs = (std::max) (source.lastReceivedMs, frame.receivedAtMs);
    if (frame.filtered || now - frame.receivedAtMs >= FRESHNESS_TIMEOUT_MS) {
        ++stats.filteredFrames;
        return false;
    }
    source.lastRoutableMs = (std::max) (source.lastRoutableMs, frame.receivedAtMs);
    _select(now);
    if (_policy != Policy::All && key != _active) {
        ++stats.filteredFrames;
        return false;
    }
    if (frame.validated && frame.messageId == 0 && frame.data.size() >= 8 &&
        static_cast<quint8>(frame.data[0]) == 0xD3) {
        frame.messageId = (static_cast<quint8>(frame.data[3]) << 4) | (static_cast<quint8>(frame.data[4]) >> 4);
    }
    ++stats.routedFrames;
    const QPointer<GPSCorrectionRouter> guard(this);
    const quint64 revision = _revision;
    const auto sinks = _sinks;
    _submitting = true;
    const QString submissionSource = key + QLatin1Char('#') + QString::number(frame.session);
    if (_lastSubmittedSource != submissionSource) {
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
    for (const auto& sink : sinks) {
        const quint64 submitted = sink(frame);
        if (!guard) {
            return false;
        }
        if (_shutdown || revision != _revision) {
            _submitting = false;
            return false;
        }
        _statistics[index].submittedBytes += submitted;
    }
    _submitting = false;
    return true;
}

void GPSCorrectionRouter::shutdown()
{
    ++_revision;
    _shutdown = true;
    for (auto& stats : _statistics) {
        stats.active = false;
    }
    _sources.clear();
    _active.clear();
    _candidate.clear();
    _sinks.clear();
}
