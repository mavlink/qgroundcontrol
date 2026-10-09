#include "GPSCorrectionSelector.h"

#include <algorithm>

#include "MonotonicClock.h"

int GPSCorrectionSelector::_index(GPSCorrectionSettings::CorrectionSource source)
{
    const int index = static_cast<int>(source);
    return index >= 0 && index < GPS_CORRECTION_SOURCE_COUNT ? index : -1;
}

bool GPSCorrectionSelector::_fresh(const Category& category, qint64 now) const
{
    const auto age =
        MonotonicClock::age(std::chrono::milliseconds(category.lastRoutableMs), std::chrono::milliseconds(now));
    return category.begun && age && *age < FRESHNESS_TIMEOUT;
}

bool GPSCorrectionSelector::_eligible(int index, qint64 now) const
{
    if (index <= 0 || !_fresh(_categories[index], now)) {
        return false;
    }
    return _configuredSource == GPSCorrectionSettings::HighestPriority || index == static_cast<int>(_configuredSource);
}

void GPSCorrectionSelector::_select(qint64 now)
{
    int best = -1;
    for (int index = 1; index < GPS_CORRECTION_SOURCE_COUNT && best < 0; ++index) {
        if (_eligible(index, now)) {
            best = index;
        }
    }
    if (!_eligible(_active, now)) {
        _active = best;
        _candidate = -1;
        return;
    }
    if (best < 0 || best >= _active) {
        _candidate = -1;
        return;
    }
    if (_candidate != best) {
        _candidate = best;
        _candidateSinceMs = now;
    } else if (const auto held =
                   MonotonicClock::age(std::chrono::milliseconds(_candidateSinceMs), std::chrono::milliseconds(now));
               held && *held >= SWITCH_HOLD_DOWN) {
        _active = _candidate;
        _candidate = -1;
    }
}

void GPSCorrectionSelector::configure(GPSCorrectionSettings::CorrectionSource source, qint64 now)
{
    if (_shutdown || _index(source) < 0 || source == _configuredSource) {
        return;
    }
    _configuredSource = source;
    _active = -1;
    _candidate = -1;
    _select(now);
}

void GPSCorrectionSelector::beginSource(GPSCorrectionSettings::CorrectionSource source, qint64 now)
{
    const int index = _index(source);
    if (_shutdown || index <= 0) {
        return;
    }
    endSource(source, now);
    _categories[index].begun = true;
}

void GPSCorrectionSelector::endSource(GPSCorrectionSettings::CorrectionSource source, qint64 now)
{
    const int index = _index(source);
    if (index <= 0) {
        return;
    }
    _categories[index] = {};
    _select(now);
}

bool GPSCorrectionSelector::submit(GPSCorrectionSettings::CorrectionSource source, const QString& instance,
                                   qint64 receivedAtMs, qint64 now)
{
    const int index = _index(source);
    if (_shutdown || index <= 0 || !_categories[index].begun) {
        return false;
    }
    const auto age = MonotonicClock::age(std::chrono::milliseconds(receivedAtMs), std::chrono::milliseconds(now));
    if (!age || *age >= FRESHNESS_TIMEOUT) {
        return false;
    }
    Category& category = _categories[index];
    if (category.instance != instance) {
        // The category keeps following its instance until that goes stale.
        if (_fresh(category, now)) {
            return false;
        }
        category.instance = instance;
    }
    category.lastRoutableMs = (std::max) (category.lastRoutableMs, receivedAtMs);
    _select(now);
    return _active == index;
}

bool GPSCorrectionSelector::hasSources() const
{
    return std::ranges::any_of(_categories, &Category::begun);
}

std::optional<GPSCorrectionStream> GPSCorrectionSelector::selectedStream(qint64 now) const
{
    if (!_eligible(_active, now)) {
        return std::nullopt;
    }
    return GPSCorrectionStream{.source = _active, .instanceId = _categories[_active].instance};
}

void GPSCorrectionSelector::shutdown()
{
    _shutdown = true;
    _categories = {};
    _active = -1;
    _candidate = -1;
}
