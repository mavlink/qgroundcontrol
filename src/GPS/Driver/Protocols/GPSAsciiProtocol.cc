#include "GPSAsciiProtocol.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "NMEA/GPSNMEAReport.h"
#include <GeographicLib/Geocentric.hpp>

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
    _pendingRTCM = false;
    _accuracyTime.reset();
    _positionTime.reset();
    _accuracyReceivedAtUs = 0;
    _accuracy = {};
    *_position = {};
    if (_satellites) {
        *_satellites = {};
    }
}

void GPSAsciiProtocol::lla2ECEF(double latitude, double longitude, double altitude, double& x, double& y, double& z)
{
    GeographicLib::Geocentric::WGS84().Forward(latitude, longitude, altitude, x, y, z);
}

int GPSAsciiProtocol::receive(unsigned timeout)
{
    const int result = receiveDecoded(timeout);
    serviceControls();
    return ioError() ? ioError() : result;
}

int GPSAsciiProtocol::decodeByte(uint8_t byte)
{
    if (_rtcm.hasPartialFrame() || byte == RTCMFramer::PREAMBLE) {
        _lineSize = 0;
        _discardLine = false;
        _lineEnded = false;
        _pendingRTCM = _rtcm.addByte(byte);
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
    if (const auto page = NMEA::gsv(*sentence); page && page->message == page->messages) {
        _publishSatellites(_satelliteAssembler.flush());
    }
    if (const auto fix = NMEA::gga(*sentence)) {
        applyNMEAGGA(*_position, *fix, now);
        _positionTime = NMEA::utcMilliseconds(sentence->fields[NMEA::Field::UTC_TIME]);
        const bool matchingAccuracy = _positionTime && _positionTime == _accuracyTime && now >= _accuracyReceivedAtUs &&
                                      now - _accuracyReceivedAtUs <= 2000000;
        _position->eph = matchingAccuracy ? _accuracy.horizontalAccuracy : NAN;
        _position->epv = matchingAccuracy ? _accuracy.verticalAccuracy : NAN;
        _position->accuracy_timestamp = matchingAccuracy ? _accuracyReceivedAtUs : 0;
        publishSatelliteUsage(fix->satellitesUsed ? std::optional<int>(*fix->satellitesUsed) : std::nullopt);
        updates |= 1;
    } else if (const auto accuracy = NMEA::gst(*sentence)) {
        _accuracyTime = NMEA::utcMilliseconds(sentence->fields[NMEA::Field::UTC_TIME]);
        _accuracyReceivedAtUs = now;
        _accuracy = *accuracy;
        if (_positionTime && _positionTime == _accuracyTime && now >= _position->timestamp &&
            now - _position->timestamp <= 2000000) {
            _position->eph = _accuracy.horizontalAccuracy;
            _position->epv = _accuracy.verticalAccuracy;
            _position->accuracy_timestamp = now;
            updates |= 1;
        }
    } else if (sentence->type() == "GSA" && satelliteUpdate.accepted) {
        const auto hdop = NMEA::number<float>(sentence->fields[NMEA::Field::GSA_HDOP]);
        const auto vdop = NMEA::number<float>(sentence->fields[NMEA::Field::GSA_VDOP]);
        _position->hdop = hdop && *hdop >= 0 ? *hdop : NAN;
        _position->vdop = vdop && *vdop >= 0 ? *vdop : NAN;
        _position->dop_timestamp = now;
    }
    return updates;
}

void GPSAsciiProtocol::_publishSatellites(const NMEA::SatelliteEpoch& epoch)
{
    if (!_satellites) {
        return;
    }
    for (const auto& system : epoch) {
        if (system.inViewTimestampUs != 0) {
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
        *_satellites = {};
        _satellites->timestamp = system.inViewTimestampUs;
        _satellites->constellation = system.constellation;
        _satellites->count = static_cast<uint16_t>(std::min(system.satellites.size(), _satellites->entries.size()));
        for (size_t index = 0; index < _satellites->count; ++index) {
            const auto& source = system.satellites[index];
            _satellites->entries[index] = {
                source.id,
                source.prn,
                source.constellation,
                system.usedIds ? std::optional<bool>(system.usedIds->contains(source.id)) : std::nullopt,
                source.elevation,
                source.azimuth,
                source.signal};
        }
        publishSatellites(*_satellites);
    }
    _pendingSatellites.erase(_pendingSatellites.begin(), _pendingSatellites.begin() + count);
}

void GPSAsciiProtocol::flushDecoded()
{
    _drainRTCM();
    _drainSatellites();
}

void GPSAsciiProtocol::_drainRTCM()
{
    while (_pendingRTCM && _decoded.events.size() + 2 < GPSDecodedBatch::MAX_EVENTS) {
        if (_rtcm.valid() && _rtcmEnabled) {
            const auto frame = _rtcm.frame();
            gotRTCMMessage(frame.data(), static_cast<int>(frame.size()));
            _decoded.updates |= GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
        _pendingRTCM = _rtcm.nextFrame();
    }
}
