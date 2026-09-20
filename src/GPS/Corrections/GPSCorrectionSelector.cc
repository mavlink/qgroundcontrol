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

QString GPSCorrectionSelector::activeInstance(qint64 now) const
{
    const auto active = _active ? _sources.constFind(*_active) : _sources.cend();
    return active != _sources.cend() && _eligible(active.value(), now) ? active->identity.instance : QString();
}

GPSCorrectionSource GPSCorrectionSelector::activeSource(qint64 now) const
{
    const auto active = _active ? _sources.constFind(*_active) : _sources.cend();
    return active != _sources.cend() && _eligible(active.value(), now) ? active->identity.category
                                                                       : GPSCorrectionSource::Unknown;
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
    const SourceIdentity identity{frame.source, frame.sourceInstance};
    if (_active != identity) {
        return false;
    }
    const auto active = _sources.constFind(identity);
    return active != _sources.cend() && _eligible(active.value(), now);
}
