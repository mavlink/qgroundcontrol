#include "GPSGoldenTranscriptTest.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QtEndian>

#include "Scenarios/GoldenScenarios.h"

namespace {

using namespace GPSGoldenScenario;

bool updateRequested()
{
    return qEnvironmentVariableIntValue("QGC_GPS_GOLDEN_UPDATE") == 1;
}

// ---------------------------------------------------------------------------------------------------------------
// Decode table: every corpus and fixture file through each receiver family that can meet it.

struct DecoderDef
{
    const char* name;
    GPSType type;
    BenchFactory bench;
    GPSReceiverConfig config;
};

const std::vector<DecoderDef>& decoders()
{
    static const std::vector<DecoderDef> rows{
        {"ublox", GPSType::ublox, ubxWire(), fixedBase(47, 8, 500, 1, 115200)},
        {"ublox-legacy", GPSType::ublox, ubxWire([](UBXBench& b) {
             b.model.legacy = true;
             b.model.module = "NEO-M8P";
         }),
         fixedBase(47, 8, 500, 1, 115200)},
        {"septentrio", GPSType::septentrio, sbf(), fixedBase(47, 8, 500, 1)},
        {"trimble", GPSType::trimble, ashtech(), surveyIn(1, 100)},
        {"femto", GPSType::femto, femto(), fixedBase(47, 8, 500, 1)},
        // The Unicore and Quectel seeds carry evidence for the fixed ECEF position (0, 6378237, 0).
        {"unicore-fixed", GPSType::unicore, unicore(), fixedBase(0, 90, 100, 0)},
        {"unicore-averaging", GPSType::unicore, unicore(), averaging(60)},
        {"quectel-survey", GPSType::quectel, quectel([](QuectelBench& b) { b.model.role = 2; }), surveyIn(15, 60)},
        {"quectel-fixed", GPSType::quectel, quectel([](QuectelBench& b) {
             b.model.role = 2;
             b.model.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
         }),
         fixedBase(0, 90, 100, 0)},
        {"passive", GPSType::passive, passive(), passiveInput(115200)},
    };
    return rows;
}

/// Empty for a data file no decoder claims; the inventory test rejects those.
QStringList decodersFor(const QString& file)
{
    if (file.endsWith(QLatin1String(".ubx")) || file.startsWith(QLatin1String("ubx-"))) {
        return {QStringLiteral("ublox"), QStringLiteral("ublox-legacy")};
    }
    if (file == QLatin1String("mixed.gps")) {
        return {QStringLiteral("ublox"), QStringLiteral("ublox-legacy"), QStringLiteral("passive")};
    }
    if (file.endsWith(QLatin1String(".sbf"))) {
        return {QStringLiteral("septentrio")};
    }
    if (file.startsWith(QLatin1String("femto-")) && file.endsWith(QLatin1String(".bin"))) {
        return {QStringLiteral("femto")};
    }
    if (file.startsWith(QLatin1String("synthetic-unicore-"))) {
        return {QStringLiteral("unicore-fixed"), QStringLiteral("unicore-averaging")};
    }
    if (file.startsWith(QLatin1String("synthetic-quectel-"))) {
        return {QStringLiteral("quectel-survey"), QStringLiteral("quectel-fixed")};
    }
    if (file.startsWith(QLatin1String("pashr"))) {
        return {QStringLiteral("trimble"), QStringLiteral("passive")};
    }
    if (file.endsWith(QLatin1String(".nmea"))) {
        return {QStringLiteral("passive"), QStringLiteral("trimble"), QStringLiteral("femto"),
                QStringLiteral("unicore-fixed"), QStringLiteral("quectel-survey")};
    }
    return {};
}

struct DataFile
{
    QString directory;
    QString name;
    QByteArray bytes;
};

const std::vector<DataFile>& dataFiles()
{
    static const std::vector<DataFile> files = [] {
        const QStringList suffixes{QStringLiteral("ascii"), QStringLiteral("bin"), QStringLiteral("gps"),
                                   QStringLiteral("nmea"),  QStringLiteral("sbf"), QStringLiteral("ubx")};
        std::vector<DataFile> result;
        const std::pair<QString, const char*> directories[] = {{QStringLiteral("corpus"), GPS_CORPUS_DIR},
                                                               {QStringLiteral("fixtures"), GPS_FIXTURE_DIR}};
        for (const auto& [label, path] : directories) {
            QStringList names = QDir(QString::fromUtf8(path)).entryList(QDir::Files);
            std::sort(names.begin(), names.end());
            for (const auto& name : names) {
                if (suffixes.contains(QFileInfo(name).suffix())) {
                    result.push_back({label, name, dataFile(path, name)});
                }
            }
        }
        return result;
    }();
    return files;
}

// ---------------------------------------------------------------------------------------------------------------
// Transcript format. A scenario golden lists the request and stream inputs, then a `configure` and (after success)
// a `stream` phase, each with:
//   result    outcome, requested->final baud, sticky failure, readiness, virtual duration
//   wire      `baud <rate> <status>` and `write <command label|-> <bytes>` with hex (and text when printable)
//   evidence  one line per completed command: label, outcome, required/optional, byte counts, duration
//   events    decoded events; host timestamps appear only as receipt=0/1
// A decode golden lists, per data file, the events of each chunking (1, 7, whole) and those after the freshness
// horizon; chunkings with identical results share one block.

const QString HEADER = QStringLiteral(
    "# Pinned native GPS protocol behaviour. Rewrite only for a justified change (QGC_GPS_GOLDEN_UPDATE=1).");

QString typeName(GPSType type)
{
    return QString::fromLatin1(QMetaEnum::fromType<GPSType>().valueToKey(static_cast<int>(type)));
}

QString goldenName(const ScenarioDef& scenario)
{
    return typeName(scenario.type) + u'/' + QString::fromLatin1(scenario.name);
}

QString decimal(double value, int decimals)
{
    if (std::isnan(value)) {
        return QStringLiteral("nan");
    }
    if (std::isinf(value)) {
        return value > 0 ? QStringLiteral("inf") : QStringLiteral("-inf");
    }
    // Negative zero and values that round to zero print as zero.
    if (std::abs(value) < 0.5 * std::pow(10.0, -decimals)) {
        value = 0;
    }
    return QString::number(value, 'f', decimals);
}

template <typename T>
QString optionalNumber(const std::optional<T>& value)
{
    return value ? QString::number(*value) : QStringLiteral("-");
}

QString flag(bool value)
{
    return value ? QStringLiteral("1") : QStringLiteral("0");
}

/// Host receipt times are normalized to whether the receipt exists.
QString receipt(uint64_t timestampUs)
{
    return flag(timestampUs != 0);
}

QString quotedBytes(QByteArrayView bytes)
{
    QString text = QStringLiteral("\"");
    for (const char character : bytes) {
        const auto byte = static_cast<uint8_t>(character);
        switch (byte) {
            case '\\':
                text += QStringLiteral("\\\\");
                break;
            case '"':
                text += QStringLiteral("\\\"");
                break;
            case '\r':
                text += QStringLiteral("\\r");
                break;
            case '\n':
                text += QStringLiteral("\\n");
                break;
            case '\t':
                text += QStringLiteral("\\t");
                break;
            default:
                if (byte >= 0x20 && byte < 0x7f) {
                    text += QChar(byte);
                } else {
                    text += QStringLiteral("\\x%1").arg(byte, 2, 16, QChar(u'0'));
                }
        }
    }
    return text + u'"';
}

QString quotedText(const QString& text)
{
    return quotedBytes(text.toUtf8());
}

QString quotedLabel(const std::string& text)
{
    return quotedBytes(QByteArrayView(text.data(), static_cast<qsizetype>(text.size())));
}

bool printable(QByteArrayView bytes)
{
    return !bytes.isEmpty() && std::all_of(bytes.begin(), bytes.end(), [](char character) {
        const auto byte = static_cast<uint8_t>(character);
        return (byte >= 0x20 && byte < 0x7f) || byte == '\r' || byte == '\n' || byte == '\t';
    });
}

QString milliseconds(uint64_t fromUs, uint64_t toUs)
{
    const int64_t delta = static_cast<int64_t>(toUs - fromUs);
    return QString::number(delta >= 0 ? (delta + 500) / 1000 : -((-delta + 500) / 1000)) + QStringLiteral("ms");
}

QString nameOf(GPSConfigurationOutcome outcome)
{
    switch (outcome) {
        case GPSConfigurationOutcome::Pending:
            return QStringLiteral("pending");
        case GPSConfigurationOutcome::Written:
            return QStringLiteral("written");
        case GPSConfigurationOutcome::Acknowledged:
            return QStringLiteral("acknowledged");
        case GPSConfigurationOutcome::ReadbackVerified:
            return QStringLiteral("readback-verified");
        case GPSConfigurationOutcome::Rejected:
            return QStringLiteral("rejected");
        case GPSConfigurationOutcome::TimedOut:
            return QStringLiteral("timed-out");
        case GPSConfigurationOutcome::Cancelled:
            return QStringLiteral("cancelled");
        case GPSConfigurationOutcome::TransportError:
            return QStringLiteral("transport-error");
    }
    return QStringLiteral("outcome-%1").arg(static_cast<int>(outcome));
}

QString nameOf(GPSWriteStatus status)
{
    switch (status) {
        case GPSWriteStatus::Completed:
            return QStringLiteral("completed");
        case GPSWriteStatus::TimedOut:
            return QStringLiteral("timed-out");
        case GPSWriteStatus::Cancelled:
            return QStringLiteral("cancelled");
        case GPSWriteStatus::Error:
            return QStringLiteral("error");
        case GPSWriteStatus::Unsupported:
            return QStringLiteral("unsupported");
        case GPSWriteStatus::InvalidData:
            return QStringLiteral("invalid-data");
    }
    return QStringLiteral("write-%1").arg(static_cast<int>(status));
}

QString nameOf(GPSBaudStatus status)
{
    switch (status) {
        case GPSBaudStatus::Configured:
            return QStringLiteral("configured");
        case GPSBaudStatus::Unsupported:
            return QStringLiteral("unsupported");
        case GPSBaudStatus::Cancelled:
            return QStringLiteral("cancelled");
        case GPSBaudStatus::Error:
            return QStringLiteral("error");
    }
    return QStringLiteral("baud-%1").arg(static_cast<int>(status));
}

QString nameOf(GPSGolden::Failure failure)
{
    switch (failure) {
        case GPSGolden::Failure::None:
            return QStringLiteral("none");
        case GPSGolden::Failure::Cancelled:
            return QStringLiteral("cancelled");
        case GPSGolden::Failure::Transport:
            return QStringLiteral("transport");
        case GPSGolden::Failure::Protocol:
            return QStringLiteral("protocol");
        case GPSGolden::Failure::InvalidArgument:
            return QStringLiteral("invalid-argument");
        case GPSGolden::Failure::ConsentRequired:
            return QStringLiteral("consent-required");
    }
    return QStringLiteral("failure-%1").arg(static_cast<int>(failure));
}

QString nameOf(GPSFixQuality fix)
{
    switch (fix) {
        case GPSFixQuality::Unknown:
            return QStringLiteral("unknown");
        case GPSFixQuality::NoFix:
            return QStringLiteral("none");
        case GPSFixQuality::Fix2D:
            return QStringLiteral("2d");
        case GPSFixQuality::Fix3D:
            return QStringLiteral("3d");
        case GPSFixQuality::Differential:
            return QStringLiteral("differential");
        case GPSFixQuality::RTKFloat:
            return QStringLiteral("rtk-float");
        case GPSFixQuality::RTKFixed:
            return QStringLiteral("rtk-fixed");
        case GPSFixQuality::Extrapolated:
            return QStringLiteral("extrapolated");
    }
    return QStringLiteral("fix-%1").arg(static_cast<int>(fix));
}

QString nameOf(GPSConstellation constellation)
{
    switch (constellation) {
        case GPSConstellation::Unknown:
            return QStringLiteral("unknown");
        case GPSConstellation::GPS:
            return QStringLiteral("gps");
        case GPSConstellation::GLONASS:
            return QStringLiteral("glonass");
        case GPSConstellation::Galileo:
            return QStringLiteral("galileo");
        case GPSConstellation::BeiDou:
            return QStringLiteral("beidou");
        case GPSConstellation::QZSS:
            return QStringLiteral("qzss");
        case GPSConstellation::SBAS:
            return QStringLiteral("sbas");
        case GPSConstellation::NavIC:
            return QStringLiteral("navic");
    }
    return QStringLiteral("constellation-%1").arg(static_cast<int>(constellation));
}

QString nameOf(GPSIntegrityReport::JammingState state)
{
    static const char* const names[] = {"unknown", "ok", "warning", "critical"};
    const auto index = static_cast<size_t>(state);
    return index < std::size(names) ? QString::fromLatin1(names[index]) : QString::number(static_cast<int>(state));
}

QString nameOf(GPSIntegrityReport::SpoofingState state)
{
    static const char* const names[] = {"unknown", "none", "indicated", "multiple"};
    const auto index = static_cast<size_t>(state);
    return index < std::size(names) ? QString::fromLatin1(names[index]) : QString::number(static_cast<int>(state));
}

QString nameOf(GPSIntegrityReport::CorrectionUse use)
{
    static const char* const names[] = {"unknown", "not-used", "used"};
    const auto index = static_cast<size_t>(use);
    return index < std::size(names) ? QString::fromLatin1(names[index]) : QString::number(static_cast<int>(use));
}

QString nameOf(GPSIntegrityReport::CorrectionProtocol protocol)
{
    static const char* const names[] = {"unknown", "rtcm3", "spartn", "has", "pmp", "qzss-l6"};
    const auto index = static_cast<size_t>(protocol);
    return index < std::size(names) ? QString::fromLatin1(names[index]) : QString::number(static_cast<int>(protocol));
}

QString describeRequest(GPSType type, const GPSReceiverConfig& config)
{
    const QString mode = std::visit(
        [](const auto& selected) -> QString {
            using Mode = std::decay_t<decltype(selected)>;
            if constexpr (std::is_same_v<Mode, GPSBaseStationConfig::SurveyIn>) {
                return QStringLiteral("survey-in accuracy=%1m duration=%2s")
                    .arg(decimal(selected.accuracyMeters, 4))
                    .arg(selected.duration.count());
            } else if constexpr (std::is_same_v<Mode, GPSBaseStationConfig::Fixed>) {
                return QStringLiteral("fixed lat=%1 lon=%2 alt=%3 accuracy=%4m")
                    .arg(decimal(selected.position.latitudeDegrees, 8), decimal(selected.position.longitudeDegrees, 8),
                         decimal(selected.position.altitudeMeters, 4), decimal(selected.accuracyMeters, 5));
            } else {
                return QStringLiteral("averaging maximum=%1s").arg(selected.maximumDuration.count());
            }
        },
        config.base.mode);
    return QStringLiteral("type=%1 role=%2 mode=%3 compact=%4 persistent=%5 baud=%6")
        .arg(typeName(type),
             config.role == GPSReceiverConfig::Role::Passive ? QStringLiteral("passive") : QStringLiteral("rtk-base"),
             mode, flag(config.base.compactObservations), flag(config.allowPersistentChanges))
        .arg(config.baudRate);
}

uint32_t fnv1a(QByteArrayView bytes)
{
    uint32_t hash = 2166136261u;
    for (const char byte : bytes) {
        hash = (hash ^ static_cast<uint8_t>(byte)) * 16777619u;
    }
    return hash;
}

QString formatEvent(const GPSGolden::Event& event)
{
    return std::visit(
        [](const auto& report) -> QString {
            using Report = std::decay_t<decltype(report)>;
            if constexpr (std::is_same_v<Report, GPSGolden::Position>) {
                const auto& n = report.navigation;
                return QStringList{
                    QStringLiteral("position fix=") + nameOf(n.fixType),
                    QStringLiteral("lat=") + decimal(n.latitudeDegrees, 8),
                    QStringLiteral("lon=") + decimal(n.longitudeDegrees, 8),
                    QStringLiteral("msl=") + decimal(n.altitudeMslMeters, 4),
                    QStringLiteral("ellipsoid=") + decimal(n.altitudeEllipsoidMeters, 4),
                    QStringLiteral("hacc=") + decimal(n.horizontalAccuracyMeters, 4),
                    QStringLiteral("vacc=") + decimal(n.verticalAccuracyMeters, 4),
                    QStringLiteral("hdop=") + decimal(n.horizontalDop, 3),
                    QStringLiteral("vdop=") + decimal(n.verticalDop, 3),
                    QStringLiteral("speed=") + decimal(n.speedMetersPerSecond, 4),
                    QStringLiteral("course=") + decimal(n.courseRadians, 6),
                    QStringLiteral("heading=") + decimal(n.headingRadians, 6),
                    QStringLiteral("headingacc=") + decimal(n.headingAccuracyRadians, 6),
                    QStringLiteral("used=") + optionalNumber(n.satellitesUsed),
                    QStringLiteral("utc=") + QString::number(n.utcTimeUs),
                    QStringLiteral("velocity=") + flag(report.velocityValid),
                    QStringLiteral("receipt=") + receipt(n.timestampUs),
                }
                    .join(u' ');
            } else if constexpr (std::is_same_v<Report, GPSIntegrityReport>) {
                return QStringList{
                    QStringLiteral("integrity receipt=") + receipt(report.timestampUs),
                    QStringLiteral("jamming=%1/%2")
                        .arg(nameOf(report.jamming.state), receipt(report.jamming.timestampUs)),
                    QStringLiteral("spoofing=%1/%2")
                        .arg(nameOf(report.spoofing.state), receipt(report.spoofing.timestampUs)),
                    QStringLiteral("rf=noise:%1,agc:%2,indicator:%3/%4")
                        .arg(optionalNumber(report.rf.noisePerMillisecond),
                             optionalNumber(report.rf.automaticGainControl), optionalNumber(report.rf.jammingIndicator),
                             receipt(report.rf.timestampUs)),
                    QStringLiteral("corrections=%1,crc-failed:%2,%3/%4")
                        .arg(nameOf(report.corrections.use),
                             report.corrections.crcFailed ? flag(*report.corrections.crcFailed) : QStringLiteral("-"),
                             nameOf(report.corrections.protocol), receipt(report.corrections.timestampUs)),
                }
                    .join(u' ');
            } else if constexpr (std::is_same_v<Report, GPSGolden::Satellites>) {
                QStringList parts{QStringLiteral("satellites full=") + flag(report.fullSnapshot)};
                for (const auto& system : report.systems) {
                    parts << QStringLiteral("%1:view=%2/%3,use=%4/%5")
                                 .arg(nameOf(system.constellation))
                                 .arg(system.inView)
                                 .arg(receipt(system.inViewTimestampUs), optionalNumber(system.inUse),
                                      receipt(system.inUseTimestampUs));
                }
                return parts.join(u' ');
            } else if constexpr (std::is_same_v<Report, GPSGolden::SatelliteUsage>) {
                return QStringLiteral("usage used=%1 receipt=%2")
                    .arg(optionalNumber(report.used), receipt(report.timestampUs));
            } else if constexpr (std::is_same_v<Report, GPSGolden::Survey>) {
                const auto& survey = report.survey;
                return QStringList{
                    QStringLiteral("survey active=") + flag(survey.active),
                    QStringLiteral("valid=") + flag(survey.valid),
                    QStringLiteral("duration=%1s").arg(survey.duration.count()),
                    QStringLiteral("accuracy=") +
                        (survey.meanAccuracyMeters ? decimal(*survey.meanAccuracyMeters, 4) : QStringLiteral("-")),
                    QStringLiteral("lat=") + decimal(survey.position.latitudeDegrees, 8),
                    QStringLiteral("lon=") + decimal(survey.position.longitudeDegrees, 8),
                    QStringLiteral("alt=") + decimal(survey.position.altitudeMeters, 4),
                    QStringLiteral("receipt=") + receipt(report.timestampUs),
                }
                    .join(u' ');
            } else {
                const QByteArray& frame = report.frame;
                const int message = frame.size() >= 5
                                        ? (static_cast<uint8_t>(frame[3]) << 4) | (static_cast<uint8_t>(frame[4]) >> 4)
                                        : -1;
                return QStringLiteral("rtcm type=%1 size=%2 fnv=%3")
                    .arg(message)
                    .arg(frame.size())
                    .arg(fnv1a(frame), 8, 16, QChar(u'0'));
            }
        },
        event);
}

/// A write belongs to the command whose evidence counts its accepted bytes; other writes stay unlabelled.
std::vector<std::optional<std::string>> commandLabels(const std::vector<GPSGolden::Record>& records)
{
    std::vector<std::optional<std::string>> labels(records.size());
    std::vector<size_t> pending;
    for (size_t index = 0; index < records.size(); ++index) {
        const auto& value = records[index].value;
        if (std::holds_alternative<GPSGolden::Write>(value)) {
            pending.push_back(index);
            continue;
        }
        const auto* evidence = std::get_if<GPSConfigurationEvidence>(&value);
        if (!evidence) {
            continue;
        }
        int remaining = evidence->acceptedBytes;
        for (auto write = pending.rbegin(); write != pending.rend() && remaining > 0; ++write) {
            labels[*write] = evidence->command;
            remaining -= std::get<GPSGolden::Write>(records[*write].value).acceptedBytes;
        }
        if (evidence->acceptedBytes == 0 && !pending.empty()) {
            const auto& last = std::get<GPSGolden::Write>(records[pending.back()].value);
            if (last.acceptedBytes == 0 && last.status != GPSWriteStatus::Completed) {
                labels[pending.back()] = evidence->command;
            }
        }
        pending.clear();
    }
    return labels;
}

/// Write-call boundaries are not behaviour: contiguous writes of one command form one entry.
QStringList wireLines(const GPSGolden::Run& run, GPSGolden::Phase phase,
                      const std::vector<std::optional<std::string>>& labels)
{
    struct Entry
    {
        std::optional<std::string> label;
        GPSGolden::Write write;
    };

    QStringList lines;
    std::optional<Entry> entry;
    const auto flush = [&lines, &entry] {
        if (!entry) {
            return;
        }
        const auto& write = entry->write;
        QString line = QStringLiteral("    write %1 %2")
                           .arg(entry->label ? quotedLabel(*entry->label) : QStringLiteral("-"))
                           .arg(write.bytes.size());
        if (write.status != GPSWriteStatus::Completed || write.acceptedBytes != write.bytes.size() ||
            write.writtenBytes != write.bytes.size()) {
            line += QStringLiteral(" status=%1 accepted=%2 written=%3")
                        .arg(nameOf(write.status))
                        .arg(write.acceptedBytes)
                        .arg(write.writtenBytes);
        }
        lines << line;
        for (qsizetype offset = 0; offset < write.bytes.size(); offset += 32) {
            lines << QStringLiteral("      ") + QString::fromLatin1(write.bytes.mid(offset, 32).toHex());
        }
        if (printable(write.bytes)) {
            lines << QStringLiteral("      text ") + quotedBytes(write.bytes);
        }
        entry.reset();
    };
    for (size_t index = 0; index < run.records.size(); ++index) {
        const auto& record = run.records[index];
        if (record.phase != phase) {
            continue;
        }
        if (const auto* write = std::get_if<GPSGolden::Write>(&record.value)) {
            if (entry && entry->label == labels[index]) {
                entry->write.bytes += write->bytes;
                entry->write.acceptedBytes += write->acceptedBytes;
                entry->write.writtenBytes += write->writtenBytes;
                if (entry->write.status == GPSWriteStatus::Completed) {
                    entry->write.status = write->status;
                }
            } else {
                flush();
                entry = Entry{labels[index], *write};
            }
        } else if (const auto* baud = std::get_if<GPSGolden::Baud>(&record.value)) {
            flush();
            lines << QStringLiteral("    baud %1 %2").arg(baud->rate).arg(nameOf(baud->status));
        } else if (std::holds_alternative<GPSConfigurationEvidence>(record.value)) {
            flush();
        }
    }
    flush();
    return lines;
}

void appendPhase(QStringList& lines, const GPSGolden::Run& run, GPSGolden::Phase phase,
                 const std::vector<std::optional<std::string>>& labels)
{
    lines << QStringLiteral("  wire");
    lines << wireLines(run, phase, labels);
    lines << QStringLiteral("  evidence");
    for (const auto& record : run.records) {
        const auto* evidence = std::get_if<GPSConfigurationEvidence>(&record.value);
        if (record.phase != phase || !evidence) {
            continue;
        }
        lines << QStringLiteral("    %1 %2 %3 accepted=%4 written=%5 uncertain=%6 elapsed=%7")
                     .arg(quotedLabel(evidence->command), nameOf(evidence->outcome),
                          evidence->required ? QStringLiteral("required") : QStringLiteral("optional"))
                     .arg(evidence->acceptedBytes)
                     .arg(evidence->writtenBytes)
                     .arg(evidence->uncertainBytes)
                     .arg(milliseconds(evidence->startedAtUs, evidence->finishedAtUs));
    }
    lines << QStringLiteral("  events");
    for (const auto& record : run.records) {
        const auto* event = std::get_if<GPSGolden::Event>(&record.value);
        if (record.phase == phase && event) {
            lines << QStringLiteral("    ") + formatEvent(*event);
        }
    }
}

QString finish(QStringList lines)
{
    for (auto& line : lines) {
        while (line.endsWith(u' ')) {
            line.chop(1);
        }
    }
    return lines.join(u'\n') + u'\n';
}

QString formatRun(const ScenarioDef& scenario, const GPSGolden::Run& run)
{
    QStringList lines{HEADER, QStringLiteral("golden ") + goldenName(scenario),
                      QStringLiteral("about ") + QString::fromLatin1(scenario.about),
                      QStringLiteral("request ") + describeRequest(scenario.type, scenario.config)};
    for (size_t index = 0; index < scenario.stream.size(); ++index) {
        lines << QStringLiteral("stream-step %1 bytes=%2 duration=%3ms")
                     .arg(index + 1)
                     .arg(scenario.stream[index].bytes.size())
                     .arg(scenario.stream[index].duration.count());
    }
    lines << QStringLiteral("configure");
    if (run.refused) {
        lines << QStringLiteral("  refused ") + quotedText(run.error);
        return finish(lines);
    }
    lines << QStringLiteral("  result configured=%1 baud=%2->%3 failure=%4 ready=%5 elapsed=%6")
                 .arg(flag(run.configured))
                 .arg(run.requestedBaud)
                 .arg(run.baud)
                 .arg(nameOf(run.failure), flag(run.ready), milliseconds(run.startedAtUs, run.configuredAtUs));
    lines << QStringLiteral("  identity ") + quotedText(run.identity);
    if (scenario.type == GPSType::automatic) {
        lines << QStringLiteral("  detected ") + (run.detected ? QStringLiteral("type=%1 baud=%2 evidence=%3")
                                                                     .arg(typeName(*run.detected))
                                                                     .arg(run.detectedBaud)
                                                                     .arg(quotedText(run.detectedEvidence))
                                                               : QStringLiteral("none"));
    }
    lines << QStringLiteral("  error ") + quotedText(run.error);
    const auto labels = commandLabels(run.records);
    appendPhase(lines, run, GPSGolden::Phase::Configure, labels);
    if (!run.configured) {
        return finish(lines);
    }
    lines << QStringLiteral("stream");
    lines << QStringLiteral("  result failure=%1 ready=%2%3")
                 .arg(nameOf(run.streamFailure), flag(run.streamReady),
                      run.receiveCalls >= GPSGolden::MAX_RECEIVE_CALLS ? QStringLiteral(" receive-limit") : QString());
    appendPhase(lines, run, GPSGolden::Phase::Stream, labels);
    return finish(lines);
}

QString formatDecode(const GPSGolden::Decode& decode)
{
    QStringList lines{QStringLiteral("    configured=") + flag(decode.configured)};
    if (decode.transportCalls != 0) {
        lines << QStringLiteral("    transport-calls=%1").arg(decode.transportCalls);
    }
    lines << QStringLiteral("    events");
    for (const auto& event : decode.events) {
        lines << QStringLiteral("      ") + formatEvent(event);
    }
    lines << QStringLiteral("    expired");
    for (const auto& event : decode.expired) {
        lines << QStringLiteral("      ") + formatEvent(event);
    }
    return lines.join(u'\n');
}

// ---------------------------------------------------------------------------------------------------------------
// Golden files.

/// A unified-diff-style hunk around the first difference.
QString difference(const QString& title, const QString& expected, const QString& actual)
{
    const QStringList before = expected.split(u'\n');
    const QStringList after = actual.split(u'\n');
    qsizetype first = 0;
    while (first < before.size() && first < after.size() && before[first] == after[first]) {
        ++first;
    }
    qsizetype beforeEnd = before.size();
    qsizetype afterEnd = after.size();
    while (beforeEnd > first && afterEnd > first && before[beforeEnd - 1] == after[afterEnd - 1]) {
        --beforeEnd;
        --afterEnd;
    }
    constexpr qsizetype WINDOW = 400;
    constexpr qsizetype CONTEXT = 3;
    constexpr int MAX_CHANGES = 40;
    const qsizetype rows = std::min(beforeEnd - first, WINDOW);
    const qsizetype columns = std::min(afterEnd - first, WINDOW);
    std::vector<std::vector<int>> common(static_cast<size_t>(rows + 1),
                                         std::vector<int>(static_cast<size_t>(columns + 1)));
    for (qsizetype row = rows - 1; row >= 0; --row) {
        for (qsizetype column = columns - 1; column >= 0; --column) {
            common[row][column] = before[first + row] == after[first + column]
                                      ? common[row + 1][column + 1] + 1
                                      : std::max(common[row + 1][column], common[row][column + 1]);
        }
    }
    QStringList script;
    for (qsizetype row = 0, column = 0; row < rows || column < columns;) {
        if (row < rows && column < columns && before[first + row] == after[first + column]) {
            script << u' ' + before[first + row++];
            ++column;
        } else if (row < rows && (column == columns || common[row + 1][column] >= common[row][column + 1])) {
            script << u'-' + before[first + row++];
        } else {
            script << u'+' + after[first + column++];
        }
    }
    QStringList lines{QStringLiteral("--- golden/%1").arg(title), QStringLiteral("+++ actual"),
                      QStringLiteral("@@ -%1 +%1 @@").arg(first + 1)};
    for (qsizetype context = std::max<qsizetype>(0, first - CONTEXT); context < first; ++context) {
        lines << u' ' + before[context];
    }
    // Unchanged runs keep CONTEXT lines on each side of a change.
    const auto nearChange = [&script](qsizetype index) {
        for (qsizetype other = std::max<qsizetype>(0, index - CONTEXT);
             other < std::min(script.size(), index + CONTEXT + 1); ++other) {
            if (!script[other].startsWith(u' ')) {
                return true;
            }
        }
        return false;
    };
    int changes = 0;
    bool elided = false;
    for (qsizetype index = 0; index < script.size() && changes < MAX_CHANGES; ++index) {
        if (!script[index].startsWith(u' ')) {
            ++changes;
        } else if (!nearChange(index)) {
            if (!elided) {
                lines << QStringLiteral("...");
            }
            elided = true;
            continue;
        }
        lines << script[index];
        elided = false;
    }
    if (changes >= MAX_CHANGES || rows < beforeEnd - first || columns < afterEnd - first) {
        lines << QStringLiteral("(further differences omitted)");
    }
    lines << QStringLiteral(
        "A port must reproduce this golden. Rewrite goldens (QGC_GPS_GOLDEN_UPDATE=1) only for "
        "a justified behaviour change.");
    return lines.join(u'\n');
}

QString goldenPath(const QString& relative)
{
    return QStringLiteral(GPS_GOLDEN_DIR) + u'/' + relative;
}

/// @return an empty string when @a actual matches, or after rewriting it in update mode.
QString checkGolden(const QString& relative, const QString& actual)
{
    QFile file(goldenPath(relative));
    QString expected;
    if (file.open(QIODevice::ReadOnly)) {
        expected = QString::fromUtf8(file.readAll());
        expected.remove(u'\r');
        file.close();
    } else if (!updateRequested()) {
        return QStringLiteral("Missing golden %1; create it with QGC_GPS_GOLDEN_UPDATE=1").arg(relative);
    }
    if (expected == actual) {
        return {};
    }
    if (!updateRequested()) {
        return difference(relative, expected, actual);
    }
    if (!QDir().mkpath(QFileInfo(file).absolutePath()) || !file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        file.write(actual.toUtf8()) < 0) {
        return QStringLiteral("Cannot write golden %1").arg(relative);
    }
    return {};
}

QString scenarioTranscript(const ScenarioDef& scenario)
{
    GPSTestClock clock(START_US);
    const auto bench = scenario.bench(clock);
    return formatRun(scenario, GPSGolden::runGolden({scenario.type, scenario.config, scenario.stream}, bench->link()));
}

QString decodeTranscript(const DecoderDef& decoder)
{
    QStringList lines{HEADER, QStringLiteral("golden decode/%1").arg(QString::fromLatin1(decoder.name)),
                      QStringLiteral("decoder ") + describeRequest(decoder.type, decoder.config)};

    const struct
    {
        qsizetype size;
        const char* label;
    } chunkings[] = {{1, "1"}, {7, "7"}, {0, "whole"}};

    for (const auto& file : dataFiles()) {
        if (!decodersFor(file.name).contains(QString::fromLatin1(decoder.name))) {
            continue;
        }
        lines << QStringLiteral("file %1/%2 bytes=%3").arg(file.directory, file.name).arg(file.bytes.size());
        // Chunkings with identical results share one block; a divergence pins each result separately.
        std::vector<std::pair<QString, QString>> results;
        for (const auto& chunking : chunkings) {
            GPSTestClock clock(START_US);
            const auto bench = decoder.bench(clock);
            const QString result = formatDecode(
                GPSGolden::decodeGolden({decoder.type, decoder.config, {}}, bench->link(), file.bytes, chunking.size));
            const auto same = std::ranges::find(results, result, &std::pair<QString, QString>::second);
            if (same != results.end()) {
                same->first += u' ' + QString::fromLatin1(chunking.label);
            } else {
                results.emplace_back(QString::fromLatin1(chunking.label), result);
            }
        }
        for (const auto& [labels, result] : results) {
            lines << QStringLiteral("  chunks ") + labels;
            lines << result;
        }
    }
    return finish(lines);
}

}  // namespace

void GPSGoldenTranscriptTest::_scenario_data()
{
    QTest::addColumn<int>("index");
    const auto& rows = scenarios();
    for (qsizetype index = 0; index < std::ssize(rows); ++index) {
        QTest::newRow(qPrintable(goldenName(rows[static_cast<size_t>(index)]))) << static_cast<int>(index);
    }
}

void GPSGoldenTranscriptTest::_scenario()
{
    QFETCH(int, index);
    const auto& scenario = scenarios()[static_cast<size_t>(index)];
    QString first;
    QString second;
    try {
        first = scenarioTranscript(scenario);
        second = scenarioTranscript(scenario);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
    QVERIFY2(first == second,
             qPrintable(difference(goldenName(scenario) + QStringLiteral(" (first run)"), first, second)));
    const QString failure = checkGolden(goldenName(scenario) + QStringLiteral(".golden"), first);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
}

void GPSGoldenTranscriptTest::_decode_data()
{
    QTest::addColumn<int>("index");
    const auto& rows = decoders();
    for (qsizetype index = 0; index < std::ssize(rows); ++index) {
        QTest::newRow(rows[static_cast<size_t>(index)].name) << static_cast<int>(index);
    }
}

void GPSGoldenTranscriptTest::_decode()
{
    QFETCH(int, index);
    const auto& decoder = decoders()[static_cast<size_t>(index)];
    QString first;
    QString second;
    try {
        first = decodeTranscript(decoder);
        second = decodeTranscript(decoder);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
    const QString name = QStringLiteral("decode/%1").arg(QString::fromLatin1(decoder.name));
    QVERIFY2(first == second, qPrintable(difference(name + QStringLiteral(" (first run)"), first, second)));
    const QString failure = checkGolden(name + QStringLiteral(".golden"), first);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
}

void GPSGoldenTranscriptTest::_decodeOnlyArming_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<bool>("armable");
    const auto& rows = decoders();
    for (qsizetype index = 0; index < std::ssize(rows); ++index) {
        const auto& decoder = rows[static_cast<size_t>(index)];
        // Femto and Trimble time the survey their configuration starts; a u-blox replay sets its decoder mode itself.
        const bool armable =
            decoder.type == GPSType::septentrio || decoder.type == GPSType::unicore || decoder.type == GPSType::quectel;
        QTest::newRow(decoder.name) << static_cast<int>(index) << armable;
    }
}

void GPSGoldenTranscriptTest::_decodeOnlyArming()
{
    QFETCH(int, index);
    QFETCH(bool, armable);
    const auto& decoder = decoders()[static_cast<size_t>(index)];
    const GPSGolden::Scenario setup{decoder.type, decoder.config, {}};
    for (const auto& file : dataFiles()) {
        if (!decodersFor(file.name).contains(QString::fromLatin1(decoder.name))) {
            continue;
        }
        GPSTestClock configuredClock(START_US);
        const auto bench = decoder.bench(configuredClock);
        const auto configured = GPSGolden::decodeGolden(setup, bench->link(), file.bytes, 0);
        GPSTestClock armedClock(START_US);
        ScriptedReceiver unreachable(std::stop_token{});
        const auto armed = GPSGolden::armedDecodeGolden(setup, {unreachable, armedClock}, file.bytes, 0);
        QCOMPARE(armed.configured, armable);
        if (!armable) {
            return;
        }
        QVERIFY(configured.configured);
        // A base the configuration verified, which a recording does not carry, is revoked when it expires.
        const auto revocation = [](const GPSGolden::Event& event) {
            const auto* survey = std::get_if<GPSGolden::Survey>(&event);
            return survey && !survey->survey.valid && !survey->survey.active;
        };
        GPSGolden::Decode expected = configured;
        GPSGolden::Decode actual = armed;
        std::erase_if(expected.expired, revocation);
        std::erase_if(actual.expired, revocation);
        const QString title =
            QStringLiteral("decode/%1 %2/%3 (armed)").arg(QString::fromLatin1(decoder.name), file.directory, file.name);
        QVERIFY2(formatDecode(actual) == formatDecode(expected),
                 qPrintable(difference(title, formatDecode(expected), formatDecode(actual))));
    }
}

void GPSGoldenTranscriptTest::_inventory()
{
    QStringList unclaimed;
    for (const auto& file : dataFiles()) {
        if (decodersFor(file.name).isEmpty()) {
            unclaimed << file.directory + u'/' + file.name;
        }
    }
    QVERIFY2(unclaimed.isEmpty(), qPrintable(QStringLiteral("No decoder for: ") + unclaimed.join(u' ')));

    QSet<QString> expected;
    for (const auto& scenario : scenarios()) {
        expected.insert(goldenName(scenario) + QStringLiteral(".golden"));
    }
    QCOMPARE(expected.size(), std::ssize(scenarios()));
    for (const auto& decoder : decoders()) {
        expected.insert(QStringLiteral("decode/%1.golden").arg(QString::fromLatin1(decoder.name)));
    }
    QSet<QString> present;
    const QDir root(QStringLiteral(GPS_GOLDEN_DIR));
    QDirIterator files(root.path(), {QStringLiteral("*.golden")}, QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        present.insert(root.relativeFilePath(files.next()));
    }
    QStringList stale = (present - expected).values();
    stale.sort();
    if (updateRequested()) {
        for (const auto& file : std::as_const(stale)) {
            QVERIFY(QFile::remove(root.filePath(file)));
        }
        stale.clear();
    }
    QStringList missing = (expected - present).values();
    missing.sort();
    QVERIFY2(stale.isEmpty(), qPrintable(QStringLiteral("Stale goldens: ") + stale.join(u' ')));
    QVERIFY2(missing.isEmpty(), qPrintable(QStringLiteral("Missing goldens: ") + missing.join(u' ')));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSGoldenTranscriptTest, TestLabel::Unit)
