#include "NTRIPGgaProvider.h"

#include "NMEAUtils.h"
#include "NTRIPTransport.h"

NTRIPGgaProvider::NTRIPGgaProvider(QObject* parent) : QObject(parent)
{
    _timer.setInterval(_normalInterval);
    connect(&_timer, &QChronoTimer::timeout, this, &NTRIPGgaProvider::_sendGGA);
}

void NTRIPGgaProvider::configure(const Configuration& configuration)
{
    _cachedSource = configuration.source;
    _normalInterval = configuration.interval.count() > 0 ? configuration.interval : kDefaultInterval;
    if (_retryPhase == RetryPhase::Normal && _timer.interval() != _normalInterval) {
        _timer.setInterval(_normalInterval);
    }
}

void NTRIPGgaProvider::setPositionProvider(PositionSource source, PositionProvider provider)
{
    _providers[source] = std::move(provider);
}

void NTRIPGgaProvider::start(NTRIPTransport* transport)
{
    const QPointer<NTRIPGgaProvider> guard(this);
    const quint64 generation = ++_generation;
    _transport = transport;
    _fastRetryCount = 0;
    _clearSource();
    if (!guard || _generation != generation || !_transport) {
        return;
    }
    _setRetryPhase(RetryPhase::Fast);
    _timer.start();
    _sendGGA();
}

void NTRIPGgaProvider::stop()
{
    ++_generation;
    _timer.stop();
    _transport = nullptr;
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
    if (!_transport) {
        return;
    }
    const QPointer<NTRIPGgaProvider> guard(this);
    const auto transport = _transport;
    const quint64 generation = _generation;
    const auto current = [this, guard, transport, generation]() {
        return guard && transport && _transport == transport && _generation == generation;
    };
    const auto position = _getBestPosition();
    if (!current()) {
        return;
    }
    if (!position.isValid()) {
        if (++_fastRetryCount >= 5 && _retryPhase == RetryPhase::Fast) {
            _setRetryPhase(RetryPhase::Normal);
        }
        return;
    }

    _fastRetryCount = 0;
    if (_retryPhase != RetryPhase::Normal) {
        _setRetryPhase(RetryPhase::Normal);
    }

    const QByteArray gga = NMEAUtils::makeGGA(position.coordinate, position.coordinate.altitude());
    transport->sendNMEA(gga);
    if (!current()) {
        return;
    }
    if (!position.source.isEmpty() && position.source != _source) {
        _source = position.source;
        emit sourceChanged(_source);
    }
}

PositionResult NTRIPGgaProvider::_getBestPosition() const
{
    const auto providers = _providers;
    if (_cachedSource != PositionSource::Auto) {
        const auto provider = providers.value(_cachedSource);
        return provider ? provider() : PositionResult{};
    }

    static constexpr PositionSource kPriority[] = {
        PositionSource::VehicleGPS,
        PositionSource::VehicleEKF,
        PositionSource::RTKBase,
        PositionSource::GCSPosition,
    };
    const QPointer<const NTRIPGgaProvider> guard(this);
    const quint64 generation = _generation;
    for (PositionSource source : kPriority) {
        const auto provider = providers.value(source);
        if (provider) {
            const auto result = provider();
            if (!guard || _generation != generation) {
                return {};
            }
            if (result.isValid()) {
                return result;
            }
        }
    }
    return {};
}
