#include "Ashtech/AshtechDecoder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>

#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSFixQuality.h"
#include "GPSNMEAReport.h"
#include "GPSProtocolMath.h"
#include "NMEAFields.h"

namespace Ashtech {

namespace {

struct ZdaFields
{
    double time = 0.0;
    int day = 0;
    int month = 0;
    int year = 0;
    int zoneHour = 0;
    int zoneMinute = 0;
};

struct PashrPositionFields
{
    double time = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    int satellites = 0;
    int quality = 0;
    double trackDegrees = 0.0;
    double groundSpeedKnots = 0.0;
    double correctionAge = 0.0;
    double horizontalDop = 99.9;
    double verticalDop = 99.9;
    double positionDop = 99.9;
    double timeDop = 99.9;
    double verticalVelocity = 0.0;
    char northSouth = '?';
    char eastWest = '?';
};

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
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::sys_days(calendar).time_since_epoch() +
                                                                 std::chrono::milliseconds(*milliseconds))
        .count();
}

}  // namespace

Decoder::Decoder(bool satelliteInfoEnabled)
    : _nmea(GPSNMEAStream::Navigation::ReceiverSpecific, satelliteInfoEnabled)
{
    _nmea.setRTCMEnabled(false);
}

void Decoder::reset(GPSStreamDemux& stream)
{
    _session = {};
    _receiverPositions = false;
    _utcReceivedUs = 0;
    _utcReference = 0;
    _headingReceivedUs = 0;
    _positionEpoch = {};
    _accuracyReceipt = {};
    _accuracy = {};
    requestSurveyReceipts(false);
    _nmea.setRTCMEnabled(false);
    _nmea.reset(stream);
}

GPSReceiveUpdates Decoder::onFrame(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (frame.kind == GPSFrameKind::RTCM3) {
        return _nmea.decodeRTCM(frame, context);
    }
    if (frame.kind != GPSFrameKind::ASCIILine) {
        return {};
    }
    GPSReceiveUpdates updates = _nmea.decodeStandard(frame.text(), context);
    // A rejected sentence does not advance the survey-in duration.
    if (const auto vendor = _decodeVendor(frame.text(), context)) {
        updates |= *vendor;
        if (_session.surveyClock.update(context.nowUs())) {
            context.sink().publishSurvey(true, false, _session.surveyClock.duration());
        }
    }
    return _nmea.finishLine(updates, context);
}

void Decoder::flush(GPSDecodeContext& context)
{
    _expireMetadata(context.nowUs());
    _nmea.flush(context);
}

void Decoder::requestSurveyReceipts(bool requested)
{
    _surveyReceiptRequested = requested;
    _surveyReceiptStartUtc.reset();
}

std::optional<GPSReceiveUpdates> Decoder::_decodeVendor(std::string_view message, GPSDecodeContext& context)
{
    const auto sentence = NMEA::sentence(message);
    if (!sentence || message.size() < 7) {
        return std::nullopt;
    }

    const uint64_t now = context.nowUs();
    const auto commas = std::count(message.begin(), message.end(), ',');
    const auto type = message.substr(3, 3);
    if (type == "ZDA" && commas == 6) {
        return _handleTime(message, now);
    }
    if (type == "GGA" && commas == 14 && !_receiverPositions) {
        return _handleGGA(*sentence, now);
    }
    if (message.starts_with("$GPHDT,") && commas == 2) {
        _handleHeading(message, now);
        return GPSReceiveUpdates{};
    }
    if (message.starts_with("$PASHR,POS,") && commas == 18) {
        return _handlePosition(message, *sentence, now);
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

std::optional<GPSReceiveUpdates> Decoder::_handleTime(std::string_view message, uint64_t nowUs)
{
    /*
    UTC day, month, and year, and local time zone offset
    An example of the ZDA message string is:

    $GPZDA,172809.456,12,07,1996,00,00*45

    ZDA message fields
    Field	Meaning
    0	Message ID $GPZDA
    1	UTC
    2	Day, ranging between 01 and 31
    3	Month, ranging between 01 and 12
    4	Year
    5	Local time zone offset from GMT, ranging from 00 through 13 hours
    6	Local time zone offset from GMT, ranging from 00 through 59 minutes
    7	The checksum data, always begins with *
    Fields 5 and 6 together yield the total offset. For example, if field 5 is -5 and field 6 is +15, local time is
    5 hours and 15 minutes earlier than GMT.
    */
    NMEAFields::Cursor fields(message.substr(7));
    ZdaFields data;
    fields.read(data.time);
    fields.read(data.day);
    fields.read(data.month);
    fields.read(data.year);
    fields.read(data.zoneHour);
    fields.read(data.zoneMinute);

    if (!fields.valid()) {
        return std::nullopt;
    }

    if (data.time < 0 || data.time >= 240000 || data.year < 1980 || data.year > 9999 || data.month < 1 ||
        data.month > 12 || data.day < 1 || data.day > 31) {
        return std::nullopt;
    }
    const int hour = static_cast<int>(data.time / 10000);
    const int minute = static_cast<int>((data.time - hour * 10000) / 100);
    const double seconds = static_cast<double>(data.time - hour * 10000 - minute * 100);
    if (minute > 59 || seconds >= 60.0) {
        return std::nullopt;
    }
    const auto micros = static_cast<uint64_t>((seconds - static_cast<uint64_t>(seconds)) * 1000000);

    tm timeinfo{};
    timeinfo.tm_year = data.year - 1900;
    timeinfo.tm_mon = data.month - 1;
    timeinfo.tm_mday = data.day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = static_cast<int>(seconds);
    _utcReference = GPSProtocolMath::utcMicroseconds(timeinfo, static_cast<int32_t>(micros * 1000));
    _nmea.position().navigation.utcTimeUs = _utcReference;

    _utcReceivedUs = nowUs;
    return GPSReceiveUpdates{};
}

std::optional<GPSReceiveUpdates> Decoder::_handleGGA(const NMEA::Sentence& sentence, uint64_t nowUs)
{
    const auto fix = NMEA::gga(sentence);
    if (!fix) {
        return std::nullopt;
    }
    applyNMEAGGA(_nmea.position(), *fix, nowUs);
    _applyMetadata(NMEA::utcMilliseconds(sentence.fields[NMEA::Field::UTC_TIME]), nowUs);
    return GPSReceiveUpdate::Position;
}

void Decoder::_handleHeading(std::string_view message, uint64_t nowUs)
{
    /*
    Heading message
    Example $GPHDT,121.2,T*35

    f1 Last computed heading value, in degrees (0-359.99)
    T "T" for "True"
     */

    float heading = 0.f;

    if (NMEAFields::Cursor(message.substr(7)).read(heading)) {
        heading *= GPSProtocolMath::DEG_TO_RAD;  // now in range [0, 2pi]

        if (heading > GPSProtocolMath::PI) {
            heading -= 2.f * GPSProtocolMath::PI;  // final range is [-pi, pi]
        }

        _nmea.position().navigation.headingRadians = heading;
        _headingReceivedUs = nowUs;
    }
}

std::optional<GPSReceiveUpdates> Decoder::_handlePosition(std::string_view message, const NMEA::Sentence& sentence,
                                                          uint64_t nowUs)
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
    NMEAFields::Cursor bufptr(message.substr(11));

    // Ashtech reports an empty coordinate (latitude, longitude or altitude) until it has a fix.
    int coordinatesFound = 0;
    PashrPositionFields data;
    bufptr.read(data.quality);
    bufptr.read(data.satellites);
    bufptr.read(data.time);
    if (bufptr.read(data.latitude)) {
        ++coordinatesFound;
    }

    bufptr.read(data.northSouth);
    if (bufptr.read(data.longitude)) {
        ++coordinatesFound;
    }

    bufptr.read(data.eastWest);
    if (bufptr.read(data.altitude)) {
        ++coordinatesFound;
    }

    bufptr.read(data.correctionAge);
    bufptr.read(data.trackDegrees);
    bufptr.read(data.groundSpeedKnots);
    bufptr.read(data.verticalVelocity);
    bufptr.read(data.positionDop);
    bufptr.read(data.horizontalDop);
    bufptr.read(data.verticalDop);
    bufptr.read(data.timeDop);

    if (!bufptr.valid()) {
        return std::nullopt;
    }

    if (data.quality < 0 || data.quality > 23 || data.satellites < 0 || data.satellites > 255 || data.latitude < 0 ||
        data.latitude > 9000 || data.longitude < 0 || data.longitude > 18000 ||
        (data.northSouth != 'N' && data.northSouth != 'S') || (data.eastWest != 'E' && data.eastWest != 'W')) {
        return std::nullopt;
    }
    if (data.northSouth == 'S') {
        data.latitude = -data.latitude;
    }

    if (data.eastWest == 'W') {
        data.longitude = -data.longitude;
    }

    auto& navigation = _nmea.position().navigation;
    navigation.latitudeDegrees = NMEA::degreesFromDegreesMinutes(data.latitude);
    navigation.longitudeDegrees = NMEA::degreesFromDegreesMinutes(data.longitude);
    navigation.altitudeEllipsoidMeters = data.altitude;
    navigation.altitudeMslMeters = NAN;
    navigation.horizontalDop = static_cast<float>(data.horizontalDop);
    navigation.verticalDop = static_cast<float>(data.verticalDop);

    if (coordinatesFound < 3) {
        navigation.fixType = GPSPositionReport::FixType::NoFix;

    } else {
        if (data.quality == 9 || data.quality == 10) {  // SBAS differential or BeiDou differential
            navigation.fixType = GPSPositionReport::FixType::Differential;

        } else if (data.quality == 12 || data.quality == 22) {  // RTK float or RTK float dithered
            navigation.fixType = GPSPositionReport::FixType::RTKFloat;

        } else if (data.quality == 13 || data.quality == 23) {  // RTK fixed or RTK fixed dithered
            navigation.fixType = GPSPositionReport::FixType::RTKFixed;

        } else {
            navigation.fixType = gpsFixQualityFromValue(3 + data.quality);
        }

        _receiverPositions = true;
        // The first valid position sets up a configured MB-Two as a base station.
        if (_session.configured && _session.board == Board::MBTwo && !_session.correctionOutputActive) {
            _session.correctionSetupPending = true;
        }
    }

    navigation.timestampUs = nowUs;
    _applyMetadata(NMEA::utcMilliseconds(sentence.fields[4]), nowUs);
    navigation.satellitesUsed = static_cast<uint8_t>(data.satellites);

    navigation.speedMetersPerSecond = static_cast<float>(data.groundSpeedKnots) / 1.9438445f;
    // Course over ground (the direction of movement, not the heading).
    navigation.courseRadians = static_cast<float>(data.trackDegrees) * GPSProtocolMath::PI / 180.0f;
    _nmea.position().velocityValid = true;
    return GPSReceiveUpdate::Position;
}

std::optional<GPSReceiveUpdates> Decoder::_handleAccuracy(const NMEA::Sentence& sentence, uint64_t nowUs)
{
    const auto error = NMEA::gst(sentence);
    if (!error) {
        return std::nullopt;
    }
    _accuracy = *error;
    _accuracyReceipt = {NMEA::utcMilliseconds(sentence.fields[NMEA::Field::UTC_TIME]), nowUs};
    if (_positionEpoch.matches(_accuracyReceipt, METADATA_MAX_AGE)) {
        _expireMetadata(nowUs);
        _nmea.position().navigation.horizontalAccuracyMeters = _accuracy.horizontalAccuracy;
        _nmea.position().navigation.verticalAccuracyMeters = _accuracy.verticalAccuracy;
        return GPSReceiveUpdate::Position;
    }
    return GPSReceiveUpdates{};
}

std::optional<GPSReceiveUpdates> Decoder::_handleSurveyReceipt(const NMEA::Sentence& sentence,
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

    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&_session.base.mode);
    if (!_session.configured || !survey || _session.board != Board::MBTwo || !_surveyReceiptRequested) {
        return std::nullopt;
    }
    if (*interval != survey->duration.count()) {
        return std::nullopt;
    }
    const uint64_t now = context.nowUs();
    const bool awaitingReceipt = _session.awaitingSurveyReceipt && context.replyPending();
    if (started) {
        if (!awaitingReceipt || _surveyReceiptStartUtc) {
            return std::nullopt;
        }
        if (_utcReference && NMEA::freshAt(_utcReceivedUs, now, METADATA_MAX_AGE) &&
            (*receiptTime < _utcReference ||
             std::chrono::microseconds(*receiptTime - _utcReference) > METADATA_MAX_AGE)) {
            return std::nullopt;
        }
        _surveyReceiptStartUtc = receiptTime;
    } else {
        if (!_surveyReceiptStartUtc || *receiptTime < *_surveyReceiptStartUtc ||
            (!failed && *receiptTime - *_surveyReceiptStartUtc < uint64_t(*interval) * 1000000)) {
            return std::nullopt;
        }
        requestSurveyReceipts(false);
    }
    if (awaitingReceipt) {
        context.resolveReply(failed ? GPSCommandOutcome::Rejected : GPSCommandOutcome::Acknowledged);
    }
    if (!started) {
        _session.surveyClock.stop(now);
        // The survey receipt's height is validated but not reported.
        context.sink().publishSurvey(false, !failed, _session.surveyClock.duration(),
                                     {.latitudeDegrees = latitude, .longitudeDegrees = longitude});
        _session.rtcmActivationPending = !failed;
    }
    return GPSReceiveUpdates{};
}

void Decoder::_expireMetadata(uint64_t nowUs)
{
    auto& navigation = _nmea.position().navigation;
    if (!_headingReceivedUs || !NMEA::freshAt(_headingReceivedUs, nowUs, METADATA_MAX_AGE)) {
        navigation.headingRadians = NAN;
        navigation.headingAccuracyRadians = NAN;
    }
    if (!_accuracyReceipt.time || !NMEA::freshAt(_accuracyReceipt.receivedAtUs, nowUs, METADATA_MAX_AGE)) {
        navigation.horizontalAccuracyMeters = NAN;
        navigation.verticalAccuracyMeters = NAN;
    }
    if (!_utcReference || !NMEA::freshAt(_utcReceivedUs, nowUs, METADATA_MAX_AGE)) {
        navigation.utcTimeUs = 0;
    }
}

void Decoder::_applyMetadata(std::optional<int> time, uint64_t nowUs)
{
    _expireMetadata(nowUs);
    _positionEpoch = {time, nowUs};
    const bool matches = _accuracyReceipt.matches(_positionEpoch, METADATA_MAX_AGE);
    auto& navigation = _nmea.position().navigation;
    navigation.horizontalAccuracyMeters = matches ? _accuracy.horizontalAccuracy : NAN;
    navigation.verticalAccuracyMeters = matches ? _accuracy.verticalAccuracy : NAN;
    navigation.utcTimeUs = NMEA::utcAtTimeOfDay(_utcReference, _utcReceivedUs, time, nowUs, METADATA_MAX_AGE);
}

}  // namespace Ashtech
