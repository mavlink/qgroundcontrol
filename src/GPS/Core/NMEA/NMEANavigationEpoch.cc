#include "NMEANavigationEpoch.h"

#include <algorithm>
#include <utility>

#include "MonotonicClock.h"

namespace {
std::optional<double> finiteValue(double value)
{
    return std::isfinite(value) ? std::optional<double>(value) : std::nullopt;
}

std::optional<double> nonnegativeNumber(std::string_view field)
{
    const auto value = NMEA::number<double>(field);
    return value && *value >= 0.0 ? value : std::nullopt;
}

/// A sentence adds the fields it reports: an empty one leaves what an earlier sentence of the epoch supplied.
void addReported(std::optional<double>& value, std::optional<double> reported)
{
    if (reported) {
        value = reported;
    }
}

void markReceipt(NMEA::NavigationEpoch& epoch, uint64_t receivedAtUs)
{
    epoch.receivedAtUs = epoch.receivedAtUs == 0 ? receivedAtUs : (std::min) (epoch.receivedAtUs, receivedAtUs);
}
}  // namespace

namespace NMEA {

std::optional<double> NavigationEpoch::altitudeEllipsoidMeters() const
{
    return altitudeMslMeters && geoidSeparationMeters
               ? std::optional<double>(*altitudeMslMeters + *geoidSeparationMeters)
               : std::nullopt;
}

void NavigationEpochAssembler::reset()
{
    *this = {};
}

std::optional<NavigationUpdate> NavigationEpochAssembler::ingest(const Sentence& sentence, uint64_t receivedAtUs)
{
    const auto sequence = ++_sequence;

    if (const auto navigation = navigationStatus(sentence)) {
        if (!navigation->valid) {
            _navigationValid = false;
            _invalidThroughSequence = sequence;
            NavigationEpoch loss;
            loss.timeMs = navigation->utcMilliseconds;
            loss.receivedAtUs = receivedAtUs;
            loss.positionReceivedAtUs = receivedAtUs;
            loss.sequence = sequence;
            loss.fixQuality = GPSFixQuality::NoFix;
            if (sentence.type() == "GGA" && sentence.count > Field::GGA_SATELLITES_USED) {
                loss.satellitesUsed = ggaSatellitesUsed(sentence.fields[Field::GGA_SATELLITES_USED]);
            }
            return NavigationUpdate{.type = NavigationUpdate::Type::FixLoss, .epoch = loss};
        }
        _navigationValid = true;
    }

    std::optional<NavigationEpoch> overdue;
    auto* joined = _joinPosition(sentence, receivedAtUs, overdue);
    if (!joined) {
        joined = _joinAccuracy(sentence, receivedAtUs);
    }
    if (!joined) {
        joined = _joinUntimed(sentence, receivedAtUs);
    }
    if (joined && _complete(*joined)) {
        joined->published = true;
        return NavigationUpdate{.type = NavigationUpdate::Type::Epoch, .epoch = joined->epoch};
    }
    if (overdue) {
        return NavigationUpdate{.type = NavigationUpdate::Type::Epoch, .epoch = *overdue};
    }
    return std::nullopt;
}

NavigationEpochAssembler::StoredEpoch& NavigationEpochAssembler::_find(std::optional<int> timeMs)
{
    if (auto* existing = _findExisting(timeMs)) {
        return *existing;
    }

    auto oldest = _epochs.begin();
    for (auto epoch = _epochs.begin(); epoch != _epochs.end(); ++epoch) {
        if (!epoch->active) {
            oldest = epoch;
            break;
        }
        if (epoch->epoch.receivedAtUs < oldest->epoch.receivedAtUs) {
            oldest = epoch;
        }
    }
    _resetEpoch(*oldest, timeMs);
    return *oldest;
}

NavigationEpochAssembler::StoredEpoch* NavigationEpochAssembler::_findExisting(std::optional<int> timeMs)
{
    const int key = timeMs.value_or(NO_TIME_KEY);
    for (auto& epoch : _epochs) {
        if (epoch.active && epoch.key == key) {
            return &epoch;
        }
    }
    return nullptr;
}

NavigationEpochAssembler::StoredEpoch* NavigationEpochAssembler::_current()
{
    if (!_hasCurrent) {
        return nullptr;
    }
    for (auto& epoch : _epochs) {
        if (epoch.active && epoch.key == _currentKey) {
            return &epoch;
        }
    }
    return nullptr;
}

void NavigationEpochAssembler::_resetEpoch(StoredEpoch& stored, std::optional<int> timeMs)
{
    stored = {};
    stored.active = true;
    stored.key = timeMs.value_or(NO_TIME_KEY);
    stored.epoch.timeMs = timeMs;
}

void NavigationEpochAssembler::_resetExpiredEpoch(StoredEpoch& stored, uint64_t receivedAtUs)
{
    // A GGA that waits for the rest of its epoch is not retained metadata: a sentence of its time completes it however
    // late, and only another GGA replaces it.
    if (stored.epoch.ggaQuality && !stored.published) {
        return;
    }
    if (stored.epoch.receivedAtUs && receivedAtUs > stored.epoch.receivedAtUs &&
        !MonotonicClock::withinAge(stored.epoch.receivedAtUs, receivedAtUs, METADATA_MAX_AGE)) {
        _resetEpoch(stored, stored.epoch.timeMs);
    }
}

NavigationEpochAssembler::StoredEpoch& NavigationEpochAssembler::_positionEpoch(std::optional<int> timeMs,
                                                                                uint64_t receivedAtUs, bool gga,
                                                                                std::optional<NavigationEpoch>& overdue)
{
    auto* const current = _current();
    auto& stored = _find(timeMs);
    // A GGA of the same time again is a new report from a receiver whose time does not advance.
    if (current && (current != &stored || (gga && stored.epoch.ggaQuality))) {
        overdue = _end(*current);
    }
    _resetExpiredEpoch(stored, receivedAtUs);
    _currentKey = stored.key;
    _hasCurrent = true;
    return stored;
}

std::optional<NavigationEpoch> NavigationEpochAssembler::_end(StoredEpoch& stored)
{
    std::optional<NavigationEpoch> overdue;
    if (!stored.published && stored.epoch.ggaQuality && !_hiddenByFixLoss(stored.epoch)) {
        overdue = stored.epoch;
    }
    stored.published = true;
    _earlierFollowers = std::exchange(_previousFollowers, stored.followers);
    return overdue;
}

bool NavigationEpochAssembler::_hiddenByFixLoss(const NavigationEpoch& epoch) const
{
    return !_navigationValid || epoch.sequence <= _invalidThroughSequence;
}

void NavigationEpochAssembler::_add(StoredEpoch& stored, Addition addition, uint64_t receivedAtUs) const
{
    stored.additions |= addition;
    // Later in the interval a sentence would hold the position for too long, and a GSA or VTG there may open the next
    // epoch's burst.
    const auto limit = _ggaInterval > std::chrono::microseconds::zero()
                           ? (std::min) (METADATA_MAX_AGE, _ggaInterval / 2)
                           : METADATA_MAX_AGE;
    if (stored.epoch.ggaQuality && MonotonicClock::withinAge(stored.epoch.positionReceivedAtUs, receivedAtUs, limit)) {
        stored.followers |= addition;
    }
}

bool NavigationEpochAssembler::_complete(const StoredEpoch& stored) const
{
    // One epoch with a follower may be a slower sentence's turn; two in a row are the receiver's order.
    const Additions awaited = _previousFollowers & _earlierFollowers;
    return !stored.published && stored.epoch.ggaQuality && !_hiddenByFixLoss(stored.epoch) &&
           (stored.additions & awaited) == awaited;
}

NavigationEpochAssembler::StoredEpoch* NavigationEpochAssembler::_joinPosition(const Sentence& sentence,
                                                                               uint64_t receivedAtUs,
                                                                               std::optional<NavigationEpoch>& overdue)
{
    // Position sentences reach here only with a receiver-declared valid fix; ingest() reports fix loss first.
    if (const auto ggaFix = gga(sentence)) {
        auto& stored = _positionEpoch(utcMilliseconds(sentence.fields[Field::UTC_TIME]), receivedAtUs, true, overdue);
        auto& epoch = stored.epoch;
        epoch.ggaQuality = ggaFix->quality;
        epoch.fixQuality = fixQuality(ggaFix->quality, GPSFixQuality::Fix3D);
        epoch.latitude = ggaFix->latitude;
        epoch.longitude = ggaFix->longitude;
        epoch.altitudeMslMeters = finiteValue(ggaFix->altitude);
        epoch.geoidSeparationMeters = finiteValue(ggaFix->geoidSeparation);
        epoch.satellitesUsed = ggaFix->satellitesUsed;
        epoch.horizontalDop = finiteValue(ggaFix->hdop);
        if (!epoch.accuracyReceivedAtUs ||
            !MonotonicClock::withinAge(epoch.accuracyReceivedAtUs, receivedAtUs, METADATA_MAX_AGE)) {
            epoch.horizontalAccuracyMeters.reset();
            epoch.verticalAccuracyMeters.reset();
            epoch.accuracyReceivedAtUs = 0;
        }
        epoch.positionReceivedAtUs = receivedAtUs;
        epoch.sequence = _sequence;
        markReceipt(epoch, receivedAtUs);
        stored.published = false;
        stored.followers = {};
        _ggaInterval = MonotonicClock::age(_ggaReceivedAtUs, receivedAtUs).value_or(std::chrono::microseconds::zero());
        _ggaReceivedAtUs = receivedAtUs;
        return &stored;
    }

    const auto rmcFix = rmc(sentence);
    std::optional<int> timeMs;
    if (rmcFix) {
        timeMs = rmcFix->utcMilliseconds;
    } else if (const auto gllFix = gll(sentence)) {
        timeMs = gllFix->utcMilliseconds;
    }
    if (!timeMs) {
        return nullptr;
    }
    auto& stored = _positionEpoch(timeMs, receivedAtUs, false, overdue);
    auto& epoch = stored.epoch;
    if (!epoch.ggaQuality) {
        // Only the GGA's position is published; until it arrives, GSA and VTG join the epoch from this receipt.
        epoch.positionReceivedAtUs = receivedAtUs;
        epoch.sequence = _sequence;
    }
    markReceipt(epoch, receivedAtUs);
    if (rmcFix) {
        addReported(epoch.speedMetersPerSecond, finiteValue(rmcFix->speedMetersPerSecond));
        addReported(epoch.courseDegrees, finiteValue(rmcFix->courseDegrees));
        _add(stored, Addition::RMC, receivedAtUs);
    }
    return &stored;
}

NavigationEpochAssembler::StoredEpoch* NavigationEpochAssembler::_joinAccuracy(const Sentence& sentence,
                                                                               uint64_t receivedAtUs)
{
    const auto accuracy = gst(sentence);
    if (!accuracy) {
        return nullptr;
    }
    const auto timeMs = utcMilliseconds(sentence.fields[Field::UTC_TIME]);
    if (!timeMs) {
        return nullptr;
    }
    auto& stored = _find(timeMs);
    _resetExpiredEpoch(stored, receivedAtUs);
    auto& epoch = stored.epoch;
    epoch.horizontalAccuracyMeters = finiteValue(accuracy->horizontalAccuracy);
    epoch.verticalAccuracyMeters = finiteValue(accuracy->verticalAccuracy);
    epoch.accuracyReceivedAtUs = receivedAtUs;
    markReceipt(epoch, receivedAtUs);
    _add(stored, Addition::GST, receivedAtUs);
    return &stored;
}

NavigationEpochAssembler::StoredEpoch* NavigationEpochAssembler::_joinUntimed(const Sentence& sentence,
                                                                              uint64_t receivedAtUs)
{
    auto* stored = _current();
    if (!stored || !stored->epoch.positionReceivedAtUs ||
        !MonotonicClock::withinAge(stored->epoch.positionReceivedAtUs, receivedAtUs, METADATA_MAX_AGE)) {
        return nullptr;
    }

    auto& epoch = stored->epoch;
    if (sentence.type() == "GSA") {
        if (sentence.count < Field::GSA_MIN_FIELDS) {
            return nullptr;
        }
        if (!epoch.horizontalDop) {
            epoch.horizontalDop = nonnegativeNumber(sentence.fields[Field::GSA_HDOP]);
        }
        addReported(epoch.verticalDop, nonnegativeNumber(sentence.fields[Field::GSA_VDOP]));
        _add(*stored, Addition::GSA, receivedAtUs);
    } else if (const auto velocity = vtg(sentence)) {
        addReported(epoch.speedMetersPerSecond, finiteValue(velocity->speedMetersPerSecond));
        addReported(epoch.courseDegrees, finiteValue(velocity->courseDegrees));
        _add(*stored, Addition::VTG, receivedAtUs);
    } else {
        return nullptr;
    }
    return stored;
}

}  // namespace NMEA
