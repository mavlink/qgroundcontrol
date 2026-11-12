#include "GPSGoldenTranscriptTest.h"

#include <algorithm>
#include <variant>
#include <vector>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>

#include "GPSCancellation.h"
#include "Protocols/Scenarios/GoldenChecks.h"
#include "Protocols/Support/GoldenFormat.h"

namespace {

using namespace GPSTest::GoldenScenario;
using GPSTest::Golden::difference;
using GPSTest::Golden::formatDecode;

/// Families whose NMEA navigation output must equal the passive decoder's for the shared NMEA files.
constexpr const char* NMEA_FORWARDERS[] = {"femto", "unicore-fixed", "quectel-survey"};

/// The events a family reports for NMEA input that passive input reports too: without the protocol
/// notices and the expiry of a base the configuration verified.
GPSTest::Golden::Decode navigationEvents(GPSTest::Golden::Decode decode)
{
    const auto familySpecific = [](const GPSTest::Golden::Event& event) {
        const auto* survey = std::get_if<GPSTest::Golden::Survey>(&event);
        return std::holds_alternative<GPSInputProtocol>(event) ||
               (survey && survey->survey.state == GPSBaseState::Inactive);
    };
    std::erase_if(decode.events, familySpecific);
    std::erase_if(decode.expired, familySpecific);
    return decode;
}

}  // namespace

void GPSGoldenTranscriptTest::init()
{
    GPSProtocolTestBase::init();
    // The transcripts pin wire behaviour, not diagnostics: the protocols warn in failure scenarios by design, and
    // GPSDriver warns with the error each transcript records.
    const GPSTest::GPSProtocolLogCapture protocolWarnings;
    ignoreLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression());
}

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
    const QString failure = checkScenario(scenarios()[static_cast<size_t>(index)]);
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
    const QString failure = checkDecoder(decoders()[static_cast<size_t>(index)]);
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
        const bool armable = decoder.type == GPSType::Septentrio || decoder.type == GPSType::Unicore ||
                             decoder.type == GPSType::Quectel || decoder.type == GPSType::UBlox;
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
            return survey && survey->survey.state == GPSBaseState::Inactive;
        };
        GPSTest::Golden::Decode expected = configured;
        GPSTest::Golden::Decode actual = armed;
        std::erase_if(expected.expired, revocation);
        std::erase_if(actual.expired, revocation);
        // Only the configured decoder runs in a GPSDriver, which forwards or withholds its RTCM frames.
        for (auto& event : expected.events) {
            if (auto* rtcm = std::get_if<GPSTest::Golden::RTCM>(&event)) {
                rtcm->forwarded.reset();
            }
        }
        const QString title =
            QStringLiteral("decode/%1 %2/%3 (armed)").arg(QString::fromLatin1(decoder.name), file.directory, file.name);
        QVERIFY2(formatDecode(actual) == formatDecode(expected),
                 qPrintable(difference(title, formatDecode(expected), formatDecode(actual))));
    }
}

void GPSGoldenTranscriptTest::_inventory()
{
    const QString failure = checkGoldenFiles();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSGoldenTranscriptTest, TestLabel::Unit)
