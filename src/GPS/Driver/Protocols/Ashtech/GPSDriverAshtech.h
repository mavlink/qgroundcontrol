#pragma once

#include <math.h>
#include <optional>

#include "GPSProtocol.h"
#include "NMEAFramer.h"
#include "NMEAMetadata.h"
#include "NMEASatelliteEpoch.h"
#include "RTCMFramer.h"

#define ASHTECH_RECV_BUFFER_SIZE 512
#define ASH_RESPONSE_TIMEOUT 200  // ms, timeout for waiting for a response

class GPSNativeAshtech : public GPSProtocol
{
public:
    explicit GPSNativeAshtech(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    ~GPSNativeAshtech() override = default;

    int configure(unsigned& baudrate, const GPSConfig& config) override;

    bool receiverReady() const override { return _configure_done; }

    int receive(unsigned timeout) override;
    int decodeByte(uint8_t byte) override;

private:
    void flushDecoded() override;
    void _expireMetadata();
    void _applyMetadata(std::optional<int> time);
    void _queueSatellites(NMEA::SatelliteEpoch epoch);
    void _drainSatellites();
    void servicePendingCommands() override;
    bool _correctionSetupPending = false;
    bool _rtcmActivationPending = false;
    enum class AshtechBoard
    {
        trimble_mb_two,
        other
    };

    enum class NMEACommand
    {
        Acked,   // Command that returns a (N)Ack
        PRT,     // port config
        RID,     // board identification
        RECEIPT  // board identification
    };

    enum class NMEACommandState
    {
        idle,
        waiting,
        nack,
        received
    };

    /**
     * enable output of correction output
     */
    void activateCorrectionOutput();

    void activateRTCMOutput();

    void decodeInit(void);

    int handleMessage(int len);

    int parseChar(uint8_t b);

    /**
     * receive data for at least the specified amount of time
     */
    void receiveWait(unsigned timeout_min);

    void sendSurveyInStatusUpdate(bool active, bool valid, double latitude = static_cast<double>(NAN),
                                  double longitude = static_cast<double>(NAN), float altitude = NAN);

    /**
     * Write a command and wait for a (N)Ack
     * @return 0 on success, <0 otherwise
     */
    int writeAckedCommand(const void* buf, int buf_length, unsigned timeout);

    int waitForReply(NMEACommand command);

    bool _correction_output_activated{false};
    bool _configure_done{false};
    bool _got_pashr_pos_message{false}; /**< If we got a PASHR,POS message, we will ignore GGA messages */

    char _port{'A'};                    /**< port we are connected to (e.g. 'A') */

    uint8_t _rx_buffer[ASHTECH_RECV_BUFFER_SIZE];
    NMEA::Framer _nmeaFramer{_rx_buffer};
    uint64_t _last_timestamp_time{0};
    uint64_t _headingTimestamp{0};
    uint64_t _utcReference = 0;
    NMEA::EpochReceipt _positionEpoch;
    NMEA::EpochReceipt _accuracyReceipt;
    NMEA::GST _accuracy;
    // ZDA/GST output is requested every three seconds.
    static constexpr uint64_t METADATA_MAX_AGE_US = 5000000;

    uint32_t _survey_duration = 0;
    uint64_t _survey_in_start{0};
    bool _surveyReceiptRequested = false;
    std::optional<uint64_t> _surveyReceiptStartUtc;

    NMEA::SatelliteAssembler _satelliteAssembler;
    NMEA::SatelliteEpoch _pendingSatellites;

    AshtechBoard _board{AshtechBoard::other}; /**< board we are connected to */

    NMEACommand _waiting_for_command;

    NMEACommandState _command_state{NMEACommandState::idle};

    std::optional<RTCMStreamDecoder> _rtcm_parsing;
};
