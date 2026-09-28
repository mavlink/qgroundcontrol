#include "GoldenScenarios.h"

namespace GPSGoldenScenario {

void addUblox(Table& table)
{
    using Receiver = UBXReceiverModel::Receiver;
    constexpr GPSType type = GPSType::ublox;
    const auto survey = surveyIn(2.0, 180);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = ubxStream(false);
    const auto wireStream = ubxStream(true);
    const auto wireSurvey = surveyIn(1.25, 60, 115200);
    const auto wireFixed = fixedBase(47, 8, 500, 1, 115200);

    const struct
    {
        const char* name;
        Receiver receiver;
    } profiles[] = {
        {"profile-m8n-survey", Receiver::M8N}, {"profile-m9n-survey", Receiver::M9N},
        {"profile-m10-survey", Receiver::M10}, {"profile-m8p-rover-survey", Receiver::M8PRover},
        {"profile-f9r-survey", Receiver::F9R}, {"profile-unidentified-survey", Receiver::Unidentified},
        {"profile-u6-survey", Receiver::U6},   {"profile-m8n-early-survey", Receiver::M8NEarly},
    };

    table.add(type, "profile-m8p-base-survey", "M8P base profile, survey-in", ubxProfile(Receiver::M8PBase), survey,
              stream);
    table.add(type, "profile-m8p-base-fixed", "M8P base profile, fixed base", ubxProfile(Receiver::M8PBase), fixed,
              stream);
    table.add(type, "profile-m8p-base-fixed-compact", "M8P base profile, fixed base with MSM4",
              ubxProfile(Receiver::M8PBase), compact(fixed), stream);
    table.add(type, "profile-f9p-survey", "F9P profile, survey-in", ubxProfile(Receiver::F9P), survey, stream);
    table.add(type, "profile-f9p-fixed", "F9P profile, fixed base", ubxProfile(Receiver::F9P), fixed, stream);
    table.add(type, "profile-f9p-survey-compact", "F9P profile, survey-in with MSM4", ubxProfile(Receiver::F9P),
              compact(survey), stream);
    table.add(type, "profile-f9p-fixed-compact", "F9P profile, fixed base with MSM4", ubxProfile(Receiver::F9P),
              compact(fixed), stream);
    for (const auto& profile : profiles) {
        table.add(type, profile.name, "Receiver profile without base support", ubxProfile(profile.receiver), survey,
                  stream);
    }
    table.add(type, "profile-f9p-survey-restart", "F9P left surveying by an earlier session",
              ubxProfile(Receiver::F9P,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.retainedSurveyDuration = 329000;
                         }),
              survey, stream);
    table.add(type, "profile-f9p-survey-stop-stuck", "F9P whose earlier survey never stops",
              ubxProfile(Receiver::F9P,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.retainedSurveyDuration = 329000;
                             b.model.surveyStopStuck = true;
                         }),
              survey, stream);
    table.add(type, "profile-m8p-base-survey-stop-stuck", "M8P whose earlier survey never stops",
              ubxProfile(Receiver::M8PBase,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.surveyStopStuck = true;
                         }),
              survey, stream);
    table.add(type, "profile-f9p-corrupt-version", "F9P with corrupt identity replies around the valid one",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.corruptVersionReplies = true; }), fixed, stream);
    table.add(type, "profile-m8p-base-corrupt-version", "M8P with corrupt identity replies around the valid one",
              ubxProfile(Receiver::M8PBase, [](UBXBench& b) { b.model.corruptVersionReplies = true; }), fixed, stream);
    table.add(
        type, "profile-f9p-disable-nak", "F9P rejects disabling time mode (required)",
        ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.disableReply = UBXReceiverModel::DisableReply::Nak; }),
        survey, stream);
    table.add(
        type, "profile-f9p-disable-timeout", "F9P never acknowledges disabling time mode",
        ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.disableReply = UBXReceiverModel::DisableReply::Timeout; }),
        survey, stream);
    table.add(type, "profile-f9p-stale-disable-ack", "F9P repeats a stale acknowledgement before disabling",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.staleDisableAck = true; }), survey, stream);
    table.add(
        type, "profile-f9p-readback-nak", "F9P rejects the time-mode readback",
        ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.readbackReply = UBXReceiverModel::ReadbackReply::Nak; }),
        survey, stream);
    table.add(type, "profile-f9p-readback-wrong-value", "F9P reads back a different time mode",
              ubxProfile(Receiver::F9P,
                         [](UBXBench& b) { b.model.readbackReply = UBXReceiverModel::ReadbackReply::WrongValue; }),
              survey, stream);
    table.add(type, "profile-f9p-survey-rtcm-rejected", "F9P rejects RTCM activation after the survey completes",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), survey, stream);
    table.add(type, "profile-f9p-fixed-rtcm-rejected", "F9P rejects RTCM activation for a fixed base",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), fixed, stream);
    table.add(type, "profile-m8p-base-fixed-rtcm-rejected", "M8P rejects RTCM activation for a fixed base",
              ubxProfile(Receiver::M8PBase, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), fixed, stream);
    table.add(type, "profile-f9p-delayed-optional-ack", "F9P acknowledges an optional command late",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.delayOptionalAck = true; }), survey, stream);
    table.add(type, "profile-f9p-maximum-fixed-accuracy", "F9P fixed base at the largest accuracy value",
              ubxProfile(Receiver::F9P), fixedBase(47, 8, 500, 429496.71875f), stream);
    table.add(type, "profile-m8p-base-maximum-fixed-accuracy", "M8P fixed base at the largest accuracy value",
              ubxProfile(Receiver::M8PBase), fixedBase(47, 8, 500, 429496.71875f), stream);
    table.add(type, "profile-f9p-explicit-baud", "F9P with the transport's own fixed baud requested",
              ubxProfile(Receiver::F9P), surveyIn(2.0, 180, 115200), stream);
    table.add(type, "profile-f9p-explicit-baud-mismatch", "Requested baud differs from the fixed transport baud",
              ubxProfile(Receiver::F9P), surveyIn(2.0, 180, 38400), stream);
    table.add(type, "profile-f9p-persistent", "Persistent changes are not supported", ubxProfile(Receiver::F9P),
              persistent(survey), stream);
    table.add(type, "profile-f9p-averaging", "Receiver averaging is not supported", ubxProfile(Receiver::F9P),
              averaging(60), stream);
    table.add(type, "profile-f9p-passive-role", "Passive input is not a u-blox role", ubxProfile(Receiver::F9P),
              withRole(survey, GPSReceiverConfig::Role::Passive), stream);
    table.add(type, "profile-f9p-invalid-fixed", "Fixed base without a position", ubxProfile(Receiver::F9P),
              {.base = {.mode = GPSBaseStationConfig::Fixed{}}}, stream);

    // Wire-level model (UBXReceiverModel::lowLevelProtocolBehavior).
    const auto module = [](const char* name, const char* hardware = "") {
        return [name, hardware](UBXBench& b) {
            b.model.module = name;
            b.model.hardware = hardware;
        };
    };
    const auto legacyModule = [](const char* name) {
        return [name](UBXBench& b) {
            b.model.legacy = true;
            b.model.module = name;
        };
    };
    table.add(type, "wire-f9p-survey", "ZED-F9P (protocol 27), survey-in", ubxWire(), wireSurvey, wireStream);
    table.add(type, "wire-f9p-fixed", "ZED-F9P (protocol 27), fixed base", ubxWire(), wireFixed, wireStream);
    table.add(type, "wire-f9p-survey-compact", "ZED-F9P, survey-in with MSM4", ubxWire(), compact(wireSurvey),
              wireStream);
    table.add(type, "wire-f9p-fixed-compact", "ZED-F9P, fixed base with MSM4", ubxWire(), compact(wireFixed),
              wireStream);
    table.add(type, "wire-f9p-l1l5-survey", "ZED-F9P L1/L5 firmware (FWVER=HPGL1L5), survey-in",
              ubxWire(module("ZED-F9P FWVER=HPGL1L5")), wireSurvey, wireStream);
    table.add(type, "wire-f9p-l1l5-fixed", "ZED-F9P L1/L5 firmware (FWVER=HPGL1L5), fixed base",
              ubxWire(module("ZED-F9P FWVER=HPGL1L5")), wireFixed, wireStream);
    table.add(type, "wire-x20-survey", "ZED-X20P, survey-in", ubxWire(module("ZED-X20P")), wireSurvey, wireStream);
    table.add(type, "wire-x20-fixed-compact", "ZED-X20P, fixed base with MSM4", ubxWire(module("ZED-X20P")),
              compact(wireFixed), wireStream);
    table.add(type, "wire-m8p-legacy-survey", "NEO-M8P (pre-protocol-27 commands), survey-in",
              ubxWire(legacyModule("NEO-M8P")), wireSurvey, wireStream);
    table.add(type, "wire-m8p-legacy-fixed", "NEO-M8P (pre-protocol-27 commands), fixed base",
              ubxWire(legacyModule("NEO-M8P")), wireFixed, wireStream);
    table.add(type, "wire-m8p-legacy-fixed-compact", "NEO-M8P, fixed base with MSM4", ubxWire(legacyModule("NEO-M8P")),
              compact(wireFixed), wireStream);
    table.add(type, "wire-f9p-legacy-protocol", "ZED-F9P hardware reporting PROTVER 20.30", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.hardware = "00190000";
                  b.model.protocol = "20.30";
              }),
              wireSurvey, wireStream);
    table.add(type, "wire-m8n-legacy", "NEO-M8N cannot be a base", ubxWire(legacyModule("NEO-M8N")), wireSurvey);
    table.add(type, "wire-m9n", "NEO-M9N cannot be a base", ubxWire(module("NEO-M9N")), wireSurvey);
    table.add(type, "wire-m10", "MAX-M10S cannot be a base", ubxWire(module("MAX-M10S", "000A0000")), wireSurvey);
    table.add(type, "wire-dan-f10n", "DAN-F10N (L1/L5 M10) cannot be a base", ubxWire(module("DAN-F10N", "000A0000")),
              wireSurvey);

    const auto uart = [](unsigned rate, std::function<void(UBXReceiverModel&)> extra = {}) {
        return ubxWire([rate, extra](UBXBench& b) {
            b.model.receiverBaud = rate;
            b.model.protocol = "27.31";
            if (extra) {
                extra(b.model);
            }
        });
    };
    table.add(type, "wire-auto-baud-9600", "Detects a receiver at 9600 and raises it to 115200", uart(9600),
              surveyIn(1, 60), wireStream);
    table.add(type, "wire-auto-baud-115200", "Detects a receiver already at 115200", uart(115200), surveyIn(1, 60),
              wireStream);
    table.add(type, "wire-auto-baud-lost-ack", "Baud handoff without an ACK forces a readback",
              uart(9600, [](UBXReceiverModel& m) { m.loseBaudAck = true; }), surveyIn(1, 60), wireStream);
    table.add(type, "wire-auto-baud-lost-ack-jamdet-nak",
              "Baud handoff without an ACK, then CFG-SEC-JAMDET rejected: falls back to CFG-ITFM",
              uart(9600,
                   [](UBXReceiverModel& m) {
                       m.loseBaudAck = true;
                       m.unsupportedKeys = {UBXWire::KEY_SEC_JAMDET_SENSITIVITY_HI};
                   }),
              surveyIn(1, 60), wireStream);
    table.add(type, "wire-auto-baud-usb", "USB port answers at every host rate",
              uart(9600, [](UBXReceiverModel& m) { m.usb = true; }), surveyIn(1, 60), wireStream);
    table.add(type, "wire-explicit-baud-9600", "Explicit 9600 matches the receiver", uart(9600), surveyIn(1, 60, 9600),
              wireStream);
    table.add(type, "wire-explicit-baud-mismatch", "Explicit 38400 while the receiver is at 9600", uart(9600),
              surveyIn(1, 60, 38400));
    table.add(type, "wire-legacy-auto-baud-9600", "NEO-M8P detected at 9600 and raised with CFG-PRT",
              ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.receiverBaud = 9600;
              }),
              surveyIn(1, 60), wireStream);
    table.add(type, "wire-legacy-auto-baud-lost-ack", "NEO-M8P baud handoff without an ACK", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.receiverBaud = 9600;
                  b.model.loseBaudAck = true;
              }),
              surveyIn(1, 60), wireStream);

    const struct
    {
        const char* name;
        const char* about;
        std::function<void(UBXReceiverModel&)> fault;
    } discovery[] = {
        {"wire-discovery-unknown-hardware", "Identity reports unknown hardware",
         [](UBXReceiverModel& m) { m.hardware = "UNKN0WN!"; }},
        {"wire-discovery-invalid-protocol", "Identity reports an invalid protocol version",
         [](UBXReceiverModel& m) { m.protocol = "invalid"; }},
        {"wire-discovery-identity-timeout", "Identity replies are always corrupt",
         [](UBXReceiverModel& m) { m.corruptIdentity = true; }},
        {"wire-discovery-silent-port-configuration", "Port configuration is never acknowledged",
         [](UBXReceiverModel& m) { m.silencePortConfiguration = true; }},
        {"wire-discovery-ignored-baud-change", "Receiver ignores the new baud and the ACK is lost",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.ignoreBaudChange = true;
         }},
        {"wire-discovery-ignored-baud-change-usb", "USB receiver ignores the new baud and the ACK is lost",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.ignoreBaudChange = true;
             m.usb = true;
         }},
        {"wire-discovery-rejected-after-baud-change", "Late ACK arrives, then the next command is dropped",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.rejectAfterBaudChange = true;
         }},
        {"wire-discovery-m9n", "NEO-M9N discovered at 9600",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.module = "NEO-M9N";
         }},
        {"wire-discovery-nak-after-baud-change", "Late ACK arrives, then the next command is rejected",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.nakAfterBaudChange = true;
         }},
        {"wire-discovery-readback-timeout", "Baud readback after a lost ACK times out",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.readbackReply = UBXReceiverModel::ReadbackReply::Timeout;
         }},
    };

    for (const auto& scenario : discovery) {
        table.add(type, scenario.name, scenario.about, uart(9600, scenario.fault), surveyIn(1, 60));
    }

    table.add(type, "wire-f9p-disable-rejected", "Receiver rejects disabling time mode (required)",
              ubxWire([](UBXBench& b) { b.model.rejectDisable = true; }), wireSurvey, wireStream);
    table.add(type, "wire-f9p-start-rejected", "Receiver rejects starting the survey (base mode)",
              ubxWire([](UBXBench& b) { b.model.rejectStart = true; }), wireSurvey, wireStream);
    table.add(type, "wire-m8p-legacy-disable-rejected", "NEO-M8P rejects disabling time mode", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.rejectDisable = true;
              }),
              wireSurvey, wireStream);
    table.add(type, "wire-f9p-readback-wrong-value", "Time-mode readback differs",
              ubxWire([](UBXBench& b) { b.model.readbackReply = UBXReceiverModel::ReadbackReply::WrongValue; }),
              wireSurvey, wireStream);

    const struct
    {
        const char* name;
        const char* about;
        uint32_t key;
    } rejectedKeys[] = {
        {"wire-f9p-nak-jamming-detection", "Optional CFG-SEC-JAMDET rejected: falls back to CFG-ITFM",
         UBXWire::KEY_SEC_JAMDET_SENSITIVITY_HI},
        {"wire-f9p-nak-correction-status", "Optional RXM-COR rejected: falls back to RXM-RTCM",
         UBXWire::KEY_MSGOUT_RXM_COR_UART1},
        {"wire-f9p-nak-spartn", "Optional SPARTN input rejected", UBXWire::KEY_UART1INPROT_SPARTN},
        {"wire-f9p-nak-epoch-end", "Optional NAV-EOE output rejected", UBXWire::KEY_MSGOUT_NAV_EOE_UART1},
        {"wire-f9p-nak-rate", "Required measurement rate rejected", UBXWire::KEY_RATE_MEAS},
    };

    for (const auto& scenario : rejectedKeys) {
        table.add(type, scenario.name, scenario.about,
                  ubxWire([key = scenario.key](UBXBench& b) { b.faults.rules.push_back(ubxNak(key)); }), wireSurvey,
                  wireStream);
    }
}

}  // namespace GPSGoldenScenario
