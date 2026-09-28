#include "GoldenScenarios.h"

namespace GPSGoldenScenario {

void addAutomatic(Table& table)
{
    using GPSTest::Dialect;
    constexpr GPSType type = GPSType::automatic;
    const QByteArray factoryNmea = nmea("GNGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,");
    const auto at = [](unsigned baud) { return [baud](auto& b) { b.receiver.setReceiverBaudrate(baud); }; };
    const auto unicoreAt = [](AutomaticBench<GPSTest::UnicoreReceiver>& b) {
        auto& model = b.model;
        model.startedUs = model.clock.nowUs();
        b.atRate = [&model] { return model.hostBaud == model.availableBaud; };
        b.wait = [&model](std::chrono::microseconds delay) {
            model.events.advanceTo(model.clock.nowUs() + static_cast<uint64_t>(delay.count()));
            return !model.cancel;
        };
    };
    const auto quectelAt = [](unsigned baud, unsigned role = 2) {
        return [baud, role](AutomaticBench<GPSTest::QuectelReceiver>& b) {
            auto& model = b.model;
            model.role = role;
            model.base = "1,10,15.0,0.0000,0.0000,0.0000,0.0";
            model.startedUs = model.clock.nowUs();
            model.activeRole = model.savedRole = model.role;
            model.activeBase = model.savedBase = model.base;
            model.savedRates = model.rates;
            b.receiver.setReceiverBaudrate(baud);
            b.wait = [&model](std::chrono::microseconds delay) {
                model.events.advanceTo(model.clock.nowUs() + static_cast<uint64_t>(delay.count()));
                return true;
            };
        };
    };

    table.add(type, "ublox-factory-38400", "Factory u-blox sending only NMEA at 38400 answers the MON-VER probe",
              automatic<UBXReceiverModel>(
                  Dialect::UBX,
                  [factoryNmea](AutomaticBench<UBXReceiverModel>& b) {
                      b.model.lowLevelProtocolBehavior = true;
                      b.model.receiverBaud = 38400;
                      b.atRate = [&b] { return b.model.hostBaud == b.model.receiverBaud; };
                      b.detection.stream = factoryNmea;
                      // The first CFG command selects UBX output, ending the factory NMEA.
                      b.faults.rules.push_back(afterCommand(QByteArray("\xb5\x62\x06", 3),
                                                            [&b](ScriptedReceiver&) { b.detection.stream.clear(); }));
                  },
                  UBXReceiverModel::Receiver::F9P),
              surveyIn(1.25, 60));
    table.add(type, "ublox-19200",
              "u-blox sending UBX at 19200, which u-blox configuration does not probe, is configured from that rate",
              automatic<UBXReceiverModel>(
                  Dialect::UBX,
                  [](AutomaticBench<UBXReceiverModel>& b) {
                      b.model.lowLevelProtocolBehavior = true;
                      b.model.receiverBaud = 19200;
                      b.atRate = [&b] { return b.model.hostBaud == b.model.receiverBaud; };
                      b.detection.stream = ubx(0x0701, QByteArray(92, '\0'));
                      b.faults.rules.push_back(afterCommand(QByteArray("\xb5\x62\x06", 3),
                                                            [&b](ScriptedReceiver&) { b.detection.stream.clear(); }));
                  },
                  UBXReceiverModel::Receiver::F9P),
              surveyIn(1.25, 60));
    table.add(type, "ublox-fixed-115200", "Fixed-rate link: every family is probed at 115200 only",
              automatic<UBXReceiverModel>(
                  Dialect::UBX,
                  [](AutomaticBench<UBXReceiverModel>& b) {
                      b.receiver.setFixedBaudrate(115200);
                      b.atRate = [] { return true; };
                  },
                  UBXReceiverModel::Receiver::F9P),
              surveyIn(2.0, 180));
    table.add(type, "septentrio-115200", "Septentrio answers the SBF prompt at 115200",
              automatic<SBFReceiverModel>(Dialect::SBF, at(115200)), surveyIn(1, 60));
    table.add(type, "septentrio-averaging-unsupported", "Detected Septentrio cannot average; nothing is configured",
              automatic<SBFReceiverModel>(Dialect::SBF, at(115200)), averaging(60));
    table.add(type, "trimble-38400", "Trimble answers the port query at 38400 and follows the speed command",
              automatic<GPSTest::AshtechReceiverModel>(
                  Dialect::Ashtech,
                  [](AutomaticBench<GPSTest::AshtechReceiverModel>& b) {
                      b.receiver.setReceiverBaudrate(38400);
                      b.faults.rules.push_back(afterCommand(
                          "$PASHS,SPD", [](ScriptedReceiver& receiver) { receiver.setReceiverBaudrate(115200); }));
                  }),
              surveyIn(1, 100));
    table.add(type, "femto-115200", "Femtomes answers the version query at 115200",
              automatic<FemtoReceiverModel>(Dialect::Femto, at(115200)), surveyIn(1, 60));
    table.add(type, "unicore-115200", "Unicore answers the Femtomes version query in its own words",
              automatic<GPSTest::UnicoreReceiver>(Dialect::Unicore, unicoreAt), averaging(5));
    table.add(type, "quectel-460800", "Quectel answers the firmware query at its default 460800",
              automatic<GPSTest::QuectelReceiver>(Dialect::Quectel, quectelAt(460800)), surveyIn(15, 10));
    table.add(type, "quectel-rover-role", "Factory Quectel in its rover role, without consent: nothing is changed",
              automatic<GPSTest::QuectelReceiver>(Dialect::Quectel, quectelAt(460800, 1)), surveyIn(15, 10));
    table.add(type, "quectel-rover-role-persistent",
              "Consent reaches the detected Quectel, which saves its base role and restarts",
              automatic<GPSTest::QuectelReceiver>(Dialect::Quectel, quectelAt(460800, 1)),
              persistent(surveyIn(15, 10)));
    table.add(type, "nothing-found", "Nothing answers at any rate: every probe at every rate, within the time limit",
              automatic<FemtoReceiverModel>(
                  Dialect::Femto, [](AutomaticBench<FemtoReceiverModel>& b) { b.atRate = [] { return false; }; }),
              surveyIn(1, 60));
}

}  // namespace GPSGoldenScenario
