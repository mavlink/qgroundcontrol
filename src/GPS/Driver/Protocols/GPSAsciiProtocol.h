#pragma once

#include <array>
#include <string_view>

#include "GPSProtocol.h"
#include "NMEALineFramer.h"
#include "NMEANavigationEpoch.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentence.h"
#include "RTCMFramer.h"

/// Mixed ASCII and RTCM input. Binary frame payloads never enter the line parser.
class GPSAsciiProtocol : public GPSProtocol
{
public:
    explicit GPSAsciiProtocol(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    int receive(unsigned timeout) override;

protected:
    /// Who turns sentences into positions: this base from standard NMEA, or the derived receiver protocol.
    enum class Navigation
    {
        StandardNMEA,
        ReceiverSpecific,
    };

    GPSAsciiProtocol(GPSProtocolIO io, bool satelliteInfoEnabled, Navigation navigation);

    void resetStream();

    void setRTCMEnabled(bool enabled) { _rtcmEnabled = enabled; }

    /// Complete printable line, without CR/LF. Vendor checksums remain the controller's responsibility.
    virtual int handleReceiverLine(std::string_view) { return 0; }

    int decodeByte(uint8_t byte) override;
    void flushDecoded() override;

private:
    int _handleNmea(std::string_view line);
    void _publishSatellites(const NMEA::SatelliteEpoch& epoch);
    void _drainSatellites();
    void _drainRTCM();

    static constexpr size_t MAX_LINE_SIZE = 4096;
    static constexpr uint64_t METADATA_MAX_AGE_US = 2000000;
    RTCMStreamDecoder _rtcm;
    NMEA::SatelliteAssembler _satelliteAssembler;
    NMEA::SatelliteEpoch _pendingSatellites;
    std::array<char, MAX_LINE_SIZE> _line{};
    NMEA::LineFramer _lineFramer;
    NMEA::NavigationEpochAssembler _navigationAssembler;
    Navigation _navigation = Navigation::StandardNMEA;
    bool _rtcmEnabled = true;
};

/// Read-only receiver input; setting the local serial baud rate never sends a receiver command.
class GPSNativePassive : public GPSAsciiProtocol
{
public:
    using GPSAsciiProtocol::GPSAsciiProtocol;

    bool configure(unsigned& baud, const GPSConfig& config) override;

    bool receiverReady() const override { return _configured; }

private:
    const QLoggingCategory& logCategory() const override;
    bool _configured = false;
};
