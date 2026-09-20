#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "GPSProtocolTestIO.h"
#include "ReceiverEventQueue.h"

namespace GPSTest {

// Literal public examples: LG290P Protocol V1.1 §§2.3.1/9/23 and Base Application Note V1.1 §3.2.2.
inline constexpr std::string_view QUECTEL_IDENTITY = "$PQTMVERNO,LG290P03AANR01A03S,2024/04/30,10:53:07*18\r\n";
inline constexpr std::string_view QUECTEL_BOOT = "$PQTMVER,1,MODULE,LG290P03AANR01A03S,2024/04/30,10:53:07*32\r\n";
inline constexpr std::string_view QUECTEL_PROGRESS =
    "$PQTMSVINSTATUS,1,291264000,1,,11,1,60,-2005560.2218,5411825.5447,2706139.7061,1.8691*0C\r\n";
inline constexpr std::string_view QUECTEL_COMPLETE =
    "$PQTMSVINSTATUS,1,291323000,2,,11,60,60,-2005559.8481,5411823.1873,2706139.3995,1.8075*3F\r\n";

inline std::string quectelSentence(std::string_view body)
{
    unsigned checksum = 0;
    for (const unsigned char byte : body) {
        checksum ^= byte;
    }
    std::array<char, 8> tail{};
    std::snprintf(tail.data(), tail.size(), "*%02X\r\n", checksum);
    return '$' + std::string(body) + tail.data();
}

struct QuectelReceiver
{
    enum class Fault
    {
        Reject,
        Silent,
        Cancel,
        Partial,
        Readback,
        Checksum,
    };

    std::string identity{QUECTEL_IDENTITY};
    unsigned role = 1;
    unsigned activeRole = 1;
    unsigned roleAfterReset = 0;
    unsigned savedRole = 0;
    std::string base = "1,60,15.0,0.0000,0.0000,0.0000,0.0";
    std::string savedBase;
    std::string activeBase;
    std::string failure;
    std::string wrongReadback;
    size_t failureOccurrence = 1;
    size_t failureMatches = 0;
    unsigned failAfterSaves = 0;
    unsigned saves = 0;
    Fault fault = Fault::Reject;
    bool failed = false;
    bool silentAfterReset = false;
    bool periodicStatus = true;
    bool periodicNmea = true;
    bool acceptObservations = true;
    bool nativeStoredSurvey = false;
    bool restarting = false;
    bool restartSurvey = false;
    size_t chunk = 7;
    uint64_t responseDelayUs = 0;
    uint64_t bootDelayUs = 20000;
    unsigned initialTow = 291263000;
    unsigned observations = 0;
    uint64_t startedUs = 0;
    unsigned bootGeneration = 0;
    std::string queued;
    std::map<std::string, std::string> rates;
    std::map<std::string, std::string> savedRates;
    std::vector<std::string> commands;
    std::vector<GPSCommandResult> outcomes;
    std::vector<GPSNativeSurveyReport> surveys;
    size_t corrections = 0;
    size_t positions = 0;
    ReceiverEventQueue events{gps_test_time};

    bool sent(std::string_view prefix) const
    {
        return std::any_of(commands.begin(), commands.end(),
                           [prefix](const auto& command) { return command.starts_with(prefix); });
    }

    unsigned tow() const
    {
        return static_cast<unsigned>((initialTow + (gps_test_time - startedUs) / 1000) % 604800000);
    }

    void save()
    {
        savedRole = role;
        savedBase = base;
        savedRates = rates;
        ++saves;
    }

    void boot()
    {
        const unsigned previousRole = activeRole;
        role = roleAfterReset ? roleAfterReset : savedRole;
        activeRole = role;
        base = savedBase;
        activeBase = base;
        rates = previousRole == activeRole ? savedRates : std::map<std::string, std::string>{};
        if (restartSurvey) {
            nativeStoredSurvey = false;
        }
        restartSurvey = false;
        observations = 0;
        restarting = false;
        queued += QUECTEL_BOOT;
        const unsigned generation = ++bootGeneration;
        events.schedule(1000000, [this, generation] { navigationTick(generation); });
    }

    void navigationTick(unsigned generation)
    {
        if (generation != bootGeneration || restarting) {
            return;
        }
        if (activeRole == 2) {
            const auto first = activeBase.find(',');
            const auto second = activeBase.find(',', first + 1);
            const unsigned count = static_cast<unsigned>(std::stoul(activeBase.substr(first + 1, second - first - 1)));
            const bool fixed = activeBase.starts_with("2,");
            const bool survey = activeBase.starts_with("1,");
            if (survey && nativeStoredSurvey) {
                observations = count;
            } else if (survey && acceptObservations && observations < count) {
                ++observations;
            }
            const bool complete = fixed || (survey && observations >= count);
            if (survey && complete) {
                nativeStoredSurvey = true;  // Receiver-native storage does not execute PQTMSAVEPAR.
            }
            if (periodicStatus && rates.contains("PQTMSVINSTATUS")) {
                std::string coordinates = "0.0000,6378237.0000,0.0000";
                if (fixed) {
                    const auto third = activeBase.find(',', second + 1);
                    coordinates = activeBase.substr(third + 1);
                    for (unsigned index = 0, commas = 0; index < coordinates.size(); ++index) {
                        if (coordinates[index] == ',' && ++commas == 3) {
                            coordinates.resize(index);
                            break;
                        }
                    }
                }
                const auto validity = !fixed && !survey ? ",0,,11," : complete ? ",2,,11," : ",1,,11,";
                queued += quectelSentence(
                    "PQTMSVINSTATUS,1," + std::to_string(tow()) + validity + std::to_string(fixed ? 0 : observations) +
                    ',' + std::to_string(fixed ? 0 : count) + ',' + coordinates + (fixed ? ",0.0000" : ",1.2500"));
            }
        }
        if (periodicNmea && (activeRole == 1 || rates.contains("GGA"))) {
            queued += "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
        }
        events.schedule(1000000, [this, generation] { navigationTick(generation); });
    }

    GPSProtocolIO io()
    {
        startedUs = gps_test_time;
        activeRole = savedRole = role;
        activeBase = savedBase = base;
        savedRates = rates;
        auto result = makeGPSProtocolTestIO();
        result.wait = [this](std::chrono::microseconds delay) {
            events.advanceTo(gps_test_time + delay.count());
            return !(failed && fault == Fault::Cancel);
        };
        result.read = [this](std::span<uint8_t> output, GPSDeadline deadline) -> GPSReadResult {
            events.advanceTo((std::min) (gps_test_time + 1000, deadline.untilUs));
            if (failed && fault == Fault::Cancel) {
                return {GPSReadStatus::Cancelled};
            }
            while (queued.empty() && gps_test_time < deadline.untilUs) {
                events.advanceToNext(deadline.untilUs);
            }
            if (queued.empty()) {
                return {GPSReadStatus::TimedOut};
            }
            const auto count = (std::min) ({queued.size(), output.size(), chunk});
            std::memcpy(output.data(), queued.data(), count);
            queued.erase(0, count);
            return {GPSReadStatus::Data, static_cast<int>(count)};
        };
        result.write = [this](std::span<const uint8_t> input, GPSDeadline) -> GPSWriteResult {
            const std::string wire(reinterpret_cast<const char*>(input.data()), input.size());
            const auto command = wire.substr(1, wire.size() - 6);
            if (quectelSentence(command) != wire) {
                throw std::runtime_error("Invalid Quectel command framing");
            }
            commands.push_back(command);
            const auto name = command.substr(0, command.find(','));
            const bool injectFailure = !failure.empty() && command.starts_with(failure) && saves >= failAfterSaves &&
                                       ++failureMatches == failureOccurrence;
            if (injectFailure) {
                failed = true;
                if (fault == Fault::Partial) {
                    return {GPSWriteStatus::TimedOut, static_cast<int>(input.size()), 4};
                }
                if (fault == Fault::Cancel || fault == Fault::Silent) {
                    if (command == "PQTMSAVEPAR") {
                        save();
                    }
                    return {GPSWriteStatus::Completed, static_cast<int>(input.size()), static_cast<int>(input.size())};
                }
                if (fault == Fault::Reject) {
                    events.schedule(responseDelayUs, [this, name] { queued += quectelSentence(name + ",ERROR,2"); });
                    return {GPSWriteStatus::Completed, static_cast<int>(input.size()), static_cast<int>(input.size())};
                }
            }
            std::string reply;
            if (command == "PQTMVERNO") {
                if (!restarting && (!silentAfterReset || !sent("PQTMSRR"))) {
                    reply = identity;
                }
            } else if (command == "PQTMCFGRCVRMODE,R") {
                reply = quectelSentence("PQTMCFGRCVRMODE,OK," + std::to_string(role));
            } else if (command == "PQTMCFGRCVRMODE,W,1" || command == "PQTMCFGRCVRMODE,W,2") {
                role = command.back() - '0';  // Readback is staged; activeRole changes only at boot.
                reply = "$PQTMCFGRCVRMODE,OK*64\r\n";
            } else if (command == "PQTMCFGFIXRATE,R") {
                reply = "$PQTMCFGFIXRATE,OK,1000*0A\r\n";
            } else if (command == "PQTMCFGSVIN,R") {
                reply = quectelSentence("PQTMCFGSVIN,OK," + base);
            } else if (command.starts_with("PQTMCFGSVIN,W,")) {
                base = command.substr(std::string("PQTMCFGSVIN,W,").size());
                restartSurvey = true;
                reply = "$PQTMCFGSVIN,OK*70\r\n";
            } else if (command == "PQTMSAVEPAR") {
                save();
                reply = "$PQTMSAVEPAR,OK*72\r\n";
            } else if (command == "PQTMSRR") {
                restarting = true;
                if (!injectFailure || fault != Fault::Checksum) {
                    events.schedule(bootDelayUs, [this] { boot(); });
                }
            } else if (command.starts_with("PQTMCFGMSGRATE,W,")) {
                const auto values = command.substr(std::string("PQTMCFGMSGRATE,W,").size());
                rates[values.substr(0, values.find(','))] = values;
                reply = "$PQTMCFGMSGRATE,OK*29\r\n";
            } else if (command.starts_with("PQTMCFGMSGRATE,R,")) {
                const auto values = command.substr(std::string("PQTMCFGMSGRATE,R,").size());
                reply = quectelSentence("PQTMCFGMSGRATE,OK," + rates.at(values.substr(0, values.find(','))));
            } else {
                throw std::runtime_error("Unexpected command: " + command);
            }
            if (injectFailure && fault == Fault::Readback) {
                reply = wrongReadback.empty() ? quectelSentence(name + ",OK,GGA,0") : wrongReadback;
            } else if (injectFailure && fault == Fault::Checksum && !reply.empty()) {
                reply[reply.size() - 4] = reply[reply.size() - 4] == '0' ? '1' : '0';
            }
            events.schedule(responseDelayUs, [this, reply] { queued += reply; });
            return {GPSWriteStatus::Completed, static_cast<int>(input.size()), static_cast<int>(input.size())};
        };
        result.commandFinished = [this](const GPSCommandResult& outcome) { outcomes.push_back(outcome); };
        result.decoded = [this](const GPSDecodedBatch& batch) {
            if (batch.events.size() > GPSDecodedBatch::MAX_EVENTS) {
                throw std::runtime_error("Quectel decoded batch overflow");
            }
            for (const auto& event : batch.events) {
                if (const auto* survey = std::get_if<GPSNativeSurveyReport>(&event)) {
                    surveys.push_back(*survey);
                }
                corrections += std::holds_alternative<GPSRTCMReport>(event);
                positions += std::holds_alternative<GPSNativePositionReport>(event);
            }
        };
        return result;
    }
};

}  // namespace GPSTest
