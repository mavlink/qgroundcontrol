#pragma once

#include <array>
#include <string_view>

#include "GPSProtocol.h"
#include "NMEA/NMEAMetadata.h"
#include "NMEA/NMEASatelliteEpoch.h"
#include "NMEA/NMEASentence.h"
#include "RTCM/RTCMFramer.h"

/// Mixed ASCII and RTCM input. Binary frame payloads never enter the line parser.
class GPSAsciiProtocol : public GPSProtocol
{
public:
    explicit GPSAsciiProtocol(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    int receive(unsigned timeout) override;

protected:
    void resetStream();

    void setRTCMEnabled(bool enabled) { _rtcmEnabled = enabled; }

    /// Complete printable line, without CR/LF. Vendor checksums remain the controller's responsibility.
    virtual int handleReceiverLine(std::string_view) { return 0; }

    int decodeByte(uint8_t byte) override;
    void flushDecoded() override;

private:
    int _handleNmea(std::string_view line);
    void _expireVdop(uint64_t now);
    void _publishSatellites(const NMEA::SatelliteEpoch& epoch);
    void _drainSatellites();
    void _drainRTCM();

    static constexpr size_t MAX_LINE_SIZE = 4096;
    static constexpr uint64_t METADATA_MAX_AGE_US = 2000000;
    GPSNativePositionReport* const _position = _gps_position;
    GPSNativeSatelliteReport* const _satellites = _satellite_info;
    RTCMStreamDecoder _rtcm;
    NMEA::SatelliteAssembler _satelliteAssembler;
    NMEA::SatelliteEpoch _pendingSatellites;
    std::array<char, MAX_LINE_SIZE> _line{};
    size_t _lineSize = 0;
    bool _discardLine = false;
    bool _lineEnded = false;
    bool _rtcmEnabled = true;
    NMEA::EpochReceipt _accuracyReceipt;
    std::optional<int> _positionTime;
    std::optional<uint64_t> _vdopReceivedAtUs;
    NMEA::GST _accuracy;
};
