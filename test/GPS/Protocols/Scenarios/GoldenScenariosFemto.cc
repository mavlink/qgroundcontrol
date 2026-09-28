#include "GoldenScenarios.h"

namespace GPSGoldenScenario {

void addFemto(Table& table)
{
    constexpr GPSType type = GPSType::femto;
    const auto survey = surveyIn(1, 60);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = femtoStream();
    const auto rules = [](std::vector<Rule> faults) {
        return femto([faults](FemtoBench& b) { b.faults.rules = faults; });
    };
    table.add(type, "survey", "Survey-in through receiver position averaging", femto(), survey, stream);
    table.add(type, "fixed", "Fixed base", femto(), fixed, stream);
    table.add(type, "survey-explicit-115200", "Explicit 115200 request", femto(), surveyIn(1, 60, 115200), stream);
    table.add(type, "explicit-baud-unsupported", "Explicit 9600 is not supported", femto(), surveyIn(1, 60, 9600),
              stream);
    table.add(type, "version-timeout", "Receiver never answers the version query (identity)",
              rules({silence("VERSION")}), survey, stream);
    table.add(type, "unlog-nak-once", "First UNLOGALL rejected; the second round succeeds",
              rules({latestReply("UNLOGALL", FEMTO_NAK, 1, 1)}), survey, stream);
    table.add(type, "survey-rejected", "Receiver rejects position averaging (base mode)",
              rules({latestReply("POSAVE", FEMTO_NAK)}), survey, stream);
    table.add(type, "fixed-rejected", "Receiver rejects the fixed position (base mode)",
              rules({latestReply("FIX POSITION", FEMTO_NAK)}), fixed, stream);
    table.add(type, "fixed-rtcm-rejected", "Receiver rejects RTCM output for a fixed base",
              rules({latestReply("LOG RTCM", FEMTO_NAK)}), fixed, stream);
    table.add(type, "survey-rtcm-rejected", "Receiver rejects RTCM output after averaging completes",
              rules({latestReply("LOG RTCM", FEMTO_NAK)}), survey, stream);
    table.add(type, "persistent", "Persistent changes are not supported", femto(), persistent(survey), stream);
    table.add(type, "averaging", "Receiver averaging is not supported", femto(), averaging(60), stream);
    table.add(type, "compact", "MSM4 corrections are not supported", femto(), compact(survey), stream);
}

}  // namespace GPSGoldenScenario
