#include "RTCMStreamContent.h"

#include <bit>

#include "RTCMFramer.h"
#include "RTCMMsmHeader.h"
#include "RTCMStationPosition.h"

namespace {
constexpr uint16_t STREAM_GLONASS_BIASES_MESSAGE = 1230;

/// The constellation of the observation messages older than MSM: 1001 to 1004 are GPS and 1009 to 1012 GLONASS.
GPSConstellation legacyObservationConstellation(uint16_t messageId)
{
    if (messageId >= 1001 && messageId <= 1004) {
        return GPSConstellation::GPS;
    }
    if (messageId >= 1009 && messageId <= 1012) {
        return GPSConstellation::GLONASS;
    }
    return GPSConstellation::Unknown;
}
}  // namespace

void RTCMStreamContent::record(QByteArrayView frame, qint64 receivedAtMs)
{
    if (!_startedAtMs) {
        _startedAtMs = receivedAtMs;
    }
    const uint16_t messageId = RTCMFramer::frameMessageId(frame);
    if (messageId == STREAM_GLONASS_BIASES_MESSAGE) {
        _glonassBiases = true;
    } else if (const GPSConstellation legacy = legacyObservationConstellation(messageId);
               legacy != GPSConstellation::Unknown) {
        _constellations[static_cast<size_t>(legacy)].observedAtMs = receivedAtMs;
    } else if (const auto header = RTCMMsmHeader::fromFrame(frame)) {
        // A message without satellites has no observations.
        if (header->satelliteMask == 0) {
            return;
        }
        Constellation& constellation = _constellations[static_cast<size_t>(header->constellation)];
        // An epoch can take several messages, each with some of its satellites.
        const bool sameEpoch = constellation.msmAtMs && constellation.epochTime == header->epochTime;
        constellation.satelliteMask = header->satelliteMask | (sameEpoch ? constellation.satelliteMask : 0);
        constellation.epochTime = header->epochTime;
        constellation.msmAtMs = receivedAtMs;
        constellation.observedAtMs = receivedAtMs;
    } else if (RTCMStationPosition::fromFrame(frame)) {
        _stationPosition = true;
    }
}

void RTCMStreamContent::clear()
{
    *this = RTCMStreamContent{};
}

bool RTCMStreamContent::_within(const std::optional<qint64>& atMs, qint64 nowMs)
{
    return atMs && nowMs - *atMs < WINDOW.count();
}

bool RTCMStreamContent::_settled(qint64 nowMs) const
{
    return _startedAtMs && nowMs - *_startedAtMs >= WINDOW.count();
}

QList<RTCMStreamContent::Observed> RTCMStreamContent::observed(qint64 nowMs) const
{
    QList<Observed> observed;
    for (size_t index = 0; index < _constellations.size(); ++index) {
        const Constellation& constellation = _constellations[index];
        if (!_within(constellation.observedAtMs, nowMs)) {
            continue;
        }
        Observed entry{.constellation = static_cast<GPSConstellation>(index), .satellites = std::nullopt};
        if (_within(constellation.msmAtMs, nowMs)) {
            entry.satellites = std::popcount(constellation.satelliteMask);
        }
        observed.append(entry);
    }
    return observed;
}

RTCMStreamContent::Problem RTCMStreamContent::problem(qint64 nowMs) const
{
    if (!_settled(nowMs)) {
        return Problem::None;
    }
    if (observed(nowMs).isEmpty()) {
        return Problem::NoObservations;
    }
    return _stationPosition ? Problem::None : Problem::NoStationPosition;
}

bool RTCMStreamContent::glonassWithoutBiases(qint64 nowMs) const
{
    return _settled(nowMs) && !_glonassBiases &&
           _within(_constellations[static_cast<size_t>(GPSConstellation::GLONASS)].observedAtMs, nowMs);
}
