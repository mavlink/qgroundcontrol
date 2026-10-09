#include "GPSProtocolTestBase.h"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>

#include "GPSReceiverFamilies.h"
#include "LogEntry.h"
#include "LogManager.h"
#include "Protocols/Support/GPSModelViolations.h"
#include "Protocols/Support/GPSProtocolTestData.h"

namespace {
GPSProtocolTestBase* s_running = nullptr;

/// Protocol categories besides each family's own.
constexpr const char* RUNTIME_CATEGORIES[] = {"GPS.Protocols.Runtime", "GPS.Protocols.Detector"};

/// Fails the current test function with every protocol violation receiver models recorded since the last check.
void verifyNoModelViolations()
{
    QStringList details;
    for (const auto& violation : GPSTest::ModelViolations::take()) {
        details.append(QString::fromStdString(violation));
    }
    QVERIFY2(details.isEmpty(),
             qPrintable(QStringLiteral("Protocol violations: ") + details.join(QStringLiteral("; "))));
}
}  // namespace

GPSProtocolTestBase* GPSProtocolTestBase::_running()
{
    return s_running;
}

void GPSProtocolTestBase::_ignoreProtocolWarnings()
{
    if (_protocolWarningsIgnored) {
        return;
    }
    _protocolWarningsIgnored = true;
    QList<QByteArray> categories;
    for (const auto* family : gpsReceiverFamilies()) {
        categories.append(family->logCategory().categoryName());
    }
    for (const auto* category : RUNTIME_CATEGORIES) {
        categories.append(category);
    }
    const QRegularExpression any;
    for (const auto& category : std::as_const(categories)) {
        ignoreLogMessage(category.constData(), QtWarningMsg, any);
    }
}

void GPSProtocolTestBase::addScenarioRows(std::span<const GPSTest::ProtocolScenario> scenarios)
{
    QTest::addColumn<int>("scenario");
    for (size_t index = 0; index < scenarios.size(); ++index) {
        QTest::newRow(scenarios[index].name) << static_cast<int>(index);
    }
}

void GPSProtocolTestBase::runScenario(std::span<const GPSTest::ProtocolScenario> scenarios)
{
    QFETCH(int, scenario);
    const QTest::ThrowOnFailEnabler endRowOnFailure;
    GPSTest::GPSTestClock clock(GPSTest::GPSTestClock::START_US);
    scenarios[static_cast<size_t>(scenario)].run(clock);
}

void GPSProtocolTestBase::init()
{
    UnitTest::init();
    _protocolWarningsIgnored = false;
    // Violations recorded outside a protocol suite's test function do not belong to this one.
    (void) GPSTest::ModelViolations::take();
    s_running = this;
}

void GPSProtocolTestBase::cleanup()
{
    s_running = nullptr;
    UnitTest::cleanup();
    verifyNoModelViolations();
}

std::optional<std::vector<uint8_t>> GPSTest::fixtureBytes(const char* name)
{
    const auto bytes = readFile(QString::fromUtf8(FIXTURE_DIR) + u'/' + QString::fromUtf8(name));
    if (!bytes) {
        return std::nullopt;
    }
    return std::vector<uint8_t>(bytes->cbegin(), bytes->cend());
}

std::optional<std::vector<uint8_t>> GPSTest::fixtureBytes(const FixtureSlice& slice)
{
    const auto bytes = fixtureBytes(slice.file);
    if (!bytes || slice.size < 2 || slice.offset + slice.size > bytes->size()) {
        return std::nullopt;
    }
    const auto first = bytes->cbegin() + static_cast<std::ptrdiff_t>(slice.offset);
    const bool ubx = first[0] == 0xB5 && first[1] == 0x62;
    const bool sbf = first[0] == '$' && first[1] == '@';
    if (!ubx && !sbf) {
        return std::nullopt;
    }
    return std::vector<uint8_t>(first, first + static_cast<std::ptrdiff_t>(slice.size));
}

namespace GPSTest {

namespace {
QList<LogEntry> diagnosticsSince(qsizetype start)
{
    const QList<LogEntry> captured = LogManager::capturedMessages();
    QList<LogEntry> result;
    for (qsizetype index = start; index < captured.size(); ++index) {
        const LogEntry& entry = captured[index];
        if (entry.level < LogEntry::Critical && entry.category.startsWith(QLatin1StringView("GPS.Protocols"))) {
            result.append(entry);
        }
    }
    return result;
}
}  // namespace

GPSProtocolLogCapture::GPSProtocolLogCapture()
    : _start(LogManager::capturedMessages().size())
{
    if (auto* test = GPSProtocolTestBase::_running()) {
        test->_ignoreProtocolWarnings();
    }
}

void GPSProtocolLogCapture::clear()
{
    _start = LogManager::capturedMessages().size();
}

QStringList GPSProtocolLogCapture::warnings() const
{
    QStringList result;
    for (const LogEntry& entry : diagnosticsSince(_start)) {
        if (entry.level == LogEntry::Warning) {
            result.append(entry.message);
        }
    }
    return result;
}

QStringList GPSProtocolLogCapture::categories() const
{
    QStringList result;
    for (const LogEntry& entry : diagnosticsSince(_start)) {
        result.append(entry.category);
    }
    return result;
}

}  // namespace GPSTest
