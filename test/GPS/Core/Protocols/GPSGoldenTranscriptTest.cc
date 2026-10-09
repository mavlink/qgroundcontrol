#include "GPSGoldenTranscriptTest.h"

#include <algorithm>
#include <utility>
#include <vector>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QStringList>

#include "GPSCancellation.h"
#include "Protocols/Scenarios/GoldenScenarios.h"
#include "Protocols/Support/GoldenFormat.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/UBXReceiverModel.h"

namespace {

using namespace GPSTest::GoldenScenario;
using GPSTest::Golden::checkGolden;
using GPSTest::Golden::checkInventory;
using GPSTest::Golden::describeRequest;
using GPSTest::Golden::difference;
using GPSTest::Golden::formatDecode;
using GPSTest::Golden::formatRun;
using GPSTest::Golden::transcriptText;
using GPSTest::Golden::typeName;

QString goldenName(const ScenarioDef& scenario)
{
    return typeName(scenario.type) + u'/' + QString::fromLatin1(scenario.name);
}

QString goldenRoot()
{
    return QString::fromUtf8(GPSTest::GOLDEN_DIR);
}

// ---------------------------------------------------------------------------------------------------------------
// Decode table: every corpus and fixture file through each receiver family that can meet it.

struct DecoderDef
{
    const char* name;
    GPSType type;
    BenchFactory bench;
    GPSReceiverConfig config;
    /// A decoder whose transcript this one's must match; it has no golden of its own.
    const char* sameAs = nullptr;
};

const std::vector<DecoderDef>& decoders()
{
    static const std::vector<DecoderDef> rows{
        {"ublox", GPSType::ublox, ubxWire(), fixedBase(47, 8, 500, 1, 115200)},
        // Configured with the pre-protocol-27 commands, it decodes as the protocol 27 configuration does.
        {"ublox-legacy", GPSType::ublox, ubxWire([](UBXBench& b) {
             b.model.legacy = true;
             b.model.module = "NEO-M8P";
         }),
         fixedBase(47, 8, 500, 1, 115200), "ublox"},
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
    // Trimble parses GGA itself; the other families forward NMEA as passive input does (_nmeaMatchesPassive).
    if (file.endsWith(QLatin1String(".nmea"))) {
        return {QStringLiteral("passive"), QStringLiteral("trimble")};
    }
    return {};
}

/// Families whose NMEA navigation output must equal the passive decoder's for the shared NMEA files.
constexpr const char* NMEA_FORWARDERS[] = {"femto", "unicore-fixed", "quectel-survey"};

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

/// The events a family reports for NMEA input that passive input reports too: without the protocol
/// notices and the expiry of a base the configuration verified.
GPSTest::Golden::Decode navigationEvents(GPSTest::Golden::Decode decode)
{
    const auto familySpecific = [](const GPSTest::Golden::Event& event) {
        const auto* survey = std::get_if<GPSTest::Golden::Survey>(&event);
        return std::holds_alternative<GPSInputProtocol>(event) ||
               (survey && !survey->survey.valid && !survey->survey.active);
    };
    std::erase_if(decode.events, familySpecific);
    std::erase_if(decode.expired, familySpecific);
    return decode;
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
    const QString first = scenarioTranscript(scenario);
    const QString second = scenarioTranscript(scenario);
    QVERIFY2(first == second,
             qPrintable(difference(goldenName(scenario) + QStringLiteral(" (first run)"), first, second)));
    if (scenario.sameAs) {
        const auto& rows = scenarios();
        const auto sibling = std::ranges::find_if(rows, [&scenario](const ScenarioDef& row) {
            return row.type == scenario.type && qstrcmp(row.name, scenario.sameAs) == 0;
        });
        QVERIFY2(sibling != rows.end() && !sibling->sameAs, scenario.sameAs);
        // The about and request lines name the scenario; what follows is its behaviour.
        const auto behaviour = [](const QString& transcript) { return transcript.section(u'\n', 2); };
        const QString expected = behaviour(scenarioTranscript(*sibling));
        QVERIFY2(behaviour(first) == expected,
                 qPrintable(difference(goldenName(scenario), expected, behaviour(first))));
        return;
    }
    const QString failure = checkGolden(goldenRoot(), goldenName(scenario) + QStringLiteral(".golden"), first);
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
    const QString first = decodeTranscript(decoder);
    const QString second = decodeTranscript(decoder);
    const QString name = QStringLiteral("decode/%1").arg(QString::fromLatin1(decoder.name));
    QVERIFY2(first == second, qPrintable(difference(name + QStringLiteral(" (first run)"), first, second)));
    for (const qsizetype chunk : {1, 7}) {
        const QString chunked = decodeTranscript(decoder, chunk);
        QVERIFY2(chunked == first,
                 qPrintable(difference(name + QStringLiteral(" (chunks of %1)").arg(chunk), first, chunked)));
    }
    if (decoder.sameAs) {
        const auto& rows = decoders();
        const auto sibling = std::ranges::find_if(
            rows, [&decoder](const DecoderDef& row) { return qstrcmp(row.name, decoder.sameAs) == 0; });
        QVERIFY2(sibling != rows.end() && !sibling->sameAs, decoder.sameAs);
        const QString expected = decodeTranscript(*sibling);
        QVERIFY2(first == expected, qPrintable(difference(name, expected, first)));
        return;
    }
    const QString failure = checkGolden(goldenRoot(), name + QStringLiteral(".golden"), first);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
}

void GPSGoldenTranscriptTest::_nmeaMatchesPassive_data()
{
    QTest::addColumn<QString>("family");
    for (const char* family : NMEA_FORWARDERS) {
        QTest::newRow(family) << QString::fromLatin1(family);
    }
}

void GPSGoldenTranscriptTest::_nmeaMatchesPassive()
{
    QFETCH(QString, family);
    const auto& rows = decoders();
    const auto find = [&rows](const QString& name) {
        return std::ranges::find_if(rows, [&name](const DecoderDef& row) { return name == QLatin1String(row.name); });
    };
    const auto decoder = find(family);
    const auto passive = find(QStringLiteral("passive"));
    QVERIFY(decoder != rows.end() && passive != rows.end());
    const auto decode = [](const DecoderDef& row, const QByteArray& bytes) {
        GPSTestClock clock(GOLDEN_START_US);
        const auto bench = row.bench(clock);
        return formatDecode(
            navigationEvents(GPSTest::Golden::decodeGolden({row.type, row.config, {}}, link(*bench), bytes, 0)));
    };
    int compared = 0;
    for (const auto& file : dataFiles()) {
        if (decodersFor(file.name) != QStringList{QStringLiteral("passive"), QStringLiteral("trimble")}) {
            continue;
        }
        const QString expected = decode(*passive, file.bytes);
        const QString actual = decode(*decoder, file.bytes);
        QVERIFY2(actual == expected, qPrintable(difference(family + u' ' + file.name, expected, actual)));
        ++compared;
    }
    QVERIFY(compared > 0);
}

void GPSGoldenTranscriptTest::_decodeOnlyArming_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<bool>("armable");
    const auto& rows = decoders();
    for (qsizetype index = 0; index < std::ssize(rows); ++index) {
        const auto& decoder = rows[static_cast<size_t>(index)];
        // Femto and Trimble time the survey their configuration starts.
        const bool armable = decoder.type == GPSType::septentrio || decoder.type == GPSType::unicore ||
                             decoder.type == GPSType::quectel || decoder.type == GPSType::ublox;
        QTest::newRow(decoder.name) << static_cast<int>(index) << armable;
    }
}

void GPSGoldenTranscriptTest::_decodeOnlyArming()
{
    QFETCH(int, index);
    QFETCH(bool, armable);
    const auto& decoder = decoders()[static_cast<size_t>(index)];
    const GPSTest::Golden::Scenario setup{decoder.type, decoder.config, {}};
    for (const auto& file : dataFiles()) {
        if (!decodersFor(file.name).contains(QString::fromLatin1(decoder.name))) {
            continue;
        }
        GPSTestClock configuredClock(GOLDEN_START_US);
        const auto bench = decoder.bench(configuredClock);
        const auto configured = GPSTest::Golden::decodeGolden(setup, link(*bench), file.bytes, 0);
        GPSTestClock armedClock(GOLDEN_START_US);
        ScriptedReceiver unreachable(GPSCancelToken{});
        const auto armed = GPSTest::Golden::armedDecodeGolden(setup, {unreachable, armedClock}, file.bytes, 0);
        QCOMPARE(armed.configured, armable);
        if (!armable) {
            return;
        }
        QVERIFY(configured.configured);
        // A base the configuration verified, which a recording does not carry, is revoked when it expires.
        const auto revocation = [](const GPSTest::Golden::Event& event) {
            const auto* survey = std::get_if<GPSTest::Golden::Survey>(&event);
            return survey && !survey->survey.valid && !survey->survey.active;
        };
        GPSTest::Golden::Decode expected = configured;
        GPSTest::Golden::Decode actual = armed;
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

    QSet<QString> names;
    QSet<QString> expected;
    for (const auto& scenario : scenarios()) {
        names.insert(goldenName(scenario));
        if (!scenario.sameAs) {
            expected.insert(goldenName(scenario) + QStringLiteral(".golden"));
        }
    }
    QCOMPARE(names.size(), std::ssize(scenarios()));
    for (const auto& decoder : decoders()) {
        if (!decoder.sameAs) {
            expected.insert(QStringLiteral("decode/%1.golden").arg(QString::fromLatin1(decoder.name)));
        }
    }
    const QString failure = checkInventory(goldenRoot(), QStringLiteral("*.golden"), expected);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSGoldenTranscriptTest, TestLabel::Unit)
