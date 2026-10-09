#include "GPSNMEAStream.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "GPSCommandChannel.h"
#include "GPSDeadline.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolMath.h"
#include "NMEASentence.h"

namespace {

void applyNavigationEpoch(GPSDecodedPosition& report, const NMEA::NavigationEpoch& epoch)
{
    report.navigation.latitudeDegrees = epoch.latitude;
    report.navigation.longitudeDegrees = epoch.longitude;
    report.navigation.altitudeMslMeters = epoch.altitudeMslMeters.value_or(std::numeric_limits<double>::quiet_NaN());
    report.navigation.altitudeEllipsoidMeters =
        epoch.altitudeEllipsoidMeters().value_or(std::numeric_limits<double>::quiet_NaN());
    report.navigation.horizontalDop = static_cast<float>(epoch.horizontalDop.value_or(NAN));
    report.navigation.verticalDop = static_cast<float>(epoch.verticalDop.value_or(NAN));
    report.navigation.horizontalAccuracyMeters = static_cast<float>(epoch.horizontalAccuracyMeters.value_or(NAN));
    report.navigation.verticalAccuracyMeters = static_cast<float>(epoch.verticalAccuracyMeters.value_or(NAN));
    report.navigation.satellitesUsed = gpsSatellitesUsed(epoch.satellitesUsed);
    report.navigation.fixType = epoch.fixQuality;
    report.navigation.timestampUs = epoch.positionReceivedAtUs;
    report.navigation.speedMetersPerSecond = static_cast<float>(epoch.speedMetersPerSecond.value_or(NAN));
    report.navigation.courseRadians =
        epoch.courseDegrees ? static_cast<float>(*epoch.courseDegrees * GPSProtocolMath::DEG_TO_RAD) : NAN;
    report.velocityValid = epoch.speedMetersPerSecond || epoch.courseDegrees;
}

GPSDecodedSatellites satelliteReport(const NMEA::SatelliteSystem& system)
{
    GPSDecodedSatellites report;
    report.fullSnapshot = false;
    auto* constellation = report.ensureConstellation(system.constellation);
    if (!constellation) {
        return report;
    }
    constellation->inViewTimestampUs = system.inViewTimestampUs;
    constellation->inView = system.inView;
    if (system.inUse) {
        constellation->inUseTimestampUs = system.inUseTimestampUs;
        constellation->inUse = *system.inUse;
    }
    return report;
}

}  // namespace

GPSNMEAStream::GPSNMEAStream(Navigation navigation)
    : _navigation(navigation)
{}

void GPSNMEAStream::reset(GPSStreamDemux& stream)
{
    stream.reset();
    _satelliteAssembler.clear();
    _pendingSatellites.clear();
    _navigationAssembler.reset();
    _position = {};
}

GPSReceiveUpdates GPSNMEAStream::_decodeRTCM(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (_rtcmEnabled) {
        context.publishRTCM(frame.bytes);
    }
    return {};
}

GPSReceiveUpdates GPSNMEAStream::_decodeStandard(std::string_view line, GPSDecodeContext& context)
{
    const auto sentence = NMEA::sentence(line);
    if (!sentence) {
        return {};
    }
    const uint64_t now = context.nowUs();
    GPSReceiveUpdates updates = GPSReceiveUpdate::Activity;
    auto satelliteUpdate = _satelliteAssembler.ingest(*sentence, now);
    _queueSatellites(satelliteUpdate.completed);
    if (_navigation == Navigation::ReceiverSpecific) {
        return updates;
    }
    const auto navigation = NMEA::navigationStatus(*sentence);
    const auto update = sentence->type() == "GSA" && navigation && navigation->valid && !satelliteUpdate.accepted
                            ? std::optional<NMEA::NavigationUpdate>()
                            : _navigationAssembler.ingest(*sentence, now);
    if (update && update->type == NMEA::NavigationUpdate::Type::FixLoss) {
        _position = {};
        _position.navigation.timestampUs = now;
        _position.navigation.fixType = GPSPositionReport::FixType::NoFix;
        // Without a fix no satellite is used, unless the sentence reports a count of its own.
        const uint8_t used = gpsSatellitesUsed(update->epoch.satellitesUsed).value_or(0);
        _position.navigation.satellitesUsed = used;
        context.publishSatelliteUsage(used);
        updates |= GPSReceiveUpdate::Position;
    } else if (update && update->type == NMEA::NavigationUpdate::Type::Epoch &&
               update->trigger == NMEA::NavigationUpdate::Trigger::Position && sentence->type() == "GGA") {
        applyNavigationEpoch(_position, update->epoch);
        context.publishSatelliteUsage(gpsSatellitesUsed(update->epoch.satellitesUsed));
        updates |= GPSReceiveUpdate::Position;
    } else if (update && update->type == NMEA::NavigationUpdate::Type::Epoch &&
               update->trigger == NMEA::NavigationUpdate::Trigger::TimedMetadata) {
        applyNavigationEpoch(_position, update->epoch);
        updates |= GPSReceiveUpdate::Position;
    }
    return updates;
}

GPSReceiveUpdates GPSNMEAStream::_finishLine(GPSReceiveUpdates updates, GPSDecodeContext& context)
{
    _publishSatellites(context);
    if (updates & GPSReceiveUpdate::Position) {
        context.publishPosition(_position);
    }
    return updates;
}

void GPSNMEAStream::flush(GPSDecodeContext& context)
{
    const uint64_t now = context.nowUs();
    if (const auto expired = _navigationAssembler.expireUntimedMetadata(now)) {
        applyNavigationEpoch(_position, *expired);
    }
    _queueSatellites(_satelliteAssembler.flushDue(now));
    _publishSatellites(context);
}

std::chrono::milliseconds GPSNMEAStream::limitReceiveTimeout(std::chrono::milliseconds timeout, uint64_t nowUs) const
{
    if (const auto deadline = _satelliteAssembler.deadlineUs()) {
        timeout = std::min(timeout, GPSDeadline{*deadline}.remaining(nowUs));
    }
    return timeout;
}

GPSReceiveUpdates GPSNMEAStream::receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) const
{
    const GPSReceiveUpdates updates = channel.receiveCycle(limitReceiveTimeout(timeout, channel.nowUs()));
    channel.serviceControls();
    return updates;
}

void GPSNMEAStream::_queueSatellites(const NMEA::SatelliteEpoch& epoch)
{
    for (const auto& system : epoch) {
        if (system.inViewTimestampUs != 0 || system.inUseTimestampUs != 0) {
            _pendingSatellites.push_back(system);
        }
    }
}

void GPSNMEAStream::_publishSatellites(GPSDecodeContext& context)
{
    for (const auto& system : _pendingSatellites) {
        context.publishSatellites(satelliteReport(system));
    }
    _pendingSatellites.clear();
}
