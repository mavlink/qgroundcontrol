#include "NTRIPGgaReporter.h"

#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QMetaEnum>
#include <QtCore/qnumeric.h>

#include "NMEASentence.h"
#include "NTRIPTransport.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPGgaReporterLog, "GPS.NTRIPGgaProvider")

namespace {
QString sourceName(NTRIPGgaReporter::PositionSource source)
{
    return QString::fromLatin1(
        QMetaEnum::fromType<NTRIPGgaReporter::PositionSource>().valueToKey(static_cast<int>(source)));
}

unsigned ggaQuality(GPSFixQuality quality)
{
    using Quality = GPSFixQuality;
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

bool reportable(const std::optional<GPSObservation>& observation)
{
    if (!observation || observation->altitudeDatum != GPSAltitudeDatum::MeanSeaLevel ||
        observation->fixQuality == GPSFixQuality::NoFix) {
        return false;
    }
    const QGeoCoordinate coordinate = observation->position.coordinate();
    return coordinate.isValid() && qIsFinite(coordinate.altitude());
}
}  // namespace

NTRIPGgaReporter::NTRIPGgaReporter(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _ggaTask(_scheduler, this)
{}

QString NTRIPGgaReporter::_sourceLabel(PositionSource source)
{
    switch (source) {
        case PositionSource::VehicleGPS:
            return tr("Vehicle GPS");
        case PositionSource::VehicleEKF:
            return tr("Vehicle EKF");
        case PositionSource::RTKReceiver:
            return tr("RTK Receiver");
        case PositionSource::GCSPosition:
            return tr("GCS Position");
        case PositionSource::Auto:
            break;
    }
    return {};
}

void NTRIPGgaReporter::configure(const Configuration& configuration)
{
    _requestedSource = configuration.source;
    const auto previousInterval = _normalInterval;
    _normalInterval = configuration.interval.count() > 0 ? configuration.interval : DEFAULT_INTERVAL;
    if (!_awaitingFirstGga && previousInterval != _normalInterval && _transport) {
        _scheduleNextGGA();
    }
}

void NTRIPGgaReporter::setPositionProvider(PositionSource source, PositionProvider provider)
{
    _providers[source] = std::move(provider);
}

void NTRIPGgaReporter::start(NTRIPTransport* transport)
{
    _ggaTask.cancel();
    _transport = transport;
    _loggedSelection.reset();
    _clearSource();
    if (!_transport) {
        return;
    }
    _awaitingFirstGga = true;
    _sendGGA();
    _scheduleNextGGA();
}

void NTRIPGgaReporter::stop()
{
    _ggaTask.cancel();
    _transport = nullptr;
    _clearSource();
}

void NTRIPGgaReporter::_scheduleNextGGA()
{
    if (!_transport) {
        _ggaTask.cancel();
        return;
    }
    // Rescheduling replaces the pending send, and start()/stop() cancel it, so a send never outlives its transport.
    _ggaTask.schedule(_currentInterval(), [this]() {
        _sendGGA();
        _scheduleNextGGA();
    });
}

std::chrono::milliseconds NTRIPGgaReporter::_currentInterval() const
{
    return _awaitingFirstGga ? FAST_RETRY_INTERVAL : _normalInterval;
}

void NTRIPGgaReporter::_clearSource()
{
    if (_source.isEmpty()) {
        return;
    }
    _source.clear();
    emit sourceChanged(_source);
}

void NTRIPGgaReporter::_sendGGA()
{
    if (!_transport) {
        return;
    }
    const auto requested = _requestedSource;
    const auto selection = _getBestPosition(requested);
    _logSelection(requested, selection);
    if (!selection.position) {
        _clearSource();
        return;
    }

    _awaitingFirstGga = false;

    const GPSObservation& position = *selection.position;
    const QGeoCoordinate coordinate = position.position.coordinate();
    const NMEA::GGA fix{
        .latitude = coordinate.latitude(),
        .longitude = coordinate.longitude(),
        .altitude = coordinate.altitude(),
        .hdop = position.horizontalDop && qIsFinite(*position.horizontalDop) && *position.horizontalDop >= 0
                    ? *position.horizontalDop
                    : qQNaN(),
        .quality = ggaQuality(position.fixQuality),
        .satellitesUsed = position.satellitesUsed && *position.satellitesUsed >= 0
                              ? std::optional<unsigned>(static_cast<unsigned>(*position.satellitesUsed))
                              : std::nullopt,
    };
    const QByteArray gga = NMEAUtils::makeGGA(fix, QDateTime::currentDateTimeUtc().time());
    _transport->sendNMEA(gga);
    if (const QString label = _sourceLabel(selection.source); label != _source) {
        _source = label;
        emit sourceChanged(_source);
    }
}

NTRIPGgaReporter::SelectedPosition NTRIPGgaReporter::_getBestPosition(PositionSource requested) const
{
    if (requested != PositionSource::Auto) {
        const auto provider = _providers.value(requested);
        auto position = provider ? provider() : std::nullopt;
        return {reportable(position) ? std::move(position) : std::nullopt, requested};
    }

    static constexpr PositionSource kPriority[] = {
        PositionSource::VehicleGPS,
        PositionSource::VehicleEKF,
        PositionSource::RTKReceiver,
        PositionSource::GCSPosition,
    };
    for (PositionSource source : kPriority) {
        if (const auto provider = _providers.value(source)) {
            if (auto position = provider(); reportable(position)) {
                return {std::move(position), source};
            }
        }
    }
    return {};
}

void NTRIPGgaReporter::_logSelection(PositionSource requested, const SelectedPosition& selection)
{
    const auto current = std::make_tuple(requested, selection.source, selection.position.has_value());
    if (std::exchange(_loggedSelection, current) == current) {
        return;
    }
    if (!selection.position) {
        qCDebug(NTRIPGgaReporterLog).noquote()
            << QStringLiteral("GGA source selection: requested=%1 no eligible source").arg(sourceName(requested));
        return;
    }
    const bool fallback = requested == PositionSource::Auto && selection.source != PositionSource::VehicleGPS;
    const QString fallbackText = fallback ? QStringLiteral("yes") : QStringLiteral("no");
    qCDebug(NTRIPGgaReporterLog).noquote()
        << QStringLiteral("GGA source selection: requested=%1 provider=%2 fallback=%3")
               .arg(sourceName(requested), sourceName(selection.source), fallbackText);
}
