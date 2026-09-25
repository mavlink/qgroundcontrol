#include "NTRIPGgaProvider.h"

#include <QtCore/QDateTime>

#include "NMEASentence.h"
#include "NMEAUtils.h"
#include "NTRIPTransport.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPGgaProviderLog, "GPS.NTRIP.NTRIPGgaProvider")

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
        case NTRIPGgaProvider::PositionSource::RTKReceiver:
            return QStringLiteral("RTKReceiver");
        case NTRIPGgaProvider::PositionSource::GCSPosition:
            return QStringLiteral("GCSPosition");
    }

    return QStringLiteral("Unknown");
}

unsigned ggaQuality(GPSObservation::FixQuality quality)
{
    using Quality = GPSObservation::FixQuality;
    switch (quality) {
        case Quality::NoFix:
            return NMEA::GgaQuality::INVALID;
        case Quality::Differential:
            return NMEA::GgaQuality::DIFFERENTIAL;
        case Quality::RTKFloat:
            return NMEA::GgaQuality::RTK_FLOAT;
        case Quality::RTKFixed:
            return NMEA::GgaQuality::RTK_FIXED;
        case Quality::Extrapolated:
        case Quality::Unknown:
            return NMEA::GgaQuality::ESTIMATED;
        case Quality::Fix2D:
        case Quality::Fix3D:
            return NMEA::GgaQuality::GPS;
    }
    return NMEA::GgaQuality::INVALID;
}
}  // namespace

NTRIPGgaProvider::NTRIPGgaProvider(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _ggaTask(_scheduler, this)
{}

void NTRIPGgaProvider::configure(const Configuration& configuration)
{
    _cachedSource = configuration.source;
    const auto previousInterval = _normalInterval;
    _normalInterval = configuration.interval.count() > 0 ? configuration.interval : kDefaultInterval;
    if (_retryPhase == RetryPhase::Normal && previousInterval != _normalInterval && _transport) {
        _scheduleNextGGA();
    }
}

void NTRIPGgaProvider::setPositionProvider(PositionSource source, PositionProvider provider)
{
    _providers[source] = std::move(provider);
}

void NTRIPGgaProvider::start(NTRIPTransport* transport)
{
    const auto session = _generation.advance(this);
    _transport = transport;
    _fastRetryCount = 0;
    _selectionDiagnostic.clear();
    _clearSource();
    if (!session.isCurrent() || !_transport) {
        return;
    }
    _setRetryPhase(RetryPhase::Fast);
    _sendGGA();
    if (session.isCurrent() && _transport == transport) {
        _scheduleNextGGA();
    }
}

void NTRIPGgaProvider::stop()
{
    _generation.invalidate();
    _ggaTask.cancel();
    _transport = nullptr;
    _clearSource();
}

void NTRIPGgaProvider::_scheduleNextGGA()
{
    _ggaTask.cancel();
    const auto transport = _transport;
    const auto session = _generation.current(this);
    if (!transport) {
        return;
    }
    _ggaTask.schedule(_currentInterval(), [this, transport, session]() {
        if (!session.isCurrent() || !transport || _transport != transport) {
            return;
        }
        _sendGGA();
        if (session.isCurrent() && transport && _transport == transport) {
            _scheduleNextGGA();
        }
    });
}

std::chrono::milliseconds NTRIPGgaProvider::_currentInterval() const
{
    return _retryPhase == RetryPhase::Fast ? kFastRetryInterval : _normalInterval;
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
    if (!_transport) {
        return;
    }
    const auto transport = _transport;
    const auto current = [this, transport, session = _generation.current(this)]() {
        return session.isCurrent() && transport && _transport == transport;
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
        _clearSource();
        if (!current()) {
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

    const NMEA::GGA fix{
        .latitude = position.coordinate.latitude(),
        .longitude = position.coordinate.longitude(),
        .altitude = position.coordinate.altitude(),
        .hdop = position.horizontalDop && qIsFinite(*position.horizontalDop) && *position.horizontalDop >= 0
                    ? *position.horizontalDop
                    : qQNaN(),
        .quality = ggaQuality(position.fixQuality),
        .satellitesUsed = position.satellitesUsed && *position.satellitesUsed >= 0
                              ? std::optional<unsigned>(static_cast<unsigned>(*position.satellitesUsed))
                              : std::nullopt,
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
        PositionSource::RTKReceiver,
        PositionSource::GCSPosition,
    };
    const auto session = _generation.current(this);
    for (PositionSource source : kPriority) {
        const auto provider = providers.value(source);
        if (provider) {
            const auto result = provider();
            if (!session.isCurrent()) {
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
