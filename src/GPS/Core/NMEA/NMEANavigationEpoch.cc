#include "NMEANavigationEpoch.h"

#include <algorithm>

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

void markReceipt(NMEA::NavigationEpoch& epoch, uint64_t receivedAtUs)
{
    epoch.receivedAtUs = epoch.receivedAtUs == 0 ? receivedAtUs : std::min(epoch.receivedAtUs, receivedAtUs);
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
                loss.satellitesUsed = number<unsigned>(sentence.fields[Field::GGA_SATELLITES_USED]);
            }
            return NavigationUpdate{
                .type = NavigationUpdate::Type::FixLoss, .trigger = NavigationUpdate::Trigger::Position, .epoch = loss};
        }
        _navigationValid = true;
    }

    if (auto update = _handlePositionSentence(sentence, receivedAtUs)) {
        return update;
    }
    if (auto update = _handleAccuracy(sentence, receivedAtUs)) {
        return update;
    }
    return _handleUntimedMetadata(sentence, receivedAtUs);
}

std::optional<NavigationEpoch> NavigationEpochAssembler::expireUntimedMetadata(uint64_t nowUs)
{
    auto* stored = _current();
    if (!stored || !stored->epoch.verticalDopReceivedAtUs ||
        MonotonicClock::withinAge(stored->epoch.verticalDopReceivedAtUs, nowUs, METADATA_MAX_AGE)) {
        return std::nullopt;
    }
    stored->epoch.verticalDop.reset();
    stored->epoch.verticalDopReceivedAtUs = 0;
    return stored->epoch;
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
    if (stored.epoch.receivedAtUs && receivedAtUs > stored.epoch.receivedAtUs &&
        !MonotonicClock::withinAge(stored.epoch.receivedAtUs, receivedAtUs, METADATA_MAX_AGE)) {
        _resetEpoch(stored, stored.epoch.timeMs);
    }
}

NavigationEpoch& NavigationEpochAssembler::_positionEpoch(std::optional<int> timeMs, uint64_t receivedAtUs)
{
    auto& stored = _find(timeMs);
    _resetExpiredEpoch(stored, receivedAtUs);
    _currentKey = stored.key;
    _hasCurrent = true;
    return stored.epoch;
}

bool NavigationEpochAssembler::_hiddenByFixLoss(const NavigationEpoch& epoch) const
{
    return !_navigationValid || epoch.sequence <= _invalidThroughSequence;
}

std::optional<NavigationUpdate> NavigationEpochAssembler::_handlePositionSentence(const Sentence& sentence,
                                                                                  uint64_t receivedAtUs)
{
    // Position sentences reach here only with a receiver-declared valid fix; ingest() reports fix loss first.
    NavigationEpoch* epoch = nullptr;
    if (const auto rmcFix = rmc(sentence)) {
        if (!rmcFix->utcMilliseconds) {
            return std::nullopt;
        }
        epoch = &_positionEpoch(rmcFix->utcMilliseconds, receivedAtUs);
        epoch->latitude = rmcFix->latitude;
        epoch->longitude = rmcFix->longitude;
        epoch->speedMetersPerSecond = finiteValue(rmcFix->speedMetersPerSecond);
        epoch->courseDegrees = finiteValue(rmcFix->courseDegrees);
    } else if (const auto ggaFix = gga(sentence)) {
        epoch = &_positionEpoch(utcMilliseconds(sentence.fields[Field::UTC_TIME]), receivedAtUs);
        epoch->ggaQuality = ggaFix->quality;
        epoch->latitude = ggaFix->latitude;
        epoch->longitude = ggaFix->longitude;
        epoch->altitudeMslMeters = finiteValue(ggaFix->altitude);
        epoch->geoidSeparationMeters = finiteValue(ggaFix->geoidSeparation);
        epoch->satellitesUsed = ggaFix->satellitesUsed;
        epoch->horizontalDop = finiteValue(ggaFix->hdop);
        if (!epoch->accuracyReceivedAtUs ||
            !MonotonicClock::withinAge(epoch->accuracyReceivedAtUs, receivedAtUs, METADATA_MAX_AGE)) {
            epoch->horizontalAccuracyMeters.reset();
            epoch->verticalAccuracyMeters.reset();
            epoch->accuracyReceivedAtUs = 0;
        }
    } else if (const auto gllFix = gll(sentence)) {
        if (!gllFix->utcMilliseconds) {
            return std::nullopt;
        }
        epoch = &_positionEpoch(gllFix->utcMilliseconds, receivedAtUs);
        epoch->latitude = gllFix->latitude;
        epoch->longitude = gllFix->longitude;
    } else {
        return std::nullopt;
    }

    epoch->fixQuality = fixQuality(epoch->ggaQuality.value_or(GgaQuality::GPS), GPSFixQuality::Fix3D);
    markReceipt(*epoch, receivedAtUs);
    epoch->positionReceivedAtUs = receivedAtUs;
    epoch->sequence = _sequence;
    return NavigationUpdate{
        .type = NavigationUpdate::Type::Epoch, .trigger = NavigationUpdate::Trigger::Position, .epoch = *epoch};
}

std::optional<NavigationUpdate> NavigationEpochAssembler::_handleAccuracy(const Sentence& sentence,
                                                                          uint64_t receivedAtUs)
{
    const auto accuracy = gst(sentence);
    if (!accuracy) {
        return std::nullopt;
    }
    const auto timeMs = utcMilliseconds(sentence.fields[Field::UTC_TIME]);
    if (!timeMs) {
        return std::nullopt;
    }
    auto& stored = _find(timeMs);
    _resetExpiredEpoch(stored, receivedAtUs);
    auto& epoch = stored.epoch;
    epoch.horizontalAccuracyMeters = finiteValue(accuracy->horizontalAccuracy);
    epoch.verticalAccuracyMeters = finiteValue(accuracy->verticalAccuracy);
    epoch.accuracyReceivedAtUs = receivedAtUs;
    markReceipt(epoch, receivedAtUs);
    if (!_hasCurrent || _currentKey != stored.key || _hiddenByFixLoss(epoch)) {
        return std::nullopt;
    }
    return NavigationUpdate{
        .type = NavigationUpdate::Type::Epoch, .trigger = NavigationUpdate::Trigger::TimedMetadata, .epoch = epoch};
}

std::optional<NavigationUpdate> NavigationEpochAssembler::_handleUntimedMetadata(const Sentence& sentence,
                                                                                 uint64_t receivedAtUs)
{
    auto* stored = _current();
    if (!stored || !stored->epoch.positionReceivedAtUs ||
        !MonotonicClock::withinAge(stored->epoch.positionReceivedAtUs, receivedAtUs, METADATA_MAX_AGE)) {
        return std::nullopt;
    }

    auto& epoch = stored->epoch;
    if (sentence.type() == "GSA") {
        if (sentence.count < Field::GSA_MIN_FIELDS) {
            return std::nullopt;
        }
        if (!epoch.horizontalDop) {
            epoch.horizontalDop = nonnegativeNumber(sentence.fields[Field::GSA_HDOP]);
        }
        const auto vdop = nonnegativeNumber(sentence.fields[Field::GSA_VDOP]);
        epoch.verticalDop = vdop;
        epoch.verticalDopReceivedAtUs = vdop ? receivedAtUs : 0;
    } else if (const auto velocity = vtg(sentence)) {
        epoch.speedMetersPerSecond = finiteValue(velocity->speedMetersPerSecond);
        epoch.courseDegrees = finiteValue(velocity->courseDegrees);
    } else {
        return std::nullopt;
    }
    if (_hiddenByFixLoss(epoch)) {
        return std::nullopt;
    }
    return NavigationUpdate{
        .type = NavigationUpdate::Type::Epoch, .trigger = NavigationUpdate::Trigger::UntimedMetadata, .epoch = epoch};
}

}  // namespace NMEA
