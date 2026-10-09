#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "Ashtech/AshtechPlan.h"
#include "GPSCommandChannel.h"
#include "GPSNMEAFamilyProtocol.h"
#include "GPSProtocolMath.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverFamilies.h"
#include "GPSReceiverReports.h"
#include "GPSStreamDemux.h"
#include "GPSSurveyClock.h"
#include "MonotonicClock.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AshtechProtocolLog, "GPS.Protocols.Ashtech")

namespace {

namespace Plan = Ashtech::Plan;

/// The number in @a field, @a fallback when the field is empty, or nothing when it is malformed.
template <typename T>
std::optional<T> numberOr(std::string_view field, T fallback)
{
    if (field.empty()) {
        return fallback;
    }
    return NMEA::number<T>(field);
}

uint64_t utcUs(std::chrono::year_month_day date, int milliseconds)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::sys_days(date).time_since_epoch() +
                                                                 std::chrono::milliseconds(milliseconds))
        .count();
}

/// Resolves a fresh time of day against the nearest day of a receiver's UTC reference.
uint64_t utcAtTimeOfDay(uint64_t referenceUtcUs, uint64_t referenceReceiptUs, std::optional<int> timeMs, uint64_t nowUs,
                        std::chrono::microseconds maximumAge)
{
    if (!referenceUtcUs || !timeMs || !MonotonicClock::withinAge(referenceReceiptUs, nowUs, maximumAge)) {
        return 0;
    }
    constexpr int64_t DAY_US = 86400000000;
    const auto reference = static_cast<int64_t>(referenceUtcUs);
    int64_t result = reference - reference % DAY_US + int64_t(*timeMs) * 1000;
    if (result - reference > DAY_US / 2) {
        result -= DAY_US;
    } else if (reference - result > DAY_US / 2) {
        result += DAY_US;
    }
    return result > 0 ? static_cast<uint64_t>(result) : 0;
}

void applyGGA(GPSDecodedPosition& report, const NMEA::GGA& fix, uint64_t receivedAtUs)
{
    report.navigation.latitudeDegrees = fix.latitude;
    report.navigation.longitudeDegrees = fix.longitude;
    report.navigation.altitudeMslMeters = fix.altitude;
    report.navigation.altitudeEllipsoidMeters = fix.altitude + fix.geoidSeparation;
    report.navigation.horizontalDop = fix.hdop;
    report.navigation.satellitesUsed = gpsSatellitesUsed(fix.satellitesUsed);
    report.navigation.fixType = NMEA::fixQuality(fix.quality, GPSFixQuality::Fix3D);
    report.navigation.timestampUs = receivedAtUs;
    report.velocityValid = false;
}

std::optional<uint64_t> receiptUtc(std::string_view date, std::string_view time)
{
    if (date.size() != 10 || date[2] != '.' || date[5] != '.') {
        return std::nullopt;
    }
    const auto day = NMEA::number<unsigned>(date.substr(0, 2));
    const auto month = NMEA::number<unsigned>(date.substr(3, 2));
    const auto year = NMEA::number<int>(date.substr(6, 4));
    const auto milliseconds = NMEA::utcMilliseconds(time);
    if (!day || !month || !year || *year < 1980 || *year > 9999 || !milliseconds) {
        return std::nullopt;
    }
    const std::chrono::year_month_day calendar{std::chrono::year{*year}, std::chrono::month{*month},
                                               std::chrono::day{*day}};
    if (!calendar.ok()) {
        return std::nullopt;
    }
    return utcUs(calendar, *milliseconds);
}

/// Configures an Ashtech receiver from Ashtech::Plan: finds its port and rate, moves the link to 115200 baud,
/// identifies the board, which must be an MB-Two, and selects the navigation output. The receiver becomes a base
/// station once it reports a position: the streaming services start the survey-in or set the fixed position, and start
/// RTCM output once the base position is known. A command the receiver rejects leaves a description in the channel's
/// error detail.
///
/// Decoding: GPSNMEAStream assembles satellites; positions come from $PASHR,POS, or from GGA until the first
/// $PASHR,POS, with ZDA time and GST accuracy as metadata. $PASHR,RECEIPT position-averaging receipts start and
/// complete a survey-in. Every other line is offered as a command reply.
class Protocol final : public GPSNMEAFamilyProtocol
{
public:
    Protocol()
        : GPSNMEAFamilyProtocol(GPSNMEAStream::Navigation::ReceiverSpecific)
    {}

    bool configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override;

    /// The board $PASHR,RID reported.
    QString identity() const override { return _board; }

    /// Sends the base-station commands decoding scheduled.
    void serviceStreaming(GPSCommandChannel& channel) override;

    /// The port query configure() probes each rate with.
    bool probe(GPSCommandChannel& channel) override { return _queryPort(channel, Plan::PORT_QUERY_ATTEMPTS); }

private:
    /// The UTC time of day a sentence names and when it arrived.
    struct EpochReceipt
    {
        std::optional<int> time;
        uint64_t receivedAtUs = 0;

        [[nodiscard]] bool matches(const EpochReceipt& position, std::chrono::microseconds maximumAge) const
        {
            return time && time == position.time &&
                   MonotonicClock::withinAge(receivedAtUs, position.receivedAtUs, maximumAge);
        }
    };

    /// Starts a configuration attempt: forgets the base setup, metadata and survey, and resets the stream.
    void _reset(GPSStreamDemux& stream);
    /// Asks for the port the receiver is connected to, up to @a attempts times. @return whether it answered.
    bool _queryPort(GPSCommandChannel& channel, unsigned attempts);
    /// A rejected sentence does not advance the survey-in duration.
    GPSReceiveUpdates _decodeLine(std::string_view line, GPSDecodeContext& context) override;

    /// Expires stale metadata.
    void _expire(GPSDecodeContext& context) override { _expireMetadata(context.nowUs()); }

    void _setUpBase(GPSCommandChannel& channel);
    void _activateRTCMOutput(GPSCommandChannel& channel);
    GPSCommandOutcome _portReply(std::string_view reply);
    GPSCommandOutcome _boardReply(std::string_view reply);
    /// Accepts survey receipts for a survey-start command, or stops accepting them; either forgets a started survey.
    void _requestSurveyReceipts(bool requested);
    /// Sentence handlers return updates, or nullopt when the sentence is rejected.
    std::optional<GPSReceiveUpdates> _decodeVendor(std::string_view line, GPSDecodeContext& context);
    std::optional<GPSReceiveUpdates> _handleTime(const NMEA::Sentence& sentence, uint64_t nowUs);
    std::optional<GPSReceiveUpdates> _handleGGA(const NMEA::Sentence& sentence, uint64_t nowUs);
    std::optional<GPSReceiveUpdates> _handlePosition(const NMEA::Sentence& sentence, uint64_t nowUs);
    std::optional<GPSReceiveUpdates> _handleAccuracy(const NMEA::Sentence& sentence, uint64_t nowUs);
    std::optional<GPSReceiveUpdates> _handleSurveyReceipt(const NMEA::Sentence& sentence, GPSDecodeContext& context);
    void _expireMetadata(uint64_t nowUs);
    void _applyMetadata(std::optional<int> time, uint64_t nowUs);

    /// Port the receiver reported, such as 'A'.
    char _port = 'A';
    /// Board the receiver reported, such as "MB2".
    QString _board;
    /// The latest port query.
    GPSCommandSequence::Result _portQuery;
    GPSBaseStationConfig _base{};
    /// The base setup ran: the fixed position or the survey-in was set.
    bool _baseSetupDone = false;
    /// The configured receiver reported its first valid position, so the base setup is due.
    bool _baseSetupPending = false;
    /// Position averaging finished, so RTCM output is due.
    bool _rtcmActivationPending = false;
    /// The survey-start command is outstanding, and its start receipt acknowledges it.
    bool _awaitingSurveyReceipt = false;
    GPSSurveyClock _surveyClock;
    /// After a $PASHR,POS, GGA no longer carries positions.
    bool _receiverPositions = false;
    uint64_t _utcReceivedUs = 0;
    uint64_t _utcReference = 0;
    EpochReceipt _positionEpoch;
    EpochReceipt _accuracyReceipt;
    NMEA::GST _accuracy;
    bool _surveyReceiptRequested = false;
    std::optional<uint64_t> _surveyReceiptStartUtc;
};

bool Protocol::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    _reset(channel.stream());
    _base = config.base;
    const auto detection = channel.detectBaud(Plan::BAUD_RATES, baud, [this, &channel]() -> GPSBaudProbe {
        const bool answered = _queryPort(channel, Plan::PORT_QUERY_ATTEMPTS);
        return answered ? GPSBaudProbe::Found : GPSBaudProbe::TryNext;
    });
    if (!detection.found) {
        if (_portQuery.outcome == GPSCommandOutcome::Rejected) {
            channel.failSequence(_portQuery);
        } else {
            channel.failNoAnswer(detection.probed);
        }
        return false;
    }

    baud = detection.baud;
    if (baud != Plan::LINK_BAUD) {
        baud = Plan::LINK_BAUD;
        const QByteArray speed = Plan::line(Plan::LINK_SPEED, _port);
        (void) channel.writeCommand({speed.trimmed(), Plan::RESPONSE_TIMEOUT}, speed);
        _nmea.reset(channel.stream());
        channel.receiveFor(Plan::LINK_SPEED_SETTLE);
        _nmea.reset(channel.stream());
        (void) channel.setBaudrate(baud);
        if (!_queryPort(channel, Plan::PORT_QUERY_ATTEMPTS_AT_LINK_BAUD)) {
            channel.failNoAnswer({&baud, 1});
            return false;
        }
    }

    const GPSCommandSequence board{{gpsCommand(Plan::line(Plan::BOARD_QUERY), Plan::RESPONSE_TIMEOUT,
                                               [this](std::string_view reply) { return _boardReply(reply); })}};
    if (!channel.runRequired(board)) {
        return false;
    }
    if (!_board.startsWith(QLatin1StringView(Plan::BASE_BOARD))) {
        channel.failControl(
            QStringLiteral("Trimble %1 cannot run as an RTK base station; only the MB-Two can").arg(_board));
        return false;
    }
    (void) channel.runSequence(Plan::sequence(Plan::OUTPUTS, _port));

    _nmea.setRTCMEnabled(true);
    channel.context().publishSurvey(true, false, {});
    _ready = true;
    return !channel.failed();
}

void Protocol::serviceStreaming(GPSCommandChannel& channel)
{
    if (_baseSetupPending) {
        _baseSetupPending = false;
        _setUpBase(channel);
    }
    if (_rtcmActivationPending) {
        _rtcmActivationPending = false;
        _activateRTCMOutput(channel);
    }
}

bool Protocol::_queryPort(GPSCommandChannel& channel, unsigned attempts)
{
    auto query = gpsCommand(Plan::line(Plan::PORT_QUERY), Plan::RESPONSE_TIMEOUT,
                            [this](std::string_view reply) { return _portReply(reply); });
    query.attempts = attempts;
    const GPSCommandSequence sequence{{std::move(query)}};
    _portQuery = channel.runSequence(sequence);
    return _portQuery.succeeded();
}

void Protocol::_setUpBase(GPSCommandChannel& channel)
{
    if (_baseSetupDone) {
        return;
    }
    GPSDecodeContext& context = channel.context();
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&_base.mode)) {
        if (!channel.runRequired({{Plan::fixedPositionCommand(fixed->position)}})) {
            return;
        }
        _activateRTCMOutput(channel);
        if (channel.failed()) {
            return;
        }
        context.publishSurvey(false, true, {}, fixed->position);
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&_base.mode)) {
        // Decoding acknowledges the command from its start receipt; the matcher only sees a NAK.
        const GPSCommandSequence start{
            {gpsCommand(Plan::surveyStart(survey->duration), Plan::RESPONSE_TIMEOUT, &Plan::rejection)}};
        _requestSurveyReceipts(true);
        _awaitingSurveyReceipt = true;
        const bool started = channel.runSequence(start).succeeded();
        _awaitingSurveyReceipt = false;
        if (!started) {
            _requestSurveyReceipts(false);
            channel.failControl(QStringLiteral("No matching Ashtech survey-start receipt"));
            return;
        }
        if (!channel.runRequired(Plan::sequence(Plan::STATION, _port))) {
            return;
        }
        // A finish receipt may already have arrived with the station commands' replies.
        if (!_rtcmActivationPending) {
            _surveyClock.start(context);
        }
    }
    _baseSetupDone = true;
}

void Protocol::_activateRTCMOutput(GPSCommandChannel& channel)
{
    (void) channel.runRequired(Plan::sequence(Plan::RTCM_OUTPUTS, _port));
}

GPSCommandOutcome Protocol::_portReply(std::string_view reply)
{
    if (!reply.starts_with("$PASHR,PRT,") || std::count(reply.begin(), reply.end(), ',') != 3) {
        return Plan::rejection(reply);
    }
    _port = reply[11];
    return GPSCommandOutcome::Acknowledged;
}

GPSCommandOutcome Protocol::_boardReply(std::string_view reply)
{
    constexpr std::string_view PREFIX = "$PASHR,RID,";
    if (!reply.starts_with(PREFIX)) {
        return Plan::rejection(reply);
    }
    const std::string_view board = reply.substr(PREFIX.size());
    _board = QString::fromLatin1(board.substr(0, board.find_first_of(",*")));
    return GPSCommandOutcome::Acknowledged;
}

void Protocol::_reset(GPSStreamDemux& stream)
{
    _base = {};
    _board.clear();
    _ready = false;
    _baseSetupDone = false;
    _baseSetupPending = false;
    _rtcmActivationPending = false;
    _awaitingSurveyReceipt = false;
    _surveyClock = {};
    _receiverPositions = false;
    _utcReceivedUs = 0;
    _utcReference = 0;
    _positionEpoch = {};
    _accuracyReceipt = {};
    _accuracy = {};
    _requestSurveyReceipts(false);
    _nmea.setRTCMEnabled(false);
    _nmea.reset(stream);
}

GPSReceiveUpdates Protocol::_decodeLine(std::string_view line, GPSDecodeContext& context)
{
    const auto vendor = _decodeVendor(line, context);
    if (vendor) {
        _surveyClock.publishProgress(context);
    }
    return vendor.value_or(GPSReceiveUpdates{});
}

void Protocol::_requestSurveyReceipts(bool requested)
{
    _surveyReceiptRequested = requested;
    _surveyReceiptStartUtc.reset();
}

std::optional<GPSReceiveUpdates> Protocol::_decodeVendor(std::string_view message, GPSDecodeContext& context)
{
    const auto sentence = NMEA::sentence(message);
    if (!sentence || message.size() < 7) {
        return std::nullopt;
    }

    const uint64_t now = context.nowUs();
    const auto commas = std::count(message.begin(), message.end(), ',');
    const auto type = message.substr(3, 3);
    if (type == "ZDA" && commas == 6) {
        return _handleTime(*sentence, now);
    }
    if (type == "GGA" && commas == 14 && !_receiverPositions) {
        return _handleGGA(*sentence, now);
    }
    if (message.starts_with("$PASHR,POS,") && commas == 18) {
        return _handlePosition(*sentence, now);
    }
    if (type == "GST" && commas == 8) {
        return _handleAccuracy(*sentence, now);
    }
    if (message.starts_with("$PASHR,RECEIPT,")) {
        return _handleSurveyReceipt(*sentence, context);
    }
    context.offerReply(message);
    return GPSReceiveUpdates{};
}

std::optional<GPSReceiveUpdates> Protocol::_handleTime(const NMEA::Sentence& sentence, uint64_t nowUs)
{
    const auto zda = NMEA::zda(sentence);
    if (!zda || !zda->utcMilliseconds || zda->date.year < 1980 || zda->date.year > 9999) {
        return std::nullopt;
    }
    const auto& date = zda->date;
    const std::chrono::year_month_day calendar{std::chrono::year{date.year}, std::chrono::month{date.month},
                                               std::chrono::day{date.day}};
    const uint64_t utc = utcUs(calendar, *zda->utcMilliseconds);
    _utcReference = utc / 1000000 > static_cast<uint64_t>(GPSProtocolMath::UTC_PLAUSIBILITY_FLOOR_SECS) ? utc : 0;
    _nmea.position().navigation.utcTimeUs = _utcReference;
    _utcReceivedUs = nowUs;
    return GPSReceiveUpdates{};
}

std::optional<GPSReceiveUpdates> Protocol::_handleGGA(const NMEA::Sentence& sentence, uint64_t nowUs)
{
    const auto fix = NMEA::gga(sentence);
    if (!fix) {
        return std::nullopt;
    }
    applyGGA(_nmea.position(), *fix, nowUs);
    _applyMetadata(NMEA::utcMilliseconds(sentence.fields[NMEA::Field::UTC_TIME]), nowUs);
    return GPSReceiveUpdate::Position;
}

std::optional<GPSReceiveUpdates> Protocol::_handlePosition(const NMEA::Sentence& sentence, uint64_t nowUs)
{
    /*
    Example
    $PASHR,POS,2,10,125410.00,5525.8138702,N,03833.9587380,E,131.555,1.0,0.0,0.007,-0.001,2.0,1.0,1.7,1.0,*34

        $PASHR,POS,d1,d2,m3,m4,c5,m6,c7,f8,f9,f10,f11,f12,f13,f14,f15,f16,s17*cc
        Parameter Description Range
          d1 Position mode 0: standalone
                           1: differential
                           2: RTK float
                           3: RTK fixed
                           5: Dead reckoning
                           9: SBAS (see NPT setting)
          d2 Number of satellite used in position fix 0-99
          m3 Current UTC time of position fix (hhmmss.ss) 000000.00-235959.99
          m4 Latitude of position (ddmm.mmmmmm) 0-90 degrees 00-59.9999999 minutes
          c5 Latitude sector N, S
          m6 Longitude of position (dddmm.mmmmmm) 0-180 degrees 00-59.9999999 minutes
          c7 Longitude sector E,W
          f8 Altitude above ellipsoid +9999.000
          f9 Differential age (data link age), seconds 0.0-600.0
          f10 True track/course over ground in degrees 0.0-359.9
          f11 Speed over ground in knots 0.0-999.9
          f12 Vertical velocity in decimeters per second +999.9
          f13 PDOP 0-99.9
          f14 HDOP 0-99.9
          f15 VDOP 0-99.9
          f16 TDOP 0-99.9
          s17 Reserved no data
          *cc Checksum
        */
    // d1 is the third field, after $PASHR and POS. An empty field keeps its default; a malformed one rejects the
    // sentence. Ashtech reports an empty coordinate (latitude, longitude or altitude) until it has a fix.
    const auto& fields = sentence.fields;
    const auto quality = numberOr(fields[2], 0);
    const auto satellites = numberOr(fields[3], 0);
    const auto latitude = numberOr(fields[5], 0.0);
    const auto longitude = numberOr(fields[7], 0.0);
    const auto altitude = numberOr(fields[9], 0.0);
    const auto trackDegrees = numberOr(fields[11], 0.0);
    const auto groundSpeedKnots = numberOr(fields[12], 0.0);
    const auto horizontalDop = numberOr(fields[15], 99.9);
    const auto verticalDop = numberOr(fields[16], 99.9);
    // Time, differential age, vertical velocity, PDOP and TDOP are validated but not reported.
    for (const size_t unused : {4, 10, 13, 14, 17}) {
        if (!numberOr(fields[unused], 0.0)) {
            return std::nullopt;
        }
    }
    const std::string_view northSouth = fields[6];
    const std::string_view eastWest = fields[8];
    if (!quality || *quality < 0 || *quality > 23 || !satellites || *satellites < 0 || *satellites > 255 || !latitude ||
        *latitude < 0 || *latitude > 9000 || !longitude || *longitude < 0 || *longitude > 18000 || !altitude ||
        !trackDegrees || !groundSpeedKnots || !horizontalDop || !verticalDop ||
        (northSouth != "N" && northSouth != "S") || (eastWest != "E" && eastWest != "W")) {
        return std::nullopt;
    }
    const bool coordinatesFound = !fields[5].empty() && !fields[7].empty() && !fields[9].empty();

    auto& navigation = _nmea.position().navigation;
    navigation.latitudeDegrees = (northSouth == "S" ? -1 : 1) * NMEA::degreesFromDegreesMinutes(*latitude);
    navigation.longitudeDegrees = (eastWest == "W" ? -1 : 1) * NMEA::degreesFromDegreesMinutes(*longitude);
    navigation.altitudeEllipsoidMeters = *altitude;
    navigation.altitudeMslMeters = NAN;
    navigation.horizontalDop = static_cast<float>(*horizontalDop);
    navigation.verticalDop = static_cast<float>(*verticalDop);

    if (!coordinatesFound) {
        navigation.fixType = GPSPositionReport::FixType::NoFix;

    } else {
        if (*quality == 9 || *quality == 10) {  // SBAS differential or BeiDou differential
            navigation.fixType = GPSPositionReport::FixType::Differential;

        } else if (*quality == 12 || *quality == 22) {  // RTK float or RTK float dithered
            navigation.fixType = GPSPositionReport::FixType::RTKFloat;

        } else if (*quality == 13 || *quality == 23) {  // RTK fixed or RTK fixed dithered
            navigation.fixType = GPSPositionReport::FixType::RTKFixed;

        } else {
            navigation.fixType = gpsFixQualityFromValue(3 + *quality);
        }

        _receiverPositions = true;
        // The first valid position sets up the configured receiver as a base station.
        if (_ready && !_baseSetupDone) {
            _baseSetupPending = true;
        }
    }

    navigation.timestampUs = nowUs;
    _applyMetadata(NMEA::utcMilliseconds(sentence.fields[4]), nowUs);
    navigation.satellitesUsed = gpsSatellitesUsed(static_cast<unsigned>(*satellites));

    navigation.speedMetersPerSecond = static_cast<float>(*groundSpeedKnots) / 1.9438445f;
    // Course over ground (the direction of movement, not the heading).
    navigation.courseRadians = static_cast<float>(*trackDegrees * GPSProtocolMath::DEG_TO_RAD);
    _nmea.position().velocityValid = true;
    return GPSReceiveUpdate::Position;
}

std::optional<GPSReceiveUpdates> Protocol::_handleAccuracy(const NMEA::Sentence& sentence, uint64_t nowUs)
{
    const auto error = NMEA::gst(sentence);
    if (!error) {
        return std::nullopt;
    }
    _accuracy = *error;
    _accuracyReceipt = {NMEA::utcMilliseconds(sentence.fields[NMEA::Field::UTC_TIME]), nowUs};
    if (_positionEpoch.matches(_accuracyReceipt, STATUS_MAX_AGE)) {
        _expireMetadata(nowUs);
        _nmea.position().navigation.horizontalAccuracyMeters = _accuracy.horizontalAccuracy;
        _nmea.position().navigation.verticalAccuracyMeters = _accuracy.verticalAccuracy;
        return GPSReceiveUpdate::Position;
    }
    return GPSReceiveUpdates{};
}

std::optional<GPSReceiveUpdates> Protocol::_handleSurveyReceipt(const NMEA::Sentence& sentence,
                                                                GPSDecodeContext& context)
{
    if (sentence.count < 9) {
        return std::nullopt;
    }
    const auto& fields = sentence.fields;
    if (fields[2] != "POS" || fields[3] != "AVG") {
        return std::nullopt;
    }
    const bool started = fields[4] == "STARTED";
    const bool failed = !started && fields[8] == "ERR";
    double latitude = NAN;
    double longitude = NAN;
    std::optional<uint64_t> receiptTime;
    std::optional<uint32_t> interval;

    if (started) {
        interval = NMEA::number<uint32_t>(fields[6]);
        receiptTime = receiptUtc(fields[8], fields[7]);
        if (sentence.count != 9 || fields[5] != "INTERVAL" || !interval || *interval == 0 || !receiptTime) {
            return std::nullopt;
        }
    } else {
        interval = NMEA::number<uint32_t>(fields[4]);
        receiptTime = receiptUtc(fields[7], fields[6]);
        if (!interval || *interval == 0 || fields[5] != "FINISHED" || !receiptTime ||
            sentence.count != (failed ? 9 : 16)) {
            return std::nullopt;
        }
        if (!failed) {
            const auto lat = NMEA::coordinate(fields[8], fields[9], true);
            const auto lon = NMEA::coordinate(fields[10], fields[11], false);
            const auto alt = NMEA::number<float>(fields[12]);
            const auto duration = NMEA::number<double>(fields[15]);
            if (!lat || !lon || !alt || fields[13] != "OK" || fields[14].empty() || !duration || *duration < 0) {
                return std::nullopt;
            }
            latitude = *lat;
            longitude = *lon;
        }
    }

    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&_base.mode);
    if (!_ready || !survey || !_surveyReceiptRequested) {
        return std::nullopt;
    }
    if (*interval != survey->duration.count()) {
        return std::nullopt;
    }
    const uint64_t now = context.nowUs();
    const bool awaitingReceipt = _awaitingSurveyReceipt && context.replyPending();
    if (started) {
        if (!awaitingReceipt || _surveyReceiptStartUtc) {
            return std::nullopt;
        }
        if (_utcReference && MonotonicClock::withinAge(_utcReceivedUs, now, STATUS_MAX_AGE) &&
            (*receiptTime < _utcReference ||
             std::chrono::microseconds(*receiptTime - _utcReference) > STATUS_MAX_AGE)) {
            return std::nullopt;
        }
        _surveyReceiptStartUtc = receiptTime;
    } else {
        if (!_surveyReceiptStartUtc || *receiptTime < *_surveyReceiptStartUtc ||
            (!failed && *receiptTime - *_surveyReceiptStartUtc < uint64_t(*interval) * 1000000)) {
            return std::nullopt;
        }
        _requestSurveyReceipts(false);
    }
    if (awaitingReceipt) {
        context.resolveReply(failed ? GPSCommandOutcome::Rejected : GPSCommandOutcome::Acknowledged);
    }
    if (!started) {
        // The survey receipt's height is validated but not reported.
        _surveyClock.finish(context, !failed, {.latitudeDegrees = latitude, .longitudeDegrees = longitude});
        _rtcmActivationPending = !failed;
    }
    return GPSReceiveUpdates{};
}

void Protocol::_expireMetadata(uint64_t nowUs)
{
    auto& navigation = _nmea.position().navigation;
    if (!_accuracyReceipt.time || !MonotonicClock::withinAge(_accuracyReceipt.receivedAtUs, nowUs, STATUS_MAX_AGE)) {
        navigation.horizontalAccuracyMeters = NAN;
        navigation.verticalAccuracyMeters = NAN;
    }
    if (!_utcReference || !MonotonicClock::withinAge(_utcReceivedUs, nowUs, STATUS_MAX_AGE)) {
        navigation.utcTimeUs = 0;
    }
}

void Protocol::_applyMetadata(std::optional<int> time, uint64_t nowUs)
{
    _expireMetadata(nowUs);
    _positionEpoch = {time, nowUs};
    const bool matches = _accuracyReceipt.matches(_positionEpoch, STATUS_MAX_AGE);
    auto& navigation = _nmea.position().navigation;
    navigation.horizontalAccuracyMeters = matches ? _accuracy.horizontalAccuracy : NAN;
    navigation.verticalAccuracyMeters = matches ? _accuracy.verticalAccuracy : NAN;
    navigation.utcTimeUs = utcAtTimeOfDay(_utcReference, _utcReceivedUs, time, nowUs, STATUS_MAX_AGE);
}

QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    return frame.kind == GPSFrameKind::ASCIILine && text.starts_with("$PASHR,") && NMEA::sentence(text)
               ? QLatin1StringView("$PASHR sentences")
               : QLatin1StringView();
}

}  // namespace

namespace Ashtech {

const GPSReceiverFamily FAMILY{
    .type = GPSType::trimble,
    .logCategory = &AshtechProtocolLog,
    .stream = GPSNMEAStream::STREAM,
    .baudCandidates = Plan::BAUD_RATES,
    // Unless a rate is selected, configure at the Trimble default rather than probing BAUD_RATES.
    .autoBaudRate = Plan::LINK_BAUD,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
};

}  // namespace Ashtech
