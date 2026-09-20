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
#include "Unicore/GPSDriverUnicore.h"

#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {

// Literal reference vectors: N4 EN R1.6 sections 3.1 and 7.3.1, and the captured ACK at
// https://s-taka.org/control-command-for-gnss-receiver-um982/ . Other packets are synthetic.
constexpr std::string_view VERSION =
    "#VERSIONA,79,GPS,FINE,2326,378237000,15434,0,18,889;"
    "\"UM982\",\"R4.10Build15434\",\"HRPT00-S10C-P\","
    "\"2310415000012-LR23A2225208904\",\"ff2740966a10124c\",\"2024/08/08\"*769fd54f\r\n";
constexpr std::string_view MODE_ROVER = "#MODE,81,GPS,FINE,2230,547967000,0,0,18,518;MODE ROVER SURVEY,*1B\r\n";
constexpr std::string_view ACK_UNLOG = "$command,unlog,response: OK*21\r\n";

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

struct Receiver
{
    enum class Fault
    {
        None,
        Silence,
        Reject,
        WrongAck,
        Corrupt,
        Cancel,
        WriteError,
        ShortWrite,
    };

    Fault fault = Fault::None;
    std::string faultCommand;
    std::string version{VERSION};
    std::string role = "MODE ROVER SURVEY";
    std::string positionType = "SINGLE";
    std::array<double, 3> coordinates{-2160489.0276, 4383620.1006, 4084738.1110};
    std::vector<std::string> commands;
    std::vector<GPSCommandResult> results;
    std::vector<GPSNativeSurveyReport> surveys;
    size_t rtcmCount = 0;
    size_t calls = 0;
    size_t chunk = 7;
    bool modeMismatch = false;
    bool positionMismatch = false;
    bool omitModeReadback = false;
    bool omitPositionReadback = false;
    bool readError = false;
    bool cancel = false;
    std::string queued;
    unsigned availableBaud = 115200;
    unsigned hostBaud = 0;

    bool sent(std::string_view prefix) const
    {
        return std::any_of(commands.begin(), commands.end(),
                           [&](const auto& text) { return text.starts_with(prefix); });
    }

    GPSProtocolIO io()
    {
        auto io = makeGPSProtocolTestIO();
        io.decoded = [this](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* survey = std::get_if<GPSNativeSurveyReport>(&event)) {
                    surveys.push_back(*survey);
                }
                rtcmCount += std::holds_alternative<GPSRTCMReport>(event);
            }
        };
        io.commandFinished = [this](const GPSCommandResult& result) { results.push_back(result); };
        io.setBaudrate = [this](unsigned baud) {
            ++calls;
            hostBaud = baud;
            return GPSBaudStatus::Configured;
        };
        io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
            ++calls;
            if (cancel) {
                return {GPSReadStatus::Cancelled};
            }
            if (readError) {
                return {GPSReadStatus::Error, 0, QStringLiteral("Unicore test disconnect")};
            }
            if (queued.empty()) {
                gps_test_time = deadline.untilUs;
                return {GPSReadStatus::TimedOut};
            }
            const auto count = std::min({queued.size(), bytes.size(), chunk});
            std::memcpy(bytes.data(), queued.data(), count);
            queued.erase(0, count);
            gps_test_time += 100;
            return {GPSReadStatus::Data, static_cast<int>(count)};
        };
        io.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) -> GPSWriteResult {
            ++calls;
            CHECK(deadline.untilUs > gps_test_time);
            const std::string wire(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            CHECK(wire.ends_with("\r\n"));
            const auto command = wire.substr(0, wire.size() - 2);
            commands.push_back(command);
            const int length = static_cast<int>(bytes.size());
            const bool fail = !faultCommand.empty() && command.starts_with(faultCommand);
            if (fail && fault == Fault::WriteError) {
                return {GPSWriteStatus::Error, 0, 0, QStringLiteral("Unicore test write failure")};
            }
            if (fail && fault == Fault::ShortWrite) {
                return {GPSWriteStatus::Completed, length - 1, length - 1};
            }
            if (fail && fault == Fault::Cancel) {
                cancel = true;
            }
            if ((fail && (fault == Fault::Silence || fault == Fault::Cancel)) || hostBaud != availableBaud) {
                return {GPSWriteStatus::Completed, length, length};
            }
            std::string ack = checked("$command," + command + ",response: OK", false);
            if (fail && fault == Fault::Reject) {
                queued += checked("$command," + command + ",response: PARSING FAILD NO MATCHING FUNC", false);
            } else if (fail && fault == Fault::WrongAck) {
                queued += checked("$command," + command + " OTHER,response: OK", false);
            } else if (fail && fault == Fault::Corrupt) {
                ack[ack.find('*') + 1] = 'Z';
                queued += ack;
            } else if (command == "VERSIONA") {
                queued += ack + version;
            } else if (command == "UNLOG") {
                queued += ACK_UNLOG;
            } else if (command == "MODE") {
                queued += ack;
                if (!omitModeReadback) {
                    queued += modeMismatch                  ? native("MODE", "MODE HEADING2,")
                              : role == "MODE ROVER SURVEY" ? std::string(MODE_ROVER)
                                                            : native("MODE", role + ',');
                }
            } else if (command.starts_with("MODE ")) {
                if (command == "MODE ROVER") {
                    role = "MODE ROVER SURVEY";
                    positionType = "SINGLE";
                } else if (command.starts_with("MODE BASE TIME ")) {
                    role = "MODE BASE TIME";
                    positionType = "SINGLE";
                } else {
                    role = "MODE BASE";
                    positionType = "FIXEDPOS";
                    std::istringstream fields(command.substr(10));
                    fields.imbue(std::locale::classic());
                    fields >> coordinates[0] >> coordinates[1] >> coordinates[2];
                    CHECK(!fields.fail());
                }
                queued += ack;
            } else if (command.starts_with("BESTNAVXYZA")) {
                auto result = coordinates;
                result[0] += positionMismatch ? 10 : 0;
                queued += ack;
                if (command != "BESTNAVXYZA" || !omitPositionReadback) {
                    queued += position(positionType, result);
                }
            } else {
                queued += ack;
            }
            return {GPSWriteStatus::Completed, length, length};
        };
        return io;
    }
};

GPSProtocol::GPSConfig baseConfig(bool fixed)
{
    GPSProtocol::GPSConfig config{};
    config.output_mode = GPSProtocol::OutputMode::RTCM;
    config.base.useFixedBase = fixed;
    config.base.surveyMode = fixed ? GPSBaseStationConfig::SurveyMode::AccuracyControlled
                                   : GPSBaseStationConfig::SurveyMode::ReceiverManaged;
    config.base.receiverAveragingDurationSecs = 60;
    config.base.fixedBaseLatitude = 47;
    config.base.fixedBaseLongitude = 8;
    config.base.fixedBaseAltitudeMeters = 500;
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
            CHECK(receiver.results.back().outcome == GPSCommandOutcome::Acknowledged);
            CHECK(std::any_of(receiver.results.begin(), receiver.results.end(), [](const auto& result) {
                return result.command == "MODE" && result.outcome == GPSCommandOutcome::ReadbackVerified;
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
        return result.command == "BESTNAVXYZA" && result.outcome == GPSCommandOutcome::ReadbackVerified;
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
    consume(driver, position("SINGLE", receiver.coordinates));
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
            CHECK(receiver.results.back().outcome == expected);
            CHECK(receiver.results.back().required);
            if (fault == Receiver::Fault::Cancel) {
                CHECK(driver.ioError() == GPSProtocol::ReadCancelled);
                CHECK(gps_test_warnings.empty());
            }
            if (fault == Receiver::Fault::WriteError) {
                CHECK(driver.ioErrorDetail() == QStringLiteral("Unicore test write failure"));
            }
            if (fault == Receiver::Fault::ShortWrite) {
                CHECK(receiver.results.back().writtenBytes > 0);
                CHECK(receiver.results.back().acceptedBytes == receiver.results.back().writtenBytes);
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
        CHECK(receiver.results.back().outcome ==
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
        consume(driver, position("FIXEDPOS", receiver.coordinates));
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

}  // namespace

int main()
{
    try {
        identityAndRole();
        rejectBeforeMutation();
        fixedBaseAndTransition();
        averagingEvidenceAndRestart();
        commandFailures();
        readbackFailures();
        corruptStatusAndExpiry();
        restartAndReadErrors();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
    return 0;
}
