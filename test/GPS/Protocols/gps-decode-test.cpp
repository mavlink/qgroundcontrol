#include <array>
#include <stdexcept>
#include <string>

#include "GPSCommandText.h"
#include "GPSNMEAReport.h"
#include "NMEASentence.h"
#include "UnitTest.h"

#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
void nmeaFixQualities()
{
    struct Case
    {
        unsigned quality;
        GPSPositionReport::FixType expected;
    };

    constexpr std::array cases{
        Case{NMEA::GgaQuality::INVALID, GPSPositionReport::FixType::NoFix},
        Case{NMEA::GgaQuality::GPS, GPSPositionReport::FixType::Fix3D},
        Case{NMEA::GgaQuality::DIFFERENTIAL, GPSPositionReport::FixType::Differential},
        Case{NMEA::GgaQuality::RTK_FIXED, GPSPositionReport::FixType::RTKFixed},
        Case{NMEA::GgaQuality::RTK_FLOAT, GPSPositionReport::FixType::RTKFloat},
        Case{NMEA::GgaQuality::ESTIMATED, GPSPositionReport::FixType::Extrapolated},
        Case{99, GPSPositionReport::FixType::Unknown},
    };
    GPSDecodedPosition report;
    CHECK(report.navigation.fixType == GPSPositionReport::FixType::Unknown);
    for (const auto& test : cases) {
        NMEA::GGA fix;
        fix.quality = test.quality;
        applyNMEAGGA(report, fix, 123);
        CHECK(report.navigation.fixType == test.expected);
        CHECK(report.navigation.timestampUs == 123);
    }
}

/// Command text keeps printf's "%.Nf" bytes: exact binary ties round to even, and negative zero keeps its sign.
void fixedDecimals()
{
    CHECK(gpsFixedDecimal(0.125, 2) == "0.12");
    CHECK(gpsFixedDecimal(0.375, 2) == "0.38");
    CHECK(gpsFixedDecimal(2.5, 0) == "2");
    CHECK(gpsFixedDecimal(500.03125, 4) == "500.0312");
    CHECK(gpsFixedDecimal(-0.0, 4) == "-0.0000");
    CHECK(gpsFixedDecimal(-0.00001, 4) == "-0.0000");
    CHECK(gpsFixedDecimal(4315616.67264, 4) == "4315616.6726");
    CHECK(gpsFixedDecimal(47.001953125, 8) == "47.00195312");
    CHECK(gpsFixedDecimal(15, 9) == "15.000000000");
}

}  // namespace

class GPSProtocolDecodeTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
};

void GPSProtocolDecodeTest::_protocol()
{
    try {
        nmeaFixQualities();
        fixedDecimals();
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolDecodeTest, TestLabel::Unit)

#include "gps-decode-test.moc"
