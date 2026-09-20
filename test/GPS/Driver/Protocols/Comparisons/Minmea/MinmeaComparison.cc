#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <minmea.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "NMEASatelliteEpoch.h"
#include "NMEASentence.h"
#include "UnitTest.h"

namespace {
constexpr size_t MAX_SENTENCE_BYTES = 256;
constexpr double FLOAT_TOLERANCE = 2 * std::numeric_limits<float>::epsilon();

void require(bool condition, std::string_view context)
{
    if (!condition) {
        throw std::runtime_error(std::string(context));
    }
}

std::string readFixture(const char* name)
{
    std::ifstream input(std::string(GPS_FIXTURE_DIR) + '/' + name, std::ios::binary);
    require(input.good(), std::string("cannot read fixture: ") + name);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::vector<std::string> fixtureSentences(const char* name)
{
    const auto bytes = readFixture(name);
    std::vector<std::string> lines;
    for (size_t offset = 0; offset < bytes.size();) {
        const auto newline = bytes.find('\n', offset);
        const auto end = newline == std::string::npos ? bytes.size() : newline + 1;
        lines.push_back(bytes.substr(offset, end - offset));
        offset = end;
    }
    return lines;
}

bool sameFloat(double actual, float reference)
{
    if (std::isnan(reference)) {
        return std::isnan(actual);
    }
    return std::isfinite(actual) && std::abs(actual - reference) <= FLOAT_TOLERANCE * std::max(1.0, std::abs(actual));
}

std::string sentence(std::string_view body)
{
    std::string wire = "$" + std::string(body);
    constexpr std::string_view HEX = "0123456789ABCDEF";
    const uint8_t checksum = minmea_checksum(wire.c_str());
    require(NMEA::checksum(body) == checksum, "checksum implementations disagree");
    wire += '*';
    wire += HEX[checksum >> 4];
    wire += HEX[checksum & 0xf];
    wire += "\r\n";
    return wire;
}

void validateInput(const std::string& wire)
{
    require(wire.size() <= MAX_SENTENCE_BYTES && wire.find('\0') == std::string::npos,
            "reference input must be bounded and NUL-free");
    require(minmea_check(wire.c_str(), true), "minmea rejected a valid checksum");
    require(NMEA::sentence(wire).has_value(), "QGC rejected a valid sentence");
}

void compareGga(const std::string& wire, double& maximumCoordinateDifference, int& absentCounts)
{
    validateInput(wire);
    minmea_sentence_gga reference{};
    require(minmea_parse_gga(&reference, wire.c_str()), "minmea could not parse GGA");
    const auto parsed = NMEA::sentence(wire);
    const auto actual = NMEA::gga(*parsed);
    require(actual.has_value(), "QGC could not parse GGA");
    const float latitude = minmea_tocoord(&reference.latitude);
    const float longitude = minmea_tocoord(&reference.longitude);
    require(sameFloat(actual->latitude, latitude), "GGA latitude exceeds the reference float precision");
    require(sameFloat(actual->longitude, longitude), "GGA longitude exceeds the reference float precision");
    maximumCoordinateDifference = std::max(
        {maximumCoordinateDifference, std::abs(actual->latitude - latitude), std::abs(actual->longitude - longitude)});
    require(sameFloat(actual->altitude, minmea_tofloat(&reference.altitude)), "GGA MSL altitude differs");
    require(sameFloat(actual->geoidSeparation, minmea_tofloat(&reference.height)), "GGA geoid separation differs");
    require(sameFloat(actual->hdop, minmea_tofloat(&reference.hdop)), "GGA HDOP differs");
    require(actual->quality == static_cast<unsigned>(reference.fix_quality), "GGA fix quality differs");
    const auto milliseconds = NMEA::utcMilliseconds(parsed->fields[NMEA::Field::UTC_TIME]);
    const int referenceMilliseconds =
        ((reference.time.hours * 60 + reference.time.minutes) * 60 + reference.time.seconds) * 1000 +
        reference.time.microseconds / 1000;
    require(milliseconds == referenceMilliseconds, "GGA UTC milliseconds differ");
    if (parsed->fields[NMEA::Field::GGA_SATELLITES_USED].empty()) {
        require(!actual->satellitesUsed && reference.satellites_tracked == 0,
                "QGC must retain the missing integer that minmea collapses to zero");
        ++absentCounts;
    } else {
        require(actual->satellitesUsed == static_cast<unsigned>(reference.satellites_tracked),
                "GGA known satellite count differs");
    }
}

void compareGst(const std::string& wire)
{
    validateInput(wire);
    minmea_sentence_gst reference{};
    require(minmea_parse_gst(&reference, wire.c_str()), "minmea could not parse GST");
    const auto actual = NMEA::gst(*NMEA::sentence(wire));
    require(actual.has_value(), "QGC could not parse GST");
    const float horizontal = std::hypot(minmea_tofloat(&reference.latitude_error_deviation),
                                        minmea_tofloat(&reference.longitude_error_deviation));
    require(sameFloat(actual->horizontalAccuracy, horizontal), "GST horizontal uncertainty differs");
    require(sameFloat(actual->verticalAccuracy, minmea_tofloat(&reference.altitude_error_deviation)),
            "GST vertical uncertainty differs");
}

void compareGsv(const std::string& wire)
{
    validateInput(wire);
    minmea_sentence_gsv reference{};
    require(minmea_parse_gsv(&reference, wire.c_str()), "minmea could not parse GSV");
    const auto parsed = NMEA::sentence(wire);
    const auto actual = NMEA::gsv(*parsed);
    require(actual.has_value(), "QGC could not parse GSV");
    require(actual->messages == reference.total_msgs && actual->message == reference.msg_nr &&
                actual->satelliteCount == reference.total_sats,
            "GSV page metadata differs");
    const int pageSatellites = std::min(4, reference.total_sats - (reference.msg_nr - 1) * 4);
    require(pageSatellites >= 0 && actual->satellites.size() == static_cast<size_t>(pageSatellites),
            "GSV satellite entries differ");
    char talker[3]{};
    require(minmea_talker_id(talker, wire.c_str()) && parsed->talker() == talker, "GSV talker differs");
    for (size_t index = 0; index < actual->satellites.size(); ++index) {
        require(index < std::size(reference.sats), "single-page GSV exceeds the oracle's four satellite slots");
        const auto& satellite = actual->satellites[index];
        const auto& expected = reference.sats[index];
        require(satellite.prn == expected.nr, "GSV wire satellite number differs");
        require(satellite.elevation == expected.elevation && satellite.azimuth == expected.azimuth,
                "GSV known satellite angles differ");
        const float signal = minmea_tofloat(&expected.snr);
        if (std::isnan(signal)) {
            require(!satellite.signal, "missing GSV signal must remain unavailable");
        } else {
            require(satellite.signal && sameFloat(*satellite.signal, signal), "GSV signal differs");
        }
    }
}

void rejectChecksumFailures(const std::string& wire)
{
    const auto star = wire.find('*');
    require(star != std::string::npos, "fixture has no checksum delimiter");
    auto corrupt = wire;
    corrupt[star + 1] = corrupt[star + 1] == '0' ? '1' : '0';
    require(!minmea_check(corrupt.c_str(), true) && !NMEA::sentence(corrupt),
            "corrupted checksum was not rejected by both parsers");
    const auto truncated = wire.substr(0, star + 1);
    require(!minmea_check(truncated.c_str(), true) && !NMEA::sentence(truncated),
            "truncated checksum was not rejected by both parsers");
}
}  // namespace

class GPSMinmeaComparisonTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _comparison();
};

void GPSMinmeaComparisonTest::_comparison()
{
    try {
        const std::array ggaBodies{
            "GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,",
            "GNGGA,000000,3352.1280,S,15112.5580,W,4,12,0.7,0.0,M,-31.5,M,,",
            "GAGGA,235959.9999,9000.0000,N,18000.0000,E,0,00,0.0,-12.3,M,0.0,M,,",
            "GPGGA,010203.0019,0000.0000,S,00000.0000,W,6,05,1.2,10.0,M,,M,,",
            "GNGGA,010203,4723.8620,N,00832.7360,E,1,,,450.0,M,,M,,",
            "GPGGA,010203,4723.8620,N,00832.7360,E,1,00,0.0,,M,,M,,",
        };
        double maximumCoordinateDifference = 0;
        int absentCounts = 0;
        int missingCoordinates = 0;
        int ggaCases = 0;
        int checksumRejections = 0;
        for (const auto body : ggaBodies) {
            const auto wire = sentence(body);
            compareGga(wire, maximumCoordinateDifference, absentCounts);
            ++ggaCases;
            rejectChecksumFailures(wire);
            checksumRejections += 2;
        }
        const auto upstream = readFixture("gga.nmea");
        compareGga(upstream, maximumCoordinateDifference, absentCounts);
        ++ggaCases;
        rejectChecksumFailures(upstream);
        checksumRejections += 2;
        for (const auto& wire : fixtureSentences("synthetic-gga.nmea")) {
            validateInput(wire);
            const auto parsed = NMEA::sentence(wire);
            if (parsed->fields[NMEA::Field::GGA_LATITUDE].empty() ||
                parsed->fields[NMEA::Field::GGA_LONGITUDE].empty()) {
                minmea_sentence_gga reference{};
                require(minmea_parse_gga(&reference, wire.c_str()) && reference.latitude.scale == 0 &&
                            reference.longitude.scale == 0 && !NMEA::gga(*parsed),
                        "missing-coordinate policy must remain distinct from lexical parsing");
                ++missingCoordinates;
            } else {
                compareGga(wire, maximumCoordinateDifference, absentCounts);
                ++ggaCases;
            }
            rejectChecksumFailures(wire);
            checksumRejections += 2;
        }
        require(absentCounts == 2 && missingCoordinates == 1, "documented semantic differences were not exercised");

        const std::array gstBodies{
            "GPGST,123519,1.0,2.0,1.0,45.0,3.0,4.0,5.0",
            "GNGST,000000,0.0,0.0,0.0,0.0,0.0,0.0,0.0",
            "GAGST,235959,1.0,2.0,1.0,45.0,,,",
        };
        for (const auto body : gstBodies) {
            compareGst(sentence(body));
        }
        const auto syntheticGst = fixtureSentences("synthetic-gst.nmea");
        require(syntheticGst.size() == 3, "synthetic GST corpus is incomplete");
        for (const auto& wire : syntheticGst) {
            compareGst(wire);
        }
        const std::array gsvBodies{
            "GPGSV,1,1,02,01,10,020,30,02,15,030,00",
            "GLGSV,1,1,02,65,10,020,30,66,15,030,",
            "GAGSV,1,1,02,301,10,020,30,302,15,030,20",
            "GPGSV,1,1,02,01,10,020,30,02,15,030,,1",
        };
        for (const auto body : gsvBodies) {
            compareGsv(sentence(body));
        }

        const QJsonObject report{
            {"oracle", "minmea"},
            {"revision", MINMEA_REFERENCE_REVISION},
            {"license", "MIT alternative"},
            {"gga_cases", ggaCases},
            {"gst_cases", int(gstBodies.size() + syntheticGst.size())},
            {"gsv_cases", int(gsvBodies.size())},
            {"checksum_rejections", checksumRejections},
            {"maximum_coordinate_difference_degrees", maximumCoordinateDifference},
            {"oracle_numeric_precision", "float32; two scaled float epsilons"},
            {"missing_integer_collapses_to_zero", true},
            {"missing_coordinate_policy_cases", missingCoordinates},
            {"scope", "Complete sentences only; no constellation/signal IDs, epoch assembly or receiver control"},
        };
        std::cout << QJsonDocument(report).toJson(QJsonDocument::Compact).constData() << '\n';
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSMinmeaComparisonTest, TestLabel::Unit)

#include "MinmeaComparison.moc"
