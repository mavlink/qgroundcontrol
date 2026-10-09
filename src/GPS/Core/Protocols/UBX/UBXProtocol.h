#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "GPSCommand.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolEvent.h"
#include "GPSReceiverConfig.h"
#include "GPSStreamDemux.h"
#include "UBX/UBXConfigKeys.h"
#include "UBX/UBXEpochAssembler.h"
#include "UBX/UBXFrame.h"
#include "UBX/UBXPlan.h"
#include "UBX/UBXReceiverController.h"

class GPSCommandChannel;
class GPSDecodeContext;
class GPSStreamDemux;

namespace UBX {

/// Receiver identity from MON-VER.
struct Identity
{
    Board board = Board::unknown;
    bool isM8p = false;
    bool protocol27 = false;
    bool timeModeUnsupported = false;
    /// MON-VER hwVersion, such as "00190000", and the PROTVER extension, such as "27.31"; empty when not reported.
    QByteArray hardware;
    QByteArray protocolVersion;
    QByteArray model;
    QByteArray firmware;
};

/// The u-blox family protocol. Configuration takes its receiver settings from UBX::Plan and holds the control flow
/// that depends on replies: baud detection and the UART handoff, jamming-detection fallback, time mode, survey-in and
/// RTCM activation. Decoding turns UBX frames and forwarded RTCM3 frames into events without I/O: it records
/// acknowledgements and readbacks for configuration, and requests the streaming commands that decoded traffic calls
/// for.
class Protocol final : public GPSFamilyProtocol
{
public:
    /// Output decoded in navigation mode, besides NAV-EOE and NAV-SVIN. Other output is dropped and the streaming
    /// services ask the receiver to stop it (any message on legacy receivers, Plan::UNEXPECTED_OUTPUT from protocol
    /// 27), so every message the configuration plan enables must be listed.
    static constexpr std::array NAVIGATION_MESSAGES{Msg::NAV_PVT,    Msg::NAV_HPPOSLLH, Msg::NAV_DOP,
                                                    Msg::NAV_STATUS, Msg::MON_HW,       Msg::MON_RF,
                                                    Msg::SEC_SIG,    Msg::NAV_SAT,      Msg::NAV_SVINFO};

    /// A receiver warning or error repeated within this interval is logged at debug level.
    static constexpr std::chrono::seconds REPEATED_WARNING_INTERVAL{60};

    bool configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override;

    bool probe(GPSCommandChannel& channel) override;

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override;

    /// Publishes epochs past their deadline.
    void flush(GPSDecodeContext& context) override;

    bool receiverReady() const override { return _ready; }

    QString identity() const override { return gpsReceiverIdentity(_identity.model, _identity.firmware); }

    /// The decoder state a successful configure() leaves: navigation, survey and corrections decoding.
    bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext& context) override;

    bool armNavigationDecode(GPSNavigationDecode decode, GPSDecodeContext& context) override;

    /// Whether a receive cycle has what it waits for: a pending readback, or else a navigation update.
    bool completeReceiveCycle(GPSReceiveUpdates handled) override;

    /// Reads briefly once part of an epoch has arrived, as the rest follows back to back.
    [[nodiscard]] std::chrono::milliseconds nextReadSlice(std::chrono::milliseconds timeout) const override;

    /// RTCM activation after survey-in and disabling unexpected output, as decoding requested.
    void serviceStreaming(GPSCommandChannel& channel) override;

private:
    struct Mode
    {
        /// Navigation, integrity and satellite messages are decoded, and navigation messages are assembled into epochs
        /// by time of week; otherwise only control traffic.
        bool navigation = false;
        /// RTCM3 frames are published.
        bool corrections = false;
    };

    /// Commands decoded traffic asks the streaming services to send.
    struct Requests
    {
        /// A completed survey-in starts RTCM output.
        bool rtcmActivation = false;
        /// A message the receiver should not send.
        uint16_t disableMessage = 0;
    };

    /// How an RTCM activation ended. An unanswered one is resumed by a later streaming service.
    enum class RTCMActivation : uint8_t
    {
        Active,
        Failed,
        Unanswered,
    };

    /// The verdict on a CFG-MSG rate.
    struct RateConfirmation
    {
        GPSCommandOutcome outcome;
        /// No reply to the rate or its poll is still to arrive, so a later CFG-MSG reply can be attributed.
        bool settled;
    };

    /// The legacy RTCM activation outputs settled so far, which a resumed activation does not repeat.
    struct LegacyRTCMProgress
    {
        size_t next = 0;
        bool stationPosition = false;
        bool observations = false;
    };

    // Decoding (UBXDecoder.cc)

    /// Replaces the mode, drops partial epochs and frames RTCM3 when @a mode has corrections.
    void _setMode(const Mode& mode, GPSStreamDemux& stream);
    GPSReceiveUpdates _decode(uint16_t message, std::span<const uint8_t> payload, GPSDecodeContext& context);
    // The decoders below receive payloads that _decode() validated against their message schema.
    bool _accept(uint16_t message, std::span<const uint8_t> payload);
    void _decodeEpoch(uint16_t message, std::span<const uint8_t> payload, EpochAssembler::Epoch& epoch);
    GPSReceiveUpdates _decodeHandled(uint16_t message, std::span<const uint8_t> payload, GPSDecodeContext& context);
    GPSReceiveUpdates _decodeSurveyIn(std::span<const uint8_t> payload, GPSDecodeContext& context);
    void _decodeMonVer(std::span<const uint8_t> payload);
    void _decodeSatellites(uint16_t message, std::span<const uint8_t> payload);

    // Configuration and streaming services (UBXConfigurator.cc)

    [[nodiscard]] bool _baseStationUnsupported() const;
    bool _identify(GPSCommandChannel& channel);
    bool _setUpPort(GPSCommandChannel& channel, unsigned detectedBaud, unsigned desiredBaud);
    bool _configureDevice(GPSCommandChannel& channel);
    bool _configureJammingDetection(GPSCommandChannel& channel);
    bool _configureLegacyDevice(GPSCommandChannel& channel);
    bool _restartSurveyIn(GPSCommandChannel& channel);
    bool _restartLegacySurveyIn(GPSCommandChannel& channel);
    bool _disableTimeMode(GPSCommandChannel& channel);
    bool _waitForSurveyStop(GPSCommandChannel& channel);
    RTCMActivation _activateRTCMOutput(GPSCommandChannel& channel);
    RTCMActivation _activateLegacyRTCMOutput(GPSCommandChannel& channel);
    void _disableUnexpectedMessage(GPSCommandChannel& channel, uint16_t message);

    /// Writes one UBX frame as a command attempt.
    bool _send(GPSCommandChannel& channel, MessageId message, std::span<const uint8_t> payload,
               GPSConfigurationStep step);
    bool _sendAcknowledged(GPSCommandChannel& channel, MessageId message, std::span<const uint8_t> payload);
    GPSConfigurationEvidence _waitForAck(GPSCommandChannel& channel, MessageId message);
    bool _setMessageRate(GPSCommandChannel& channel, Plan::MessageRate rate);
    bool _setMessageRateAcknowledged(GPSCommandChannel& channel, Plan::MessageRate rate);
    GPSCommandOutcome _setMessageRateReply(GPSCommandChannel& channel, Plan::MessageRate rate);
    /// As above, but polls a rate left unanswered, as an M8 may acknowledge up to a second late: ReadbackVerified or
    /// Rejected from the polled rates or a NAK, TimedOut when the poll is unanswered too. A poll that expires with only
    /// some of its replies is unsettled.
    RateConfirmation _confirmMessageRate(GPSCommandChannel& channel, Plan::MessageRate rate);
    bool _runPlan(GPSCommandChannel& channel, const std::vector<Plan::ValsetBatch>& batches);
    /// An ambiguous VALSET refuses later ones, unless each is proven by readback.
    [[nodiscard]] bool _valsetsRefused() const;
    void _load(const Plan::ValsetBatch& batch);
    bool _writeValset(GPSCommandChannel& channel, const Plan::ValsetBatch& batch);
    GPSConfigurationEvidence _transactValset(GPSCommandChannel& channel, const Plan::ValsetBatch& batch);
    /// Waits for the loaded batch's acknowledgement; an optional batch left unanswered is settled by readback.
    GPSConfigurationEvidence _awaitValsetAck(GPSCommandChannel& channel);
    /// Proves the loaded batch by CFG-VALGET, when its acknowledgement cannot be trusted.
    GPSConfigurationEvidence _verifyValset(GPSCommandChannel& channel, GPSConfigurationStep step);

    // What configuration and the streaming services share with decoding.
    Identity _identity{};
    ReceiverController _controller{};
    Mode _mode{};
    Requests _requests{};
    /// CFG-TMODE3 is decoded while configuration polls it.
    bool _timeModeReadbackPending = false;
    std::optional<uint8_t> _timeModeReadback = std::nullopt;
    /// NAV-SVIN reported survey-in neither active nor valid.
    bool _surveyStopped = false;
    bool _ready = false;
    /// SEC-SIG jammingState supersedes the deprecated MON-RF flags.
    bool _secSigSeen = false;

    // Decoding
    EpochAssembler _epochs;
    GPSDecodedSatellites _satellites;
    GPSIntegrityReport _integrity;
    /// Part of an epoch arrived since the last completed receive cycle.
    bool _epochStarted = false;
    /// The latest receiver warning or error logged as a warning, and when.
    QString _lastWarning;
    uint64_t _lastWarningUs = 0;

    // Configuration
    GPSBaseStationConfig _base;
    CheckedValsetBatch<Plan::VALSET_CAPACITY> _valset;
    /// A VALSET left unanswered, and not settled by readback, leaves the receiver state unknown; later VALSETs are
    /// refused unless verified.
    bool _valsetAckAmbiguous = false;
    LegacyRTCMProgress _legacyRTCM;
    uint64_t _lastDisableUs = 0;
};

}  // namespace UBX
