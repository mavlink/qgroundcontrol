#pragma once

#include "GPSProtocol.h"
#include "GPSSurveyClock.h"
#include "RTCMFramer.h"
#include "SBFMessages.h"

class GPSNativeSBF : public GPSProtocol
{
public:
    explicit GPSNativeSBF(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    ~GPSNativeSBF() override = default;

    bool receiverReady() const override { return _configured; }

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

    bool configure(unsigned& baudrate, const GPSConfig& config) override;

private:
    const QLoggingCategory& logCategory() const override;
    /**
     * @brief Parse the binary SBF packet
     */
    int parseChar(const uint8_t b);

    struct NavigationEpoch
    {
        uint64_t receiverTimeMs = 0;
        uint64_t receiptUs = 0;
        GPSNativePositionReport position;
        bool hasPosition = false;
    };

    NavigationEpoch* navigationEpoch(uint64_t receiverTimeMs);
    void finishEpoch(std::optional<NavigationEpoch>& epoch);
    void flushDecoded() override;
    std::array<std::optional<NavigationEpoch>, 2> _epochs;
    std::optional<uint64_t> _lastPublishedEpoch;
    static constexpr uint64_t EPOCH_MAX_AGE_US = 200000;
    static constexpr uint64_t WEEK_MS = 604800000;
    static constexpr uint16_t PVT_GEODETIC_LENGTH = 94;

    /**
     * @brief Add payload rx byte
     */
    int payloadRxAdd(const uint8_t b);

    /**
     * @brief Parses incoming SBF blocks
     */
    int payloadRxDone();

    /// Fills @a position from one PVTGeodetic block. @return whether its coordinates are usable.
    bool applyPvtGeodetic(const sbf_payload_pvt_geodetic_t& pvt, GPSNativePositionReport& position);
    void publishSurveyStatus(const sbf_payload_pvt_geodetic_t& pvt, const GPSNativePositionReport& position,
                             bool coordinatesValid);

    /**
     * @brief Reset the parse state machine for a fresh start
     */
    void decodeInit();

    /// @return true when every byte was written.
    bool sendMessage(const char* msg);

    /// Sends a command and waits for the receiver to echo it as "$R: <command>".
    /// @return true when acknowledged; false after a rejection ("$R?"), timeout, or I/O failure.
    bool sendMessageAndWaitForAck(const char* msg, bool required = true);

    bool _configured{false};
    sbf_decode_state_t _decode_state{SBF_DECODE_SYNC1};
    uint16_t _rx_payload_index{0};
    sbf_buf_t _buf;
    std::array<uint8_t, 110> _wire{};
    std::optional<RTCMStreamDecoder> _rtcm_parsing;

    GPSSurveyClock _surveyClock;
    bool _survey_active{false};
};

uint16_t crc16(const uint8_t* buf, uint32_t len);
