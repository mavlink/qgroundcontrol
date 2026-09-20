#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "GPSProtocolTestIO.h"
#include "ReceiverEventQueue.h"

namespace GPSTest {

// Literal public vectors: N4 EN R1.6 §§3.1/7.3.1 and s-taka.org's published UM982 command capture.
inline constexpr std::string_view UNICORE_VERSION =
    "#VERSIONA,79,GPS,FINE,2326,378237000,15434,0,18,889;"
    "\"UM982\",\"R4.10Build15434\",\"HRPT00-S10C-P\","
    "\"2310415000012-LR23A2225208904\",\"ff2740966a10124c\",\"2024/08/08\"*769fd54f\r\n";
inline constexpr std::string_view UNICORE_MODE_ROVER =
    "#MODE,81,GPS,FINE,2230,547967000,0,0,18,518;MODE ROVER SURVEY,*1B\r\n";

inline std::string unicoreChecked(std::string body, bool crc)
{
    static const auto table = [] {
        std::array<uint32_t, 256> values{};
        for (size_t index = 0; index < values.size(); ++index) {
            uint32_t value = static_cast<uint32_t>(index);
            for (unsigned bit = 0; bit < 8; ++bit) {
                value = value & 1 ? (value >> 1) ^ 0xedb88320U : value >> 1;
            }
            values[index] = value;
        }
        return values;
    }();
    uint32_t checksum = 0;
    for (size_t index = crc ? 1 : 0; index < body.size(); ++index) {
        const auto byte = static_cast<unsigned char>(body[index]);
        checksum = crc ? table[(checksum ^ byte) & 0xff] ^ (checksum >> 8) : checksum ^ byte;
    }
    std::array<char, 16> tail{};
    std::snprintf(tail.data(), tail.size(), crc ? "*%08x\r\n" : "*%02x\r\n", checksum);
    return body + tail.data();
}

inline std::string unicoreNative(std::string_view name, std::string_view body, uint32_t tow = 378238000,
                                 unsigned week = 2326)
{
    return unicoreChecked("#" + std::string(name) + ",79,GPS,FINE," + std::to_string(week) + ',' + std::to_string(tow) +
                              ",15434,0,18,0;" + std::string(body),
                          name != "MODE");
}

inline std::string unicorePosition(std::string_view type, const std::array<double, 3>& coordinates,
                                   uint32_t tow = 378238000, unsigned week = 2326)
{
    std::ostringstream body;
    body.imbue(std::locale::classic());
    body << std::fixed << std::setprecision(4) << "SOL_COMPUTED," << type;
    for (const auto coordinate : coordinates) {
        body << ',' << coordinate;
    }
    body << ",1,2,3,SOL_COMPUTED,DOPPLER_VELOCITY,0,0,0,0,0,0,\"\",0,0,0,47,28,28,0,0,12,0,09";
    return unicoreNative("BESTNAVXYZA", body.str(), tow, week);
}

struct UnicoreReceiver
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
    std::string version{UNICORE_VERSION};
    std::string role = "MODE ROVER SURVEY";
    std::string positionType = "SINGLE";
    std::array<double, 3> coordinates{-2160489.0276, 4383620.1006, 4084738.1110};
    std::vector<std::string> commands;
    std::vector<GPSCommandResult> results;
    std::vector<GPSNativeSurveyReport> surveys;
    size_t rtcmCount = 0;
    size_t calls = 0;
    size_t chunk = 7;
    uint64_t responseDelayUs = 0;
    uint64_t startedUs = 0;
    unsigned initialTow = 378237000;
    unsigned initialWeek = 2326;
    unsigned modeGeneration = 0;
    bool periodicStatus = true;
    bool statusEnabled = false;
    bool allowAveragingCompletion = true;
    bool modeMismatch = false;
    bool positionMismatch = false;
    bool omitModeReadback = false;
    bool omitPositionReadback = false;
    bool readError = false;
    bool cancel = false;
    std::string queued;
    unsigned availableBaud = 115200;
    unsigned hostBaud = 0;
    ReceiverEventQueue events{gps_test_time};

    bool sent(std::string_view prefix) const
    {
        return std::any_of(commands.begin(), commands.end(),
                           [prefix](const auto& text) { return text.starts_with(prefix); });
    }

    std::string position() const
    {
        auto result = coordinates;
        result[0] += positionMismatch ? 10 : 0;
        const uint64_t elapsed = initialTow + (gps_test_time - startedUs) / 1000;
        return unicorePosition(positionType, result, elapsed % 604800000, initialWeek + elapsed / 604800000);
    }

    void navigationTick(unsigned generation)
    {
        if (generation != modeGeneration || !statusEnabled) {
            return;
        }
        if (periodicStatus) {
            queued += position();
        }
        events.schedule(1000000, [this, generation] { navigationTick(generation); });
    }

    void boot()
    {
        ++modeGeneration;
        statusEnabled = false;
        role = "MODE ROVER SURVEY";
        positionType = "SINGLE";
        queued += version;
    }

    GPSProtocolIO io()
    {
        startedUs = gps_test_time;
        auto io = makeGPSProtocolTestIO();
        io.decoded = [this](const GPSDecodedBatch& batch) {
            if (batch.events.size() > GPSDecodedBatch::MAX_EVENTS) {
                throw std::runtime_error("Unicore decoded batch overflow");
            }
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
        io.wait = [this](std::chrono::microseconds delay) {
            events.advanceTo(gps_test_time + delay.count());
            return !cancel;
        };
        io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
            ++calls;
            events.advanceTo((std::min) (gps_test_time + 100, deadline.untilUs));
            if (cancel) {
                return {GPSReadStatus::Cancelled};
            }
            if (readError) {
                return {GPSReadStatus::Error, 0, QStringLiteral("Unicore test disconnect")};
            }
            while (queued.empty() && gps_test_time < deadline.untilUs) {
                events.advanceToNext(deadline.untilUs);
            }
            if (queued.empty()) {
                return {GPSReadStatus::TimedOut};
            }
            const auto count = (std::min) ({queued.size(), bytes.size(), chunk});
            std::memcpy(bytes.data(), queued.data(), count);
            queued.erase(0, count);
            return {GPSReadStatus::Data, static_cast<int>(count)};
        };
        io.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) -> GPSWriteResult {
            ++calls;
            const std::string wire(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            if (!wire.ends_with("\r\n") || deadline.untilUs <= gps_test_time) {
                throw std::runtime_error("Invalid Unicore command framing/deadline");
            }
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
            const auto ack = unicoreChecked("$command," + command + ",response: OK", false);
            std::string reply;
            if (fail && fault == Fault::Reject) {
                reply = unicoreChecked("$command," + command + ",response: PARSING FAILD NO MATCHING FUNC", false);
            } else if (fail && fault == Fault::WrongAck) {
                reply = unicoreChecked("$command," + command + " OTHER,response: OK", false);
            } else if (fail && fault == Fault::Corrupt) {
                reply = ack;
                reply[reply.find('*') + 1] = 'Z';
            } else if (command == "VERSIONA") {
                reply = ack + version;
            } else if (command == "UNLOG") {
                statusEnabled = false;
                reply = "$command,unlog,response: OK*21\r\n";
            } else if (command == "MODE") {
                reply = ack;
                if (!omitModeReadback) {
                    reply += modeMismatch                  ? unicoreNative("MODE", "MODE HEADING2,")
                             : role == "MODE ROVER SURVEY" ? std::string(UNICORE_MODE_ROVER)
                                                           : unicoreNative("MODE", role + ',');
                }
            } else if (command.starts_with("MODE ")) {
                const auto generation = ++modeGeneration;
                if (command == "MODE ROVER") {
                    role = "MODE ROVER SURVEY";
                    positionType = "SINGLE";
                } else if (command.starts_with("MODE BASE TIME ")) {
                    role = "MODE BASE TIME";
                    positionType = "SINGLE";
                    const auto duration = std::stoul(command.substr(15));
                    events.schedule(duration * 1000000ULL, [this, generation] {
                        if (generation == modeGeneration && allowAveragingCompletion) {
                            positionType = "FIXEDPOS";
                        }
                    });
                } else {
                    role = "MODE BASE";
                    positionType = "FIXEDPOS";
                    std::istringstream values(command.substr(10));
                    values.imbue(std::locale::classic());
                    values >> coordinates[0] >> coordinates[1] >> coordinates[2];
                    if (values.fail()) {
                        throw std::runtime_error("Malformed fixed-base command");
                    }
                }
                reply = ack;
            } else if (command == "BESTNAVXYZA 1") {
                statusEnabled = true;
                const auto generation = modeGeneration;
                events.schedule(1000000, [this, generation] { navigationTick(generation); });
                reply = ack;
            } else if (command == "BESTNAVXYZA") {
                reply = ack + (omitPositionReadback ? "" : position());
            } else {
                reply = ack;
            }
            events.schedule(responseDelayUs, [this, reply] { queued += reply; });
            return {GPSWriteStatus::Completed, length, length};
        };
        return io;
    }
};

}  // namespace GPSTest
