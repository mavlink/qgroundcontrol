#include "GoldenScenarios.h"

namespace GPSGoldenScenario {

void addQuectel(Table& table)
{
    using Fault = GPSTest::QuectelReceiver::Fault;
    constexpr GPSType type = GPSType::quectel;
    constexpr const char* SURVEY_BASE = "1,10,15.0,0.0000,0.0000,0.0000,0.0";
    constexpr const char* FIXED_BASE = "2,0,0,0.0000,6378237.0000,0.0000,0";
    constexpr const char* NO_BASE = "0,0,0,0,0,0,0";
    const auto survey = surveyIn(15, 10);
    const auto fixed = fixedBase(0, 90, 100, 0);
    const auto stream = quectelStream();
    const auto receiver = [](unsigned role, const char* base, std::function<void(QuectelBench&)> extra = {}) {
        return quectel([role, base, extra](QuectelBench& b) {
            b.model.role = role;
            b.model.base = base;
            if (extra) {
                extra(b);
            }
        });
    };
    const auto fault = [receiver](const char* base, Fault kind, const char* command) {
        return receiver(2, base, [kind, command](QuectelBench& b) {
            b.model.fault = kind;
            b.model.failure = command;
        });
    };
    table.add(type, "survey", "Saved base role and matching survey settings", receiver(2, SURVEY_BASE), survey, stream);
    table.add(type, "fixed", "Saved base role and matching fixed position", receiver(2, FIXED_BASE), fixed, stream);
    table.add(type, "survey-persistent", "Consent restarts from saved settings first", receiver(2, SURVEY_BASE),
              persistent(survey), stream);
    table.add(type, "rover-role", "Rover role without consent to change it", receiver(1, SURVEY_BASE), survey, stream);
    table.add(type, "rover-role-persistent", "Rover role changed, saved and restarted with consent",
              receiver(1, SURVEY_BASE), persistent(survey), stream);
    table.add(type, "base-mismatch", "Base settings differ, without consent", receiver(2, NO_BASE), fixed, stream);
    table.add(type, "base-mismatch-persistent", "Base settings written, saved and restarted with consent",
              receiver(2, NO_BASE), persistent(fixed), stream);
    table.add(type, "auto-baud-115200", "Detects a receiver at 115200",
              receiver(2, SURVEY_BASE, [](QuectelBench& b) { enforceBaud(b, 115200); }), survey, stream);
    table.add(type, "explicit-460800", "Explicit 460800 request", receiver(2, SURVEY_BASE), surveyIn(15, 10, 460800),
              stream);
    table.add(type, "explicit-baud-mismatch", "Explicit 9600 while the receiver is at 460800",
              receiver(2, SURVEY_BASE, [](QuectelBench& b) { enforceBaud(b, 460800); }), surveyIn(15, 10, 9600),
              stream);
    table.add(type, "identity-timeout", "Receiver never answers the identity query",
              receiver(2, SURVEY_BASE, [](QuectelBench& b) { b.faults.rules.push_back(silence("$PQTMVERNO")); }),
              survey, stream);
    table.add(type, "wrong-module", "Identity reports an unqualified LG580P",
              receiver(2, SURVEY_BASE,
                       [](QuectelBench& b) {
                           b.model.identity =
                               GPSTest::quectelSentence("PQTMVERNO,LG580P03AANR01A03S,2024/04/30,10:53:07");
                       }),
              survey, stream);
    table.add(type, "message-rate-rejected", "Receiver rejects a required NMEA rate",
              fault(SURVEY_BASE, Fault::Reject, "PQTMCFGMSGRATE,W,GGA"), survey, stream);
    table.add(type, "message-rate-readback-mismatch", "NMEA rate readback differs",
              fault(SURVEY_BASE, Fault::Readback, "PQTMCFGMSGRATE,R,GGA"), survey, stream);
    table.add(type, "base-write-rejected", "Receiver rejects the fixed position (base mode)",
              quectel([](QuectelBench& b) {
                  b.model.role = 2;
                  b.model.base = "0,0,0,0,0,0,0";
                  b.model.fault = Fault::Reject;
                  b.model.failure = "PQTMCFGSVIN,W";
              }),
              persistent(fixed), stream);
    table.add(type, "save-rejected", "Receiver rejects saving the role change", quectel([](QuectelBench& b) {
                  b.model.role = 1;
                  b.model.base = "1,10,15.0,0.0000,0.0000,0.0000,0.0";
                  b.model.fault = Fault::Reject;
                  b.model.failure = "PQTMSAVEPAR";
              }),
              persistent(survey), stream);
    table.add(type, "restart-silent", "Receiver is silent after the restart",
              receiver(2, SURVEY_BASE, [](QuectelBench& b) { b.model.silentAfterReset = true; }), survey, stream);
    table.add(type, "restart-checksum", "Restart reply fails its checksum and the receiver never boots",
              fault(SURVEY_BASE, Fault::Checksum, "PQTMSRR"), survey, stream);
    table.add(type, "cancelled", "Stop requested during configuration",
              fault(SURVEY_BASE, Fault::Cancel, "PQTMCFGSVIN,R"), survey, stream);
    table.add(type, "partial-write", "Transport times out part-way through a command",
              fault(SURVEY_BASE, Fault::Partial, "PQTMCFGMSGRATE,W,GSV"), survey, stream);
    table.add(type, "survey-too-long", "Survey-in beyond 24 hours is not supported", receiver(2, SURVEY_BASE),
              surveyIn(15, 90000), stream);
    table.add(type, "averaging", "Receiver averaging is not supported", receiver(2, SURVEY_BASE), averaging(60),
              stream);
    table.add(type, "compact", "MSM4 corrections are not supported", receiver(2, SURVEY_BASE), compact(survey), stream);
}

}  // namespace GPSGoldenScenario
