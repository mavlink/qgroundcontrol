#include "Protocols/Scenarios/GoldenScenarios.h"
#include "Protocols/Support/AshtechReceiverModel.h"
#include "Protocols/Support/FemtoReceiverModel.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/SBFReceiverModel.h"
#include "Protocols/Support/UBXReceiverModel.h"

namespace GPSTest::GoldenScenario {

namespace {

/// A receiver behind receiver detection: it parses only its own dialect, and answers and streams only while the host
/// runs the link at the receiver's rate.
template <typename Model, typename... Args>
BenchFactory automatic(GPSTest::Dialect dialect, std::function<void(ModelReceiver<Model>&)> setup, Args... args)
{
    return [dialect, setup = std::move(setup), args...](GPSTestClock& clock) -> std::unique_ptr<ReceiverBench> {
        auto result = std::make_unique<ModelReceiver<Model>>(clock, args...);
        result->detect(dialect);
        if (setup) {
            setup(*result);
        }
        return result;
    };
}

}  // namespace

void addAutomatic(Table& table)
{
    using GPSTest::Dialect;
    constexpr GPSType type = GPSType::automatic;
    const auto quectelRover = [](QuectelBench& b) {
        b.model.role = 1;
        b.model.base = "1,10,15.0,0.0000,0.0000,0.0000,0.0";
        b.setReceiverBaudrate(460800);
    };

    table.add(type, "ublox-19200",
              "u-blox sending UBX at 19200, which u-blox configuration does not probe, is configured from that rate",
              automatic<UBXReceiverModel>(Dialect::UBX,
                                          [](UBXBench& b) {
                                              b.model.receiverBaud = 19200;
                                              b.atRate = [&b] { return b.model.hostBaud == b.model.receiverBaud; };
                                              b.detection->stream = ubx(0x0701, QByteArray(92, '\0'));
                                              b.faults.rules.push_back(afterCommand(
                                                  QByteArray("\xb5\x62\x06", 3),
                                                  [&b](ScriptedReceiver&) { b.detection->stream.clear(); }));
                                          }),
              surveyIn(1.25, 60));
    table.add(type, "septentrio-230400", "Septentrio streaming SBF at 230400 is identified and configured at that rate",
              automatic<SBFReceiverModel>(Dialect::SBF,
                                          [](SBFBench& b) {
                                              b.setReceiverBaudrate(230400);
                                              b.detection->stream = SBFReceiverModel::pvt(12, 1000);
                                              // Forcing command input ends the stream.
                                              b.faults.rules.push_back(afterCommand(
                                                  "SSSS", [&b](ScriptedReceiver&) { b.detection->stream.clear(); }));
                                          }),
              surveyIn(1, 60));
    table.add(type, "trimble-38400", "Trimble answers the port query at 38400 and follows the speed command",
              automatic<GPSTest::AshtechReceiverModel>(
                  Dialect::Ashtech,
                  [](AshtechBench& b) {
                      b.setReceiverBaudrate(38400);
                      b.faults.rules.push_back(afterCommand(
                          "$PASHS,SPD", [](ScriptedReceiver& receiver) { receiver.setReceiverBaudrate(115200); }));
                  }),
              surveyIn(1, 100));
    table.add(type, "quectel-rover-role", "Factory Quectel in its rover role, without consent: nothing is changed",
              automatic<GPSTest::QuectelReceiverModel>(Dialect::Quectel, quectelRover), surveyIn(15, 10));
    table.add(type, "quectel-rover-role-persistent",
              "Consent reaches the detected Quectel, which saves its base role and restarts",
              automatic<GPSTest::QuectelReceiverModel>(Dialect::Quectel, quectelRover), persistent(surveyIn(15, 10)));
    table.add(type, "nothing-found", "Nothing answers at any rate: every probe at every rate, within the time limit",
              automatic<FemtoReceiverModel>(Dialect::Femto, [](FemtoBench& b) { b.atRate = [] { return false; }; }),
              surveyIn(1, 60));

    // The detected family configures as its own goldens pin. Three keep the whole transcript, as the rate detection
    // found carries into their configuration.
    for (auto& row : table.rows) {
        if (row.type == type && qstrcmp(row.name, "ublox-19200") != 0 && qstrcmp(row.name, "trimble-38400") != 0 &&
            qstrcmp(row.name, "septentrio-230400") != 0) {
            row.detectionOnly = true;
        }
    }
}

}  // namespace GPSTest::GoldenScenario
