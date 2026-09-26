#include "GPSNMEAStream.h"

#include <algorithm>

#include "GPSCommandChannel.h"
#include "GPSDeadline.h"
#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSNMEAReport.h"
#include "NMEASentence.h"

GPSNMEAStream::GPSNMEAStream(Navigation navigation, bool satelliteInfoEnabled)
    : _satellites(satelliteInfoEnabled ? std::optional<GPSDecodedSatellites>(GPSDecodedSatellites{}) : std::nullopt)
    , _navigationAssembler({.metadataMaxAge = METADATA_MAX_AGE,
                            .untimedMetadataMaxAge = METADATA_MAX_AGE,
                            .autonomousFixQuality = GPSFixQuality::Fix3D,
                            .useGsaDimensionForAutonomousFix = false,
                            .requirePositionTime = false,
                            .reconstructDate = false,
                            .enforceNavigationOrder = false,
                            .untimedMetadataUsesPositionReceipt = true})
    , _navigation(navigation)
{}

void GPSNMEAStream::reset(GPSStreamDemux& stream)
{
    stream.reset(GPSFrameKind::RTCM3);
    _satelliteAssembler.clear();
    _pendingSatellites.clear();
    stream.reset(GPSFrameKind::ASCIILine);
    _navigationAssembler.reset();
    _position = {};
    if (_satellites) {
        *_satellites = {};
    }
}

GPSReceiveUpdates GPSNMEAStream::decodeRTCM(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (_rtcmEnabled) {
        context.sink().publishRTCM(frame.bytes);
    }
    return {};
}

GPSReceiveUpdates GPSNMEAStream::decodeStandard(std::string_view line, GPSDecodeContext& context)
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
        const auto used = gpsSatellitesUsed(update->epoch.satellitesUsed);
        if (used) {
            _position.navigation.satellitesUsed = used;
        }
        context.sink().publishSatelliteUsage(used);
        updates |= GPSReceiveUpdate::Position;
    } else if (update && update->type == NMEA::NavigationUpdate::Type::Epoch &&
               update->trigger == NMEA::NavigationUpdate::Trigger::Position && sentence->type() == "GGA") {
        applyNMEANavigationEpoch(_position, update->epoch);
        context.sink().publishSatelliteUsage(
            update->epoch.satellitesUsed ? std::optional<int>(*update->epoch.satellitesUsed) : std::nullopt);
        updates |= GPSReceiveUpdate::Position;
    } else if (update && update->type == NMEA::NavigationUpdate::Type::Epoch &&
               update->trigger == NMEA::NavigationUpdate::Trigger::TimedMetadata) {
        applyNMEANavigationEpoch(_position, update->epoch);
        updates |= GPSReceiveUpdate::Position;
    }
    return updates;
}

GPSReceiveUpdates GPSNMEAStream::finishLine(GPSReceiveUpdates updates, GPSDecodeContext& context)
{
    _drainSatellites(context);
    if (updates & GPSReceiveUpdate::Position) {
        context.sink().publishPosition(_position);
    }
    return updates;
}

GPSReceiveUpdates GPSNMEAStream::onFrame(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (frame.kind == GPSFrameKind::RTCM3) {
        return decodeRTCM(frame, context);
    }
    if (frame.kind != GPSFrameKind::ASCIILine) {
        return {};
    }
    return finishLine(decodeStandard(frame.text(), context), context);
}

void GPSNMEAStream::flush(GPSDecodeContext& context)
{
    const uint64_t now = context.nowUs();
    if (const auto expired = _navigationAssembler.expireUntimedMetadata(now)) {
        applyNMEANavigationEpoch(_position, *expired);
    }
    _queueSatellites(_satelliteAssembler.flushDue(now));
    context.drainDeferredFrames();
    _drainSatellites(context);
}

std::chrono::milliseconds GPSNMEAStream::limitReceiveTimeout(std::chrono::milliseconds timeout, uint64_t nowUs) const
{
    if (const auto deadline = _satelliteAssembler.deadlineUs()) {
        timeout = std::min(timeout, GPSDeadline{*deadline}.remaining(nowUs));
    }
    return timeout;
}

GPSTask<GPSReceiveUpdates> GPSNMEAStream::receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) const
{
    const GPSReceiveUpdates updates = co_await channel.receiveCycle(limitReceiveTimeout(timeout, channel.nowUs()));
    co_await channel.serviceControls();
    co_return updates;
}

void GPSNMEAStream::_queueSatellites(const NMEA::SatelliteEpoch& epoch)
{
    if (!_satellites) {
        return;
    }
    for (const auto& system : epoch) {
        if (system.inViewTimestampUs != 0 || system.inUseTimestampUs != 0) {
            _pendingSatellites.push_back(system);
        }
    }
}

void GPSNMEAStream::_drainSatellites(GPSDecodeContext& context)
{
    auto& sink = context.sink();
    size_t count = 0;
    // Leave space for the position and vendor event belonging to the current line.
    while (count < _pendingSatellites.size() && sink.hasRoomForDeferred()) {
        const auto& system = _pendingSatellites[count++];
        *_satellites = gpsNMEASatelliteReport(system);
        sink.publishSatellites(*_satellites);
    }
    _pendingSatellites.erase(_pendingSatellites.begin(), _pendingSatellites.begin() + static_cast<ptrdiff_t>(count));
}
