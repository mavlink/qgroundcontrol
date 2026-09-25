#pragma once

#include "GPSProtocol.h"
#include "UBXFrameDecoder.h"
#include "UBXMessages.h"
#include "UBXNavigationEpoch.h"
#include "UBXReceiverController.h"
#include "UBXReceiverProfile.h"

class UBXProtocol : public GPSProtocol
{
public:
    explicit UBXProtocol(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    virtual ~UBXProtocol();

    bool configure(unsigned& baudrate, const GPSConfig& config) override;

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

    bool receiverReady() const override { return _configured; }

    using Board = UBX::Board;

    const Board& board() const { return _identity.board; }

    const char* modelName() const { return _identity.modelName; }

    const char* firmwareVersion() const { return _identity.firmwareVersion; }

    std::string receiverIdentity() const override;
    enum class BaseStationCapability : uint8_t
    {
        Unknown,
        Unsupported,
        Supported,
    };
    BaseStationCapability baseStationCapability() const;

    struct DecodeContext
    {
        bool navigation = false;
        bool useNavPvt = true;
        bool corrections = false;
        bool assembleEpochs = false;
    };

    void setDecodeContext(DecodeContext context);

private:
    const QLoggingCategory& logCategory() const override;
    void servicePendingCommands() override;
    bool _rtcmActivationPending = false;
    uint16_t _pendingDisableMessage = 0;
    UBX::ReceiverController _controller;

    /** Reads until the configured completion condition, a timeout, or an I/O failure; returns update flags. */
    int receiveInternal(unsigned timeout);

    void requestCommsDiagnostics();
    void logCommsDiagnostics(std::span<const uint8_t> payload);

    bool activateRTCMOutput();

    /**
     * Convert a relative position heading into a vehicle heading.
     * @param heading baseline heading as reported by the receiver [1e-5 deg]
     * @return heading normalized to [-pi, pi]
     */
    float relPosHeadingToYaw(int32_t heading) const;

    /**
     * Calculate & add checksum for given buffer
     */
    void calcChecksum(const uint8_t* buffer, const uint16_t length, ubx_checksum_t* checksum);

    /**
     * Send configuration values and desired message rates
     * @return true on success
     */
    bool configureDevice();

    /// Pre-protocol-27 configuration through CFG-MSG/CFG-RATE (UBXLegacyConfiguration.cc).
    bool configureDevicePreV27();
    bool restartSurveyInPreV27();
    bool activateRTCMOutputPreV27();
    /// CFG-MSG rate; deprecated with protocol version >= 27. @return true when written.
    bool configureMessageRate(uint16_t msg, uint8_t rate, bool required = true);
    /// CFG-MSG rate followed by its acknowledgement.
    bool configureMessageRateAndAck(uint16_t msg, uint8_t rate, bool report_ack_error = false);

    /**
     * Add a configuration value to the pending CFG-VALSET batch.
     * The value width on the wire comes from the key ID's size field, not from T; T documents the
     * call site and must agree with the key.
     * @param key_id one of the UBX_CFG_KEY_* constants
     * @param value configuration value
     * @return false for an invalid value or full batch; failure prevents sending until initCfgValset()
     */
    template <typename T>
    bool cfgValset(uint32_t key_id, T value)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t), "CFG-VALSET values wider than 4 bytes are not supported");
        return cfgValsetRaw(key_id, static_cast<uint32_t>(static_cast<std::make_unsigned_t<T>>(value)));
    }

    /**
     * Non-template body for cfgValset(): appends key_id and the low N bytes of value, where N is
     * given by the key ID's size field (bits 28-30: 1 and 2 -> 1 byte, 3 -> 2 bytes, 4 -> 4 bytes).
     */
    bool cfgValsetRaw(uint32_t key_id, uint32_t value);

    /** A fixed key/value pair for cfgValset(items) */
    struct CfgValsetItem
    {
        uint32_t key;
        uint8_t value;
    };

    /**
     * Add a fixed list of 1-byte configuration values
     * @return true on success, false if buffer too small
     */
    bool cfgValset(std::span<const CfgValsetItem> items);

    /**
     * Add the same 1-byte value for a list of keys
     * @return true on success, false if buffer too small
     */
    bool cfgValset(std::span<const uint32_t> keys, uint8_t value);

    /**
     * Add a configuration value that is port-specific (MSGOUT messages).
     * Note: Key ID must be the one for I2C, and the implementation assumes the
     *       Key ID's are in increasing order for the other ports: I2C, UART1, UART2, USB, SPI
     *       (this is a safe assumption for all MSGOUT messages according to u-blox).
     *
     * @param key_id I2C key ID
     * @param value configuration value
     * @return true on success, false if buffer too small
     */
    bool cfgValsetPort(uint32_t key_id, uint8_t value);

    /** cfgValsetPort() for a list of I2C key IDs sharing one value */
    bool cfgValsetPort(std::span<const uint32_t> keys, uint8_t value);

    /**
     * Reset the parse state machine for a fresh start
     */
    void decodeInit();

    /**
     * Start a new CFG-VALSET batch (header only, no config values yet)
     */
    void initCfgValset();

    /**
     * Send the CFG-VALSET built up by initCfgValset() and cfgValset*() calls
     * @return true on success
     */
    bool sendCfgValset(bool required = true, unsigned timeout = UBX_CONFIG_TIMEOUT);

    /**
     * sendCfgValset() followed by waitForAck()
     * @param required whether receiver acknowledgement is required by the configuration policy
     * @return retained evidence for this attempt
     */
    GPSCommandResult sendCfgValsetAcked(bool required = true);

    /**
     * Start or restart the survey-in process. This is only used in RTCM output mode.
     * It will be called automatically after configuring.
     * @return true on success
     */
    bool restartSurveyIn();
    bool disableTimeMode();
    bool verifyConfigValue(uint32_t key, uint8_t value);
    bool waitForSurveyStop();

    /** Route one received byte to the RTCM or UBX frame decoder. */
    int parseChar(const uint8_t b);

    /** Decide whether a validated message is decoded, ignored, or scheduled for disabling. */
    bool payloadRxInit(uint16_t message, std::span<const uint8_t> payload);

    /** Decoders for variable-length payloads that fill identity or satellite working state. */
    void decodeMonVer(std::span<const uint8_t> payload);
    void decodeNavSat(std::span<const uint8_t> payload);
    void decodeNavSvinfo(std::span<const uint8_t> payload);

    /** Decode a handled payload into the working reports: 0 unhandled, 1 handled, 2 satellite information. */
    int payloadRxDone(uint16_t message, std::span<const uint8_t> payload, GPSDecodedPosition& position);
    int decodeNavPvt(std::span<const uint8_t> payload, GPSDecodedPosition& position);
    int decodeNavigation(uint16_t message, std::span<const uint8_t> payload, GPSDecodedPosition& position);
    int decodeHeading(uint16_t message, std::span<const uint8_t> payload, GPSDecodedPosition& position);
    int decodeSurveyIn(std::span<const uint8_t> payload);
    int decodeIntegrity(uint16_t message, std::span<const uint8_t> payload);
    int decodeControl(uint16_t message, std::span<const uint8_t> payload);

    /**
     * Send a message
     * @return true on success, false on write error
     */
    bool sendMessage(uint16_t msg, const uint8_t* payload, uint16_t length,
                     GPSConfigurationStep step = {{}, std::chrono::milliseconds(UBX_CONFIG_TIMEOUT)});

    bool sendMessage(uint16_t msg, std::span<const uint8_t> payload,
                     GPSConfigurationStep step = {{}, std::chrono::milliseconds(UBX_CONFIG_TIMEOUT)})
    {
        return payload.size() <= UINT16_MAX &&
               sendMessage(msg, payload.data(), static_cast<uint16_t>(payload.size()), std::move(step));
    }

    /**
     * Wait for message acknowledge
     */
    GPSCommandResult waitForAck(uint16_t msg);
    GPSCommandResult verifyCfgValset(GPSConfigurationStep step);

    /**
     * Wait out the GNSS subsystem reset that follows a constellation change
     */

    int decodeValidatedPayload(uint16_t message, std::span<const uint8_t> payload);
    void flushDecoded() override;
    void publishEpoch(const GPSDecodedPosition& report);
    UBXNavigationEpoch _navigationEpochs;
    DecodeContext _decodeContext;
    bool _epochHasHighPrecision = false;

    struct ReceiverIdentity
    {
        Board board = Board::unknown;
        bool isM8p = false;
        bool protocol27 = false;
        bool timeModeUnsupported = false;
        char modelName[30]{};
        char firmwareVersion[30]{};
    };

    struct CommsPoll
    {
        bool pending = false;
        uint64_t nextUs = 0;
        uint64_t deadlineUs = 0;
    };

    struct TimeModeReadback
    {
        bool pending = false;
        std::optional<uint8_t> response = std::nullopt;
    };

    ReceiverIdentity _identity;
    UBX::CheckedValsetBatch<UBX_CFG_VALSET_BUF_SIZE> _valset;
    CommsPoll _comms;
    TimeModeReadback _timeModeReadback;

    uint64_t _disable_cmd_last{0};
    UBX::FrameDecoder _frameDecoder;
    bool _valsetAckAmbiguous = false;
    bool _configured{false};
    bool _survey_in_stopped{false};
    bool _got_posllh{false};
    bool _got_velned{false};
    bool _got_sec_sig{false};  ///< SEC-SIG jammingState supersedes deprecated MON-RF flags

    std::optional<RTCMStreamDecoder> _rtcm_parsing;
};
