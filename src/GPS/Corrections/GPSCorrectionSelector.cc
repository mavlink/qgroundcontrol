#include "GPSCorrectionSelector.h"

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionSelectorLog, "GPS.Corrections.GPSCorrectionSelector")

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

QString GPSCorrectionSelector::key(GPSCorrectionSource source, const QString& instance)
{
    return QString::number(static_cast<int>(source)) + QLatin1Char('/') + instance;
}

bool GPSCorrectionSelector::_eligible(const Source& source, qint64 now) const
{
    if (source.lastRoutableMs <= 0 || now < source.lastRoutableMs ||
        now - source.lastRoutableMs >= FRESHNESS_TIMEOUT_MS) {
        return false;
    }
    return _configuration.policy != Policy::Manual ||
           (source.category == _configuration.source &&
            (_configuration.instance.isEmpty() || source.instance == _configuration.instance));
}

void GPSCorrectionSelector::_select(qint64 now)
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
        _configuration.policy != Policy::Automatic) {
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

QString GPSCorrectionSelector::activeInstance(qint64 now) const
{
    const auto active = _sources.constFind(_active);
    return active != _sources.cend() && _eligible(active.value(), now) ? active->instance : QString();
}

GPSCorrectionSource GPSCorrectionSelector::activeSource(qint64 now) const
{
    const auto active = _sources.constFind(_active);
    return active != _sources.cend() && _eligible(active.value(), now) ? active->category
                                                                       : GPSCorrectionSource::Unknown;
}

void GPSCorrectionSelector::configure(const Configuration& configuration, qint64 now)
{
    _configuration = configuration;
    _active.clear();
    _candidate.clear();
    _select(now);
}

void GPSCorrectionSelector::clear()
{
    _sources.clear();
    _active.clear();
    _candidate.clear();
}

void GPSCorrectionSelector::retire(GPSCorrectionSource source, qint64 now)
{
    for (auto it = _sources.begin(); it != _sources.end();) {
        if (it->category == source)
            it = _sources.erase(it);
        else
            ++it;
    }
    _select(now);
}

void GPSCorrectionSelector::observe(const GPSCorrectionFrame& frame, bool routable, qint64 now)
{
    const QString id = key(frame.source, frame.sourceInstance);
    if (!_sources.contains(id) && _sources.size() >= MAX_SOURCE_INSTANCES) {
        auto oldest = _sources.end();
        for (auto it = _sources.begin(); it != _sources.end(); ++it) {
            if (it.key() != _active && (oldest == _sources.end() || it->lastReceivedMs < oldest->lastReceivedMs))
                oldest = it;
        }
        if (oldest != _sources.end())
            _sources.erase(oldest);
    }
    auto& source = _sources[id];
    source.category = frame.source;
    source.instance = frame.sourceInstance;
    source.session = frame.session;
    source.lastReceivedMs = (std::max) (source.lastReceivedMs, frame.receivedAtMs);
    if (routable) {
        source.lastRoutableMs = (std::max) (source.lastRoutableMs, frame.receivedAtMs);
        _select(now);
    }
}

bool GPSCorrectionSelector::selected(const GPSCorrectionFrame& frame, qint64 now) const
{
    return _configuration.policy == Policy::All ||
           (key(frame.source, frame.sourceInstance) == _active && _sources.contains(_active) &&
            _eligible(_sources.value(_active), now));
}
