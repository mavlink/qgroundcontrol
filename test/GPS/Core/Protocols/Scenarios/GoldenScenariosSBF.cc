#include "Protocols/Scenarios/GoldenScenarios.h"
#include "Protocols/Support/SBFReceiverModel.h"

namespace GPSTest::GoldenScenario {

namespace {

const QByteArray SBF_NAK = "$R? rejected\n";

}  // namespace

void addSeptentrio(Table& table)
{
    constexpr GPSType type = GPSType::septentrio;
    const auto survey = surveyIn(1, 60);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = sbfStream();
    const auto rules = [](std::vector<Rule> faults) { return sbf([faults](SBFBench& b) { b.faults.rules = faults; }); };
    table.add(type, "survey", "USB connection, survey-in", sbf(), survey, stream);
    table.add(type, "fixed", "USB connection, fixed base", sbf(), fixed);
    table.add(type, "survey-explicit-baud", "Explicit 230400 request; the link runs at it", sbf(),
              surveyIn(1, 60, 230400));
    table.add(type, "com-port-explicit-baud", "Serial COM1 connection at an explicit 230400 keeps that port rate",
              rules({latestReply("\n\r", "COM1>")}), surveyIn(1, 60, 230400));
    table.add(type, "com-port-fixed", "Serial COM1 connection, fixed base", rules({latestReply("\n\r", "COM1>")}),
              fixed);
    table.add(type, "port-detection-timeout", "Receiver never answers the port prompt", rules({silence("\n\r")}),
              survey, stream);
    table.add(type, "nak-optional-output-disable", "Receiver rejects disabling a COM output (optional)",
              rules({latestReply("setDataInOut,COM1", SBF_NAK)}), survey);
    table.add(type, "nak-required-reset", "Receiver rejects resetting the SBF output (required)",
              rules({latestReply("setSBFOutput, Stream1, USB1, none", SBF_NAK)}), survey, stream);
}

}  // namespace GPSTest::GoldenScenario
