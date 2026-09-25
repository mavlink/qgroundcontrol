#pragma once

#include <algorithm>
#include <atomic>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "../ProtocolTestPackets.h"
#include "Ashtech/GPSDriverAshtech.h"
#include "GPSProtocolTestIO.h"
#include "Support/ScriptedReceiver.h"

namespace GPSTest {

inline constexpr std::string_view ASHTECH_SURVEY_STARTED =
    "PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,100,114502.56,28.12.2011";
inline constexpr std::string_view ASHTECH_SURVEY_FINISHED =
    "PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,5542.5178481,N,03739.2954994,E,176.334,OK,CONTINUOUS,100."
    "20";
inline constexpr std::string_view ASHTECH_SURVEY_FAILED = "PASHR,RECEIPT,POS,AVG,100,FINISHED,124628.01,28.12.2011,ERR";

inline QByteArray ashtechPacket(std::string_view body)
{
    const auto bytes = nmeaPacket(body);
    return QByteArray(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));
}

class AshtechReceiverModel : public ScriptedReceiver::Model
{
public:
    AshtechReceiverModel()
        : scripted(stop, *this)
        , driver(captureGPSReports(io(), position), false)
    {}

    std::vector<std::string> commands;
    std::vector<GPSCommandResult> results;
    std::vector<GPSNativeSurveyReport> surveys;
    std::vector<std::vector<uint8_t>> corrections;
    std::string surveyReply{ASHTECH_SURVEY_STARTED};
    std::string failedCommand;
    bool silentFailure = false;
    GPSNativePositionReport position;
    size_t transportCalls = 0;
    std::atomic_bool stop{false};
    ScriptedReceiver scripted;
    GPSNativeAshtech driver;

    std::optional<QByteArray> takeCommand(QByteArray& pending) override
    {
        const int end = pending.indexOf("\r\n");
        if (end < 0) {
            return std::nullopt;
        }
        const QByteArray command = pending.first(end + 2);
        pending.remove(0, end + 2);
        return command;
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        if (receiver.hasQueuedReadData()) {
            return;
        }
        gps_test_time += uint64_t(deadline.remainingMilliseconds(gps_test_time)) * 1000 + 1;
    }

    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& bytes,
                                 const ScriptedReceiver::WriteContext&) override
    {
        ++transportCalls;
        const std::string command(bytes.constData(), static_cast<size_t>(bytes.size()));
        commands.push_back(command);
        if (!command.ends_with("\r\n")) {
            receiver.clearReplies();
            return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        }
        QByteArray reply;
        if (command == failedCommand) {
            reply = silentFailure ? QByteArray{} : ashtechPacket("PASHR,NAK");
        } else if (command.starts_with("$PASHQ,PRT")) {
            reply = ashtechPacket("PASHR,PRT,A,115200");
        } else if (command.starts_with("$PASHQ,RID")) {
            reply = ashtechPacket("PASHR,RID,MB2");
        } else if (command.starts_with("$PASHS,POS,AVG")) {
            reply = ashtechPacket(surveyReply);
        } else {
            reply = ashtechPacket("PASHR,ACK");
        }
        receiver.clearReplies();
        receiver.queueReply(reply);
        return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
    }

    GPSProtocolIO io()
    {
        auto io = makeGPSProtocolTestIO();
        scripted.clearReplies();
        scripted.clearCommands();
        scripted.setReadHandler([this](uint8_t*, int, int) -> std::optional<GPSReadResult> {
            ++transportCalls;
            return std::nullopt;
        });
        io.setBaudrate = [this](unsigned) {
            ++transportCalls;
            return GPSBaudStatus::Configured;
        };
        io.commandFinished = [this](const GPSCommandResult& result) { results.push_back(result); };
        io.decoded = [this](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* survey = std::get_if<GPSNativeSurveyReport>(&event)) {
                    surveys.push_back(*survey);
                } else if (const auto* frame = std::get_if<GPSRTCMReport>(&event)) {
                    corrections.emplace_back(frame->bytes.begin(), frame->bytes.begin() + frame->size);
                }
            }
        };
        return scripted.makeIO(std::move(io));
    }

    void configure(bool fixed = false)
    {
        GPSProtocol::GPSConfig config{};
        config.base = {
            .mode = fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                                .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}}
                          : GPSBaseStationConfig::Mode{
                                GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 100}}};
        unsigned baudrate = 115200;
        if (!driver.configure(baudrate, config)) {
            throw std::runtime_error("Ashtech test receiver failed to configure");
        }
        if (!driver.receiverReady()) {
            throw std::runtime_error("Ashtech test receiver did not become ready");
        }
        driver.consume({});
        surveys.clear();
    }

    bool startSurvey()
    {
        driver.consume(nmeaPacket("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,"));
        (void) driver.receive(1);
        return !driver.hasIOError();
    }
};

}  // namespace GPSTest
