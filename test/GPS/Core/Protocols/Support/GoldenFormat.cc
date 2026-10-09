#include "Protocols/Support/GoldenFormat.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>

namespace GPSTest::Golden {

// ---------------------------------------------------------------------------------------------------------------
// The checked-in goldens, compared with or, when QGC_GPS_GOLDEN_UPDATE=1, rewritten.

namespace {

/// Whether QGC_GPS_GOLDEN_UPDATE=1 asks for the files to be rewritten.
bool updateRequested()
{
    return qEnvironmentVariableIntValue("QGC_GPS_GOLDEN_UPDATE") == 1;
}

}  // namespace

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
    constexpr qsizetype CONTEXT = 3;
    constexpr qsizetype MAX_LINES = 40;
    QStringList lines{QStringLiteral("--- golden/%1").arg(title), QStringLiteral("+++ actual"),
                      QStringLiteral("@@ -%1 +%1 @@").arg(first + 1)};
    for (qsizetype index = (std::max<qsizetype>) (0, first - CONTEXT); index < first; ++index) {
        lines << u' ' + before[index];
    }
    // One hunk spans every changed line, which a single change keeps short.
    const auto changed = [&lines](const QStringList& side, qsizetype end, QChar mark) {
        for (qsizetype index = 0; index < end && index < MAX_LINES; ++index) {
            lines << mark + side[index];
        }
        if (end > MAX_LINES) {
            lines << QStringLiteral("(%1 more lines)").arg(end - MAX_LINES);
        }
    };
    changed(before.mid(first), beforeEnd - first, u'-');
    changed(after.mid(first), afterEnd - first, u'+');
    for (qsizetype index = beforeEnd; index < (std::min) (before.size(), beforeEnd + CONTEXT); ++index) {
        lines << u' ' + before[index];
    }
    lines << QStringLiteral(
        "A port must reproduce this golden. Rewrite goldens (QGC_GPS_GOLDEN_UPDATE=1) only for "
        "a justified behaviour change.");
    return lines.join(u'\n');
}

QString checkGolden(const QString& root, const QString& relative, const QString& actual)
{
    QFile file(QDir(root).filePath(relative));
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

QString checkInventory(const QString& root, const QString& pattern, const QSet<QString>& expected)
{
    const QDir directory(root);
    QSet<QString> present;
    QDirIterator files(directory.path(), {pattern}, QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        present.insert(directory.relativeFilePath(files.next()));
    }
    QStringList stale = (present - expected).values();
    stale.sort();
    if (updateRequested()) {
        for (const auto& file : std::as_const(stale)) {
            if (!QFile::remove(directory.filePath(file))) {
                return QStringLiteral("Cannot remove stale golden %1").arg(file);
            }
        }
        stale.clear();
    }
    QStringList missing = (expected - present).values();
    missing.sort();
    QStringList problems;
    if (!stale.isEmpty()) {
        problems << QStringLiteral("Stale goldens: ") + stale.join(u' ');
    }
    if (!missing.isEmpty()) {
        problems << QStringLiteral("Missing goldens: ") + missing.join(u' ');
    }
    return problems.join(u'\n');
}

// ---------------------------------------------------------------------------------------------------------------
// Transcript format. A scenario golden describes the scenario and its request, then a `configure` and (after
// success) a `stream` phase. Each phase line has its outcome: requested->final baud, sticky failure, readiness and,
// for configure, the virtual duration. Under it:
//   `baud <rate> <status>`
//   `write <command label|-> <bytes> [<outcome> required|optional]`, then the bytes as text when printable, else hex
//   `evidence <command label> <outcome> required|optional accepted=<n> written=<n>` for a command that wrote nothing
//   decoded events; host timestamps appear only as receipt=0/1
// A decode golden lists, per data file, the events of each chunking (1, 7, whole) and those after the freshness
// horizon, marked `expired`; chunkings with identical results share one block.

QString typeName(GPSType type)
{
    return QString::fromLatin1(QMetaEnum::fromType<GPSType>().valueToKey(static_cast<int>(type)));
}

namespace {

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

/// The golden name of @a value, indexed by its numeric value.
template <typename Enum, size_t N>
QString nameOf(Enum value, const char* const (&names)[N])
{
    const auto index = static_cast<size_t>(value);
    return index < N && names[index] ? QString::fromLatin1(names[index]) : QString::number(static_cast<int>(value));
}

QString nameOf(GPSCommandOutcome outcome)
{
    return nameOf(outcome, {"pending", "written", "acknowledged", "readback-verified", "rejected", "timed-out",
                            "cancelled", "transport-error"});
}

QString nameOf(GPSWriteStatus status)
{
    return nameOf(status, {"completed", "timed-out", "cancelled", "error", "unsupported"});
}

QString nameOf(GPSBaudStatus status)
{
    return nameOf(status, {"configured", "unsupported", "cancelled", "error"});
}

QString nameOf(Failure failure)
{
    return nameOf(failure, {"none", "cancelled", "transport", "protocol", "invalid-argument", "consent-required"});
}

QString nameOf(GPSFixQuality fix)
{
    return nameOf(fix,
                  {"unknown", "none", "2d", "3d", "differential", "rtk-float", "rtk-fixed", nullptr, "extrapolated"});
}

QString nameOf(GPSConstellation constellation)
{
    return nameOf(constellation, {"unknown", "gps", "glonass", "galileo", "beidou", "qzss", "sbas", "navic"});
}

QString nameOf(GPSIntegrityReport::JammingState state)
{
    return nameOf(state, {"unknown", "ok", "warning", "critical"});
}

QString nameOf(GPSIntegrityReport::SpoofingState state)
{
    return nameOf(state, {"unknown", "none", "indicated", "multiple"});
}

QString nameOf(GPSIntegrityReport::AntennaState state)
{
    return nameOf(state, {"unknown", "ok", "open", "short"});
}

}  // namespace

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
        .arg(typeName(type), type == GPSType::passive ? QStringLiteral("passive") : QStringLiteral("rtk-base"), mode,
             flag(config.base.compactObservations), flag(config.allowPersistentChanges))
        .arg(config.baudRate);
}

namespace {

uint32_t fnv1a(QByteArrayView bytes)
{
    uint32_t hash = 2166136261u;
    for (const char byte : bytes) {
        hash = (hash ^ static_cast<uint8_t>(byte)) * 16777619u;
    }
    return hash;
}

QString formatEvent(const Event& event)
{
    return std::visit(
        [](const auto& report) -> QString {
            using Report = std::decay_t<decltype(report)>;
            if constexpr (std::is_same_v<Report, Position>) {
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
                    QStringLiteral("antenna=%1/%2")
                        .arg(nameOf(report.antenna.state), receipt(report.antenna.timestampUs)),
                    QStringLiteral("output-overflow=") + receipt(report.outputOverflowUs),
                }
                    .join(u' ');
            } else if constexpr (std::is_same_v<Report, Satellites>) {
                QStringList parts{QStringLiteral("satellites full=") + flag(report.fullSnapshot)};
                for (const auto& system : report.systems) {
                    parts << QStringLiteral("%1:view=%2/%3,use=%4/%5")
                                 .arg(nameOf(system.constellation))
                                 .arg(system.inView)
                                 .arg(receipt(system.inViewTimestampUs), optionalNumber(system.inUse),
                                      receipt(system.inUseTimestampUs));
                }
                return parts.join(u' ');
            } else if constexpr (std::is_same_v<Report, SatelliteUsage>) {
                return QStringLiteral("usage used=%1 receipt=%2")
                    .arg(optionalNumber(report.used), receipt(report.timestampUs));
            } else if constexpr (std::is_same_v<Report, Survey>) {
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
            } else if constexpr (std::is_same_v<Report, GPSInputProtocol>) {
                return QStringLiteral("input type=") + typeName(report.family);
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
std::vector<std::optional<QByteArray>> commandLabels(const std::vector<Record>& records)
{
    std::vector<std::optional<QByteArray>> labels(records.size());
    std::vector<size_t> pending;
    for (size_t index = 0; index < records.size(); ++index) {
        const auto& value = records[index].value;
        if (std::holds_alternative<Write>(value)) {
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
            remaining -= std::get<Write>(records[*write].value).acceptedBytes;
        }
        if (evidence->acceptedBytes == 0 && !pending.empty()) {
            const auto& last = std::get<Write>(records[pending.back()].value);
            if (last.acceptedBytes == 0 && last.status != GPSWriteStatus::Completed) {
                labels[pending.back()] = evidence->command;
            }
        }
        pending.clear();
    }
    return labels;
}

/// Writes, baud changes and command outcomes, in order, then the decoded events. Write-call boundaries are not
/// behaviour: contiguous writes of one command form one entry, which ends with the command's outcome. A command that
/// wrote nothing shows its outcome alone. Only records before @a end count.
QStringList phaseLines(const Run& run, Phase phase, const std::vector<std::optional<QByteArray>>& labels, size_t end)
{
    struct Entry
    {
        std::optional<QByteArray> label;
        Write write;
    };

    /// The line of the latest labelled entry, until an outcome resolves it.
    struct Open
    {
        qsizetype line;
        QByteArray label;
        int acceptedBytes;
        int writtenBytes;
    };

    QStringList lines;
    std::optional<Entry> entry;
    std::optional<Open> open;
    const auto flush = [&lines, &entry, &open] {
        if (!entry) {
            return;
        }
        const auto& write = entry->write;
        QString line = QStringLiteral("  write %1 %2")
                           .arg(entry->label ? quotedBytes(*entry->label) : QStringLiteral("-"))
                           .arg(write.bytes.size());
        if (write.status != GPSWriteStatus::Completed || write.acceptedBytes != write.bytes.size() ||
            write.writtenBytes != write.bytes.size()) {
            line += QStringLiteral(" status=%1 accepted=%2 written=%3")
                        .arg(nameOf(write.status))
                        .arg(write.acceptedBytes)
                        .arg(write.writtenBytes);
        }
        if (entry->label) {
            open = Open{lines.size(), *entry->label, write.acceptedBytes, write.writtenBytes};
        }
        lines << line;
        if (printable(write.bytes)) {
            lines << QStringLiteral("    text ") + quotedBytes(write.bytes);
        } else {
            for (qsizetype offset = 0; offset < write.bytes.size(); offset += 32) {
                lines << QStringLiteral("    ") + QString::fromLatin1(write.bytes.mid(offset, 32).toHex());
            }
        }
        entry.reset();
    };
    QStringList events;
    for (size_t index = 0; index < std::min(end, run.records.size()); ++index) {
        const auto& record = run.records[index];
        if (record.phase != phase) {
            continue;
        }
        if (const auto* write = std::get_if<Write>(&record.value)) {
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
        } else if (const auto* baud = std::get_if<Baud>(&record.value)) {
            flush();
            lines << QStringLiteral("  baud %1 %2").arg(baud->rate).arg(nameOf(baud->status));
        } else if (const auto* evidence = std::get_if<GPSConfigurationEvidence>(&record.value)) {
            flush();
            QString outcome =
                QStringLiteral("%1 %2").arg(nameOf(evidence->outcome), evidence->required ? QStringLiteral("required")
                                                                                          : QStringLiteral("optional"));
            if (open && open->label == evidence->command) {
                if (evidence->acceptedBytes != open->acceptedBytes || evidence->writtenBytes != open->writtenBytes) {
                    outcome += QStringLiteral(" accepted=%1 written=%2")
                                   .arg(evidence->acceptedBytes)
                                   .arg(evidence->writtenBytes);
                }
                lines[open->line] += u' ' + outcome;
            } else {
                lines << QStringLiteral("  evidence %1 %2 accepted=%3 written=%4")
                             .arg(quotedBytes(evidence->command), outcome)
                             .arg(evidence->acceptedBytes)
                             .arg(evidence->writtenBytes);
            }
            open.reset();
        } else if (const auto* event = std::get_if<Event>(&record.value)) {
            events << QStringLiteral("  ") + formatEvent(*event);
        }
    }
    flush();
    return lines + events;
}

}  // namespace

QString transcriptText(QStringList lines)
{
    for (auto& line : lines) {
        while (line.endsWith(u' ')) {
            line.chop(1);
        }
    }
    return lines.join(u'\n') + u'\n';
}

QString formatRun(const char* about, const Scenario& setup, bool detectionOnly, const Run& run)
{
    QStringList lines{QStringLiteral("about ") + QString::fromLatin1(about),
                      QStringLiteral("request ") + describeRequest(setup.type, setup.config)};
    lines << QStringLiteral("configure configured=%1 baud=%2->%3 failure=%4 ready=%5 elapsed=%6")
                 .arg(flag(run.configured))
                 .arg(run.requestedBaud)
                 .arg(run.baud)
                 .arg(nameOf(run.failure), flag(run.ready), milliseconds(run.startedAtUs, run.configuredAtUs));
    if (!run.identity.isEmpty()) {
        lines << QStringLiteral("  identity ") + quotedText(run.identity);
    }
    if (setup.type == GPSType::automatic) {
        lines << QStringLiteral("  detected ") + (run.detected ? QStringLiteral("type=%1 baud=%2 evidence=%3")
                                                                     .arg(typeName(*run.detected))
                                                                     .arg(run.detectedBaud)
                                                                     .arg(quotedText(run.detectedEvidence))
                                                               : QStringLiteral("none"));
    }
    if (!run.error.isEmpty()) {
        lines << QStringLiteral("  error ") + quotedText(run.error);
    }
    const auto labels = commandLabels(run.records);
    const size_t end = detectionOnly ? run.detectionRecords : run.records.size();
    lines << phaseLines(run, Phase::Configure, labels, end);
    if (!run.configured || detectionOnly) {
        return transcriptText(lines);
    }
    lines << QStringLiteral("stream failure=%1 ready=%2%3")
                 .arg(nameOf(run.streamFailure), flag(run.streamReady),
                      run.receiveCalls >= MAX_RECEIVE_CALLS ? QStringLiteral(" receive-limit") : QString());
    lines << phaseLines(run, Phase::Stream, labels, end);
    return transcriptText(lines);
}

QString formatDecode(const Decode& decode)
{
    QStringList lines;
    if (!decode.configured) {
        lines << QStringLiteral("    configured=0");
    }
    if (decode.transportCalls != 0) {
        lines << QStringLiteral("    transport-calls=%1").arg(decode.transportCalls);
    }
    for (const auto& event : decode.events) {
        lines << QStringLiteral("    ") + formatEvent(event);
    }
    for (const auto& event : decode.expired) {
        lines << QStringLiteral("    expired ") + formatEvent(event);
    }
    return lines.join(u'\n');
}

}  // namespace GPSTest::Golden
