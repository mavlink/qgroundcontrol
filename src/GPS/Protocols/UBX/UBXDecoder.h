#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>

#include <QtCore/QByteArray>

#include "GPSDecodedReports.h"
#include "GPSFrame.h"
#include "GPSReceiveUpdates.h"
#include "UBXNavigationEpoch.h"
#include "UBXReceiverController.h"
#include "UBXReceiverProfile.h"

class GPSDecodeContext;
class GPSStreamDemux;

/// Receiver identity from MON-VER.
struct UBXIdentity
{
    UBX::Board board = UBX::Board::unknown;
    bool isM8p = false;
    bool protocol27 = false;
    bool timeModeUnsupported = false;
    QByteArray model;
    QByteArray firmware;
};

/// Decodes UBX frames and forwarded RTCM3 frames into events. It performs no I/O: it records acknowledgements and
/// readbacks for configuration, and requests the streaming commands that decoded traffic calls for.
class UBXDecoder
{
public:
    struct Mode
    {
        /// Navigation, integrity and satellite messages are decoded; otherwise only control traffic.
        bool navigation = false;
        /// NAV-PVT, or the NAV-POSLLH, NAV-SOL, NAV-VELNED and NAV-TIMEUTC of receivers without it.
        bool useNavPvt = true;
        /// RTCM3 frames are published.
        bool corrections = false;
        /// Navigation messages are assembled into epochs by time of week.
        bool assembleEpochs = false;
    };

    /// Commands decoded traffic asks the streaming services to send.
    struct Requests
    {
        /// A txbuf warning asks for a MON-COMMS snapshot.
        bool commsDiagnostics = false;
        /// A completed survey-in starts RTCM output.
        bool rtcmActivation = false;
        /// A message the receiver should not send.
        uint16_t disableMessage = 0;
    };

    /// What configuration and the streaming services share with decoding.
    struct State
    {
        UBXIdentity identity{};
        UBX::ReceiverController controller{};
        Mode mode{};
        Requests requests{};
        /// CFG-TMODE3 is decoded while configuration polls it.
        bool timeModeReadbackPending = false;
        std::optional<uint8_t> timeModeReadback = std::nullopt;
        /// NAV-SVIN reported survey-in neither active nor valid.
        bool surveyStopped = false;
        bool configured = false;
        /// SEC-SIG jammingState supersedes the deprecated MON-RF flags.
        bool secSigSeen = false;
        /// MON-COMMS replies are logged until this time.
        uint64_t commsReplyDeadlineUs = 0;
    };

    explicit UBXDecoder(bool satelliteInfoEnabled);

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context);

    /// Publishes queued RTCM3 frames, then epochs past their deadline.
    void flush(GPSDecodeContext& context);

    /// Replaces the mode, drops partial epochs, frames RTCM3 when @a mode has corrections and restarts UBX framing.
    void setMode(const Mode& mode, GPSStreamDemux& stream);

    /// Whether a receive cycle has what it waits for: a pending readback, or else a navigation update.
    bool completeReceiveCycle(GPSReceiveUpdates handled);

    /// Reads briefly once part of an epoch has arrived, as the rest follows back to back.
    [[nodiscard]] std::chrono::milliseconds nextReadSlice(std::chrono::milliseconds timeout) const;

    [[nodiscard]] State& state() { return _state; }

    [[nodiscard]] const State& state() const { return _state; }

    [[nodiscard]] bool satelliteInfoEnabled() const { return _satelliteInfo; }

    /// The position the latest navigation message updated, outside epoch assembly.
    [[nodiscard]] const GPSDecodedPosition& position() const { return _position; }

    [[nodiscard]] const GPSDecodedSatellites& satellites() const { return _satellites; }

private:
    GPSReceiveUpdates _decode(uint16_t message, std::span<const uint8_t> payload, GPSDecodeContext& context);
    bool _accept(uint16_t message, std::span<const uint8_t> payload);
    GPSReceiveUpdates _decodeHandled(uint16_t message, std::span<const uint8_t> payload, GPSDecodedPosition& position,
                                     GPSDecodeContext& context);
    GPSReceiveUpdates _decodeNavPvt(std::span<const uint8_t> payload, GPSDecodedPosition& position, uint64_t now);
    GPSReceiveUpdates _decodeNavigation(uint16_t message, std::span<const uint8_t> payload,
                                        GPSDecodedPosition& position, uint64_t now);
    GPSReceiveUpdates _decodeHeading(uint16_t message, std::span<const uint8_t> payload, GPSDecodedPosition& position);
    GPSReceiveUpdates _decodeSurveyIn(std::span<const uint8_t> payload, GPSDecodeContext& context);
    bool _decodeIntegrity(uint16_t message, std::span<const uint8_t> payload, uint64_t now);
    GPSReceiveUpdates _decodeControl(uint16_t message, std::span<const uint8_t> payload, uint64_t now);
    void _decodeMonVer(std::span<const uint8_t> payload);
    void _decodeSatellites(uint16_t message, std::span<const uint8_t> payload);
    void _logCommsDiagnostics(std::span<const uint8_t> payload, uint64_t now);
    void _publishEpoch(const GPSDecodedPosition& report, GPSDecodeContext& context);

    State _state;
    UBXNavigationEpoch _epochs;
    GPSDecodedPosition _position;
    GPSDecodedSatellites _satellites;
    GPSIntegrityReport _integrity;
    bool _satelliteInfo;
    bool _epochHasHighPrecision = false;
    bool _gotPosition = false;
    bool _gotVelocity = false;
};
