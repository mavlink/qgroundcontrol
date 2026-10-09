#pragma once

// The scenario table of the golden transcripts, and the wire builders, request builders and receiver benches its
// rows use. Each receiver family registers its scenarios in its own GoldenScenarios<Family>.cc and includes the
// receiver model it configures.

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>

#include "Protocols/Support/GPSProtocolTestData.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/GoldenTranscript.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/ScriptedFaults.h"
#include "Protocols/Support/ScriptedReceiver.h"

namespace GPSTest {
class AshtechReceiverModel;
class FemtoReceiverModel;
struct QuectelReceiverModel;
class SBFReceiverModel;
class UBXReceiverModel;
struct UnicoreReceiverModel;
}  // namespace GPSTest

namespace GPSTest::GoldenScenario {

using namespace std::chrono_literals;
using GPSTest::dataFile;
using GPSTest::FemtoReceiverModel;
using GPSTest::GPSTestClock;
using GPSTest::SBFReceiverModel;
using GPSTest::ScriptedReceiver;
using GPSTest::UBXReceiverModel;
using GPSTest::Golden::StreamStep;

// Nonzero, so a zero receipt timestamp always means "never received".
inline constexpr uint64_t GOLDEN_START_US = 1000000000;

// ---------------------------------------------------------------------------------------------------------------
// Receiver wire builders, independent of the protocol implementation.

namespace UBXWire {
inline constexpr uint16_t NAV_STATUS = 0x0301;
inline constexpr uint16_t NAV_PVT = 0x0701;
inline constexpr uint16_t NAV_SAT = 0x3501;
inline constexpr uint16_t NAV_SVIN = 0x3b01;
inline constexpr uint16_t NAV_EOE = 0x6101;
inline constexpr uint16_t INF_WARNING = 0x0104;
inline constexpr uint16_t ACK_NAK = 0x0005;
inline constexpr uint16_t CFG_VALSET = 0x8a06;
inline constexpr uint16_t MON_HW = 0x090a;
inline constexpr uint16_t MON_RF = 0x380a;
inline constexpr uint32_t KEY_RATE_MEAS = 0x30210001;
inline constexpr uint32_t KEY_SEC_JAMDET_SENSITIVITY_HI = 0x10f60051;
}  // namespace UBXWire

class Payload
{
public:
    explicit Payload(qsizetype size)
        : _bytes(size, '\0')
    {}

    template <typename T>
    Payload& set(qsizetype offset, T value)
    {
        if constexpr (sizeof(T) == 1) {
            _bytes[offset] = static_cast<char>(value);
        } else {
            (void) LittleEndian::write<T>(mutableBytesOf(_bytes), static_cast<size_t>(offset), value);
        }
        return *this;
    }

    const QByteArray& bytes() const { return _bytes; }

private:
    QByteArray _bytes;
};

inline QByteArray ubx(uint16_t message, const QByteArray& payload)
{
    return GPSTest::ubxBytes(message, payload);
}

inline QByteArray nmea(std::string_view body)
{
    return GPSTest::nmeaBytes(body);
}

inline QByteArray rtcm(const QByteArray& payload)
{
    return GPSTest::rtcmBytes(payload);
}

/// RTCM 1005 station coordinates, as published in Quectel's base-station application note.
QByteArray rtcm1005();
/// A short, well-framed 1077 header; decoders pass it through without interpreting observations.
QByteArray rtcm1077();
QByteArray navPvt(uint32_t tow);
QByteArray navEoe(uint32_t tow);
/// Synthetic mean ECEF position (0, 6378237 m, 0): latitude 0, longitude 90.
QByteArray navSvin(uint32_t tow, uint32_t duration, bool valid, bool active);
QByteArray navSat(uint32_t tow);
QByteArray monRf();
QByteArray navStatus();

std::vector<StreamStep> ubxStream();
std::vector<StreamStep> sbfStream();
std::vector<StreamStep> ashtechStream();
std::vector<StreamStep> femtoStream();
std::vector<StreamStep> unicoreStream();
std::vector<StreamStep> quectelStream();
std::vector<StreamStep> passiveStream();

// ---------------------------------------------------------------------------------------------------------------
// Request builders.

GPSReceiverConfig surveyIn(double accuracyMeters, int64_t seconds, uint32_t baud = 0);
GPSReceiverConfig fixedBase(double latitude, double longitude, float altitude, float accuracy, uint32_t baud = 0);
GPSReceiverConfig averaging(int64_t seconds, uint32_t baud = 0);
GPSReceiverConfig passiveInput(uint32_t baud);
GPSReceiverConfig compact(GPSReceiverConfig config);
GPSReceiverConfig persistent(GPSReceiverConfig config);

// ---------------------------------------------------------------------------------------------------------------
// Fault injection around any receiver model (Support/ScriptedFaults.h), and the UBX rules.

using GPSTest::afterCommand;
using GPSTest::latestReply;
using GPSTest::reply;
using GPSTest::ScriptedFaults;
using GPSTest::silence;
using GPSTest::startsWith;
using Rule = ScriptedFaults::Rule;

/// A reply that rejects the UBX CFG-VALSET frames setting @a key.
Rule ubxNak(uint32_t key);

// ---------------------------------------------------------------------------------------------------------------
// Receiver benches (Support/ReceiverBench.h): a model, optional fault rules, and the transport the protocol drives.

using GPSTest::ModelReceiver;
using GPSTest::ReceiverBench;
using BenchFactory = std::function<std::unique_ptr<ReceiverBench>(GPSTestClock&)>;
using UBXBench = ModelReceiver<UBXReceiverModel>;
using SBFBench = ModelReceiver<SBFReceiverModel>;
using AshtechBench = ModelReceiver<GPSTest::AshtechReceiverModel>;
using FemtoBench = ModelReceiver<FemtoReceiverModel>;
using UnicoreBench = ModelReceiver<GPSTest::UnicoreReceiverModel>;
using QuectelBench = ModelReceiver<GPSTest::QuectelReceiverModel>;
using PassiveBench = ModelReceiver<ScriptedReceiver::Model>;

/// The golden link of @a bench, whose receiver starts its session.
inline GPSTest::Golden::Link link(ReceiverBench& bench)
{
    bench.startSession();
    return {bench, bench.clock, bench.wait};
}

template <typename BenchType, typename... Args>
inline BenchFactory bench(std::function<void(BenchType&)> setup, Args... args)
{
    return [setup = std::move(setup), args...](GPSTestClock& clock) -> std::unique_ptr<ReceiverBench> {
        auto result = std::make_unique<BenchType>(clock, args...);
        if (setup) {
            setup(*result);
        }
        return result;
    };
}

/// A UBXReceiverModel its fields describe, a ZED-F9P unless the scenario changes them, on a UART without a fixed baud.
BenchFactory ubxWire(std::function<void(UBXBench&)> setup = {});
BenchFactory sbf(std::function<void(SBFBench&)> setup = {});
BenchFactory ashtech(std::function<void(AshtechBench&)> setup = {});
BenchFactory femto(std::function<void(FemtoBench&)> setup = {});
/// UnicoreReceiverModel's own services: waits run its receiver events, and its link faults fail reads.
BenchFactory unicore(std::function<void(UnicoreBench&)> setup = {});
/// QuectelReceiverModel's own services: its configured role and base are active and saved at connection time.
BenchFactory quectel(std::function<void(QuectelBench&)> setup = {});
/// A receiver that never answers.
BenchFactory passive(std::function<void(PassiveBench&)> setup = {});

/// The receiver only answers at @a baud.
void enforceBaud(ReceiverBench& b, unsigned baud);

// ---------------------------------------------------------------------------------------------------------------
// Scenario table. Names are golden file names; `about` is part of the golden.

struct ScenarioDef
{
    GPSType type;
    const char* name;
    const char* about;
    BenchFactory bench;
    GPSReceiverConfig config;
    /// Recorded once per distinct stream transcript: a scenario whose stream would repeat another's byte for byte
    /// leaves it empty.
    std::vector<StreamStep> stream;
    /// A scenario of the same type whose golden this one's transcript must match, beyond its about and request lines;
    /// it has no golden of its own.
    const char* sameAs = nullptr;
    /// GPSType::automatic: only detection and the result are recorded, as the family's own goldens pin its
    /// configuration.
    bool detectionOnly = false;
};

struct Table
{
    std::vector<ScenarioDef> rows;

    ScenarioDef& add(GPSType type, const char* name, const char* about, BenchFactory factory, GPSReceiverConfig config,
                     std::vector<StreamStep> stream = {})
    {
        return rows.emplace_back(
            ScenarioDef{type, name, about, std::move(factory), std::move(config), std::move(stream)});
    }
};

void addUblox(Table& table);
void addSeptentrio(Table& table);
void addTrimble(Table& table);
void addFemto(Table& table);
void addUnicore(Table& table);
void addQuectel(Table& table);
void addPassive(Table& table);
void addAutomatic(Table& table);

/// Every golden scenario, in golden-file order.
const std::vector<ScenarioDef>& scenarios();

}  // namespace GPSTest::GoldenScenario
