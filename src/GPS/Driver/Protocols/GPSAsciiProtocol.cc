#include "GPSAsciiProtocol.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "NMEA/GPSNMEAReport.h"
#include "NMEA/GPSNMEASatelliteReport.h"

GPSAsciiProtocol::GPSAsciiProtocol(GPSProtocolIO io, GPSNativePositionReport* position,
                                   GPSNativeSatelliteReport* satellites)
    : GPSBaseProtocol(std::move(io))
    , _position(position ? position : &_fallbackPosition)
    , _satellites(satellites)
{}

void GPSAsciiProtocol::resetStream()
{
    _rtcm.reset();
    _satelliteAssembler.clear();
    _pendingSatellites.clear();
    _lineSize = 0;
    _discardLine = false;
    _lineEnded = false;
    _accuracyReceipt = {};
    _positionTime.reset();
    _vdopReceivedAtUs.reset();
    _accuracy = {};
    *_position = {};
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
        _lineSize = 0;
        _discardLine = false;
        _lineEnded = false;
        _rtcm.addByte(byte);
        _drainRTCM();
        return 0;
    }
    if (byte == '\r') {
        _lineEnded = true;
        return 0;
    }
    if (byte == '\n') {
        const std::string_view line{_line.data(), _lineSize};
        int updates = 0;
        if (!_discardLine && !line.empty()) {
            updates = _handleNmea(line);
            updates |= handleReceiverLine(line);
        }
        _lineSize = 0;
        _discardLine = false;
        _lineEnded = false;
        _drainSatellites();
        return updates;
    }
    if (byte == '$' || byte == '#') {
        _lineSize = 0;
        _discardLine = false;
        _lineEnded = false;
    }
    if (_lineEnded || byte < ' ' || byte > '~' || _lineSize == _line.size()) {
        _lineSize = 0;
        _discardLine = true;
        return 0;
    }
    if (!_discardLine) {
        _line[_lineSize++] = static_cast<char>(byte);
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
    if (navigation && !navigation->valid) {
        *_position = {};
        _position->timestamp = now;
        _position->fix_type = GPSPositionReport::FixType::NoFix;
        _positionTime = navigation->utcMilliseconds;
        _accuracyReceipt = {};
        _vdopReceivedAtUs.reset();
        std::optional<int> used;
        if (sentence->type() == "GGA" && sentence->count > NMEA::Field::GGA_SATELLITES_USED) {
            const auto count = NMEA::number<unsigned>(sentence->fields[NMEA::Field::GGA_SATELLITES_USED]);
            if (count && *count < UINT8_MAX) {
                used = static_cast<int>(*count);
                _position->satellites_used = static_cast<uint8_t>(*count);
            }
        }
        publishSatelliteUsage(used);
        updates |= 1;
    } else if (const auto fix = NMEA::gga(*sentence)) {
        const auto positionTime = navigation ? navigation->utcMilliseconds : std::nullopt;
        if (!positionTime || positionTime != _positionTime) {
            _vdopReceivedAtUs.reset();
        }
        _expireVdop(now);
        applyNMEAGGA(*_position, *fix, now);
        _positionTime = positionTime;
        const bool matchingAccuracy = _accuracyReceipt.matches({_positionTime, now}, METADATA_MAX_AGE_US);
        _position->eph = matchingAccuracy ? _accuracy.horizontalAccuracy : NAN;
        _position->epv = matchingAccuracy ? _accuracy.verticalAccuracy : NAN;
        _position->accuracy_timestamp = matchingAccuracy ? _accuracyReceipt.receivedAtUs : 0;
        publishSatelliteUsage(fix->satellitesUsed ? std::optional<int>(*fix->satellitesUsed) : std::nullopt);
        updates |= 1;
    } else if (const auto accuracy = NMEA::gst(*sentence)) {
        _accuracyReceipt = {NMEA::utcMilliseconds(sentence->fields[NMEA::Field::UTC_TIME]), now};
        _accuracy = *accuracy;
        if (NMEA::EpochReceipt{_positionTime, _position->timestamp}.matches(_accuracyReceipt, METADATA_MAX_AGE_US)) {
            _expireVdop(now);
            _position->eph = _accuracy.horizontalAccuracy;
            _position->epv = _accuracy.verticalAccuracy;
            _position->accuracy_timestamp = now;
            updates |= 1;
        }
    } else if (sentence->type() == "GSA" && satelliteUpdate.accepted && _positionTime && now >= _position->timestamp &&
               now - _position->timestamp <= METADATA_MAX_AGE_US) {
        // GSA has no UTC field, so it can only supplement the preceding fresh GGA.
        const auto hdop = NMEA::number<float>(sentence->fields[NMEA::Field::GSA_HDOP]);
        const auto vdop = NMEA::number<float>(sentence->fields[NMEA::Field::GSA_VDOP]);
        _position->hdop = hdop && *hdop >= 0 ? *hdop : NAN;
        _position->vdop = vdop && *vdop >= 0 ? *vdop : NAN;
        _vdopReceivedAtUs = now;
        _position->dop_timestamp = now;
    }
    return updates;
}

void GPSAsciiProtocol::_expireVdop(uint64_t now)
{
    // GGA refreshes HDOP only; it must not renew an older GSA's VDOP.
    if (!_vdopReceivedAtUs || now < *_vdopReceivedAtUs || now - *_vdopReceivedAtUs > METADATA_MAX_AGE_US) {
        _vdopReceivedAtUs.reset();
        _position->vdop = NAN;
    }
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
    _expireVdop(nowUs());
    _publishSatellites(_satelliteAssembler.flushDue(nowUs()));
    _drainRTCM();
    _drainSatellites();
}

void GPSAsciiProtocol::_drainRTCM()
{
    drainRTCM(_rtcm, _rtcmEnabled);
}
