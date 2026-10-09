#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ScriptedReceiver.h"

namespace GPSTest {

inline constexpr std::string_view ASHTECH_SURVEY_STARTED =
    "PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,100,114502.56,28.12.2011";
inline constexpr std::string_view ASHTECH_SURVEY_FINISHED =
    "PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,5542.5178481,N,03739.2954994,E,176.334,OK,CONTINUOUS,100."
    "20";
inline constexpr std::string_view ASHTECH_SURVEY_FAILED = "PASHR,RECEIPT,POS,AVG,100,FINISHED,124628.01,28.12.2011,ERR";

inline QByteArray ashtechPacket(std::string_view body)
{
    return toByteArray(nmeaPacket(body));
}

/// An MB-Two, or the board it reports: it answers every $PASH command at once, with the latest reply only.
class AshtechReceiverModel : public ScriptedReceiver::Model
{
public:
    explicit AshtechReceiverModel(GPSTestClock& testClock)
        : clock(testClock)
    {}

    std::string surveyReply{ASHTECH_SURVEY_STARTED};
    /// The receiver type $PASHR,RID reports.
    std::string board = "MB2";
    GPSTestClock& clock;

    std::optional<QByteArray> takeCommand(QByteArray& pending) override { return takeLine(pending); }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        if (receiver.hasQueuedReadData()) {
            return;
        }
        clock.advanceBy(static_cast<uint64_t>(std::chrono::microseconds(deadline.remaining(clock.nowUs())).count()) +
                        1);
    }

    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& bytes,
                                 const ScriptedReceiver::WriteContext&) override
    {
        const std::string command(bytes.constData(), static_cast<size_t>(bytes.size()));
        QByteArray reply;
        if (command.starts_with("$PASHQ,PRT")) {
            reply = ashtechPacket("PASHR,PRT,A,115200");
        } else if (command.starts_with("$PASHQ,RID")) {
            reply = ashtechPacket("PASHR,RID," + board);
        } else if (command.starts_with("$PASHS,POS,AVG")) {
            reply = ashtechPacket(surveyReply);
        } else {
            reply = ashtechPacket("PASHR,ACK");
        }
        receiver.clearReplies();
        receiver.queueReply(reply);
        return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
    }
};

}  // namespace GPSTest
