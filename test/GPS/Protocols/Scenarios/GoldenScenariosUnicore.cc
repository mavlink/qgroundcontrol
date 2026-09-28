#include "GoldenScenarios.h"

namespace GPSGoldenScenario {

void addUnicore(Table& table)
{
    using Fault = GPSTest::UnicoreReceiver::Fault;
    constexpr GPSType type = GPSType::unicore;
    const auto average = averaging(5);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = unicoreStream();
    const auto fault = [](Fault kind, const char* command) {
        return unicore([kind, command](UnicoreBench& b) {
            b.model.fault = kind;
            b.model.faultCommand = command;
        });
    };
    table.add(type, "averaging", "UM982 receiver averaging", unicore(), average, stream);
    table.add(type, "fixed", "UM982 fixed base", unicore(), fixed, stream);
    table.add(type, "averaging-explicit-115200", "Explicit 115200 request", unicore(), averaging(5, 115200), stream);
    table.add(type, "auto-baud-460800", "Detects a receiver at 460800",
              unicore([](UnicoreBench& b) { b.model.availableBaud = 460800; }), average, stream);
    table.add(type, "explicit-baud-mismatch", "Explicit 9600 while the receiver is at 115200", unicore(),
              averaging(5, 9600), stream);
    table.add(type, "version-timeout", "Receiver never answers the version query (identity)",
              fault(Fault::Silence, "VERSIONA"), average, stream);
    table.add(type, "version-rejected", "Receiver rejects the version query", fault(Fault::Reject, "VERSIONA"), average,
              stream);
    table.add(type, "base-rejected", "Receiver rejects base mode", fault(Fault::Reject, "MODE BASE"), average, stream);
    table.add(type, "fixed-rejected", "Receiver rejects the fixed base", fault(Fault::Reject, "MODE BASE"), fixed,
              stream);
    table.add(type, "log-rejected", "Receiver rejects a required NMEA output", fault(Fault::Reject, "GPGSV 1"), average,
              stream);
    table.add(type, "wrong-ack", "Acknowledgement names another command", fault(Fault::WrongAck, "UNLOG"), average,
              stream);
    table.add(type, "corrupt-ack", "Acknowledgement fails its checksum", fault(Fault::Corrupt, "MODE ROVER"), average,
              stream);
    table.add(type, "write-error", "Transport fails while writing", fault(Fault::WriteError, "RTCM1005 1"), average,
              stream);
    table.add(type, "short-write", "Transport accepts a truncated command", fault(Fault::ShortWrite, "RTCM1074 1"),
              average, stream);
    table.add(type, "cancelled", "Stop requested during configuration", fault(Fault::Cancel, "GPGGA 1"), average,
              stream);
    table.add(type, "read-error", "Transport read fails", unicore([](UnicoreBench& b) { b.model.readError = true; }),
              average, stream);
    table.add(type, "mode-mismatch", "Mode readback reports another role",
              unicore([](UnicoreBench& b) { b.model.modeMismatch = true; }), average, stream);
    table.add(type, "fixed-position-mismatch", "Position readback differs from the fixed base",
              unicore([](UnicoreBench& b) { b.model.positionMismatch = true; }), fixed, stream);
    table.add(type, "fixed-position-missing", "Position readback never arrives",
              unicore([](UnicoreBench& b) { b.model.omitPositionReadback = true; }), fixed, stream);
    table.add(type, "averaging-incomplete", "Averaging never reaches a fixed position",
              unicore([](UnicoreBench& b) { b.model.allowAveragingCompletion = false; }), average, stream);
    table.add(type, "survey", "Accuracy-controlled survey-in is not supported", unicore(), surveyIn(1, 60), stream);
    table.add(type, "persistent", "Persistent changes are not supported", unicore(), persistent(average), stream);
    table.add(type, "compact", "MSM4 corrections are not supported", unicore(), compact(average), stream);
}

}  // namespace GPSGoldenScenario
