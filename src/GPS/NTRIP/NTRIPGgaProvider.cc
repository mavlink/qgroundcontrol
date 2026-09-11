#include "NTRIPGgaProvider.h"

#include <QtCore/QDateTime>

#include "GPSSourceHealth.h"
#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPGgaProviderLog, "GPS.NTRIP.NTRIPGgaProvider")

bool PositionResult::isValid(quint64 nowUs) const
{
    if (!observation.acceptedPosition(GPSObservation::PositionUse::Gga).isValid()) {
        return false;
    }
    const qint64 age = nowUs >= observation.monotonicTimestampUs
                           ? static_cast<qint64>((nowUs - observation.monotonicTimestampUs) / 1000)
                           : -1;
    return fixedReference ||
           (observation.monotonicTimestampUs != 0 && age >= 0 && age < GPSSourceHealth::FRESHNESS_TIMEOUT_MS);
}

NTRIPGgaProvider::NTRIPGgaProvider(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _task(_scheduler, this)
{
    qCDebug(NTRIPGgaProviderLog) << this;
}

NTRIPGgaProvider::~NTRIPGgaProvider()
{
    qCDebug(NTRIPGgaProviderLog) << this;
}

void NTRIPGgaProvider::configure(const Configuration& config)
{
    _cachedSource = config.source;
    _normalInterval = config.interval.count() > 0 ? config.interval : kDefaultInterval;
    if (_writer)
        _scheduleNext();
}

void NTRIPGgaProvider::_scheduleNext()
{
    const auto generation = _generation;
    _task.schedule(_retryPhase == RetryPhase::Fast ? kFastRetryInterval : _normalInterval, [this, generation] {
        const QPointer<NTRIPGgaProvider> guard(this);
        _sendGGA();
        if (guard && generation == _generation && _writer)
            _scheduleNext();
    });
}

void NTRIPGgaProvider::setPositionProvider(PositionSource source, PositionProvider provider)
{
    _providers[source] = std::move(provider);
}

void NTRIPGgaProvider::start(SentenceWriter writer)
{
    const QPointer<NTRIPGgaProvider> guard(this);
    const auto generation = ++_generation;
    _task.cancel();
    _writer = std::move(writer);
    _fastRetryCount = 0;
    _clearSource();
    if (!guard || generation != _generation || !_writer) {
        return;
    }
    _setRetryPhase(RetryPhase::Fast);
    _sendGGA();
    if (guard && generation == _generation) {
        _scheduleNext();
    }
}

void NTRIPGgaProvider::stop()
{
    ++_generation;
    _task.cancel();
    _writer = {};
    _clearSource();
}

void NTRIPGgaProvider::_setRetryPhase(RetryPhase phase)
{
    _retryPhase = phase;
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
    if (!_writer || !_scheduler) {
        return;
    }

    const QPointer<NTRIPGgaProvider> guard(this);
    const auto generation = _generation;
    const auto writer = _writer;

    const auto position = _getBestPosition();
    if (!guard || generation != _generation || !_scheduler) {
        return;
    }

    if (!position.isValid(_scheduler->nowUs())) {
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
            if (!guard || generation != _generation || !_scheduler) {
                return {};
            }
            if (result.isValid(_scheduler->nowUs())) {
                return result;
            }
        }
    }

    return {};
}
