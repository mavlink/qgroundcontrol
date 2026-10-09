#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <QtCore/QStringList>

#include "GPSProtocolEvent.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "UnitTest.h"

namespace GPSTest {

class GPSProtocolLogCapture;

/// One scenario of a protocol suite, run as its own data row on a fresh virtual clock.
struct ProtocolScenario
{
    const char* name;
    void (*run)(GPSTestClock& clock);
};

}  // namespace GPSTest

/// Base of the native GPS protocol suites. Scenario tables run as data rows, and a protocol violation that a receiver
/// model records fails the test function that caused it. Needs no QGCApplication.
class GPSProtocolTestBase : public UnitTest
{
    Q_OBJECT

protected:
    /// A "scenario" column with one row per entry of @a scenarios.
    static void addScenarioRows(std::span<const GPSTest::ProtocolScenario> scenarios);

    /// Runs the scenario of the current row. A failed check in any helper the scenario calls ends the row.
    static void runScenario(std::span<const GPSTest::ProtocolScenario> scenarios);

protected slots:
    void init() override;
    void cleanup() override;

private:
    friend class GPSTest::GPSProtocolLogCapture;

    /// The protocol suite running a test function; null between test functions.
    static GPSProtocolTestBase* _running();

    /// Accepts the protocol warnings (GPS.Protocols.*) for the rest of the current test function, which the
    /// suites assert through GPSProtocolLogCapture.
    void _ignoreProtocolWarnings();

    bool _protocolWarningsIgnored = false;
};

namespace GPSTest {

/// The protocol diagnostics (GPS.Protocols.*) logged while alive, read from the test log capture. Creating a
/// capture accepts their warnings in the strict log check for the rest of the running GPSProtocolTestBase test
/// function, so only scenarios that warn by design create one, and assert the warnings they read here.
class GPSProtocolLogCapture
{
public:
    GPSProtocolLogCapture();

    GPSProtocolLogCapture(const GPSProtocolLogCapture&) = delete;
    GPSProtocolLogCapture& operator=(const GPSProtocolLogCapture&) = delete;

    [[nodiscard]] QStringList warnings() const;

    /// Forgets the diagnostics logged so far.
    void clear();

    /// The category of every diagnostic, warnings and debug output alike, in order.
    [[nodiscard]] QStringList categories() const;

private:
    qsizetype _start;
};

/// The bytes of the recorded or synthetic fixture @a name, or nothing when it cannot be read.
[[nodiscard]] std::optional<std::vector<uint8_t>> fixtureBytes(const char* name);

/// A run of whole frames inside a recorded fixture, at the offsets documented in fixtures/SOURCES.md.
struct FixtureSlice
{
    const char* file;
    size_t offset;
    size_t size;
};

inline constexpr FixtureSlice NAV_PVT{"navigation.ubx", 0, 100};
inline constexpr FixtureSlice NAV_SAT{"navigation.ubx", 758, 532};
inline constexpr FixtureSlice NAV_DOP{"navigation.ubx", 1906, 26};
inline constexpr FixtureSlice PVT_GEODETIC{"geodetic.sbf", 0, 96};

/// The bytes of @a slice, or nothing when the file is short or the slice does not start with UBX/SBF sync.
[[nodiscard]] std::optional<std::vector<uint8_t>> fixtureBytes(const FixtureSlice& slice);

/// Whether @a actual is within @a tolerance of @a expected, or both are NaN.
inline bool matches(double actual, double expected, double tolerance = 1e-6)
{
    return std::isnan(expected) ? std::isnan(actual) : std::abs(actual - expected) < tolerance;
}

/// Survey state as bits: 1 when valid, 2 when active.
inline int surveyFlags(const GPSSurveyReport& report)
{
    return (report.valid ? 1 : 0) | (report.active ? 2 : 0);
}

inline uint32_t surveyDuration(const GPSSurveyReport& report)
{
    return static_cast<uint32_t>(report.duration.count());
}

}  // namespace GPSTest
