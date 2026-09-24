#pragma once

#include <math.h>
#include <optional>

#include "GPSAsciiProtocol.h"
#include "GPSSurveyClock.h"
#include "NMEAMetadata.h"

/// Ashtech/Trimble proprietary commands and $PASHR,POS positions over the shared NMEA/RTCM stream.
class GPSNativeAshtech : public GPSAsciiProtocol
{
public:
    explicit GPSNativeAshtech(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    ~GPSNativeAshtech() override = default;

    bool configure(unsigned& baudrate, const GPSConfig& config) override;

    bool receiverReady() const override { return _configure_done; }

private:
    static constexpr unsigned ASH_RESPONSE_TIMEOUT = 200;  // ms, timeout for waiting for a response

    const QLoggingCategory& logCategory() const override;
    int handleReceiverLine(std::string_view line) override;

    /// Sentence handlers return update flags, or nullopt when the sentence is rejected.
    std::optional<int> _handleTime(std::string_view message);
    std::optional<int> _handleGGA(const NMEA::Sentence& sentence);
    void _handleHeading(std::string_view message);
    std::optional<int> _handlePosition(std::string_view message, const NMEA::Sentence& sentence);
    std::optional<int> _handleAccuracy(const NMEA::Sentence& sentence);
    void _handleCommandReply(std::string_view message);
    std::optional<int> _handleSurveyReceipt(const NMEA::Sentence& sentence);
    void _updateSurveyDuration();
    void flushDecoded() override;
    void _expireMetadata();
    void _applyMetadata(std::optional<int> time);
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
        RECEIPT  // survey receipt
    };

    /**
     * enable output of correction output
     */
    void activateCorrectionOutput();

    void activateRTCMOutput();

    /**
     * receive data for at least the specified amount of time
     */
    void receiveWait(unsigned timeout_min);

    /// Writes @a command, normalized to one CR/LF-terminated line, and waits for @a reply or a NAK.
    bool sendCommand(std::string_view command, NMEACommand reply = NMEACommand::Acked);

    bool _awaitingReply(NMEACommand reply) const { return replyPending() && _waiting_for_command == reply; }

    bool _correction_output_activated{false};
    bool _configure_done{false};
    bool _got_pashr_pos_message{false}; /**< If we got a PASHR,POS message, we will ignore GGA messages */

    char _port{'A'};                    /**< port we are connected to (e.g. 'A') */

    uint64_t _last_timestamp_time{0};
    uint64_t _headingTimestamp{0};
    uint64_t _utcReference = 0;
    NMEA::EpochReceipt _positionEpoch;
    NMEA::EpochReceipt _accuracyReceipt;
    NMEA::GST _accuracy;
    // ZDA/GST output is requested every three seconds.
    static constexpr uint64_t METADATA_MAX_AGE_US = 5000000;

    GPSSurveyClock _surveyClock;
    bool _surveyReceiptRequested = false;
    std::optional<uint64_t> _surveyReceiptStartUtc;

    AshtechBoard _board{AshtechBoard::other}; /**< board we are connected to */

    NMEACommand _waiting_for_command{NMEACommand::Acked};
};
