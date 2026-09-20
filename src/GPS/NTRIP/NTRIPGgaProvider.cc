#include "NTRIPGgaProvider.h"

#include <QtCore/QDateTime>

#include "NMEASentence.h"
#include "NMEAUtils.h"
#include "NTRIPTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPGgaProviderLog, "GPS.NTRIPGgaProvider")

namespace {
QString sourceName(NTRIPGgaProvider::PositionSource source)
{
    switch (source) {
        case NTRIPGgaProvider::PositionSource::Auto:
            return QStringLiteral("Auto");
        case NTRIPGgaProvider::PositionSource::VehicleGPS:
            return QStringLiteral("VehicleGPS");
        case NTRIPGgaProvider::PositionSource::VehicleEKF:
            return QStringLiteral("VehicleEKF");
        case NTRIPGgaProvider::PositionSource::RTKBase:
            return QStringLiteral("RTKBase");
        case NTRIPGgaProvider::PositionSource::GCSPosition:
            return QStringLiteral("GCSPosition");
    }
    return QStringLiteral("Unknown");
}
}  // namespace

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
    _selectionDiagnostic.clear();
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
    const auto requested = _cachedSource;
    const auto selection = _getBestPosition(requested);
    if (!current()) {
        return;
    }
    _updateSelectionDiagnostic(requested, selection);
    if (!current()) {
        return;
    }
    const auto& position = selection.position;
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

    // Preserve nominal fix metadata; position providers do not supply geoid separation.
    const NMEA::GGA fix{
        .latitude = position.coordinate.latitude(),
        .longitude = position.coordinate.longitude(),
        .altitude = position.coordinate.altitude(),
        .hdop = 1.0,
        .quality = NMEA::GgaQuality::GPS,
        .satellitesUsed = 12,
    };
    const QByteArray gga = NMEAUtils::makeGGA(fix, QDateTime::currentDateTimeUtc().time());
    transport->sendNMEA(gga);
    if (!current()) {
        return;
    }
    if (!position.source.isEmpty() && position.source != _source) {
        _source = position.source;
        emit sourceChanged(_source);
    }
}

NTRIPGgaProvider::SelectedPosition NTRIPGgaProvider::_getBestPosition(PositionSource requested) const
{
    const auto providers = _providers;
    if (requested != PositionSource::Auto) {
        const auto provider = providers.value(requested);
        return {provider ? provider() : PositionResult{}, requested};
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
                return {result, source};
            }
        }
    }
    return {};
}

void NTRIPGgaProvider::_updateSelectionDiagnostic(PositionSource requested, const SelectedPosition& selection)
{
    const bool fallback = requested == PositionSource::Auto && selection.source != PositionSource::VehicleGPS;
    const QString diagnostic =
        selection.position.isValid()
            ? QStringLiteral("GGA source selection: requested=%1 provider=%2 fallback=%3")
                  .arg(sourceName(requested), sourceName(selection.source),
                       fallback ? QStringLiteral("yes") : QStringLiteral("no"))
            : QStringLiteral("GGA source selection: requested=%1 no eligible source").arg(sourceName(requested));
    if (_selectionDiagnostic == diagnostic) {
        return;
    }
    _selectionDiagnostic = diagnostic;
    qCDebug(NTRIPGgaProviderLog).noquote() << diagnostic;
}
