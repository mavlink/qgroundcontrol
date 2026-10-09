#include "Protocols/Scenarios/GoldenScenarios.h"
#include "Protocols/Support/AshtechReceiverModel.h"

namespace GPSTest::GoldenScenario {

void addTrimble(Table& table)
{
    constexpr GPSType type = GPSType::trimble;
    const auto survey = surveyIn(1, 100);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = ashtechStream();
    const QByteArray nak = nmea("PASHR,NAK");
    const auto rules = [](std::vector<Rule> faults) {
        return ashtech([faults](AshtechBench& b) { b.faults.rules = faults; });
    };
    const auto at38400 = [](bool followSpeedChange) {
        return ashtech([followSpeedChange](AshtechBench& b) {
            enforceBaud(b, 38400);
            if (followSpeedChange) {
                b.faults.rules.push_back(afterCommand(
                    "$PASHS,SPD", [](ScriptedReceiver& receiver) { receiver.setReceiverBaudrate(115200); }));
            }
        });
    };
    table.add(type, "survey", "MB-Two, survey-in started by the first position", ashtech(), survey, stream);
    table.add(type, "fixed", "MB-Two, fixed base", ashtech(), fixed, stream);
    table.add(type, "explicit-baud-change", "Receiver at 38400 follows the speed command to 115200", at38400(true),
              surveyIn(1, 100, 38400));
    table.add(type, "explicit-baud-change-ignored", "Receiver at 38400 ignores the speed command", at38400(false),
              surveyIn(1, 100, 38400), stream);
    table.add(type, "explicit-baud-unlisted", "Explicit 4800, not an Ashtech probe rate, is tried as given", ashtech(),
              surveyIn(1, 100, 4800), stream);
    table.add(type, "port-query-timeout", "Receiver never answers the port query (identity)",
              rules({silence("$PASHQ,PRT")}), survey, stream);
    table.add(type, "port-query-nak", "Receiver rejects the port query", rules({reply(startsWith("$PASHQ,PRT"), nak)}),
              survey, stream);
    table.add(type, "board-query-timeout", "Receiver never answers the board query", rules({silence("$PASHQ,RID")}),
              survey, stream);
    table.add(type, "survey-rejected", "Receiver rejects position averaging (base mode)",
              rules({reply(startsWith("$PASHS,POS,AVG"), nak)}), survey, stream);
    table.add(type, "survey-failed-receipt", "Position averaging reports an error receipt",
              ashtech([](AshtechBench& b) { b.model.surveyReply = GPSTest::ASHTECH_SURVEY_FAILED; }), survey, stream);
    table.add(type, "fixed-rejected", "Receiver rejects the fixed position (base mode)",
              rules({reply(startsWith("$PASHS,POS,"), nak)}), fixed, stream);
    table.add(type, "rtcm-output-rejected", "Receiver rejects an RTCM output message",
              rules({reply(startsWith("$PASHS,RT3,1074"), nak)}), fixed, stream);
    table.add(type, "optional-pop-rejected", "Receiver rejects the optional 20 Hz update rate",
              rules({reply(startsWith("$PASHS,POP,20"), nak)}), survey);
    table.add(type, "optional-nme-timeout", "Receiver never answers disabling NMEA output (optional)",
              rules({silence("$PASHS,NME,ALL,A,OFF")}), survey);
}

}  // namespace GPSTest::GoldenScenario
