#pragma once

// Golden-transcript driver seam.
//
// GPSGoldenTranscriptTest pins the observable behaviour of every native receiver family: host-to-receiver bytes,
// baud changes, configuration evidence, identity and decoded events. The test and the checked-in goldens under
// test/GPS/Core/Protocols/golden/ only use this header. GoldenTranscript.cc is the only file that knows the protocol
// implementation; a re-architecture re-implements that file, and must reproduce every golden byte for byte.
//
// Contract of an implementation:
// - Configure exactly as GPSDriver::configure() does: gpsReceiverConfigError() validation first, then the baud
//   selection (the configured rate, else the transport's fixed rate, else the family's automatic rate), then
//   configuration, then completion of any outstanding write-only command, then an empty decode that flushes events
//   published by the final command. A validation or fixed-baud failure performs no I/O and reports only `error`.
//   GPSType::automatic first detects the family (GPSReceiverDetector) at the configured or fixed rate, else at every
//   candidate rate, then fits the request to it and configures it from the detected rate. The mismatch hint GPSDriver
//   appends to a configured type's error is a diagnostic, not part of the transcript.
// - Drive all I/O through Link::receiver on Link::clock. Time never passes except through a read, which ends at
//   its deadline when no data arrives, or through Link::wait. Real sleeps are not allowed.
// - Record, in order, every transport write call (bytes and result), every baud change (rate and status), every
//   completed configuration command (GPSConfigurationEvidence), and every decoded event. Write-call boundaries are
//   not significant: the transcript merges the contiguous writes that one command's evidence accounts for.
// - Host timestamps only matter as differences (evidence and configuration durations, rounded to milliseconds)
//   and as zero versus nonzero receipts.
// - After a successful configuration, run each StreamStep: queue its bytes on the receiver, then call the receive
//   loop with RECEIVE_TIMEOUT slices until `duration` of virtual time has passed, a terminal I/O failure occurs, or
//   MAX_RECEIVE_CALLS slices were used.
// - Never let protocol diagnostics reach the test log: failure scenarios emit warnings by design, and the
//   transcripts pin wire behaviour, not diagnostic text.
// - decodeGolden() configures Link as above (without stream steps), then decodes the bytes in chunks with the
//   receiver unreachable (any attempted I/O is counted and fails), then decodes nothing after FRESHNESS_HORIZON.
//   armedDecodeGolden() decodes the same way after arming a fresh decoder for the request, with no I/O at all.

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "GPSCommand.h"
#include "GPSProtocolEvent.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"
#include "GPSType.h"

namespace GPSTest {
class GPSTestClock;
class ScriptedReceiver;
}  // namespace GPSTest

namespace GPSTest::Golden {

/// GPSReceiverWorker's receive slice.
inline constexpr std::chrono::milliseconds RECEIVE_TIMEOUT{1200};
inline constexpr int MAX_RECEIVE_CALLS = 2000;
/// Beyond every diagnostic and base-status freshness limit.
inline constexpr std::chrono::microseconds FRESHNESS_HORIZON{5000001};

struct Link
{
    GPSTest::ScriptedReceiver& receiver;
    GPSTest::GPSTestClock& clock;
    /// Advances virtual time by the requested duration; false reports cancellation. Empty: advance the clock.
    std::function<bool(std::chrono::microseconds)> wait = {};
};

struct StreamStep
{
    QByteArray bytes;
    std::chrono::milliseconds duration{0};
};

struct Scenario
{
    GPSType type = GPSType::ublox;
    GPSReceiverConfig config{};
    std::vector<StreamStep> stream{};
};

enum class Phase
{
    Configure,
    Stream,
};

enum class Failure
{
    None,
    Cancelled,
    Transport,
    Protocol,
    InvalidArgument,
    ConsentRequired,
};

struct Write
{
    QByteArray bytes;
    GPSWriteStatus status = GPSWriteStatus::Completed;
    int acceptedBytes = 0;
    int writtenBytes = 0;
};

struct Baud
{
    unsigned rate = 0;
    GPSBaudStatus status = GPSBaudStatus::Configured;
};

struct Position
{
    GPSNavigationValues navigation{};
    bool velocityValid = false;
};

struct Satellites
{
    struct System
    {
        GPSConstellation constellation = GPSConstellation::Unknown;
        int inView = 0;
        std::optional<int> inUse = std::nullopt;
        uint64_t inViewTimestampUs = 0;
        uint64_t inUseTimestampUs = 0;
    };

    bool fullSnapshot = true;
    std::vector<System> systems{};
};

struct SatelliteUsage
{
    uint64_t timestampUs = 0;
    std::optional<int> used = std::nullopt;
};

struct Survey
{
    uint64_t timestampUs = 0;
    GPSSurveyReport survey{};
};

struct RTCM
{
    QByteArray frame;
};

using Event = std::variant<Position, GPSIntegrityReport, Satellites, SatelliteUsage, Survey, RTCM, GPSInputProtocol>;

struct Record
{
    Phase phase = Phase::Configure;
    std::variant<Write, Baud, GPSConfigurationEvidence, Event> value;
};

struct Run
{
    /// GPSDriver::configurationError(): empty after success.
    QString error;
    bool configured = false;
    unsigned requestedBaud = 0;
    unsigned baud = 0;
    Failure failure = Failure::None;
    bool ready = false;
    /// Trimmed model and firmware, also after a failed configuration.
    QString identity;
    /// GPSType::automatic: the family detection found, its rate and what identified it.
    std::optional<GPSType> detected;
    unsigned detectedBaud = 0;
    QString detectedEvidence;
    /// GPSType::automatic: records before this index are detection; the detected family's configuration follows.
    size_t detectionRecords = 0;
    uint64_t startedAtUs = 0;
    uint64_t configuredAtUs = 0;
    Failure streamFailure = Failure::None;
    bool streamReady = false;
    int receiveCalls = 0;
    std::vector<Record> records;
};

[[nodiscard]] Run runGolden(const Scenario& scenario, const Link& link);

struct Decode
{
    bool configured = false;
    std::vector<Event> events;
    /// Events from an empty decode after FRESHNESS_HORIZON.
    std::vector<Event> expired;
    int transportCalls = 0;
};

/// @a chunk of zero decodes all bytes at once.
[[nodiscard]] Decode decodeGolden(const Scenario& setup, const Link& link, QByteArrayView bytes, qsizetype chunk);

/// decodeGolden() with the decoder armed for @a setup without I/O (GPSProtocolRuntime::armDecodeOnly()) instead of
/// configured through Link; Decode::configured reports whether the family armed, and nothing is decoded otherwise.
[[nodiscard]] Decode armedDecodeGolden(const Scenario& setup, const Link& link, QByteArrayView bytes, qsizetype chunk);

}  // namespace GPSTest::Golden
