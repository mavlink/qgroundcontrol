#include "GoldenChecks.h"

#include <algorithm>
#include <utility>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QSet>

#include "Protocols/Support/GoldenFormat.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/UBXReceiverModel.h"

namespace GPSTest::GoldenScenario {

namespace {

using GPSTest::Golden::checkGolden;
using GPSTest::Golden::checkInventory;
using GPSTest::Golden::describeRequest;
using GPSTest::Golden::difference;
using GPSTest::Golden::formatDecode;
using GPSTest::Golden::formatRun;
using GPSTest::Golden::transcriptText;
using GPSTest::Golden::typeName;

QString goldenRoot()
{
    return QString::fromUtf8(GPSTest::GOLDEN_DIR);
}

QString goldenFile(const QString& name)
{
    return name + QStringLiteral(".golden");
}

/// The stream phase the golden @a name records, from its `stream` line on; empty when no event follows that line.
QString recordedStream(const QString& name)
{
    QString golden = QString::fromUtf8(readFile(QDir(goldenRoot()).filePath(goldenFile(name))).value_or(QByteArray()));
    golden.remove(u'\r');
    const qsizetype phase = golden.indexOf(QLatin1String("\nstream "));
    const bool events = phase >= 0 && golden.indexOf(u'\n', phase + 1) < golden.size() - 1;
    return events ? golden.mid(phase + 1) : QString();
}

QString scenarioTranscript(const ScenarioDef& scenario)
{
    GPSTestClock clock(GOLDEN_START_US);
    const auto bench = scenario.bench(clock);
    const GPSTest::Golden::Scenario setup{scenario.type, scenario.config, scenario.stream};
    return formatRun(scenario.about, setup, scenario.detectionOnly, GPSTest::Golden::runGolden(setup, link(*bench)));
}

/// @a chunk 0 delivers each file whole.
QString decodeTranscript(const DecoderDef& decoder, qsizetype chunk = 0)
{
    QStringList lines{QStringLiteral("decoder ") + describeRequest(decoder.type, decoder.config)};
    for (const auto& file : dataFiles()) {
        if (!decodersFor(file.name).contains(QString::fromLatin1(decoder.name))) {
            continue;
        }
        lines << QStringLiteral("file %1/%2 bytes=%3").arg(file.directory, file.name).arg(file.bytes.size());
        GPSTestClock clock(GOLDEN_START_US);
        const auto bench = decoder.bench(clock);
        const QString result = formatDecode(
            GPSTest::Golden::decodeGolden({decoder.type, decoder.config, {}}, link(*bench), file.bytes, chunk));
        if (!result.isEmpty()) {
            lines << result;
        }
    }
    return transcriptText(lines);
}

}  // namespace

const std::vector<DecoderDef>& decoders()
{
    static const std::vector<DecoderDef> rows{
        {"ublox", GPSType::UBlox, ubxWire(), fixedBase(47, 8, 500, 1, 115200)},
        // Configured with the pre-protocol-27 commands, it decodes as the protocol 27 configuration does.
        {"ublox-legacy", GPSType::UBlox, ubxWire([](UBXBench& b) {
             b.model.legacy = true;
             b.model.module = "NEO-M8P";
         }),
         fixedBase(47, 8, 500, 1, 115200), "ublox"},
        {"septentrio", GPSType::Septentrio, sbf(), fixedBase(47, 8, 500, 1)},
        {"trimble", GPSType::Trimble, ashtech(), surveyIn(1, 100)},
        {"femto", GPSType::Femto, femto(), fixedBase(47, 8, 500, 1)},
        // The Unicore and Quectel seeds carry evidence for the fixed ECEF position (0, 6378237, 0).
        {"unicore-fixed", GPSType::Unicore, unicore(), fixedBase(0, 90, 100, 0)},
        {"unicore-averaging", GPSType::Unicore, unicore(), averaging(60)},
        {"quectel-survey", GPSType::Quectel, quectel([](QuectelBench& b) { b.model.role = 2; }), surveyIn(15, 60)},
        {"quectel-fixed", GPSType::Quectel, quectel([](QuectelBench& b) {
             b.model.role = 2;
             b.model.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
         }),
         fixedBase(0, 90, 100, 0)},
        {"passive", GPSType::Passive, passive(), passiveInput(115200)},
    };
    return rows;
}

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
    // Trimble parses GGA itself; the other families forward NMEA as passive input does, which
    // GPSGoldenTranscriptTest checks.
    if (file.endsWith(QLatin1String(".nmea"))) {
        return {QStringLiteral("passive"), QStringLiteral("trimble")};
    }
    return {};
}

const std::vector<DataFile>& dataFiles()
{
    static const std::vector<DataFile> files = [] {
        const QStringList suffixes{QStringLiteral("ascii"), QStringLiteral("bin"), QStringLiteral("gps"),
                                   QStringLiteral("nmea"),  QStringLiteral("sbf"), QStringLiteral("ubx")};
        std::vector<DataFile> result;
        const std::pair<QString, const char*> directories[] = {{QStringLiteral("corpus"), GPSTest::CORPUS_DIR},
                                                               {QStringLiteral("fixtures"), GPSTest::FIXTURE_DIR}};
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

QString goldenName(const ScenarioDef& scenario)
{
    return typeName(scenario.type) + u'/' + QString::fromLatin1(scenario.name);
}

QString goldenName(const DecoderDef& decoder)
{
    return QStringLiteral("decode/") + QString::fromLatin1(decoder.name);
}

QString checkScenario(const ScenarioDef& scenario)
{
    const QString name = goldenName(scenario);
    const QString first = scenarioTranscript(scenario);
    const QString second = scenarioTranscript(scenario);
    if (first != second) {
        return difference(name + QStringLiteral(" (first run)"), first, second);
    }
    if (!scenario.sameAs) {
        return checkGolden(goldenRoot(), goldenFile(name), first);
    }
    const auto& rows = scenarios();
    const auto sibling = std::ranges::find_if(rows, [&scenario](const ScenarioDef& row) {
        return row.type == scenario.type && qstrcmp(row.name, scenario.sameAs) == 0;
    });
    if (sibling == rows.end() || sibling->sameAs) {
        return QStringLiteral("%1 is the same as %2, which has no golden")
            .arg(name, QString::fromLatin1(scenario.sameAs));
    }
    // The about and request lines name the scenario; what follows is its behaviour.
    const auto behaviour = [](const QString& transcript) { return transcript.section(u'\n', 2); };
    const QString expected = behaviour(scenarioTranscript(*sibling));
    return behaviour(first) == expected ? QString() : difference(name, expected, behaviour(first));
}

QString checkDecoder(const DecoderDef& decoder)
{
    const QString name = goldenName(decoder);
    const QString first = decodeTranscript(decoder);
    const QString second = decodeTranscript(decoder);
    if (first != second) {
        return difference(name + QStringLiteral(" (first run)"), first, second);
    }
    for (const qsizetype chunk : {1, 7}) {
        const QString chunked = decodeTranscript(decoder, chunk);
        if (chunked != first) {
            return difference(name + QStringLiteral(" (chunks of %1)").arg(chunk), first, chunked);
        }
    }
    if (!decoder.sameAs) {
        return checkGolden(goldenRoot(), goldenFile(name), first);
    }
    const auto& rows = decoders();
    const auto sibling = std::ranges::find_if(
        rows, [&decoder](const DecoderDef& row) { return qstrcmp(row.name, decoder.sameAs) == 0; });
    if (sibling == rows.end() || sibling->sameAs) {
        return QStringLiteral("%1 is the same as %2, which has no golden")
            .arg(name, QString::fromLatin1(decoder.sameAs));
    }
    const QString expected = decodeTranscript(*sibling);
    return first == expected ? QString() : difference(name, expected, first);
}

QString checkGoldenFiles()
{
    QStringList problems;
    QStringList unclaimed;
    for (const auto& file : dataFiles()) {
        if (decodersFor(file.name).isEmpty()) {
            unclaimed << file.directory + u'/' + file.name;
        }
    }
    if (!unclaimed.isEmpty()) {
        problems << QStringLiteral("No decoder for: ") + unclaimed.join(u' ');
    }

    QSet<QString> names;
    QSet<QString> expected;
    // The first golden of its family to record each stream phase.
    QHash<QString, QString> streams;
    for (const auto& scenario : scenarios()) {
        const QString name = goldenName(scenario);
        if (names.contains(name)) {
            problems << QStringLiteral("Two scenarios are named ") + name;
        }
        names.insert(name);
        if (scenario.sameAs) {
            continue;
        }
        expected.insert(goldenFile(name));
        const QString stream = recordedStream(name);
        if (stream.isEmpty()) {
            continue;
        }
        QString& first = streams[typeName(scenario.type) + u'\n' + stream];
        if (first.isEmpty()) {
            first = name;
        } else {
            problems << QStringLiteral("%1 repeats the stream phase of %2: leave its stream empty").arg(name, first);
        }
    }
    for (const auto& decoder : decoders()) {
        if (!decoder.sameAs) {
            expected.insert(goldenFile(goldenName(decoder)));
        }
    }
    const QString files = checkInventory(goldenRoot(), QStringLiteral("*.golden"), expected);
    if (!files.isEmpty()) {
        problems << files;
    }
    return problems.join(u'\n');
}

}  // namespace GPSTest::GoldenScenario
