#include "NTRIPGgaProvider.h"

#include <QtCore/QDateTime>

#include "Fact.h"
#include "GPSSourceHealth.h"
#include "NMEAUtils.h"
#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPGgaProviderLog, "GPS.NTRIP.NTRIPGgaProvider")

bool PositionResult::isValid() const
{
    if (!observation.acceptedPosition(GPSObservation::PositionUse::Gga).isValid()) {
        return false;
    }
    const qint64 age = observation.ageMilliseconds();
    return fixedReference ||
           (observation.monotonicTimestampUs != 0 && age >= 0 && age < GPSSourceHealth::FRESHNESS_TIMEOUT_MS);
}

NTRIPGgaProvider::NTRIPGgaProvider(QObject* parent) : QObject(parent)
{
    qCDebug(NTRIPGgaProviderLog) << this;
    _timer.setInterval(_normalInterval);
    connect(&_timer, &QChronoTimer::timeout, this, &NTRIPGgaProvider::_sendGGA);
}

NTRIPGgaProvider::~NTRIPGgaProvider()
{
    qCDebug(NTRIPGgaProviderLog) << this;
}

void NTRIPGgaProvider::init(NTRIPSettings* settings)
{
    // Cache the user-selected source and interval so the hot path (_sendGGA)
    // avoids a SettingsManager::instance()->ntripSettings()->...->rawValue()
    // chain per tick.
    if (!settings) {
        return;
    }

    auto* sourceFact = settings->ntripGgaPositionSource();
    auto refreshSource = [this, sourceFact]() {
        _cachedSource = static_cast<PositionSource>(sourceFact->rawValue().toUInt());
    };
    refreshSource();
    connect(sourceFact, &Fact::rawValueChanged, this, refreshSource);

    auto* intervalFact = settings->ntripGgaIntervalSec();
    auto refreshInterval = [this, intervalFact]() {
        const uint seconds = intervalFact->rawValue().toUInt();
        // Guard against 0 from a stale config — fall back to the default.
        _normalInterval =
            (seconds > 0) ? std::chrono::milliseconds{static_cast<qint64>(seconds) * 1000} : kDefaultInterval;
        if (_retryPhase == RetryPhase::Normal) {
            _timer.setInterval(_normalInterval);
        }
    };
    refreshInterval();
    connect(intervalFact, &Fact::rawValueChanged, this, refreshInterval);
}

void NTRIPGgaProvider::setPositionProvider(PositionSource source, PositionProvider provider)
{
    _providers[source] = std::move(provider);
}

void NTRIPGgaProvider::start(SentenceWriter writer)
{
    const QPointer<NTRIPGgaProvider> guard(this);
    const auto generation = ++_generation;
    _timer.stop();
    _writer = std::move(writer);
    _fastRetryCount = 0;
    _clearSource();
    if (!guard || generation != _generation || !_writer) {
        return;
    }
    _setRetryPhase(RetryPhase::Fast);
    _sendGGA();
    if (guard && generation == _generation) {
        _timer.start();
    }
}

void NTRIPGgaProvider::stop()
{
    ++_generation;
    _timer.stop();
    _writer = {};
    _clearSource();
}

void NTRIPGgaProvider::_setRetryPhase(RetryPhase phase)
{
    _retryPhase = phase;
    _timer.setInterval(phase == RetryPhase::Fast ? kFastRetryInterval : _normalInterval);
}

void NTRIPGgaProvider::_clearSource()
{
    if (_source.isEmpty()) {
        return;
    }
    _source.clear();
    emit sourceChanged(_source);
}

void NTRIPGgaProvider::_sendGGA()
{
    if (!_writer) {
        return;
    }

    const QPointer<NTRIPGgaProvider> guard(this);
    const auto generation = _generation;
    const auto writer = _writer;

    const auto position = _getBestPosition();
    if (!guard || generation != _generation) {
        return;
    }

    if (!position.isValid()) {
        _clearSource();
        if (!guard || generation != _generation) {
            return;
        }
        if (++_fastRetryCount >= 5 && _retryPhase == RetryPhase::Fast) {
            _setRetryPhase(RetryPhase::Normal);
        }
        return;
    }

    _fastRetryCount = 0;
    if (_retryPhase != RetryPhase::Normal) {
        _setRetryPhase(RetryPhase::Normal);
    }

    const QByteArray gga = NMEAUtils::makeGGA(position.observation);
    writer(gga);
    if (!guard || generation != _generation) {
        return;
    }

    if (!position.source.isEmpty() && position.source != _source) {
        _source = position.source;
        emit sourceChanged(_source);
    }
}

PositionResult NTRIPGgaProvider::_getBestPosition() const
{
    const QPointer<const NTRIPGgaProvider> guard(this);
    const auto generation = _generation;
    const PositionSource source = _cachedSource;

    // If a specific source is requested, try only that one
    if (source != PositionSource::Auto) {
        auto it = _providers.find(source);
        if (it != _providers.end()) {
            const auto provider = it.value();
            return provider();
        }
        return {};
    }

    // Auto: try each provider in priority order
    static constexpr PositionSource kPriority[] = {
        PositionSource::VehicleGPS,
        PositionSource::VehicleEKF,
        PositionSource::RTKBase,
        PositionSource::GCSPosition,
    };

    for (PositionSource s : kPriority) {
        auto it = _providers.find(s);
        if (it != _providers.end()) {
            const auto provider = it.value();
            auto result = provider();
            if (!guard || generation != _generation) {
                return {};
            }
            if (result.isValid()) {
                return result;
            }
        }
    }

    return {};
}
