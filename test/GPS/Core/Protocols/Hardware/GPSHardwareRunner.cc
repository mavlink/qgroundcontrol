#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QSaveFile>
#include <QtCore/QScopeGuard>
#include <QtCore/QTextStream>

#include "GPSCancellation.h"
#include "GPSDecodedData.h"
#include "GPSDriver.h"
#include "GPSEvidenceTransport.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverFamilies.h"
#include "MonotonicClock.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "RTCMFramer.h"
#include "ReplayGPSTransport.h"
#include "TCPGPSTransport.h"
#include "Transport/Support/GPSRecordingTransport.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#endif

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

int rtcmMessageId(const QByteArray& frame)
{
    return RTCMFramer::frameMessageId(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(frame.constData()),
                                                               static_cast<size_t>(frame.size())));
}

std::atomic_flag interrupted = ATOMIC_FLAG_INIT;

void interruptHandler(int)
{
    interrupted.test_and_set(std::memory_order_relaxed);
}

struct Options
{
    QString action;
    QString transport;
    QString device;
    QString model;
    QString surveyState;
    QString fault;
    QString familyName;
    QString outputPath;
    QString recordDirectory;
    QString replayPath;
    unsigned replayBaud = 0;
    GPSType family = GPSType::ublox;
    /// The requested role: passive input rather than an RTK base.
    bool passive = false;
    GPSReceiverConfig config;
    int observeMs = 1000;
    int cancelAfterMs = 50;
    /// How long a cancelled receive may take to return, beyond cancelAfterMs.
    int cancelSlackMs = 500;
    int timeoutMs = 15000;
};

QString saveEvidence(const QJsonObject& report, const QString& path)
{
    if (path.isEmpty()) {
        return {};
    }
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) {
        return QString("Cannot open evidence file %1: %2").arg(path, file.errorString());
    }
    const QByteArray bytes = QJsonDocument(report).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        return QString("Cannot commit evidence file %1: %2").arg(path, file.errorString());
    }
    return {};
}

int evidenceError(QJsonObject report, const QString& error);

int output(QJsonObject report, int code, const QString& path = {})
{
    report.insert("schema_version", 1);
    if (const QString error = saveEvidence(report, path); !error.isEmpty()) {
        return evidenceError(report, error);
    }
    QTextStream(stdout) << QJsonDocument(report).toJson(QJsonDocument::Compact) << Qt::endl;
    return code;
}

int evidenceError(QJsonObject report, const QString& error)
{
    report.insert("operation_outcome", report.value("outcome"));
    report.insert("outcome", "evidence_error");
    report.insert("detail", error);
    return output(report, 4);
}

QJsonObject check(const QString& name, const QString& status, const QString& detail)
{
    return {{"name", name}, {"status", status}, {"detail", detail}};
}

QJsonObject requestedConfig(const GPSReceiverConfig& config, bool passive)
{
    QJsonObject result{{"role", passive ? "passive" : "base"},
                       {"baud_rate", static_cast<qint64>(config.baudRate)},
                       {"allow_persistent_changes", config.allowPersistentChanges}};
    if (!passive) {
        result.insert("compact_observations", config.base.compactObservations);
        if (std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode)) {
            result.insert("base_mode", "fixed");
            result.insert("latitude_deg",
                          std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position.latitudeDegrees);
            result.insert("longitude_deg",
                          std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position.longitudeDegrees);
            result.insert("ellipsoid_altitude_m",
                          std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position.altitudeMeters);
        } else if (std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode)) {
            result.insert("base_mode", "receiver-averaging");
            result.insert(
                "averaging_maximum_s",
                static_cast<qint64>(
                    std::get<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode).maximumDuration.count()));
            result.insert("survey_accuracy_m", QJsonValue::Null);
        } else {
            result.insert("base_mode", "survey");
            result.insert(
                "survey_duration_s",
                static_cast<qint64>(std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration.count()));
            result.insert("survey_accuracy_m",
                          std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters);
        }
    }
    return result;
}

QString parseOptions(QCommandLineParser& parser, Options& options)
{
    options.action = parser.value("action");
    options.transport = parser.value("transport");
    options.device = parser.value("device");
    options.model = parser.value("model");
    options.surveyState = parser.value("survey-state");
    options.fault = parser.value("fault");
    const QString family = parser.value("family");
    options.familyName = family;
    options.outputPath = parser.value("output");
    if (parser.isSet("output") && options.outputPath.isEmpty()) {
        return "--output requires a nonempty path";
    }
    options.recordDirectory = parser.value("record");
    if (parser.isSet("record") && options.recordDirectory.isEmpty()) {
        return "--record requires a nonempty directory";
    }
    options.replayPath = parser.value("replay");
    if (parser.isSet("replay") && options.replayPath.isEmpty()) {
        return "--replay requires a nonempty path";
    }
    const QString role = parser.value("role");
    if (!QStringList{"plan", "configure", "suite", "cancel"}.contains(options.action) ||
        !QStringList{"scripted", "serial", "tcp"}.contains(options.transport) ||
        !QStringList{"ublox", "trimble", "septentrio", "femto", "unicore", "quectel", "passive"}.contains(family) ||
        !QStringList{"base", "passive"}.contains(role) || !QStringList{"f9p", "m8p"}.contains(options.model) ||
        !QStringList{"fresh", "retained", "none"}.contains(options.surveyState) ||
        !QStringList{"none", "nak", "wrong-readback", "cancel", "rtcm-nak", "rtcm-nak-cancel"}.contains(
            options.fault)) {
        return "Unsupported option value";
    }
    if (!parser.positionalArguments().isEmpty()) {
        return "Unexpected positional arguments";
    }
    if (parser.isSet("replay")) {
        if (parser.isSet("transport") || parser.isSet("device")) {
            return "--replay replaces --transport and --device";
        }
        if (parser.isSet("record")) {
            return "--replay cannot be combined with --record";
        }
        options.transport = "replay";
    } else if (parser.isSet("replay-baud")) {
        return "--replay-baud requires --replay";
    }
    if (family == "trimble") {
        options.family = GPSType::trimble;
    } else if (family == "septentrio") {
        options.family = GPSType::septentrio;
    } else if (family == "femto") {
        options.family = GPSType::femto;
    } else if (family == "unicore") {
        options.family = GPSType::unicore;
    } else if (family == "quectel") {
        options.family = GPSType::quectel;
    } else if (family == "passive") {
        options.family = GPSType::passive;
    }
    options.passive = role == "passive";
    if (options.transport == "scripted" && options.family != GPSType::ublox) {
        return "The scripted peer supports UBX only";
    }
    if (options.fault.startsWith("rtcm-nak") && role != "base") {
        return "RTCM activation faults require --role base";
    }
    if (options.fault == "rtcm-nak-cancel" && options.action != "cancel") {
        return "rtcm-nak-cancel requires --action cancel";
    }
    if (options.transport != "scripted" &&
        (parser.isSet("fault") || parser.isSet("survey-state") || parser.isSet("model"))) {
        return "Scripted receiver options cannot be applied to a physical transport";
    }
    bool valid = true;
    auto integer = [&](const QString& name, int minimum, int maximum) {
        bool ok = false;
        const int value = parser.value(name).toInt(&ok);
        valid = valid && ok && value >= minimum && value <= maximum;
        return value;
    };
    options.observeMs = integer("observe-ms", 1, 3600000);
    options.cancelAfterMs = integer("cancel-after-ms", 1, 1000);
    options.cancelSlackMs = integer("cancel-slack-ms", 1, 60000);
    options.timeoutMs = integer("timeout-ms", 100, 120000);
    options.replayBaud = static_cast<unsigned>(integer("replay-baud", 0, 4000000));
    if (options.family == GPSType::quectel && !parser.isSet("timeout-ms")) {
        // Role and base changes can require multiple receiver restarts within the driver's 45-second budget.
        options.timeoutMs = 60000;
    }
    options.config.baudRate = static_cast<uint32_t>(integer("baud", 0, 4000000));
    options.config.allowPersistentChanges = parser.isSet("allow-save");
    options.config.base.compactObservations = parser.isSet("compact-rtcm");
    if (options.config.base.compactObservations && options.passive) {
        return "--compact-rtcm requires --role base";
    }
    const QString baseMode = parser.value("base-mode");
    if (!QStringList{"survey", "fixed", "receiver-averaging"}.contains(baseMode)) {
        return "Unsupported base mode";
    }
    if (!options.passive) {
        if ((baseMode != "survey" && (parser.isSet("survey-duration") || parser.isSet("survey-accuracy"))) ||
            (baseMode != "receiver-averaging" && parser.isSet("averaging-duration")) ||
            (baseMode != "fixed" &&
             (parser.isSet("latitude") || parser.isSet("longitude") || parser.isSet("altitude")))) {
            return "Base options do not match the selected base mode";
        }
        if (baseMode == "fixed") {
            const auto coordinate = [&](const QString& name) {
                bool ok = false;
                const double value = parser.value(name).toDouble(&ok);
                valid = valid && parser.isSet(name) && ok && std::isfinite(value);
                return value;
            };
            options.config.base.mode = GPSBaseStationConfig::Fixed{};
            std::get<GPSBaseStationConfig::Fixed>(options.config.base.mode).position = {
                .latitudeDegrees = coordinate("latitude"),
                .longitudeDegrees = coordinate("longitude"),
                .altitudeMeters = coordinate("altitude")};
        } else if (baseMode == "receiver-averaging") {
            options.config.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
            std::get<GPSBaseStationConfig::ReceiverAveraging>(options.config.base.mode).maximumDuration =
                std::chrono::seconds(integer("averaging-duration", 1, 3600));
        } else {
            std::get<GPSBaseStationConfig::SurveyIn>(options.config.base.mode).duration =
                std::chrono::seconds(integer("survey-duration", 1, 86400));
            bool accuracyValid = false;
            std::get<GPSBaseStationConfig::SurveyIn>(options.config.base.mode).accuracyMeters =
                parser.value("survey-accuracy").toDouble(&accuracyValid);
            valid = valid && accuracyValid &&
                    std::isfinite(std::get<GPSBaseStationConfig::SurveyIn>(options.config.base.mode).accuracyMeters) &&
                    std::get<GPSBaseStationConfig::SurveyIn>(options.config.base.mode).accuracyMeters > 0;
        }
    } else if (parser.isSet("base-mode") || parser.isSet("survey-duration") || parser.isSet("survey-accuracy") ||
               parser.isSet("averaging-duration") || parser.isSet("latitude") || parser.isSet("longitude") ||
               parser.isSet("altitude")) {
        return "Base options require --role base";
    }
    // A replay configures nothing; its role and base options only prepare the decoder.
    // The role a family supports is its own: passive input for the passive family, a base for the others.
    if (!valid || (options.transport != "replay" &&
                   (options.passive != (options.family == GPSType::passive) ||
                    gpsValidateReceiverConfig(options.family, options.config) != GPSReceiverConfigError::None))) {
        return "Invalid receiver configuration or numeric option";
    }
    if (options.transport == "replay" && options.action != "plan" && options.action != "configure") {
        return "--replay supports --action plan or configure";
    }
    if (options.transport == "serial" && options.device.isEmpty()) {
        return "--device is required for serial";
    }
    if (options.transport == "tcp") {
        const auto separator = options.device.lastIndexOf(':');
        bool portValid = false;
        const uint port = separator > 0 ? options.device.mid(separator + 1).toUInt(&portValid) : 0;
        if (!portValid || port == 0 || port > 65535) {
            return "--device must be host:port for tcp";
        }
    }
#ifdef QGC_NO_SERIAL_LINK
    if (options.transport == "serial") {
        return "Serial transport is disabled in this build";
    }
#endif
    if (options.action != "plan" && options.transport != "scripted" && options.transport != "replay" &&
        !parser.isSet("allow-reconfigure")) {
        return "Physical operations require --allow-reconfigure; no device was opened";
    }
    return {};
}

QJsonObject configurationEvidence(const GPSDriver& driver)
{
    QJsonArray commands;
    for (const auto& evidence : driver.configurationEvidence()) {
        QString outcome;
        switch (evidence.outcome) {
            case GPSCommandOutcome::Pending:
                outcome = "pending";
                break;
            case GPSCommandOutcome::Written:
                outcome = "transport_written";
                break;
            case GPSCommandOutcome::Acknowledged:
                outcome = "acknowledged";
                break;
            case GPSCommandOutcome::ReadbackVerified:
                outcome = "readback_verified";
                break;
            case GPSCommandOutcome::Rejected:
                outcome = "rejected";
                break;
            case GPSCommandOutcome::TimedOut:
                outcome = "timed_out";
                break;
            case GPSCommandOutcome::Cancelled:
                outcome = "cancelled";
                break;
            case GPSCommandOutcome::TransportError:
                outcome = "transport_error";
                break;
        }
        commands.append(QJsonObject{{"command", QString::fromUtf8(evidence.command)},
                                    {"outcome", outcome},
                                    {"required", evidence.required},
                                    {"started_at_us", static_cast<qint64>(evidence.startedAtUs)},
                                    {"accepted_bytes", evidence.acceptedBytes},
                                    {"written_bytes", evidence.writtenBytes}});
    }
    return {{"commands", commands}};
}

std::unique_ptr<GPSTransport> physicalTransport(const Options& options, GPSCancelToken cancelToken)
{
    if (options.transport == "tcp") {
        const auto separator = options.device.lastIndexOf(':');
        return std::make_unique<TCPGPSTransport>(options.device.left(separator),
                                                 static_cast<quint16>(options.device.mid(separator + 1).toUInt()),
                                                 std::move(cancelToken));
    }
#ifndef QGC_NO_SERIAL_LINK
    if (options.transport == "serial") {
        return std::make_unique<SerialGPSTransport>(options.device, std::move(cancelToken));
    }
#endif
    return {};
}

void injectMeasurements(UBXReceiverModel& receiver, const Options& options, bool cancellation = false)
{
    const bool rejectActivation = options.fault == "rtcm-nak" || (cancellation && options.fault == "rtcm-nak-cancel");
    receiver.rejectRtcmActivation = rejectActivation;
    if (!options.passive && (options.surveyState != "none" || rejectActivation)) {
        QByteArray survey(40, '\0');
        const bool retained = options.surveyState == "retained" || rejectActivation;
        (void) LittleEndian::write<uint32_t>(mutableBytesOf(survey), 8, retained ? 600 : 0);
        (void) LittleEndian::write<uint32_t>(mutableBytesOf(survey), 28, 10000);
        (void) LittleEndian::write<uint32_t>(mutableBytesOf(survey), 32, retained ? 600 : 1);
        survey[36] = retained ? 1 : 0;
        survey[37] = retained ? 0 : 1;
        receiver.queueFrame(0x01, 0x3b, survey);
    }
    QByteArray position(92, '\0');
    (void) LittleEndian::write<uint32_t>(mutableBytesOf(position), 0, cancellation ? 1000 : 0);
    position[20] = 3;
    position[21] = 1;
    position[23] = 12;
    (void) LittleEndian::write<int32_t>(mutableBytesOf(position), 24, 85000000);
    (void) LittleEndian::write<int32_t>(mutableBytesOf(position), 28, 473000000);
    (void) LittleEndian::write<uint32_t>(mutableBytesOf(position), 40, 1000);
    receiver.queueFrame(0x01, 0x07, position);
}

QString fixName(GPSFixQuality fix)
{
    switch (fix) {
        case GPSFixQuality::NoFix:
            return "none";
        case GPSFixQuality::Fix2D:
            return "2d";
        case GPSFixQuality::Fix3D:
            return "3d";
        case GPSFixQuality::Differential:
            return "differential";
        case GPSFixQuality::RTKFloat:
            return "rtk-float";
        case GPSFixQuality::RTKFixed:
            return "rtk-fixed";
        case GPSFixQuality::Extrapolated:
            return "extrapolated";
        case GPSFixQuality::Unknown:
            break;
    }
    return "unknown";
}

/// Longer than any family waits for the rest of an epoch.
constexpr std::chrono::seconds END_OF_RECORDING_SILENCE{10};

/// Decodes a recorded receiver stream with the chosen family until it ends. Decode-only: the family is never
/// configured and nothing is written, so any family decodes the traffic its receivers send unprompted; base status
/// needs passive input or a family that can arm its decoder for the requested base without I/O, else the survey is
/// "unsupported".
/// Reports only fix, satellite, correction and survey summaries, never a position.
int replayRecording(const Options& options, QJsonObject report)
{
    GPSCancelSource stop;
    ReplayGPSTransport replay(options.replayPath, stop.token(), options.replayBaud);
    QJsonObject replayed{
        {"file", options.replayPath}, {"pacing_baud", static_cast<qint64>(options.replayBaud)}, {"decode_only", true}};
    const GPSReceiverFamily* family = gpsReceiverFamily(options.family);
    if (const auto opened = replay.open(); !family || opened.status != GPSOpenStatus::Opened) {
        report.insert("replay", replayed);
        report.insert("outcome", "failed");
        report.insert("detail", family ? QString("Cannot open recording: %1").arg(opened.detail)
                                       : QString("No decoder for receiver family %1").arg(options.familyName));
        return output(report, 1, options.outputPath);
    }
    int positions = 0;
    QMap<QString, int> fixes;
    int satellites = 0;
    int maxInView = -1;
    int maxUsed = -1;
    int correctionFrames = 0;
    QMap<int, int> correctionMessages;
    QMap<int, qint64> correctionBytes;
    int surveys = 0;
    std::optional<GPSSurveyReport> lastSurvey;
    GPSIntegrityReport integrity;
    GPSDecodedData::SatelliteCounts satelliteCounts;
    std::chrono::microseconds clockOffset{0};
    const auto nowUs = [&clockOffset] { return MonotonicClock::nowUs() + static_cast<uint64_t>(clockOffset.count()); };
    GPSRuntimeObserver observer;
    // The same projections GPSDriver publishes to its sinks.
    observer.decoded = [&](const GPSEventBatch& batch) {
        for (const auto& event : batch.events) {
            std::visit(
                [&](const auto& decoded) {
                    using Event = std::decay_t<decltype(decoded)>;
                    if constexpr (std::is_same_v<Event, GPSIntegrityReport>) {
                        integrity = decoded;
                    } else if constexpr (std::is_same_v<Event, GPSDecodedPosition>) {
                        ++positions;
                        ++fixes[fixName(GPSDecodedData::position(decoded, integrity).navigation.fixType)];
                    } else if constexpr (std::is_same_v<Event, GPSDecodedSatellites> ||
                                         std::is_same_v<Event, GPSDecodedSatelliteUsage>) {
                        ++satellites;
                        const GPSSatelliteReport satellite = satelliteCounts.update(decoded, nowUs());
                        maxInView = std::max(maxInView, satellite.inView.value_or(-1));
                        maxUsed = std::max(maxUsed, satellite.used.value_or(-1));
                    } else if constexpr (std::is_same_v<Event, GPSSurveyReport>) {
                        ++surveys;
                        lastSurvey = decoded;
                    } else if constexpr (std::is_same_v<Event, GPSRTCMFrame>) {
                        ++correctionFrames;
                        const int messageId = rtcmMessageId(decoded.bytes);
                        ++correctionMessages[messageId];
                        correctionBytes[messageId] += static_cast<qint64>(decoded.bytes.size());
                    }
                },
                event);
        }
    };
    GPSRuntimeIO io;
    io.clock.nowUs = nowUs;
    GPSProtocolRuntime runtime(*family, std::move(io), std::move(observer));
    // Configuration would enable correction framing, and u-blox navigation decoding, once the receiver answered.
    if (family->stream.framers.testFlag(GPSFrameKind::RTCM3)) {
        runtime.stream().setEnabled(GPSFrameKind::RTCM3, true);
    }
    // Passive input reports whatever base status its receiver sends. Other families report it only as configuring
    // the requested base would leave their decoder.
    bool surveyDecoded = true;
    if (!runtime.armNavigationDecode({.corrections = true})) {
        surveyDecoded = options.passive || runtime.armDecodeOnly({.base = options.config.base});
    }
    QElapsedTimer elapsed;
    elapsed.start();
    std::array<uint8_t, ReplayGPSTransport::DEFAULT_CHUNK_BYTES> buffer{};
    QString readError;
    while (!replay.finished()) {
        if (interrupted.test(std::memory_order_relaxed)) {
            stop.cancel();
        }
        const GPSReadResult read = replay.read(buffer, 50ms);
        if (read.status == GPSReadStatus::Data) {
            (void) runtime.consume(std::span<const uint8_t>(buffer).first(static_cast<size_t>(read.bytesRead)));
        } else if (read.status != GPSReadStatus::TimedOut) {
            readError = read.status == GPSReadStatus::Cancelled ? QString("Replay cancelled")
                                                                : QString("Recording read failed: %1").arg(read.detail);
            break;
        }
    }
    // Epochs still waiting for more messages expire once no traffic follows, as on a live link that falls silent.
    clockOffset = END_OF_RECORDING_SILENCE;
    (void) runtime.consume({});
    replayed.insert("bytes", replay.bytesDelivered());
    replayed.insert("complete", replay.finished());
    replayed.insert("elapsed_ms", elapsed.elapsed());
    report.insert("replay", replayed);
    QJsonObject fixTypes;
    for (auto it = fixes.cbegin(); it != fixes.cend(); ++it) {
        fixTypes.insert(it.key(), it.value());
    }
    report.insert("positions", QJsonObject{{"messages", positions}, {"fix_types", fixTypes}});
    report.insert("satellites",
                  QJsonObject{{"messages", satellites}, {"max_in_view", maxInView}, {"max_used", maxUsed}});
    QJsonObject messages;
    for (auto it = correctionMessages.cbegin(); it != correctionMessages.cend(); ++it) {
        messages.insert(QString::number(it.key()),
                        QJsonObject{{"frames", it.value()}, {"bytes", correctionBytes.value(it.key())}});
    }
    report.insert("corrections", QJsonObject{{"frames", correctionFrames}, {"messages", messages}});
    if (surveyDecoded) {
        QJsonObject survey{{"messages", surveys}};
        if (lastSurvey) {
            survey.insert("active", lastSurvey->active);
            survey.insert("valid", lastSurvey->valid);
            survey.insert("duration_s", static_cast<qint64>(lastSurvey->duration.count()));
        }
        report.insert("survey", survey);
    } else {
        report.insert("survey", "unsupported");
    }
    report.insert("interrupted", interrupted.test(std::memory_order_relaxed));
    const bool complete = readError.isEmpty() && replay.finished();
    if (!readError.isEmpty()) {
        report.insert("detail", readError);
    }
    report.insert("outcome", complete ? "replayed" : "failed");
    return output(report, complete ? 0 : 1, options.outputPath);
}

int run(const Options& options)
{
    const bool scripted = options.transport == "scripted";
    const bool replaying = options.transport == "replay";
    QJsonObject report{{"action", options.action},
                       {"transport", options.transport},
                       {"evidence_origin", scripted    ? "scripted"
                                           : replaying ? "recording"
                                                       : "physical_transport"}};
    // A replay configures nothing.
    if (!replaying) {
        report.insert("requested", requestedConfig(options.config, options.passive));
    }
    report.insert("receiver_family", options.familyName);
    report.insert("schema_version", 1);
    report.insert("deadlines", QJsonObject{{"open_and_configure_ms", options.timeoutMs},
                                           {"observation_ms", options.observeMs},
                                           {"cancel_after_ms", options.cancelAfterMs},
                                           {"cancel_slack_ms", options.cancelSlackMs},
                                           {"cooperative_cancellation", true}});
    report.insert("started_at_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (scripted) {
        report.insert(
            "script",
            QJsonObject{{"model", options.model}, {"survey_state", options.surveyState}, {"fault", options.fault}});
    } else if (replaying) {
        report.insert("replay", QJsonObject{{"file", options.replayPath},
                                            {"pacing_baud", static_cast<qint64>(options.replayBaud)}});
    } else {
        report.insert("endpoint", QJsonObject{{"device", options.device}, {"transport", options.transport}});
    }
    report.insert("outcome", "running");
    if (!options.outputPath.isEmpty()) {
        QFile reservation(options.outputPath);
        if (!reservation.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
            return evidenceError(
                report,
                QString("Cannot reserve new evidence file %1: %2").arg(options.outputPath, reservation.errorString()));
        }
        reservation.close();
        if (const QString error = saveEvidence(report, options.outputPath); !error.isEmpty()) {
            return evidenceError(report, error);
        }
    }
    if (options.action == "plan") {
        report.insert("outcome", "not_run");
        report.insert("detail", "No transport constructed or opened; no receiver configuration sent.");
        return output(report, 0, options.outputPath);
    }
    if (replaying) {
        return replayRecording(options, report);
    }

    GPSCancelSource stop;
    std::atomic_bool deadlineExpired = false;
    std::atomic<qint64> operationDeadline = 0;
    const auto now = [] { return static_cast<qint64>(MonotonicClock::nowUs() / 1000); };
    std::atomic_bool watchdogDone = false;
    std::thread watchdog([&] {
        while (!watchdogDone.load()) {
            const auto deadline = operationDeadline.load();
            const bool signalReceived = interrupted.test(std::memory_order_relaxed);
            if (signalReceived || (deadline != 0 && now() >= deadline)) {
                deadlineExpired.store(!signalReceived);
                stop.cancel();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    const auto joinWatchdog = qScopeGuard([&] {
        watchdogDone.store(true);
        watchdog.join();
    });
    QJsonArray recordings;
    // Each physical session gets its own pair of files; the scripted peer keeps one session across stages.
    const auto recorded = [&](std::unique_ptr<GPSTransport> transport) -> std::unique_ptr<GPSTransport> {
        if (options.recordDirectory.isEmpty() || !transport) {
            return transport;
        }
        const auto files =
            GPSRecordingTransport::sessionFiles(options.recordDirectory, options.family, QDateTime::currentDateTime());
        recordings.append(QJsonObject{{"received", files.received}, {"sent", files.sent}});
        report.insert("recordings", recordings);
        return std::make_unique<GPSRecordingTransport>(std::move(transport), files);
    };
    GPSTestClock scriptedClock;
    std::unique_ptr<UBXReceiverModel> receiver;
    std::unique_ptr<GPSTransport> link;
    if (scripted) {
        receiver = std::make_unique<UBXReceiverModel>(
            options.model == "f9p" ? UBXReceiverModel::Receiver::F9P : UBXReceiverModel::Receiver::M8PBase,
            scriptedClock);
        link = recorded(std::make_unique<ScriptedReceiver>(stop, receiver.get()));
        if (options.fault == "nak") {
            receiver->disableReply = UBXReceiverModel::DisableReply::Nak;
        } else if (options.fault == "wrong-readback") {
            receiver->readbackReply = UBXReceiverModel::ReadbackReply::WrongValue;
        } else if (options.fault == "cancel") {
            receiver->disableReply = UBXReceiverModel::DisableReply::Cancelled;
        }
    } else {
        link = recorded(physicalTransport(options, stop.token()));
    }

    QStringList stages{"configured"};
    if (options.action == "suite") {
        stages.append("reconnected_base");
    }
    QJsonArray results;
    bool failed = false;
    bool incomplete = false;
    for (const QString& name : stages) {
        report.insert("active_stage", QJsonObject{{"name", name}, {"phase", "opening_or_configuring"}});
        if (const QString error = saveEvidence(report, options.outputPath); !error.isEmpty()) {
            return evidenceError(report, error);
        }
        operationDeadline.store(now() + options.timeoutMs);
        if (name == "reconnected_base" && !scripted) {
            link.reset();
            link = recorded(physicalTransport(options, stop.token()));
        }
        GPSTransport& transport = *link;
        GPSEvidenceTransport evidence(transport, stop.token());
        QJsonObject stage{{"name", name}};
        QJsonArray checks;
        if (name == stages.first() || name == "reconnected_base") {
            const auto opened = evidence.open();
            if (opened.status != GPSOpenStatus::Opened) {
                stage.insert("outcome", "failed");
                stage.insert("detail", "Transport open failed or was cancelled");
                results.append(stage);
                failed = true;
                break;
            }
            if (name == "reconnected_base") {
                checks.append(check("reconnect", "passed",
                                    scripted ? "Scripted peer reopened; state retained, no "
                                               "physical reconnect exercised"
                                             : "Transport destroyed and reopened; not a power "
                                               "cycle or physical unplug"));
            }
        }
        GPSReceiverConfig config = options.config;
        stage.insert("requested", requestedConfig(config, options.passive));
        QJsonArray surveys;
        QJsonObject lastPosition;
        int positions = 0;
        int satellites = 0;
        int correctionFrames = 0;
        QMap<int, int> correctionMessages;
        QMap<int, qint64> correctionBytes;
        QElapsedTimer elapsed;
        elapsed.start();
        GPSDriverSinks sinks;
        sinks.onPosition = [&](const GPSPositionReport& position) {
            ++positions;
            const auto& navigation = position.navigation;
            lastPosition = {{"fix_type", static_cast<int>(navigation.fixType)},
                            {"latitude_deg", navigation.latitudeDegrees},
                            {"longitude_deg", navigation.longitudeDegrees},
                            {"ellipsoid_altitude_m", navigation.altitudeEllipsoidMeters},
                            {"horizontal_accuracy_m", navigation.horizontalAccuracyMeters}};
        };
        sinks.onSatelliteInfo = [&](const GPSSatelliteReport&) { ++satellites; };
        sinks.onRTCM = [&](const QByteArray& frame) {
            ++correctionFrames;
            const int messageId = rtcmMessageId(frame);
            ++correctionMessages[messageId];
            correctionBytes[messageId] += static_cast<qint64>(frame.size());
        };
        sinks.onSurveyIn = [&](const GPSSurveyReport& survey) {
            const bool prior = survey.duration.count() > elapsed.elapsed() / 1000 + 2;
            QString observation = "indeterminate";
            if (prior) {
                observation = "retained_or_preexisting";
            } else if (survey.active && !survey.valid) {
                observation = "consistent_with_fresh";
            } else if (survey.valid) {
                observation = "completed_origin_unknown";
            }
            if (surveys.size() < 64) {
                surveys.append(
                    QJsonObject{{"duration_s", static_cast<qint64>(survey.duration.count())},
                                {"active", survey.active},
                                {"valid", survey.valid},
                                {"mean_accuracy_m", survey.meanAccuracyMeters ? QJsonValue(*survey.meanAccuracyMeters)
                                                                              : QJsonValue(QJsonValue::Null)},
                                {"observed_after_ms", elapsed.elapsed()},
                                {"origin_assessment", observation},
                                {"fresh_survey_proven", false}});
            }
        };
        GPSDriver driver(options.family, evidence, config, std::move(sinks));
        const bool configured = driver.configure();
        checks.append(check("configure_return", configured ? "passed" : "failed",
                            "Facade return value only, not independent confirmation of every requested setting"));
        stage.insert("configuration_evidence", configurationEvidence(driver));
        auto snapshot = [&] {
            stage.insert("position_messages", positions);
            stage.insert("last_position", lastPosition);
            stage.insert("satellite_messages", satellites);
            stage.insert("correction_frames", correctionFrames);
            QJsonObject messages;
            for (auto it = correctionMessages.cbegin(); it != correctionMessages.cend(); ++it) {
                messages.insert(QString::number(it.key()),
                                QJsonObject{{"frames", it.value()}, {"bytes", correctionBytes.value(it.key())}});
            }
            stage.insert("correction_messages", messages);
            stage.insert("survey_observations", surveys);
            stage.insert("wire_evidence", evidence.evidence());
            if (options.family == GPSType::ublox) {
                stage.insert("requested_setting_observations", evidence.requestedSettings(config, options.passive));
            }
            stage.insert("checks", checks);
        };
        auto checkpoint = [&] {
            snapshot();
            report.insert("active_stage", stage);
            report.insert("stages", results);
            return saveEvidence(report, options.outputPath);
        };
        stage.insert("phase", configured ? "observing" : "configuration_failed");
        if (const QString error = checkpoint(); !error.isEmpty()) {
            stop.cancel();
            return evidenceError(report, error);
        }
        if (!configured) {
            failed = true;
        } else {
            bool receiveFailed = false;
            const auto recordReceive = [&](const GPSReceiveResult& result) {
                if (!result.terminal()) {
                    return false;
                }
                receiveFailed = true;
                failed = true;
                const QString detail =
                    result.error == GPSProtocolError::Protocol || result.error == GPSProtocolError::ConsentRequired
                        ? "terminal_protocol_error"
                        : "terminal_transport_error";
                checks.append(check("receive_outcome", "failed", detail));
                stage.insert("receive_error_detail", result.detail);
                stage.insert("transport_healthy_at_receive_failure", !evidence.fatalError());
                return true;
            };
            operationDeadline.store(now() + options.observeMs + options.timeoutMs);
            if (scripted) {
                injectMeasurements(*receiver, options);
            }
            QElapsedTimer observation;
            observation.start();
            qint64 lastCheckpointMs = 0;
            while (!stop.isCancelled() && !evidence.fatalError() && observation.elapsed() < options.observeMs) {
                const auto result = driver.receiveOutcome(50ms);
                if (recordReceive(result)) {
                    break;
                }
                if (result.error == GPSProtocolError::Cancelled) {
                    failed = true;
                    checks.append(check("observation_window", "failed", "Receive cancelled during observation"));
                    break;
                }
                if (!options.outputPath.isEmpty() && observation.elapsed() - lastCheckpointMs >= 1000) {
                    if (const QString error = checkpoint(); !error.isEmpty()) {
                        stop.cancel();
                        return evidenceError(report, error);
                    }
                    lastCheckpointMs = observation.elapsed();
                }
            }
            if (stop.isCancelled()) {
                failed = true;
                checks.append(check("observation_window", "failed", "Observation interrupted or deadline expired"));
            }
            const bool cancellationRequested =
                options.action == "cancel" || (options.action == "suite" && name == stages.last());
            if (receiveFailed && cancellationRequested) {
                checks.append(check("receive_cancellation", "not_run", "Prior terminal receive failure"));
            }
            if (!failed && !stop.isCancelled() && cancellationRequested) {
                if (scripted) {
                    injectMeasurements(*receiver, options, true);
                }
                operationDeadline.store(now() + options.cancelAfterMs);
                QElapsedTimer cancellation;
                cancellation.start();
                int receiveCalls = 0;
                GPSReceiveResult result;
                do {
                    result = driver.receiveOutcome(2000ms);
                    ++receiveCalls;
                    if (recordReceive(result)) {
                        break;
                    }
                } while (result.error != GPSProtocolError::Cancelled && !stop.isCancelled() && !evidence.fatalError() &&
                         cancellation.elapsed() < options.cancelAfterMs + options.cancelSlackMs);
                const qint64 cancelMs = cancellation.elapsed();
                operationDeadline.store(0);
                const bool timely = !receiveFailed && stop.isCancelled() && !evidence.fatalError() &&
                                    cancelMs < options.cancelAfterMs + options.cancelSlackMs;
                const bool cancelled = timely && result.error == GPSProtocolError::Cancelled;
                checks.append(check("receive_cancellation", cancelled ? "passed" : "failed",
                                    QString("Receive loop returned after %1 ms across %2 calls; stop requested=%3. "
                                            "Cancellation requires a typed cancellation outcome.")
                                        .arg(cancelMs)
                                        .arg(receiveCalls)
                                        .arg(stop.isCancelled())));
                stage.insert("cancellation_ms", cancelMs);
                failed = failed || !cancelled;
                // Cancellation is the final operation; never send a cleanup configuration.
                stop.cancel();
            }
            if (evidence.fatalError()) {
                failed = true;
                checks.append(check("transport_health", "failed", "Transport reports a fatal error"));
            }
            checks.append(check("position_observation", positions > 0 ? "passed" : "inconclusive",
                                "Decoded position messages observed; does not certify fix quality"));
            if (!options.passive && !std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode)) {
                checks.append(check("survey_observation", surveys.isEmpty() ? "inconclusive" : "passed",
                                    "Freshness is reported separately; retained completion is not a fresh survey"));
                checks.append(check("fresh_survey", "inconclusive",
                                    "A short active duration is consistent with freshness, not proof of a restart"));
            }
            checks.append(check("requested_settings_readback", "inconclusive",
                                "Review setting-specific driver evidence and raw replies; unverified values "
                                "must not be promoted to pass"));
            incomplete = true;
        }
        snapshot();
        stage.insert("phase", "finished");
        stage.insert("outcome", failed ? "failed" : "inconclusive");
        results.append(stage);
        report.remove("active_stage");
        report.insert("stages", results);
        if (const QString error = saveEvidence(report, options.outputPath); !error.isEmpty()) {
            stop.cancel();
            return evidenceError(report, error);
        }
        operationDeadline.store(0);
        if (failed || stop.isCancelled()) {
            break;
        }
    }
    report.insert("stages", results);
    report.remove("active_stage");
    report.insert("interrupted", interrupted.test(std::memory_order_relaxed));
    report.insert("deadline_or_cancellation_requested", deadlineExpired.load());
    if (scripted) {
        report.insert("scripted_wire_valid", receiver->wireValid);
        failed = failed || !receiver->wireValid;
    }
    report.insert("outcome", failed ? "failed" : incomplete ? "inconclusive" : "not_run");
    report.insert("limitations",
                  QJsonArray{"No survey restart or full requested-settings verification inferred.",
                             "No radio correction delivery, antenna accuracy, or physical power-cycle "
                             "validation.",
                             "Last requested receiver base settings may remain active; no implicit rollback."});
    return output(report, failed ? 1 : 3, options.outputPath);
}
}  // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("QGCGPSHardwareRunner");
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Opt-in native GPS receiver validation. Default action prints a plan and opens nothing.");
    parser.addHelpOption();
    parser.addOptions({
        {{"a", "action"}, "plan|configure|suite|cancel", "action", "plan"},
        {"transport", "scripted|serial|tcp", "transport", "scripted"},
        {"family", "ublox|trimble|septentrio|femto|unicore|quectel|passive", "family", "ublox"},
        {"role", "base|passive", "role", "base"},
        {"allow-reconfigure", "Authorize physical receiver writes and role changes"},
        {"allow-save", "Explicitly permit LG290P settings to be saved to flash and the receiver restarted"},
        {"compact-rtcm", "Request compact MSM4 instead of MSM7 RTCM observations from a supporting base"},
        {"output", "New JSON evidence path (atomic progress snapshots; never overwrites a previous run)", "path"},
        {"record", "Record each receiver session's raw received and sent bytes into this directory", "directory"},
        {"replay",
         "Decode a recorded receiver stream with --family, as configured for --role and the base options, instead of "
         "a live transport; nothing is configured or written",
         "file"},
        {"replay-baud", "Replay pacing: 0 reads the recording as fast as possible, otherwise at this line rate", "baud",
         "0"},
        {"device", "Explicit serial device path, or host:port for tcp", "path"},
        {"baud", "Serial baud rate (0 for managed detection; passive requires an explicit rate)", "baud", "0"},
        {"survey-duration", "Requested survey minimum seconds", "seconds", "60"},
        {"survey-accuracy", "Requested survey accuracy limit, metres", "metres", "2"},
        {"base-mode", "survey|fixed|receiver-averaging", "mode", "survey"},
        {"averaging-duration", "Receiver-managed maximum averaging seconds (not a minimum or accuracy guarantee)",
         "seconds", "60"},
        {"latitude", "Fixed base latitude, degrees", "degrees"},
        {"longitude", "Fixed base longitude, degrees", "degrees"},
        {"altitude", "Fixed base ellipsoid altitude, metres", "metres"},
        {"observe-ms", "Per-stage observation window", "milliseconds", "1000"},
        {"timeout-ms", "Cancellation deadline for each open/configure operation", "milliseconds", "15000"},
        {"cancel-after-ms", "Delay before requesting blocked-receive cancellation", "milliseconds", "50"},
        {"cancel-slack-ms", "Time a cancelled receive may take to return after the request", "milliseconds", "500"},
        {"model", "Scripted receiver: f9p|m8p", "model", "f9p"},
        {"survey-state", "Scripted observations: fresh|retained|none", "state", "fresh"},
        {"fault", "Scripted fault: none|nak|wrong-readback|cancel|rtcm-nak|rtcm-nak-cancel", "fault", "none"},
    });
    if (!parser.parse(app.arguments())) {
        return output({{"outcome", "rejected"}, {"detail", parser.errorText()}}, 2);
    }
    if (parser.isSet("help")) {
        QTextStream(stdout) << parser.helpText();
        return 0;
    }
    Options options;
    if (const QString error = parseOptions(parser, options); !error.isEmpty()) {
        return output({{"outcome", "rejected"}, {"detail", error}}, 2);
    }
    std::signal(SIGINT, interruptHandler);
    std::signal(SIGTERM, interruptHandler);
    return run(options);
}
