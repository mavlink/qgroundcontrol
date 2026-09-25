#pragma once

#include <math.h>
#include <optional>

#include "GPSAsciiProtocol.h"
#include "GPSSurveyClock.h"
#include "NMEAMetadata.h"

/// Ashtech/Trimble proprietary commands and $PASHR,POS positions over the shared NMEA/RTCM stream.
class AshtechProtocol : public GPSAsciiProtocol
{
public:
    explicit AshtechProtocol(GPSProtocolIO io, bool satelliteInfoEnabled = true);

    ~AshtechProtocol() override = default;

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

    /**
     * enable output of correction output
     */
    void activateCorrectionOutput();

    void activateRTCMOutput();

    /**
     * receive data for at least the specified amount of time
     */
    void receiveWait(unsigned timeout_min);

    /// Reply matchers. A NAK rejects any command; $PASHS settings are acknowledged by an ACK, queries by their
    /// $PASHR reply, and the survey receipt by the receipt decoder.
    static GPSCommandOutcome rejection(std::string_view reply);
    static GPSCommandOutcome acknowledgement(std::string_view reply);
    GPSCommandOutcome portReply(std::string_view reply);
    GPSCommandOutcome boardReply(std::string_view reply);

    /// @a text normalized to one CR/LF-terminated command line.
    GPSConfigurationSequence::Command command(std::string_view text, GPSReplyMatcher reply = acknowledgement,
                                              bool required = true) const;

    bool sendCommand(std::string_view text, GPSReplyMatcher reply = acknowledgement);

    bool _awaitingReceipt() const { return _awaitingSurveyReceipt && replyPending(); }

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
    bool _awaitingSurveyReceipt = false;
    bool _surveyReceiptRequested = false;
    std::optional<uint64_t> _surveyReceiptStartUtc;

    AshtechBoard _board{AshtechBoard::other}; /**< board we are connected to */
};
