#include <algorithm>
#include <chrono>
#include <ctime>
#include <math.h>

#include "Ashtech/GPSDriverAshtech.h"
#include "GPSFixQuality.h"
#include "GPSNMEAReport.h"
#include "NMEAFields.h"
#include "NMEASentence.h"

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

int GPSNativeAshtech::handleReceiverLine(std::string_view message)
{
    const auto sentence = NMEA::sentence(message);
    if (!sentence || message.size() < 7) {
        return 0;
    }

    const auto commas = std::count(message.begin(), message.end(), ',');
    const auto type = message.substr(3, 3);
    std::optional<int> updates = 0;
    if (type == "ZDA" && commas == 6) {
        updates = _handleTime(message);
    } else if (type == "GGA" && commas == 14 && !_got_pashr_pos_message) {
        updates = _handleGGA(*sentence);
    } else if (message.starts_with("$GPHDT,") && commas == 2) {
        _handleHeading(message);
    } else if (message.starts_with("$PASHR,POS,") && commas == 18) {
        updates = _handlePosition(message, *sentence);
    } else if (type == "GST" && commas == 8) {
        updates = _handleAccuracy(*sentence);
    } else if (message.starts_with("$PASHR,RECEIPT,")) {
        updates = _handleSurveyReceipt(*sentence);
    } else {
        _handleCommandReply(message);
    }
    // A rejected sentence does not advance the survey-in duration.
    if (!updates) {
        return 0;
    }
    _updateSurveyDuration();
    return *updates;
}

std::optional<int> GPSNativeAshtech::_handleTime(std::string_view message)
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
    int ashtech_hour = static_cast<int>(data.time / 10000);
    int ashtech_minute = static_cast<int>((data.time - ashtech_hour * 10000) / 100);
    double ashtech_sec = static_cast<double>(data.time - ashtech_hour * 10000 - ashtech_minute * 100);
    if (ashtech_minute > 59 || ashtech_sec >= 60.0) {
        return std::nullopt;
    }
    uint64_t usecs = static_cast<uint64_t>((ashtech_sec - static_cast<uint64_t>(ashtech_sec)) * 1000000);

    tm timeinfo{};
    timeinfo.tm_year = data.year - 1900;
    timeinfo.tm_mon = data.month - 1;
    timeinfo.tm_mday = data.day;
    timeinfo.tm_hour = ashtech_hour;
    timeinfo.tm_min = ashtech_minute;
    timeinfo.tm_sec = int(ashtech_sec);
    _utcReference = timeFromUtc(timeinfo, usecs * 1000);
    _position.navigation.utcTimeUs = _utcReference;

    _last_timestamp_time = nowUs();
    return 0;
}

std::optional<int> GPSNativeAshtech::_handleGGA(const NMEA::Sentence& sentence)
{
    const auto fix = NMEA::gga(sentence);
    if (!fix) {
        return std::nullopt;
    }
    applyNMEAGGA(_position, *fix, nowUs());
    _applyMetadata(NMEA::utcMilliseconds(sentence.fields[NMEA::Field::UTC_TIME]));
    return GPSDecodedBatch::POSITION_UPDATE;
}

void GPSNativeAshtech::_handleHeading(std::string_view message)
{
    /*
    Heading message
    Example $GPHDT,121.2,T*35

    f1 Last computed heading value, in degrees (0-359.99)
    T "T" for "True"
     */

    float heading = 0.f;

    if (NMEAFields::Cursor(message.substr(7)).read(heading)) {
        heading *= GPS_PI / 180.0f;  // deg to rad, now in range [0, 2pi]

        if (heading > GPS_PI) {
            heading -= 2.f * GPS_PI;  // final range is [-pi, pi]
        }

        _position.navigation.headingRadians = heading;
        _headingTimestamp = nowUs();
    }
}

std::optional<int> GPSNativeAshtech::_handlePosition(std::string_view message, const NMEA::Sentence& sentence)
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

    /*
     * Ashtech would return empty space as coordinate (lat, lon or alt) if it doesn't have a fix yet
     */
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

    _position.navigation.latitudeDegrees = NMEA::degreesFromDegreesMinutes(data.latitude);
    _position.navigation.longitudeDegrees = NMEA::degreesFromDegreesMinutes(data.longitude);
    _position.navigation.altitudeEllipsoidMeters = data.altitude;
    _position.navigation.altitudeMslMeters = NAN;
    _position.navigation.horizontalDop = static_cast<float>(data.horizontalDop);
    _position.navigation.verticalDop = static_cast<float>(data.verticalDop);

    if (coordinatesFound < 3) {
        _position.navigation.fixType = GPSPositionReport::FixType::NoFix;

    } else {
        if (data.quality == 9 || data.quality == 10) {  // SBAS differential or BeiDou differential
            _position.navigation.fixType = GPSPositionReport::FixType::Differential;

        } else if (data.quality == 12 || data.quality == 22) {  // RTK float or RTK float dithered
            _position.navigation.fixType = GPSPositionReport::FixType::RTKFloat;

        } else if (data.quality == 13 || data.quality == 23) {  // RTK fixed or RTK fixed dithered
            _position.navigation.fixType = GPSPositionReport::FixType::RTKFixed;

        } else {
            _position.navigation.fixType = gpsFixQualityFromValue(3 + data.quality);
        }

        _got_pashr_pos_message = true;
        // we got a valid position, activate correction output if needed
        if (_configure_done && _board == AshtechBoard::trimble_mb_two && !_correction_output_activated) {
            _correctionSetupPending = true;
        }
    }

    _position.navigation.timestampUs = nowUs();
    _applyMetadata(NMEA::utcMilliseconds(sentence.fields[4]));
    _position.navigation.satellitesUsed = static_cast<uint8_t>(data.satellites);

    float track_rad = static_cast<float>(data.trackDegrees) * GPS_PI / 180.0f;

    float velocity_ms = static_cast<float>(data.groundSpeedKnots) / 1.9438445f; /** knots to m/s */
    _position.navigation.speedMetersPerSecond = velocity_ms;                    /** GPS ground speed (m/s) */
    _position.navigation.courseRadians =
        track_rad;                  /** Course over ground (NOT heading, but direction of movement) in rad, -PI..PI */
    _position.velocityValid = true; /** Flag to indicate if NED speed is valid */
    return GPSDecodedBatch::POSITION_UPDATE;
}

std::optional<int> GPSNativeAshtech::_handleAccuracy(const NMEA::Sentence& sentence)
{
    const auto error = NMEA::gst(sentence);
    if (!error) {
        return std::nullopt;
    }
    _accuracy = *error;
    _accuracyReceipt = {NMEA::utcMilliseconds(sentence.fields[NMEA::Field::UTC_TIME]), nowUs()};
    if (_positionEpoch.matches(_accuracyReceipt, METADATA_MAX_AGE_US)) {
        _expireMetadata();
        _position.navigation.horizontalAccuracyMeters = _accuracy.horizontalAccuracy;
        _position.navigation.verticalAccuracyMeters = _accuracy.verticalAccuracy;
        return GPSDecodedBatch::POSITION_UPDATE;
    }
    return 0;
}

void GPSNativeAshtech::_handleCommandReply(std::string_view message)
{
    if (message.starts_with("$PASHR,NAK*")) {
        resolveReply(GPSCommandOutcome::Rejected);
    } else if (message.starts_with("$PASHR,ACK*")) {
        if (_awaitingReply(NMEACommand::Acked)) {
            resolveReply(GPSCommandOutcome::Acknowledged);
        }
    } else if (message.starts_with("$PASHR,PRT,") && std::count(message.begin(), message.end(), ',') == 3) {
        if (_awaitingReply(NMEACommand::PRT)) {
            resolveReply(GPSCommandOutcome::Acknowledged);
            _port = message[11];
        }
    } else if (message.starts_with("$PASHR,RID,")) {
        if (_awaitingReply(NMEACommand::RID)) {
            resolveReply(GPSCommandOutcome::Acknowledged);
            _board = message.substr(11).starts_with("MB2") ? AshtechBoard::trimble_mb_two : AshtechBoard::other;
        }
    }
}

std::optional<int> GPSNativeAshtech::_handleSurveyReceipt(const NMEA::Sentence& sentence)
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

    if (!_configure_done || std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) ||
        _board != AshtechBoard::trimble_mb_two || !_surveyReceiptRequested) {
        return std::nullopt;
    }
    if (*interval != std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).durationSecs) {
        return std::nullopt;
    }
    if (started) {
        if (!_awaitingReply(NMEACommand::RECEIPT) || _surveyReceiptStartUtc) {
            return std::nullopt;
        }
        if (_utcReference && NMEA::freshAt(_last_timestamp_time, nowUs(), METADATA_MAX_AGE_US) &&
            (*receiptTime < _utcReference || *receiptTime - _utcReference > METADATA_MAX_AGE_US)) {
            return std::nullopt;
        }
        _surveyReceiptStartUtc = receiptTime;
    } else {
        if (!_surveyReceiptStartUtc || *receiptTime < *_surveyReceiptStartUtc ||
            (!failed && *receiptTime - *_surveyReceiptStartUtc < uint64_t(*interval) * 1000000)) {
            return std::nullopt;
        }
        _surveyReceiptRequested = false;
        _surveyReceiptStartUtc.reset();
    }
    if (_awaitingReply(NMEACommand::RECEIPT)) {
        resolveReply(failed ? GPSCommandOutcome::Rejected : GPSCommandOutcome::Acknowledged);
    }
    if (!started) {
        _surveyClock.stop(nowUs());
        // The survey receipt's height is validated but not reported.
        publishSurvey(false, !failed, _surveyClock.duration(),
                      {.latitudeDegrees = latitude, .longitudeDegrees = longitude});
        _rtcmActivationPending = !failed;
    }
    return 0;
}

void GPSNativeAshtech::_updateSurveyDuration()
{
    if (_surveyClock.update(nowUs())) {
        publishSurvey(true, false, _surveyClock.duration());
    }
}

void GPSNativeAshtech::flushDecoded()
{
    _expireMetadata();
    GPSAsciiProtocol::flushDecoded();
}

void GPSNativeAshtech::_expireMetadata()
{
    const auto now = nowUs();
    if (!_headingTimestamp || !NMEA::freshAt(_headingTimestamp, now, METADATA_MAX_AGE_US)) {
        _position.navigation.headingRadians = NAN;
        _position.navigation.headingAccuracyRadians = NAN;
    }
    if (!_accuracyReceipt.time || !NMEA::freshAt(_accuracyReceipt.receivedAtUs, now, METADATA_MAX_AGE_US)) {
        _position.navigation.horizontalAccuracyMeters = NAN;
        _position.navigation.verticalAccuracyMeters = NAN;
    }
    if (!_utcReference || !NMEA::freshAt(_last_timestamp_time, now, METADATA_MAX_AGE_US)) {
        _position.navigation.utcTimeUs = 0;
    }
}

void GPSNativeAshtech::_applyMetadata(std::optional<int> time)
{
    _expireMetadata();
    const auto now = nowUs();
    _positionEpoch = {time, now};
    const bool matches = _accuracyReceipt.matches(_positionEpoch, METADATA_MAX_AGE_US);
    _position.navigation.horizontalAccuracyMeters = matches ? _accuracy.horizontalAccuracy : NAN;
    _position.navigation.verticalAccuracyMeters = matches ? _accuracy.verticalAccuracy : NAN;
    _position.navigation.utcTimeUs =
        NMEA::utcAtTimeOfDay(_utcReference, _last_timestamp_time, time, now, METADATA_MAX_AGE_US);
}

GPSNativeAshtech::GPSNativeAshtech(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSAsciiProtocol(std::move(io), satelliteInfoEnabled, Navigation::ReceiverSpecific)
{
    setRTCMEnabled(false);
}

void GPSNativeAshtech::receiveWait(unsigned timeout_min)
{
    uint64_t time_started = nowUs();

    while (nowUs() < time_started + timeout_min * 1000) {
        receive(timeout_min);
        if (hasIOError()) {
            return;
        }
    }
}

void GPSNativeAshtech::servicePendingCommands()
{
    if (_correctionSetupPending) {
        _correctionSetupPending = false;
        activateCorrectionOutput();
    }
    if (_rtcmActivationPending) {
        _rtcmActivationPending = false;
        activateRTCMOutput();
    }
}
