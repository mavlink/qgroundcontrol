#include "NMEANavigationEpoch.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QDateTime>
#include <QtCore/QTime>
#include <QtCore/QTimeZone>

#include "NMEAMetadata.h"

namespace {
QDate qtDate(const NMEA::UtcDate& date)
{
    return QDate(date.year, static_cast<int>(date.month), static_cast<int>(date.day));
}

std::optional<double> finiteValue(double value)
{
    return std::isfinite(value) ? std::optional<double>(value) : std::nullopt;
}

std::optional<double> nonnegativeNumber(std::string_view field)
{
    const auto value = NMEA::number<double>(field);
    return value && *value >= 0.0 ? value : std::nullopt;
}
}  // namespace

namespace NMEA {

bool NavigationEpoch::hasCoordinate() const
{
    return std::isfinite(latitude) && std::isfinite(longitude);
}

std::optional<double> NavigationEpoch::altitudeEllipsoidMeters() const
{
    return altitudeMslMeters && geoidSeparationMeters
               ? std::optional<double>(*altitudeMslMeters + *geoidSeparationMeters)
               : std::nullopt;
}

NavigationEpochAssembler::NavigationEpochAssembler(NavigationEpochPolicy policy)
    : _policy(policy)
{}

void NavigationEpochAssembler::reset()
{
    _epochs = {};
    _currentKey = NO_TIME_KEY;
    _hasCurrent = false;
    _dateReference = {};
    _dateReferenceTimeMs.reset();
    _statusReceiptUs = 0;
    _statusTimeMs.reset();
    _statusDate = {};
    _sequence = 0;
    _invalidThroughSequence = 0;
    _nextRevision = 0;
    _navigationValid = true;
}

std::optional<NavigationUpdate> NavigationEpochAssembler::ingest(const Sentence& sentence, uint64_t receivedAtUs)
{
    const auto sequence = ++_sequence;
    const auto type = sentence.type();

    if (type == "ZDA") {
        return _handleDatedSentence(sentence, receivedAtUs);
    }

    if (const auto navigation = navigationStatus(sentence)) {
        QDate date;
        if (navigation->utcMilliseconds) {
            if (type == "RMC") {
                if (const auto dateField = rmcDate(sentence.fields[Field::RMC_DATE])) {
                    date = qtDate(*dateField);
                }
            }
            if (!date.isValid()) {
                date = _dateForTime(*navigation->utcMilliseconds);
            }
        }
        if (!_acceptNavigationStatus(navigation->utcMilliseconds, date, receivedAtUs)) {
            return std::nullopt;
        }
        if (navigation->utcMilliseconds && date.isValid() && type == "RMC") {
            _setDateReference(date, *navigation->utcMilliseconds);
        }
        if (!navigation->valid) {
            _navigationValid = false;
            _invalidThroughSequence = sequence;
            NavigationEpoch loss;
            loss.timeMs = navigation->utcMilliseconds;
            loss.date = date;
            loss.receivedAtUs = receivedAtUs;
            loss.positionReceivedAtUs = receivedAtUs;
            loss.sequence = sequence;
            loss.revision = ++_nextRevision;
            loss.receiverFixValid = false;
            loss.fixQuality = GPSFixQuality::NoFix;
            if (type == "GGA" && sentence.count > Field::GGA_SATELLITES_USED) {
                loss.satellitesUsed = number<unsigned>(sentence.fields[Field::GGA_SATELLITES_USED]);
            }
            return NavigationUpdate{
                .type = NavigationUpdate::Type::FixLoss, .trigger = NavigationUpdate::Trigger::Position, .epoch = loss};
        }
        _navigationValid = true;
    }

    if (auto update = _handlePositionSentence(sentence, receivedAtUs)) {
        update->epoch.sequence = sequence;
        if (auto* stored = _findExisting(update->epoch.timeMs)) {
            stored->epoch.sequence = sequence;
            update->epoch = stored->epoch;
        }
        if (!_navigationValid || update->epoch.sequence <= _invalidThroughSequence) {
            return std::nullopt;
        }
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
        freshAt(stored->epoch.verticalDopReceivedAtUs, nowUs, _policy.metadataMaxAgeUs)) {
        return std::nullopt;
    }
    stored->epoch.verticalDop.reset();
    stored->epoch.verticalDopReceivedAtUs = 0;
    return stored->epoch;
}

NavigationEpochAssembler::StoredEpoch* NavigationEpochAssembler::_find(std::optional<int> timeMs)
{
    if (!timeMs && _policy.requirePositionTime) {
        return nullptr;
    }

    const int key = timeMs.value_or(NO_TIME_KEY);
    if (auto* existing = _findExisting(timeMs)) {
        return existing;
    }

    auto oldest = _epochs.begin();
    for (auto epoch = _epochs.begin(); epoch != _epochs.end(); ++epoch) {
        if (!epoch->active) {
            _resetEpoch(*epoch, timeMs);
            return &*epoch;
        }
        if (epoch->epoch.receivedAtUs < oldest->epoch.receivedAtUs) {
            oldest = epoch;
        }
    }
    _resetEpoch(*oldest, timeMs);
    oldest->key = key;
    return &*oldest;
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
    stored.epoch.revision = ++_nextRevision;
}

void NavigationEpochAssembler::_resetExpiredEpoch(StoredEpoch& stored, const QDate& date, uint64_t receivedAtUs)
{
    const bool expired = stored.epoch.receivedAtUs && receivedAtUs > stored.epoch.receivedAtUs &&
                         !freshAt(stored.epoch.receivedAtUs, receivedAtUs, _policy.metadataMaxAgeUs);
    const bool dateChanged = date.isValid() && stored.epoch.date.isValid() && stored.epoch.date != date;
    if (!expired && !dateChanged) {
        return;
    }
    const auto timeMs = stored.epoch.timeMs;
    _resetEpoch(stored, timeMs);
}

QDate NavigationEpochAssembler::_dateForTime(int timeMs) const
{
    if (!_policy.reconstructDate || !_dateReference.isValid() || !_dateReferenceTimeMs) {
        return {};
    }
    if (*_dateReferenceTimeMs - timeMs < -HALF_DAY_MS) {
        return _dateReference.addDays(-1);
    }
    if (*_dateReferenceTimeMs - timeMs > HALF_DAY_MS) {
        return _dateReference.addDays(1);
    }
    return _dateReference;
}

void NavigationEpochAssembler::_setDateReference(const QDate& date, int timeMs)
{
    if (!date.isValid() || !_policy.reconstructDate) {
        return;
    }
    _dateReference = date;
    _dateReferenceTimeMs = timeMs;
}

bool NavigationEpochAssembler::_acceptNavigationStatus(std::optional<int> timeMs, const QDate& date,
                                                       uint64_t receivedAtUs)
{
    if (!_policy.enforceNavigationOrder) {
        return true;
    }
    if (!receivedAtUs || receivedAtUs < _statusReceiptUs) {
        return false;
    }
    if (timeMs && _statusTimeMs) {
        if (date.isValid() && _statusDate.isValid()) {
            const auto lhs = QDateTime(date, QTime::fromMSecsSinceStartOfDay(*timeMs), QTimeZone::UTC);
            const auto rhs = QDateTime(_statusDate, QTime::fromMSecsSinceStartOfDay(*_statusTimeMs), QTimeZone::UTC);
            if (lhs < rhs) {
                return false;
            }
        } else {
            int difference = *_statusTimeMs - *timeMs;
            if (difference < -HALF_DAY_MS) {
                difference += DAY_MS;
            } else if (difference > HALF_DAY_MS) {
                difference -= DAY_MS;
            }
            if (difference > 0) {
                return false;
            }
        }
    }
    _statusReceiptUs = receivedAtUs;
    if (timeMs) {
        _statusTimeMs = timeMs;
        _statusDate = date;
    }
    return true;
}

GPSFixQuality NavigationEpochAssembler::_fixQuality(const NavigationEpoch& epoch) const
{
    auto autonomous = _policy.autonomousFixQuality;
    if (_policy.useGsaDimensionForAutonomousFix) {
        autonomous = epoch.dimension == FixDimension::TWO_D     ? GPSFixQuality::Fix2D
                     : epoch.dimension == FixDimension::THREE_D ? GPSFixQuality::Fix3D
                                                                : GPSFixQuality::Unknown;
    }
    return fixQuality(epoch.ggaQuality.value_or(GgaQuality::GPS), autonomous);
}

void NavigationEpochAssembler::_markReceipt(NavigationEpoch& epoch, uint64_t receivedAtUs) const
{
    epoch.receivedAtUs = epoch.receivedAtUs == 0 ? receivedAtUs : std::min(epoch.receivedAtUs, receivedAtUs);
}

std::optional<NavigationUpdate> NavigationEpochAssembler::_handleDatedSentence(const Sentence& sentence,
                                                                               uint64_t receivedAtUs)
{
    const auto dated = zda(sentence);
    if (!dated || !dated->utcMilliseconds) {
        return std::nullopt;
    }
    const QDate date = qtDate(dated->date);
    if (!_acceptNavigationStatus(dated->utcMilliseconds, date, receivedAtUs)) {
        return std::nullopt;
    }
    _setDateReference(date, *dated->utcMilliseconds);
    auto* stored = _findExisting(*dated->utcMilliseconds);
    if (!stored) {
        return std::nullopt;
    }
    stored->epoch.date = date;
    if (!_navigationValid || stored->epoch.sequence <= _invalidThroughSequence) {
        return std::nullopt;
    }
    return NavigationUpdate{
        .type = NavigationUpdate::Type::Epoch, .trigger = NavigationUpdate::Trigger::Date, .epoch = stored->epoch};
}

std::optional<NavigationUpdate> NavigationEpochAssembler::_handlePositionSentence(const Sentence& sentence,
                                                                                  uint64_t receivedAtUs)
{
    std::optional<int> timeMs;
    if (const auto rmcFix = rmc(sentence)) {
        if (!rmcFix->utcMilliseconds) {
            return std::nullopt;
        }
        timeMs = rmcFix->utcMilliseconds;
        if (rmcFix->date) {
            _setDateReference(qtDate(*rmcFix->date), *timeMs);
        }
        auto* stored = _find(timeMs);
        if (!stored) {
            return std::nullopt;
        }
        const QDate date = _dateForTime(*timeMs);
        _resetExpiredEpoch(*stored, date, receivedAtUs);
        auto& epoch = stored->epoch;
        epoch.date = date;
        epoch.latitude = rmcFix->latitude;
        epoch.longitude = rmcFix->longitude;
        epoch.speedMetersPerSecond = finiteValue(rmcFix->speedMetersPerSecond);
        epoch.courseDegrees = finiteValue(rmcFix->courseDegrees);
        epoch.receiverFixValid = true;
        epoch.fixQuality = _fixQuality(epoch);
        _markReceipt(epoch, receivedAtUs);
        epoch.positionReceivedAtUs = receivedAtUs;
    } else if (const auto ggaFix = gga(sentence)) {
        timeMs = utcMilliseconds(sentence.fields[Field::UTC_TIME]);
        if ((!timeMs && _policy.requirePositionTime) || ggaFix->quality == GgaQuality::INVALID) {
            return std::nullopt;
        }
        auto* stored = _find(timeMs);
        if (!stored) {
            return std::nullopt;
        }
        const QDate date = timeMs ? _dateForTime(*timeMs) : QDate();
        _resetExpiredEpoch(*stored, date, receivedAtUs);
        auto& epoch = stored->epoch;
        epoch.date = date;
        epoch.ggaQuality = ggaFix->quality;
        epoch.latitude = ggaFix->latitude;
        epoch.longitude = ggaFix->longitude;
        epoch.altitudeMslMeters = finiteValue(ggaFix->altitude);
        epoch.geoidSeparationMeters = finiteValue(ggaFix->geoidSeparation);
        epoch.satellitesUsed = ggaFix->satellitesUsed;
        epoch.horizontalDop = finiteValue(ggaFix->hdop);
        epoch.horizontalDopReceivedAtUs = receivedAtUs;
        epoch.dopReceivedAtUs = receivedAtUs;
        epoch.fixQuality = _fixQuality(epoch);
        epoch.receiverFixValid = true;
        if (!epoch.accuracyReceivedAtUs ||
            !freshAt(epoch.accuracyReceivedAtUs, receivedAtUs, _policy.metadataMaxAgeUs)) {
            epoch.horizontalAccuracyMeters.reset();
            epoch.verticalAccuracyMeters.reset();
            epoch.accuracyReceivedAtUs = 0;
        }
        _markReceipt(epoch, receivedAtUs);
        epoch.positionReceivedAtUs = receivedAtUs;
    } else if (const auto gllFix = gll(sentence)) {
        if (!gllFix->utcMilliseconds) {
            return std::nullopt;
        }
        timeMs = gllFix->utcMilliseconds;
        auto* stored = _find(timeMs);
        if (!stored) {
            return std::nullopt;
        }
        const QDate date = _dateForTime(*timeMs);
        _resetExpiredEpoch(*stored, date, receivedAtUs);
        auto& epoch = stored->epoch;
        epoch.date = date;
        epoch.latitude = gllFix->latitude;
        epoch.longitude = gllFix->longitude;
        epoch.receiverFixValid = true;
        epoch.fixQuality = _fixQuality(epoch);
        _markReceipt(epoch, receivedAtUs);
        epoch.positionReceivedAtUs = receivedAtUs;
    } else {
        return std::nullopt;
    }

    _currentKey = timeMs.value_or(NO_TIME_KEY);
    _hasCurrent = true;
    auto* stored = _findExisting(timeMs);
    if (!stored) {
        return std::nullopt;
    }
    return NavigationUpdate{
        .type = NavigationUpdate::Type::Epoch, .trigger = NavigationUpdate::Trigger::Position, .epoch = stored->epoch};
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
    auto* stored = _find(timeMs);
    if (!stored) {
        return std::nullopt;
    }
    const QDate date = _dateForTime(*timeMs);
    _resetExpiredEpoch(*stored, date, receivedAtUs);
    auto& epoch = stored->epoch;
    if (date.isValid()) {
        epoch.date = date;
    }
    epoch.horizontalAccuracyMeters = finiteValue(accuracy->horizontalAccuracy);
    epoch.verticalAccuracyMeters = finiteValue(accuracy->verticalAccuracy);
    epoch.accuracyReceivedAtUs = receivedAtUs;
    _markReceipt(epoch, receivedAtUs);
    if (!_hasCurrent || _currentKey != stored->key) {
        return std::nullopt;
    }
    if (!_navigationValid || epoch.sequence <= _invalidThroughSequence) {
        return std::nullopt;
    }
    return NavigationUpdate{
        .type = NavigationUpdate::Type::Epoch, .trigger = NavigationUpdate::Trigger::TimedMetadata, .epoch = epoch};
}

std::optional<NavigationUpdate> NavigationEpochAssembler::_handleUntimedMetadata(const Sentence& sentence,
                                                                                 uint64_t receivedAtUs)
{
    auto* stored = _current();
    if (!stored) {
        return std::nullopt;
    }
    const auto freshnessReceipt =
        _policy.untimedMetadataUsesPositionReceipt ? stored->epoch.positionReceivedAtUs : stored->epoch.receivedAtUs;
    if (!freshnessReceipt || !freshAt(freshnessReceipt, receivedAtUs, _policy.untimedMetadataMaxAgeUs)) {
        return std::nullopt;
    }

    auto& epoch = stored->epoch;
    const auto type = sentence.type();
    if (type == "GSA") {
        if (sentence.count < Field::GSA_MIN_FIELDS) {
            return std::nullopt;
        }
        const auto hdop = nonnegativeNumber(sentence.fields[Field::GSA_HDOP]);
        const auto vdop = nonnegativeNumber(sentence.fields[Field::GSA_VDOP]);
        if (!epoch.horizontalDop) {
            epoch.horizontalDop = hdop;
            if (hdop) {
                epoch.horizontalDopReceivedAtUs = receivedAtUs;
            }
        }
        epoch.verticalDop = vdop;
        epoch.verticalDopReceivedAtUs = vdop ? receivedAtUs : 0;
        epoch.dopReceivedAtUs = receivedAtUs;
        const auto dimension = number<unsigned>(sentence.fields[Field::GSA_DIMENSION]);
        epoch.dimension = dimension;
        epoch.fixQuality = _fixQuality(epoch);
    } else if (const auto velocity = vtg(sentence)) {
        epoch.speedMetersPerSecond = finiteValue(velocity->speedMetersPerSecond);
        epoch.courseDegrees = finiteValue(velocity->courseDegrees);
    } else {
        return std::nullopt;
    }
    if (!_navigationValid || epoch.sequence <= _invalidThroughSequence) {
        return std::nullopt;
    }
    return NavigationUpdate{
        .type = NavigationUpdate::Type::Epoch, .trigger = NavigationUpdate::Trigger::UntimedMetadata, .epoch = epoch};
}

}  // namespace NMEA
