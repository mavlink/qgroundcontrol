#include "Protocols/Scenarios/GoldenScenarios.h"
#include "Protocols/Support/UnicoreReceiverModel.h"

namespace GPSTest::GoldenScenario {

void addUnicore(Table& table)
{
    constexpr GPSType type = GPSType::unicore;
    const auto average = averaging(5);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = unicoreStream();
    const auto rules = [](std::vector<Rule> faults) {
        return unicore([faults](UnicoreBench& b) { b.faults.rules = faults; });
    };
    table.add(type, "averaging", "UM982 receiver averaging", unicore(), average, stream);
    table.add(type, "fixed", "UM982 fixed base", unicore(), fixed, stream);
    table.add(type, "auto-baud-460800", "Detects a receiver at 460800",
              unicore([](UnicoreBench& b) { b.model.availableBaud = 460800; }), average);
    table.add(type, "explicit-baud-mismatch", "Explicit 9600 while the receiver is at 115200", unicore(),
              averaging(5, 9600), stream);
    table.add(type, "version-timeout", "Receiver never answers the version query (identity)",
              rules({silence("VERSIONA")}), average, stream);
    table.add(type, "base-rejected", "Receiver rejects base mode", unicore([](UnicoreBench& b) {
                  b.model.fault = GPSTest::UnicoreReceiverModel::Fault::Reject;
                  b.model.faultCommand = "MODE BASE";
              }),
              average, stream);
    table.add(type, "short-write", "Transport accepts a truncated command",
              rules({GPSTest::writeFails("RTCM1074 1",
                                         [](const QByteArray& command) {
                                             const int written = static_cast<int>(command.size()) - 1;
                                             return GPSWriteResult{GPSWriteStatus::Completed, written, written};
                                         })}),
              average, stream);
    table.add(type, "read-error", "Transport read fails", unicore([](UnicoreBench& b) { b.model.readError = true; }),
              average, stream);
    table.add(type, "averaging-incomplete", "Averaging never reaches a fixed position",
              unicore([](UnicoreBench& b) { b.model.allowAveragingCompletion = false; }), average, stream);
}

}  // namespace GPSTest::GoldenScenario
