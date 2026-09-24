#include "GPSAsciiProtocol.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

#include "GPSNMEAReport.h"

GPSAsciiProtocol::GPSAsciiProtocol(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSProtocol(std::move(io), satelliteInfoEnabled)
    , _lineFramer(_line, {.requireStart = false, .hashStartsLine = true})
    , _navigationAssembler({.metadataMaxAgeUs = METADATA_MAX_AGE_US,
                            .untimedMetadataMaxAgeUs = METADATA_MAX_AGE_US,
                            .autonomousFixQuality = GPSFixQuality::Fix3D,
                            .useGsaDimensionForAutonomousFix = false,
                            .requirePositionTime = false,
                            .reconstructDate = false,
                            .enforceNavigationOrder = false,
                            .untimedMetadataUsesPositionReceipt = true})
{}

void GPSAsciiProtocol::resetStream()
{
    _rtcm.reset();
    _satelliteAssembler.clear();
    _pendingSatellites.clear();
    _lineFramer.reset();
    _navigationAssembler.reset();
    _position = {};
    if (_satellites) {
        *_satellites = {};
    }
}

int GPSAsciiProtocol::receive(unsigned timeout)
{
    if (const auto deadline = _satelliteAssembler.deadlineUs()) {
        timeout = std::min(timeout, static_cast<unsigned>(remainingMilliseconds(*deadline)));
    }
    const int result = receiveDecoded(timeout);
    serviceControls();
    return ioError() ? ioError() : result;
}

int GPSAsciiProtocol::decodeByte(uint8_t byte)
{
    if (_rtcm.ownsByte(byte)) {
        _lineFramer.reset();
        _rtcm.addByte(byte);
        _drainRTCM();
        return 0;
    }
    const auto framed = _lineFramer.addByte(byte);
    if (framed.line) {
        int updates = 0;
        updates = _handleNmea(*framed.line);
        updates |= handleReceiverLine(*framed.line);
        _drainSatellites();
        if (updates & GPSDecodedBatch::POSITION_UPDATE) {
            publishPosition(_position);
        }
        return updates;
    }
    return 0;
}

int GPSAsciiProtocol::_handleNmea(std::string_view line)
{
    const auto sentence = NMEA::sentence(line);
    if (!sentence) {
        return 0;
    }
    const uint64_t now = nowUs();
    int updates = GPSDecodedBatch::PROTOCOL_ACTIVITY;
    auto satelliteUpdate = _satelliteAssembler.ingest(*sentence, now);
    _publishSatellites(satelliteUpdate.completed);
    const auto navigation = NMEA::navigationStatus(*sentence);
    const auto update = sentence->type() == "GSA" && navigation && navigation->valid && !satelliteUpdate.accepted
                            ? std::optional<NMEA::NavigationUpdate>()
                            : _navigationAssembler.ingest(*sentence, now);
    if (update && update->type == NMEA::NavigationUpdate::Type::FixLoss) {
        _position = {};
        _position.navigation.timestampUs = now;
        _position.navigation.fixType = GPSPositionReport::FixType::NoFix;
        std::optional<int> used = update->epoch.satellitesUsed && *update->epoch.satellitesUsed < UINT8_MAX
                                      ? std::optional<int>(static_cast<int>(*update->epoch.satellitesUsed))
                                      : std::nullopt;
        if (used) {
            _position.navigation.satellitesUsed = static_cast<uint8_t>(*used);
        }
        publishSatelliteUsage(used);
        updates |= GPSDecodedBatch::POSITION_UPDATE;
    } else if (update && update->type == NMEA::NavigationUpdate::Type::Epoch &&
               update->trigger == NMEA::NavigationUpdate::Trigger::Position && sentence->type() == "GGA") {
        applyNMEANavigationEpoch(_position, update->epoch);
        publishSatelliteUsage(update->epoch.satellitesUsed ? std::optional<int>(*update->epoch.satellitesUsed)
                                                           : std::nullopt);
        updates |= GPSDecodedBatch::POSITION_UPDATE;
    } else if (update && update->type == NMEA::NavigationUpdate::Type::Epoch &&
               update->trigger == NMEA::NavigationUpdate::Trigger::TimedMetadata) {
        applyNMEANavigationEpoch(_position, update->epoch);
        updates |= GPSDecodedBatch::POSITION_UPDATE;
    }
    return updates;
}

void GPSAsciiProtocol::_publishSatellites(const NMEA::SatelliteEpoch& epoch)
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

void GPSAsciiProtocol::_drainSatellites()
{
    size_t count = 0;
    // Leave space for the position and vendor event belonging to the current line.
    while (count < _pendingSatellites.size() && _decoded.events.size() + 2 < GPSDecodedBatch::MAX_EVENTS) {
        const auto& system = _pendingSatellites[count++];
        *_satellites = gpsNMEASatelliteReport(system);
        publishSatellites(*_satellites);
    }
    _pendingSatellites.erase(_pendingSatellites.begin(), _pendingSatellites.begin() + count);
}

void GPSAsciiProtocol::flushDecoded()
{
    if (const auto expired = _navigationAssembler.expireUntimedMetadata(nowUs())) {
        applyNMEANavigationEpoch(_position, *expired);
    }
    _publishSatellites(_satelliteAssembler.flushDue(nowUs()));
    _drainRTCM();
    _drainSatellites();
}

void GPSAsciiProtocol::_drainRTCM()
{
    drainRTCM(_rtcm, _rtcmEnabled);
}

int GPSNativePassive::configure(unsigned& baud, const GPSConfig& config)
{
    _configured = false;
    resetIOError();
    resetStream();
    if (config.allowPersistentChanges || config.base != GPSBaseStationConfig{} || baud < 1200 || baud > 4000000) {
        log(GPSProtocolLogLevel::Warning, "Passive input requires an explicit baud rate and no receiver configuration");
        return -1;
    }
    if (setBaudrate(baud) < 0) {
        if (ioError() != ReadCancelled) {
            log(GPSProtocolLogLevel::Warning, "Could not set the passive input baud rate");
        }
        return -1;
    }
    setRTCMEnabled(true);
    _configured = true;
    return 0;
}
