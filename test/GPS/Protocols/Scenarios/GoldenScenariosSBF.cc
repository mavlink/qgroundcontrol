#include "GoldenScenarios.h"

namespace GPSGoldenScenario {

void addSeptentrio(Table& table)
{
    constexpr GPSType type = GPSType::septentrio;
    const auto survey = surveyIn(1, 60);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = sbfStream();
    const auto rules = [](std::vector<Rule> faults) { return sbf([faults](SBFBench& b) { b.faults.rules = faults; }); };
    table.add(type, "survey", "USB connection, survey-in", sbf(), survey, stream);
    table.add(type, "fixed", "USB connection, fixed base", sbf(), fixed, stream);
    table.add(type, "survey-explicit-baud", "Explicit 230400 request; the driver selects its own rate", sbf(),
              surveyIn(1, 60, 230400), stream);
    table.add(type, "com-port-survey", "Serial COM1 connection, survey-in", rules({latestReply("\n\r", "COM1>")}),
              survey, stream);
    table.add(type, "com-port-fixed", "Serial COM1 connection, fixed base", rules({latestReply("\n\r", "COM1>")}),
              fixed, stream);
    table.add(type, "port-detection-timeout", "Receiver never answers the port prompt", rules({silence("\n\r")}),
              survey, stream);
    table.add(type, "nak-optional-output-disable", "Receiver rejects disabling a COM output (optional)",
              rules({latestReply("setDataInOut,COM1", SBF_NAK)}), survey, stream);
    table.add(type, "nak-required-reset", "Receiver rejects resetting the SBF output (required)",
              rules({latestReply("setSBFOutput, Stream1, USB1, none", SBF_NAK)}), survey, stream);
    table.add(type, "data-io-retry", "Second data-port application rejected twice, then accepted",
              rules({latestReply("setDataInOut, USB1, Auto, SBF", SBF_NAK, 2, 3)}), survey, stream);
    table.add(type, "data-io-retry-exhausted", "Second data-port application rejected on every attempt",
              rules({latestReply("setDataInOut, USB1, Auto, SBF", SBF_NAK, 2)}), survey, stream);
    table.add(type, "survey-rejected", "Receiver rejects survey-in (base mode)",
              rules({latestReply("setPVTMode", SBF_NAK)}), survey, stream);
    table.add(type, "fixed-rejected", "Receiver rejects the static position (base mode)",
              rules({latestReply("setStaticPosGeodetic", SBF_NAK)}), fixed, stream);
    table.add(type, "persistent", "Persistent changes are not supported", sbf(), persistent(survey), stream);
    table.add(type, "averaging", "Receiver averaging is not supported", sbf(), averaging(60), stream);
    table.add(type, "compact", "MSM4 corrections are not supported", sbf(), compact(survey), stream);
}

}  // namespace GPSGoldenScenario
