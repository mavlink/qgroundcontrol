#include "GoldenScenarios.h"

namespace GPSGoldenScenario {

void addPassive(Table& table)
{
    constexpr GPSType type = GPSType::passive;
    const auto stream = passiveStream();
    table.add(type, "baud-115200", "NMEA and RTCM input at 115200", passive(), passiveInput(115200), stream);
    table.add(type, "baud-9600", "NMEA and RTCM input at 9600", passive(), passiveInput(9600), stream);
    table.add(type, "auto-baud", "Passive input requires an explicit baud", passive(), passiveInput(0), stream);
    table.add(type, "base-settings", "Passive input has no base settings", passive(),
              withRole(surveyIn(1, 60, 115200), GPSReceiverConfig::Role::Passive), stream);
    table.add(type, "persistent", "Passive input makes no persistent changes", passive(),
              persistent(passiveInput(115200)), stream);
    table.add(type, "rtk-base-role", "Passive input cannot be an RTK base", passive(), surveyIn(1, 60, 115200), stream);
    table.add(type, "baud-unsupported", "Transport cannot change its baud",
              passive([](PassiveBench& b) { b.receiver.setBaudrateResult(false); }), passiveInput(115200), stream);
}

}  // namespace GPSGoldenScenario
