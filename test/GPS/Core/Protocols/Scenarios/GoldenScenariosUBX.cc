#include "Protocols/Scenarios/GoldenScenarios.h"
#include "Protocols/Support/UBXReceiverModel.h"

namespace GPSTest::GoldenScenario {

namespace {

/// A UBXReceiverModel::Receiver preset on its fixed 115200 baud transport, as GPSDriverTest drives it.
BenchFactory ubxProfile(UBXReceiverModel::Receiver receiver, std::function<void(UBXBench&)> setup = {})
{
    return bench<UBXBench>(std::move(setup), receiver);
}

}  // namespace

void addUblox(Table& table)
{
    using Receiver = UBXReceiverModel::Receiver;
    constexpr GPSType type = GPSType::ublox;
    const auto survey = surveyIn(2.0, 180);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = ubxStream();
    const auto wireSurvey = surveyIn(1.25, 60, 115200);
    const auto wireFixed = fixedBase(47, 8, 500, 1, 115200);

    table.add(type, "profile-m8p-base-fixed", "M8P base profile, fixed base", ubxProfile(Receiver::M8PBase), fixed,
              stream);
    table.add(type, "profile-m8p-base-fixed-compact", "M8P base profile, fixed base with MSM4",
              ubxProfile(Receiver::M8PBase), compact(fixed), stream);
    table.add(type, "profile-f9p-survey", "F9P profile, survey-in", ubxProfile(Receiver::F9P), survey, stream);
    table.add(type, "profile-f9p-fixed", "F9P profile, fixed base", ubxProfile(Receiver::F9P), fixed);
    // UBXProtocolTest checks the other receivers without base support.
    table.add(type, "profile-m8n-survey", "Receiver profile without base support", ubxProfile(Receiver::M8N), survey,
              stream);
    table
        .add(type, "profile-f9p-survey-restart", "F9P left surveying by an earlier session",
             ubxProfile(Receiver::F9P,
                        [](UBXBench& b) {
                            b.model.timeMode = 1;
                            b.model.retainedSurveyDuration = 329000;
                        }),
             survey, stream)
        .sameAs = "profile-f9p-survey";
    table.add(type, "profile-f9p-survey-stop-stuck", "F9P whose earlier survey never stops",
              ubxProfile(Receiver::F9P,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.retainedSurveyDuration = 329000;
                             b.model.surveyStopStuck = true;
                         }),
              survey, stream);
    table.add(type, "profile-m8p-base-survey-restart", "M8P left surveying by an earlier session",
              ubxProfile(Receiver::M8PBase,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.retainedSurveyDuration = 329000;
                         }),
              survey, stream);
    table.add(type, "profile-m8p-base-survey-stop-stuck", "M8P whose earlier survey never stops",
              ubxProfile(Receiver::M8PBase,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.surveyStopStuck = true;
                         }),
              survey, stream);
    table
        .add(type, "profile-f9p-corrupt-version", "F9P with corrupt identity replies around the valid one",
             ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.corruptVersionReplies = true; }), fixed)
        .sameAs = "profile-f9p-fixed";
    table
        .add(type, "profile-m8p-base-corrupt-version", "M8P with corrupt identity replies around the valid one",
             ubxProfile(Receiver::M8PBase, [](UBXBench& b) { b.model.corruptVersionReplies = true; }), fixed, stream)
        .sameAs = "profile-m8p-base-fixed";
    table.add(
        type, "profile-f9p-disable-nak", "F9P rejects disabling time mode (required)",
        ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.disableReply = UBXReceiverModel::DisableReply::Nak; }),
        survey, stream);
    table
        .add(type, "profile-f9p-stale-disable-ack", "F9P repeats a stale acknowledgement before disabling",
             ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.staleDisableAck = true; }), survey, stream)
        .sameAs = "profile-f9p-survey";
    table.add(type, "profile-f9p-survey-rtcm-rejected", "F9P rejects RTCM activation after the survey completes",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), survey, stream);
    table.add(type, "profile-f9p-fixed-rtcm-rejected", "F9P rejects RTCM activation for a fixed base",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), fixed, stream);
    table.add(type, "profile-m8p-base-fixed-rtcm-rejected", "M8P rejects RTCM activation for a fixed base",
              ubxProfile(Receiver::M8PBase, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), fixed, stream);

    // Receivers the model's fields describe, on a UART whose rate the driver finds, for the receivers and faults the
    // presets above do not cover.
    const auto module = [](const char* name, const char* hardware = "") {
        return [name, hardware](UBXBench& b) {
            b.model.module = name;
            b.model.hardware = hardware;
        };
    };
    table.add(type, "wire-f9p-l1l5-survey", "ZED-F9P L1/L5 firmware (FWVER=HPGL1L5), survey-in",
              ubxWire(module("ZED-F9P FWVER=HPGL1L5")), wireSurvey);
    table.add(type, "wire-x20-fixed-compact", "ZED-X20P, fixed base with MSM4", ubxWire(module("ZED-X20P")),
              compact(wireFixed), stream);
    table.add(type, "wire-f9p-legacy-protocol", "ZED-F9P hardware reporting PROTVER 20.30", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.hardware = "00190000";
                  b.model.protocol = "20.30";
              }),
              wireSurvey);
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
              surveyIn(1, 60));
    table.add(type, "wire-auto-baud-115200", "Detects a receiver already at 115200", uart(115200), surveyIn(1, 60));
    table.add(type, "wire-auto-baud-lost-ack", "Baud handoff without an ACK forces a readback",
              uart(9600, [](UBXReceiverModel& m) { m.loseBaudAck = true; }), surveyIn(1, 60), stream);
    table.add(type, "wire-auto-baud-usb", "USB port answers at every host rate",
              uart(9600, [](UBXReceiverModel& m) { m.usb = true; }), surveyIn(1, 60));
    table.add(type, "wire-auto-baud-usb-lost-ack", "USB port answers at every host rate; the baud ACK is lost",
              uart(9600,
                   [](UBXReceiverModel& m) {
                       m.usb = true;
                       m.loseBaudAck = true;
                   }),
              surveyIn(1, 60));
    table.add(type, "wire-explicit-baud-9600", "Explicit 9600 matches the receiver", uart(9600), surveyIn(1, 60, 9600));
    table.add(type, "wire-explicit-baud-115200", "Explicit 115200 matches the receiver", uart(115200),
              surveyIn(1, 60, 115200));
    table.add(type, "wire-explicit-baud-mismatch", "Explicit 38400 while the receiver is at 9600", uart(9600),
              surveyIn(1, 60, 38400));
    table.add(type, "wire-legacy-auto-baud-9600", "NEO-M8P detected at 9600 and raised with CFG-PRT",
              ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.receiverBaud = 9600;
              }),
              surveyIn(1, 60));
    table.add(type, "wire-legacy-auto-baud-lost-ack", "NEO-M8P baud handoff without an ACK", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.receiverBaud = 9600;
                  b.model.loseBaudAck = true;
              }),
              surveyIn(1, 60));

    const struct
    {
        const char* name;
        const char* about;
        std::function<void(UBXReceiverModel&)> fault;
        const char* sameAs = nullptr;
    } discovery[] = {
        {"wire-discovery-unknown-hardware", "Identity reports unknown hardware",
         [](UBXReceiverModel& m) { m.hardware = "UNKN0WN!"; }},
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
         },
         "wire-discovery-nak-after-baud-change"},
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
        table.add(type, scenario.name, scenario.about, uart(9600, scenario.fault), surveyIn(1, 60)).sameAs =
            scenario.sameAs;
    }

    table.add(type, "wire-f9p-start-rejected", "Receiver rejects starting the survey (base mode)",
              ubxWire([](UBXBench& b) { b.model.rejectStart = true; }), wireSurvey, stream);
    table.add(type, "wire-m8p-legacy-disable-rejected", "NEO-M8P rejects disabling time mode", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.disableReply = UBXReceiverModel::DisableReply::Nak;
              }),
              wireSurvey, stream);

    const struct
    {
        const char* name;
        const char* about;
        uint32_t key;
    } rejectedKeys[] = {
        {"wire-f9p-nak-jamming-detection", "Optional CFG-SEC-JAMDET rejected: falls back to CFG-ITFM",
         UBXWire::KEY_SEC_JAMDET_SENSITIVITY_HI},
        {"wire-f9p-nak-rate", "Required measurement rate rejected", UBXWire::KEY_RATE_MEAS},
    };

    for (const auto& scenario : rejectedKeys) {
        table.add(type, scenario.name, scenario.about,
                  ubxWire([key = scenario.key](UBXBench& b) { b.faults.rules.push_back(ubxNak(key)); }), wireSurvey);
    }
}

}  // namespace GPSTest::GoldenScenario
