#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

#include "GPSBaseStationConfig.h"
#include "GPSFrame.h"
#include "GPSNMEAStream.h"
#include "GPSReceiveUpdates.h"
#include "GPSSurveyClock.h"
#include "NMEAMetadata.h"
#include "NMEASentence.h"

class GPSDecodeContext;
class GPSStreamDemux;

namespace Ashtech {

enum class Board
{
    MBTwo,
    Other,
};

/// What configuration established about the receiver, and the base-station commands the decoder leaves for the
/// configurator's streaming services.
struct Session
{
    GPSBaseStationConfig base{};
    Board board = Board::Other;
    bool configured = false;
    bool correctionOutputActive = false;
    /// A configured MB-Two reported its first valid position, so the base setup is due.
    bool correctionSetupPending = false;
    /// Position averaging finished, so RTCM output is due.
    bool rtcmActivationPending = false;
    /// The survey-start command is outstanding, and its start receipt acknowledges it.
    bool awaitingSurveyReceipt = false;
    GPSSurveyClock surveyClock;
};

/// Decodes the Ashtech stream without I/O. GPSNMEAStream assembles satellites; positions come from $PASHR,POS, or
/// from GGA until the first $PASHR,POS, with ZDA time, GST accuracy and HDT heading as metadata. $PASHR,RECEIPT
/// position-averaging receipts start and complete a survey-in. Every other line is offered as a command reply.
class Decoder
{
public:
    explicit Decoder(bool satelliteInfoEnabled);

    /// Starts a configuration attempt: forgets the session, metadata and survey, and resets the stream.
    void reset(GPSStreamDemux& stream);

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context);

    /// Expires stale metadata, then flushes the NMEA stream.
    void flush(GPSDecodeContext& context);

    /// Accepts survey receipts for a survey-start command, or stops accepting them; either forgets a started survey.
    void requestSurveyReceipts(bool requested);

    [[nodiscard]] Session& session() { return _session; }

    [[nodiscard]] const Session& session() const { return _session; }

    [[nodiscard]] GPSNMEAStream& nmea() { return _nmea; }

private:
    /// Sentence handlers return updates, or nullopt when the sentence is rejected.
    std::optional<GPSReceiveUpdates> _decodeVendor(std::string_view line, GPSDecodeContext& context);
    std::optional<GPSReceiveUpdates> _handleTime(std::string_view message, uint64_t nowUs);
    std::optional<GPSReceiveUpdates> _handleGGA(const NMEA::Sentence& sentence, uint64_t nowUs);
    void _handleHeading(std::string_view message, uint64_t nowUs);
    std::optional<GPSReceiveUpdates> _handlePosition(std::string_view message, const NMEA::Sentence& sentence,
                                                     uint64_t nowUs);
    std::optional<GPSReceiveUpdates> _handleAccuracy(const NMEA::Sentence& sentence, uint64_t nowUs);
    std::optional<GPSReceiveUpdates> _handleSurveyReceipt(const NMEA::Sentence& sentence, GPSDecodeContext& context);
    void _expireMetadata(uint64_t nowUs);
    void _applyMetadata(std::optional<int> time, uint64_t nowUs);

    // ZDA and GST output is requested every three seconds.
    static constexpr std::chrono::microseconds METADATA_MAX_AGE{5000000};

    GPSNMEAStream _nmea;
    Session _session;
    /// After a $PASHR,POS, GGA no longer carries positions.
    bool _receiverPositions = false;
    uint64_t _utcReceivedUs = 0;
    uint64_t _utcReference = 0;
    uint64_t _headingReceivedUs = 0;
    NMEA::EpochReceipt _positionEpoch;
    NMEA::EpochReceipt _accuracyReceipt;
    NMEA::GST _accuracy;
    bool _surveyReceiptRequested = false;
    std::optional<uint64_t> _surveyReceiptStartUtc;
};

}  // namespace Ashtech
