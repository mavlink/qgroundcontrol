#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <memory>
#include <thread>
#include <utility>

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
#include <QtCore/QTextStream>
#include <QtCore/QtEndian>

#include "GPSDriver.h"
#include "GPSEvidenceTransport.h"
#include "GPSReceiverCapabilities.h"
#include "MonotonicClock.h"
#include "RTCMFramer.h"
#include "Support/ScriptedReceiver.h"
#include "Support/UBXReceiverModel.h"
#include "TCPGPSTransport.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#endif

namespace {
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
    GPSType family = GPSType::ublox;
    GPSReceiverConfig config;
    int observeMs = 1000;
    int cancelAfterMs = 50;
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

QJsonObject requestedConfig(const GPSReceiverConfig& config)
{
    QJsonObject result{{"role", config.role == GPSReceiverConfig::Role::RTKBase   ? "base"
                                : config.role == GPSReceiverConfig::Role::Passive ? "passive"
                                                                                  : "invalid"},
                       {"baud_rate", static_cast<qint64>(config.baudRate)},
                       {"allow_persistent_changes", config.allowPersistentChanges}};
    if (config.role == GPSReceiverConfig::Role::RTKBase) {
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
            result.insert("averaging_maximum_s",
                          static_cast<qint64>(
                              std::get<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode).maximumDurationSecs));
            result.insert("survey_accuracy_m", QJsonValue::Null);
        } else {
            result.insert("base_mode", "survey");
            result.insert("survey_duration_s",
                          static_cast<qint64>(std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).durationSecs));
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
    options.config.role = role == "base"      ? GPSReceiverConfig::Role::RTKBase
                          : role == "passive" ? GPSReceiverConfig::Role::Passive
                                              : static_cast<GPSReceiverConfig::Role>(-1);
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
    options.timeoutMs = integer("timeout-ms", 100, 120000);
    if (options.family == GPSType::quectel && !parser.isSet("timeout-ms")) {
        // Role and base changes can require multiple receiver restarts within the driver's 45-second budget.
        options.timeoutMs = 60000;
    }
    options.config.baudRate = static_cast<uint32_t>(integer("baud", 0, 4000000));
    options.config.allowPersistentChanges = parser.isSet("allow-save");
    options.config.base.compactObservations = parser.isSet("compact-rtcm");
    if (options.config.base.compactObservations && options.config.role != GPSReceiverConfig::Role::RTKBase) {
        return "--compact-rtcm requires --role base";
    }
    const QString baseMode = parser.value("base-mode");
    if (!QStringList{"survey", "fixed", "receiver-averaging"}.contains(baseMode)) {
        return "Unsupported base mode";
    }
    if (options.config.role == GPSReceiverConfig::Role::RTKBase) {
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
                .altitudeMeters = static_cast<float>(coordinate("altitude"))};
        } else if (baseMode == "receiver-averaging") {
            options.config.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
            std::get<GPSBaseStationConfig::ReceiverAveraging>(options.config.base.mode).maximumDurationSecs =
                static_cast<uint32_t>(integer("averaging-duration", 1, 3600));
        } else {
            std::get<GPSBaseStationConfig::SurveyIn>(options.config.base.mode).durationSecs =
                integer("survey-duration", 1, 86400);
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
    if (!valid || gpsValidateReceiverConfig(options.family, options.config) != GPSReceiverConfigError::None) {
        return "Invalid receiver configuration or numeric option";
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
    if (options.action != "plan" && options.transport != "scripted" && !parser.isSet("allow-reconfigure")) {
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
            case GPSConfigurationOutcome::Pending:
                outcome = "pending";
                break;
            case GPSConfigurationOutcome::Written:
                outcome = "transport_written";
                break;
            case GPSConfigurationOutcome::Acknowledged:
                outcome = "acknowledged";
                break;
            case GPSConfigurationOutcome::ReadbackVerified:
                outcome = "readback_verified";
                break;
            case GPSConfigurationOutcome::Rejected:
                outcome = "rejected";
                break;
            case GPSConfigurationOutcome::TimedOut:
                outcome = "timed_out";
                break;
            case GPSConfigurationOutcome::Cancelled:
                outcome = "cancelled";
                break;
            case GPSConfigurationOutcome::TransportError:
                outcome = "transport_error";
                break;
        }
        commands.append(QJsonObject{{"command", QString::fromStdString(evidence.command)},
                                    {"outcome", outcome},
                                    {"required", evidence.required},
                                    {"started_at_us", static_cast<qint64>(evidence.startedAtUs)},
                                    {"finished_at_us", static_cast<qint64>(evidence.finishedAtUs)},
                                    {"accepted_bytes", evidence.acceptedBytes},
                                    {"written_bytes", evidence.writtenBytes},
                                    {"uncertain_bytes", evidence.uncertainBytes}});
    }
    return {{"source", "native_driver"}, {"commands", commands}};
}

std::unique_ptr<GPSTransport> physicalTransport(const Options& options, const std::atomic_bool& stop)
{
    if (options.transport == "tcp") {
        const auto separator = options.device.lastIndexOf(':');
        return std::make_unique<TCPGPSTransport>(
            options.device.left(separator), static_cast<quint16>(options.device.mid(separator + 1).toUInt()), stop);
    }
#ifndef QGC_NO_SERIAL_LINK
    if (options.transport == "serial") {
        return std::make_unique<SerialGPSTransport>(options.device, stop);
    }
#endif
    return {};
}

void injectMeasurements(UBXReceiverModel& receiver, const Options& options, const GPSReceiverConfig& config,
                        bool cancellation = false)
{
    const bool rejectActivation = options.fault == "rtcm-nak" || (cancellation && options.fault == "rtcm-nak-cancel");
    receiver.rejectRtcmActivation = rejectActivation;
    if (config.role == GPSReceiverConfig::Role::RTKBase && (options.surveyState != "none" || rejectActivation)) {
        QByteArray survey(40, '\0');
        const bool retained = options.surveyState == "retained" || rejectActivation;
        qToLittleEndian<quint32>(retained ? 600 : 0, survey.data() + 8);
        qToLittleEndian<quint32>(10000, survey.data() + 28);
        qToLittleEndian<quint32>(retained ? 600 : 1, survey.data() + 32);
        survey[36] = retained ? 1 : 0;
        survey[37] = retained ? 0 : 1;
        receiver.queueFrame(0x01, 0x3b, survey);
    }
    QByteArray position(92, '\0');
    qToLittleEndian<quint32>(cancellation ? 1000 : 0, position.data());
    position[20] = 3;
    position[21] = 1;
    position[23] = 12;
    qToLittleEndian<qint32>(85000000, position.data() + 24);
    qToLittleEndian<qint32>(473000000, position.data() + 28);
    qToLittleEndian<quint32>(1000, position.data() + 40);
    receiver.queueFrame(0x01, 0x07, position);
}

int run(const Options& options)
{
    const bool scripted = options.transport == "scripted";
    QJsonObject report{{"backend", "native"},
                       {"action", options.action},
                       {"transport", options.transport},
                       {"evidence_origin", scripted ? "scripted" : "physical_transport"},
                       {"physical_hardware_verified", false},
                       {"requested", requestedConfig(options.config)}};
    report.insert("receiver_family", options.familyName);
    report.insert("receive_outcome_semantics", "typed_native");
    report.insert("schema_version", 1);
    report.insert("deadlines", QJsonObject{{"open_and_configure_ms", options.timeoutMs},
                                           {"observation_ms", options.observeMs},
                                           {"cancel_after_ms", options.cancelAfterMs},
                                           {"cooperative_cancellation", true}});
    report.insert("started_at_utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (scripted) {
        report.insert(
            "script",
            QJsonObject{{"model", options.model}, {"survey_state", options.surveyState}, {"fault", options.fault}});
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

    std::atomic_bool stop = false;
    std::atomic_bool deadlineExpired = false;
    std::atomic<qint64> operationDeadline = 0;
    const auto now = [] { return static_cast<qint64>(MonotonicClock::nowUs() / 1000); };
    std::jthread watchdog([&](std::stop_token token) {
        while (!token.stop_requested()) {
            const auto deadline = operationDeadline.load();
            const bool signalReceived = interrupted.test(std::memory_order_relaxed);
            if (signalReceived || (deadline != 0 && now() >= deadline)) {
                deadlineExpired.store(!signalReceived);
                stop.store(true);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    std::unique_ptr<UBXReceiverModel> receiver;
    std::unique_ptr<ScriptedReceiver> scriptedTransport;
    std::unique_ptr<GPSTransport> physical;
    if (scripted) {
        receiver = std::make_unique<UBXReceiverModel>(options.model == "f9p" ? UBXReceiverModel::Receiver::F9P
                                                                             : UBXReceiverModel::Receiver::M8PBase);
        scriptedTransport = std::make_unique<ScriptedReceiver>(stop, *receiver);
        if (options.fault == "nak") {
            receiver->disableReply = UBXReceiverModel::DisableReply::Nak;
        } else if (options.fault == "wrong-readback") {
            receiver->readbackReply = UBXReceiverModel::ReadbackReply::WrongValue;
        } else if (options.fault == "cancel") {
            receiver->disableReply = UBXReceiverModel::DisableReply::Cancelled;
        }
    } else {
        physical = physicalTransport(options, stop);
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
            physical.reset();
            physical = physicalTransport(options, stop);
        }
        GPSTransport& transport = scripted ? static_cast<GPSTransport&>(*scriptedTransport) : *physical;
        GPSEvidenceTransport evidence(transport, stop);
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
        stage.insert("requested", requestedConfig(config));
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
        sinks.onRTCM = [&](std::span<const uint8_t> frame) {
            ++correctionFrames;
            const int messageId = RTCMFramer::frameMessageId(frame);
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
                stage.insert("requested_setting_observations", evidence.requestedSettings(config));
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
            stop.store(true);
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
                const QString detail = result.status == GPSReceiveStatus::ProtocolError    ? "terminal_protocol_error"
                                       : result.status == GPSReceiveStatus::TransportError ? "terminal_transport_error"
                                                                                           : "not_configured";
                checks.append(check("receive_outcome", "failed", detail));
                stage.insert("receive_error_detail", result.detail);
                stage.insert("transport_healthy_at_receive_failure", !evidence.fatalError());
                return true;
            };
            operationDeadline.store(now() + options.observeMs + options.timeoutMs);
            if (scripted) {
                injectMeasurements(*receiver, options, config);
            }
            QElapsedTimer observation;
            observation.start();
            qint64 lastCheckpointMs = 0;
            while (!stop.load() && !evidence.fatalError() && observation.elapsed() < options.observeMs) {
                const auto result = driver.receiveOutcome(50);
                if (recordReceive(result)) {
                    break;
                }
                if (result.status == GPSReceiveStatus::Cancelled) {
                    failed = true;
                    checks.append(check("observation_window", "failed", "Receive cancelled during observation"));
                    break;
                }
                if (!options.outputPath.isEmpty() && observation.elapsed() - lastCheckpointMs >= 1000) {
                    if (const QString error = checkpoint(); !error.isEmpty()) {
                        stop.store(true);
                        return evidenceError(report, error);
                    }
                    lastCheckpointMs = observation.elapsed();
                }
            }
            if (stop.load()) {
                failed = true;
                checks.append(check("observation_window", "failed", "Observation interrupted or deadline expired"));
            }
            const bool cancellationRequested =
                options.action == "cancel" || (options.action == "suite" && name == stages.last());
            if (receiveFailed && cancellationRequested) {
                checks.append(check("receive_cancellation", "not_run", "Prior terminal receive failure"));
            }
            if (!failed && !stop.load() && cancellationRequested) {
                if (scripted) {
                    injectMeasurements(*receiver, options, config, true);
                }
                operationDeadline.store(now() + options.cancelAfterMs);
                QElapsedTimer cancellation;
                cancellation.start();
                int receiveCalls = 0;
                GPSReceiveResult result;
                do {
                    result = driver.receiveOutcome(2000);
                    ++receiveCalls;
                    if (recordReceive(result)) {
                        break;
                    }
                } while (result.status != GPSReceiveStatus::Cancelled && !stop.load() && !evidence.fatalError() &&
                         cancellation.elapsed() < options.cancelAfterMs + 500);
                const qint64 cancelMs = cancellation.elapsed();
                operationDeadline.store(0);
                const bool timely =
                    !receiveFailed && stop.load() && !evidence.fatalError() && cancelMs < options.cancelAfterMs + 500;
                const bool cancelled = timely && result.status == GPSReceiveStatus::Cancelled;
                checks.append(check("receive_cancellation", cancelled ? "passed" : "failed",
                                    QString("Receive loop returned after %1 ms across %2 calls; stop requested=%3. "
                                            "Cancellation requires a typed cancellation outcome.")
                                        .arg(cancelMs)
                                        .arg(receiveCalls)
                                        .arg(stop.load())));
                stage.insert("cancellation_ms", cancelMs);
                failed = failed || !cancelled;
                // Cancellation is the final operation; never send a cleanup configuration.
                stop.store(true);
            }
            if (evidence.fatalError()) {
                failed = true;
                checks.append(check("transport_health", "failed", "Transport reports a fatal error"));
            }
            checks.append(check("position_observation", positions > 0 ? "passed" : "inconclusive",
                                "Decoded position messages observed; does not certify fix quality"));
            if (config.role == GPSReceiverConfig::Role::RTKBase &&
                !std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode)) {
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
            stop.store(true);
            return evidenceError(report, error);
        }
        operationDeadline.store(0);
        if (failed || stop.load()) {
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
