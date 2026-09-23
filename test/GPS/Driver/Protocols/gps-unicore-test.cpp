#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "GPSProtocolTestIO.h"
#include "RTCMFramer.h"
#include "Support/UnicoreReceiverModel.h"
#include "Unicore/GPSDriverUnicore.h"
#include "UnitTest.h"

#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {

// Literal reference vector: N4 EN R1.6 section 3.1. Other packets are synthetic.
constexpr std::string_view VERSION =
    "#VERSIONA,79,GPS,FINE,2326,378237000,15434,0,18,889;"
    "\"UM982\",\"R4.10Build15434\",\"HRPT00-S10C-P\","
    "\"2310415000012-LR23A2225208904\",\"ff2740966a10124c\",\"2024/08/08\"*769fd54f\r\n";

std::string checked(std::string body, bool crc)
{
    uint32_t value = 0;
    if (crc) {
        // Table implementation, independent from the production bitwise decoder.
        std::array<uint32_t, 256> table{};
        for (size_t i = 0; i < table.size(); ++i) {
            uint32_t entry = static_cast<uint32_t>(i);
            for (unsigned bit = 0; bit < 8; ++bit) {
                entry = entry & 1 ? (entry >> 1) ^ 0xedb88320U : entry >> 1;
            }
            table[i] = entry;
        }
        for (size_t i = 1; i < body.size(); ++i) {
            value = table[(value ^ static_cast<unsigned char>(body[i])) & 0xff] ^ (value >> 8);
        }
    } else {
        for (const unsigned char byte : body) {
            value ^= byte;
        }
    }
    char suffix[16]{};
    std::snprintf(suffix, sizeof(suffix), crc ? "*%08x\r\n" : "*%02x\r\n", value);
    return body + suffix;
}

std::string native(std::string_view name, std::string_view body, uint32_t milliseconds = 378238000)
{
    return checked("#" + std::string(name) + ",79,GPS,FINE,2326," + std::to_string(milliseconds) + ",15434,0,18,0;" +
                       std::string(body),
                   name != "MODE");
}

std::string position(std::string_view type, const std::array<double, 3>& coordinates, uint32_t milliseconds = 378238000)
{
    std::ostringstream body;
    body.imbue(std::locale::classic());
    body << std::fixed << std::setprecision(4) << "SOL_COMPUTED," << type;
    for (const double coordinate : coordinates) {
        body << ',' << coordinate;
    }
    body << ",1,2,3,SOL_COMPUTED,DOPPLER_VELOCITY,0,0,0,0,0,0,\"\",0,0,0,47,28,28,0,0,12,0,09";
    return native("BESTNAVXYZA", body.str(), milliseconds);
}

std::vector<uint8_t> correction(std::string_view payload = "\x43\x20")
{
    std::vector<uint8_t> bytes{0xd3, static_cast<uint8_t>(payload.size() >> 8), static_cast<uint8_t>(payload.size())};
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    const auto crc = RTCMFramer::crc24q(bytes);
    bytes.push_back(crc >> 16);
    bytes.push_back(crc >> 8);
    bytes.push_back(crc);
    return bytes;
}

void consume(GPSNativeUnicore& driver, std::string_view line, size_t chunk = 5)
{
    while (!line.empty()) {
        const auto count = std::min(chunk, line.size());
        driver.consume({reinterpret_cast<const uint8_t*>(line.data()), count});
        line.remove_prefix(count);
    }
}

using Receiver = GPSTest::UnicoreReceiver;

GPSProtocol::GPSConfig baseConfig(bool fixed)
{
    GPSProtocol::GPSConfig config{};
    config.output_mode = GPSProtocol::OutputMode::RTCM;
    config.base.useFixedBase = fixed;
    config.base.surveyMode = fixed ? GPSBaseStationConfig::SurveyMode::AccuracyControlled
                                   : GPSBaseStationConfig::SurveyMode::ReceiverManaged;
    config.base.receiverAveragingDurationSecs = 60;
    config.base.fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500};
    return config;
}

void resetClock()
{
    gps_test_time = 0;
    gps_test_warnings.clear();
}

void identityAndRole()
{
    for (const auto* model : {"UM980", "UM982"}) {
        for (const unsigned baud : {0U, 115200U}) {
            resetClock();
            Receiver receiver;
            receiver.chunk = 1;
            if (std::string_view(model) == "UM980") {
                receiver.version =
                    native("VERSIONA", "\"UM980\",\"R4.10Build15434\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"");
            }
            receiver.role = "MODE BASE TIME";
            if (baud == 0) {
                receiver.availableBaud = 460800;
            }
            GPSNativePositionReport positionReport;
            GPSNativeUnicore driver(receiver.io(), &positionReport);
            unsigned rate = baud;
            CHECK(driver.configure(rate, {}) == 0);
            CHECK(driver.receiverReady());
            CHECK(rate == receiver.availableBaud);
            CHECK(driver.model() == model);
            CHECK(driver.firmware() == "R4.10Build15434");
            const auto mutation = std::find(receiver.commands.begin(), receiver.commands.end(), "UNLOG");
            CHECK(mutation != receiver.commands.end());
            CHECK(std::all_of(receiver.commands.begin(), mutation,
                              [](const auto& command) { return command == "VERSIONA"; }));
            CHECK(receiver.sent("MODE ROVER"));
            CHECK(receiver.role == "MODE ROVER SURVEY");
            CHECK(receiver.sent("GPGGA 1") && receiver.sent("GPGST 1") && receiver.sent("GPGSV 1"));
            CHECK(!receiver.sent("SAVECONFIG") && !receiver.sent("FRESET") && !receiver.sent("RTCM"));
            CHECK(receiver.results.back().evidence.outcome == GPSCommandOutcome::Acknowledged);
            CHECK(std::any_of(receiver.results.begin(), receiver.results.end(), [](const auto& result) {
                return result.evidence.command == "MODE" &&
                       result.evidence.outcome == GPSCommandOutcome::ReadbackVerified;
            }));
            driver.consume(correction());
            CHECK(receiver.rtcmCount == 0);
        }
    }
}

void rejectBeforeMutation()
{
    for (const auto* body : {
             "\"UM960\",\"R4.10Build15434\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM982 PRO\",\"R4.10Build15434\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM980\",\"R4.10Build7922\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM982\",\"R4.10Build7649\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM982\",\"R5.00Build20000\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM982\",\"\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
         }) {
        resetClock();
        Receiver receiver;
        receiver.version = native("VERSIONA", body);
        GPSNativeUnicore driver(receiver.io(), nullptr);
        unsigned rate = 115200;
        CHECK(driver.configure(rate, {}) < 0);
        CHECK(!driver.receiverReady());
        CHECK(receiver.commands == std::vector<std::string>{"VERSIONA"});
    }
    for (unsigned variant = 0; variant < 5; ++variant) {
        resetClock();
        Receiver receiver;
        GPSNativeUnicore driver(receiver.io(), nullptr);
        auto config = baseConfig(false);
        if (variant == 0) {
            config.base.surveyMode = GPSBaseStationConfig::SurveyMode::AccuracyControlled;
            config.base.surveyInAccMeters = 1;
            config.base.surveyInDurationSecs = 60;
        } else if (variant == 1) {
            config.base.receiverAveragingDurationSecs = 3601;
        } else if (variant == 2) {
            config.base.receiverAveragingDurationSecs = 0;
        } else if (variant == 3) {
            config.dynamicModel = 1;
        } else {
            config.gnss_systems = GPSProtocol::GNSSSystemsMask::ENABLE_GPS;
        }
        unsigned rate = 115200;
        CHECK(driver.configure(rate, config) < 0);
        CHECK(receiver.calls == 0);
        CHECK(!driver.receiverReady());
    }
}

void fixedBaseAndTransition()
{
    resetClock();
    Receiver receiver;
    GPSNativeUnicore driver(receiver.io(), nullptr);
    unsigned rate = 115200;
    CHECK(driver.configure(rate, baseConfig(true)) == 0);
    CHECK(driver.receiverReady());
    CHECK(receiver.sent("MODE BASE 4315616."));
    CHECK(receiver.sent("BESTNAVXYZA") && receiver.sent("RTCM1005 1") && receiver.sent("RTCM1124 1"));
    CHECK(!receiver.surveys.empty());
    CHECK(receiver.surveys.back().flags == 1);
    CHECK(!receiver.surveys.back().accuracyKnown);
    CHECK(receiver.surveys.back().duration == 0);
    CHECK(receiver.surveys.back().altitudeDatum == GPSNativeSurveyReport::AltitudeDatum::Ellipsoid);
    CHECK(std::abs(receiver.surveys.back().latitude - 47) < 1e-7);
    CHECK(std::abs(receiver.surveys.back().longitude - 8) < 1e-7);
    CHECK(std::abs(receiver.surveys.back().altitude - 500) < 0.01);
    CHECK(std::any_of(receiver.results.begin(), receiver.results.end(), [](const auto& result) {
        return result.evidence.command == "BESTNAVXYZA" &&
               result.evidence.outcome == GPSCommandOutcome::ReadbackVerified;
    }));
    const auto calls = receiver.calls;
    const auto frame = correction();
    for (const auto byte : frame) {
        driver.consume({&byte, 1});
    }
    CHECK(receiver.rtcmCount == 1);
    CHECK(receiver.calls == calls);
    auto corrupt = frame;
    corrupt.back() ^= 1;
    driver.consume(corrupt);
    CHECK(receiver.rtcmCount == 1);
    CHECK(driver.configure(rate, {}) == 0);
    CHECK(driver.receiverReady() && receiver.role == "MODE ROVER SURVEY");
    driver.consume(frame);
    CHECK(receiver.rtcmCount == 1);
}

void averagingEvidenceAndRestart()
{
    resetClock();
    Receiver receiver;
    GPSNativeUnicore driver(receiver.io(), nullptr);
    unsigned rate = 115200;
    CHECK(driver.configure(rate, baseConfig(false)) == 0);
    CHECK(receiver.sent("MODE BASE TIME 60 0"));
    CHECK(receiver.surveys.back().flags == 2);
    CHECK(std::isnan(receiver.surveys.back().latitude));
    CHECK(std::isnan(receiver.surveys.back().longitude));
    CHECK(std::isnan(receiver.surveys.back().altitude));
    CHECK(!receiver.surveys.back().accuracyKnown && receiver.surveys.back().duration == 0);
    gps_test_time += 7200000000ULL;
    driver.consume(correction());
    CHECK(receiver.rtcmCount == 0);
    CHECK(receiver.surveys.back().flags == 2);
    driver.consume(correction(position("FIXEDPOS", receiver.coordinates)));
    CHECK(receiver.rtcmCount == 0);
    CHECK(receiver.surveys.back().flags == 2);

    // BASEPOS is a real-time monitoring solution, not evidence of a frozen average.
    consume(driver, native("BASEPOSA", "SOL_COMPUTED,SINGLE,47,8,450,50,WGS84,0.001,0.001,0.001"));
    driver.consume(correction());
    CHECK(receiver.rtcmCount == 0);
    const auto calls = receiver.calls;
    consume(driver, position("FIXEDPOS", receiver.coordinates));
    CHECK(receiver.calls == calls);
    CHECK(receiver.surveys.back().flags == 1);
    CHECK(!receiver.surveys.back().accuracyKnown && receiver.surveys.back().duration == 0);
    driver.consume(correction());
    CHECK(receiver.rtcmCount == 1);

    // Loss of the receiver's fixed solution cannot silently resume with a previous average.
    consume(driver, position("SINGLE", receiver.coordinates, 378239000));
    CHECK(!driver.receiverReady());
    CHECK(receiver.surveys.back().flags == 0);
    CHECK(std::isnan(receiver.surveys.back().latitude));
    CHECK(std::isnan(receiver.surveys.back().longitude));
    CHECK(std::isnan(receiver.surveys.back().altitude));
    consume(driver, position("FIXEDPOS", receiver.coordinates));
    driver.consume(correction());
    CHECK(receiver.rtcmCount == 1);
    CHECK(driver.configure(rate, baseConfig(false)) == 0);
    CHECK(receiver.surveys.back().flags == 2);
    CHECK(std::count(receiver.commands.begin(), receiver.commands.end(), "MODE BASE TIME 60 0") == 2);
    driver.consume(correction());
    CHECK(receiver.rtcmCount == 1);
    consume(driver, position("FIXEDPOS", receiver.coordinates));
    driver.consume(correction());
    CHECK(receiver.rtcmCount == 2);
}

void commandFailures()
{
    for (const auto* command : {"VERSIONA", "UNLOG", "MODE ROVER", "MODE BASE TIME", "BESTNAVXYZA 1", "RTCM1074 1"}) {
        for (const auto fault :
             {Receiver::Fault::Silence, Receiver::Fault::Reject, Receiver::Fault::WrongAck, Receiver::Fault::Corrupt,
              Receiver::Fault::Cancel, Receiver::Fault::WriteError, Receiver::Fault::ShortWrite}) {
            resetClock();
            Receiver receiver;
            receiver.fault = fault;
            receiver.faultCommand = command;
            GPSNativeUnicore driver(receiver.io(), nullptr);
            unsigned rate = 115200;
            CHECK(driver.configure(rate, baseConfig(false)) < 0);
            CHECK(!driver.receiverReady());
            CHECK(receiver.commands.back().starts_with(command));
            CHECK(!receiver.results.empty());
            const auto expected = fault == Receiver::Fault::Reject   ? GPSCommandOutcome::Rejected
                                  : fault == Receiver::Fault::Cancel ? GPSCommandOutcome::Cancelled
                                  : fault == Receiver::Fault::WriteError || fault == Receiver::Fault::ShortWrite
                                      ? GPSCommandOutcome::TransportError
                                      : GPSCommandOutcome::TimedOut;
            CHECK(receiver.results.back().evidence.outcome == expected);
            CHECK(receiver.results.back().evidence.required);
            if (fault == Receiver::Fault::Cancel) {
                CHECK(driver.ioError() == GPSProtocol::ReadCancelled);
                CHECK(gps_test_warnings.empty());
            }
            if (fault == Receiver::Fault::WriteError) {
                CHECK(driver.ioErrorDetail() == QStringLiteral("Unicore test write failure"));
            }
            if (fault == Receiver::Fault::ShortWrite) {
                CHECK(receiver.results.back().evidence.writtenBytes > 0);
                CHECK(receiver.results.back().evidence.acceptedBytes == receiver.results.back().evidence.writtenBytes);
            }
            CHECK(gps_test_time < 45000000);
            driver.consume(correction());
            CHECK(receiver.rtcmCount == 0);
        }
    }
}

void readbackFailures()
{
    for (unsigned variant = 0; variant < 6; ++variant) {
        resetClock();
        Receiver receiver;
        receiver.modeMismatch = variant == 0;
        receiver.positionMismatch = variant == 1;
        if (variant == 2) {
            receiver.version[receiver.version.find("*") + 1] = 'Z';
        } else if (variant == 3) {
            receiver.version.clear();
        } else if (variant == 4) {
            receiver.omitPositionReadback = true;
        } else if (variant == 5) {
            receiver.omitModeReadback = true;
        }
        GPSNativeUnicore driver(receiver.io(), nullptr);
        unsigned rate = 115200;
        CHECK(driver.configure(rate, baseConfig(true)) < 0);
        CHECK(!driver.receiverReady());
        CHECK(receiver.results.back().evidence.outcome ==
              (variant < 2 ? GPSCommandOutcome::Rejected : GPSCommandOutcome::TimedOut));
        CHECK(!receiver.sent("RTCM"));
        if (variant == 4) {
            CHECK(receiver.commands.back() == "BESTNAVXYZA");
        } else if (variant == 5) {
            CHECK(receiver.commands.back() == "MODE");
        }
    }
}

void corruptStatusAndExpiry()
{
    for (unsigned variant = 0; variant < 6; ++variant) {
        resetClock();
        Receiver receiver;
        GPSNativeUnicore driver(receiver.io(), nullptr);
        unsigned rate = 115200;
        CHECK(driver.configure(rate, baseConfig(false)) == 0);
        std::string data = position("FIXEDPOS", receiver.coordinates);
        if (variant == 0) {
            data[data.find('*') + 1] = 'Z';
        } else if (variant == 1) {
            data[data.find("FIXEDPOS")] = 'X';
        } else if (variant == 2) {
            data = native("BESTNAVXYZA", "SOL_COMPUTED,FIXEDPOS,nan,1,2");
        } else if (variant == 3) {
            data = '#' + std::string(5000, 'A') + "\r\n";
        } else if (variant == 4) {
            data = native("BESTNAVXYZA", "SOL_COMPUTED,FIXEDPOS,6378137,0,0");
        } else {
            data = position("FIXEDPOS", receiver.coordinates, 604800000);
        }
        const auto calls = receiver.calls;
        consume(driver, data);
        driver.consume(correction());
        CHECK(receiver.calls == calls);
        CHECK(receiver.rtcmCount == 0);
        CHECK(receiver.surveys.back().flags == 2);
        consume(driver, position("FIXEDPOS", receiver.coordinates, 378239000));
        driver.consume(correction());
        CHECK(receiver.rtcmCount == 1);
        gps_test_time += 5000001;
        driver.consume(correction());
        CHECK(receiver.rtcmCount == 1);
        CHECK(!driver.receiverReady());
        CHECK(receiver.surveys.back().flags == 0);
    }
}

void restartAndReadErrors()
{
    for (unsigned variant = 0; variant < 4; ++variant) {
        resetClock();
        Receiver receiver;
        GPSNativeUnicore driver(receiver.io(), nullptr);
        unsigned rate = 115200;
        CHECK(driver.configure(rate, baseConfig(true)) == 0);
        if (variant == 0) {
            consume(driver, VERSION);
        } else if (variant == 1) {
            consume(driver, native("MODE", "MODE ROVER SURVEY,"));
        } else if (variant == 2) {
            consume(driver, position("FIXEDPOS", receiver.coordinates, 1000));
        } else {
            receiver.readError = true;
            CHECK(driver.receive(100) < 0);
            CHECK(driver.ioErrorDetail() == QStringLiteral("Unicore test disconnect"));
        }
        CHECK(!driver.receiverReady());
        driver.consume(correction());
        CHECK(receiver.rtcmCount == 0);
        CHECK(receiver.surveys.back().flags == 0);
    }
}

void measurementFreshnessAndRollover()
{
    for (const bool rollover : {false, true}) {
        resetClock();
        Receiver receiver;
        GPSNativeUnicore driver(receiver.io(), nullptr);
        unsigned baud = 115200;
        CHECK(driver.configure(baud, baseConfig(false)) == 0);
        const auto complete = GPSTest::unicorePosition("FIXEDPOS", receiver.coordinates, 604799000, 2326);
        consume(driver, complete, 1);
        CHECK(receiver.surveys.back().flags == 1);
        CHECK(!receiver.surveys.back().accuracyKnown);
        CHECK(receiver.surveys.back().duration == 0);
        const auto reports = receiver.surveys.size();
        const auto receipt = receiver.surveys.back().timestamp;
        gps_test_time += 4000000;
        consume(driver, complete);
        CHECK(receiver.surveys.size() == reports);
        CHECK(receiver.surveys.back().timestamp == receipt);
        if (rollover) {
            consume(driver, GPSTest::unicorePosition("FIXEDPOS", receiver.coordinates, 0, 2327));
            CHECK(driver.receiverReady());
            CHECK(receiver.surveys.back().flags == 1);
            CHECK(receiver.surveys.back().timestamp > receipt);
            driver.consume(correction());
            CHECK(receiver.rtcmCount == 1);
            consume(driver, complete);  // Previous week cannot revive/replace the new week's evidence.
        } else {
            gps_test_time += 1000001;
            driver.consume({});
        }
        CHECK(!driver.receiverReady());
        CHECK(receiver.surveys.back().flags == 0);
        CHECK(!receiver.surveys.back().accuracyKnown);
        CHECK(receiver.surveys.back().duration == 0);
        CHECK(std::isnan(receiver.surveys.back().latitude));
        CHECK(std::isnan(receiver.surveys.back().longitude));
        CHECK(std::isnan(receiver.surveys.back().altitude));
        consume(driver, complete);
        const auto count = receiver.rtcmCount;
        driver.consume(correction());
        CHECK(receiver.rtcmCount == count);
    }
}

void scheduledAveragingAndBoot()
{
    for (const size_t chunk : {size_t(1), size_t(150)}) {
        resetClock();
        Receiver receiver;
        receiver.chunk = chunk;
        receiver.initialTow = 604798000;
        GPSNativeUnicore driver(receiver.io(), nullptr);
        auto config = baseConfig(false);
        config.base.receiverAveragingDurationSecs = 1;
        unsigned baud = 115200;
        CHECK(driver.configure(baud, config) == 0);
        const auto commands = receiver.commands.size();
        for (unsigned slices = 0; gps_test_time < 2500000; ++slices) {
            CHECK(slices < 100);
            driver.receive(1000);
            CHECK(driver.receiverReady());
        }
        CHECK(receiver.surveys.back().flags == 1);
        CHECK(receiver.surveys.back().duration == 0);
        CHECK(!receiver.surveys.back().accuracyKnown);
        CHECK(receiver.commands.size() == commands);
        driver.consume(correction());
        CHECK(receiver.rtcmCount == 1);
        receiver.events.schedule(500000, [&receiver] { receiver.boot(); });
        for (unsigned slices = 0; driver.receiverReady(); ++slices) {
            CHECK(slices < 100);
            driver.receive(1000);
        }
        CHECK(receiver.surveys.back().flags == 0);
        CHECK(std::isnan(receiver.surveys.back().altitude));
        driver.consume(correction());
        CHECK(receiver.rtcmCount == 1);
    }
}

}  // namespace

class GPSProtocolUnicoreTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
};

void GPSProtocolUnicoreTest::_protocol()
{
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        identityAndRole();
        rejectBeforeMutation();
        fixedBaseAndTransition();
        averagingEvidenceAndRestart();
        commandFailures();
        readbackFailures();
        corruptStatusAndExpiry();
        restartAndReadErrors();
        measurementFreshnessAndRollover();
        scheduledAveragingAndBoot();
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolUnicoreTest, TestLabel::Unit)

#include "gps-unicore-test.moc"
