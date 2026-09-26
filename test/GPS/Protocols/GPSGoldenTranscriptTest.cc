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

#include "Checksums.h"
#include "Support/AshtechReceiverModel.h"
#include "Support/DetectionReceiver.h"
#include "Support/FemtoReceiverModel.h"
#include "Support/GPSTestClock.h"
#include "Support/GoldenTranscript.h"
#include "Support/QuectelReceiverModel.h"
#include "Support/SBFReceiverModel.h"
#include "Support/ScriptedReceiver.h"
#include "Support/UBXReceiverModel.h"
#include "Support/UnicoreReceiverModel.h"

using namespace std::chrono_literals;

namespace {

using GPSGolden::StreamStep;

// Nonzero, so a zero receipt timestamp always means "never received".
constexpr uint64_t START_US = 1000000000;

bool updateRequested()
{
    return qEnvironmentVariableIntValue("QGC_GPS_GOLDEN_UPDATE") == 1;
}

// ---------------------------------------------------------------------------------------------------------------
// Receiver wire builders, independent of the protocol implementation.

namespace UBXWire {
constexpr uint16_t NAV_STATUS = 0x0301;
constexpr uint16_t NAV_PVT = 0x0701;
constexpr uint16_t NAV_SAT = 0x3501;
constexpr uint16_t NAV_SVIN = 0x3b01;
constexpr uint16_t NAV_EOE = 0x6101;
constexpr uint16_t INF_ERROR = 0x0004;
constexpr uint16_t ACK_NAK = 0x0005;
constexpr uint16_t CFG_VALSET = 0x8a06;
constexpr uint16_t MON_RF = 0x380a;
constexpr uint32_t KEY_RATE_MEAS = 0x30210001;
constexpr uint32_t KEY_SEC_JAMDET_SENSITIVITY_HI = 0x10f60051;
constexpr uint32_t KEY_UART1INPROT_SPARTN = 0x10730005;
constexpr uint32_t KEY_MSGOUT_RXM_COR_UART1 = 0x209106b7;
constexpr uint32_t KEY_MSGOUT_NAV_EOE_UART1 = 0x20910160;
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
            qToLittleEndian<T>(value, _bytes.data() + offset);
        }
        return *this;
    }

    const QByteArray& bytes() const { return _bytes; }

private:
    QByteArray _bytes;
};

QByteArray ubx(uint16_t message, const QByteArray& payload)
{
    QByteArray frame;
    frame.append(char(0xb5)).append(char(0x62));
    frame.append(static_cast<char>(message & 0xff)).append(static_cast<char>(message >> 8));
    frame.append(static_cast<char>(payload.size() & 0xff)).append(static_cast<char>(payload.size() >> 8));
    frame.append(payload);
    uint8_t a = 0;
    uint8_t b = 0;
    for (qsizetype index = 2; index < frame.size(); ++index) {
        a = static_cast<uint8_t>(a + static_cast<uint8_t>(frame[index]));
        b = static_cast<uint8_t>(b + a);
    }
    return frame.append(static_cast<char>(a)).append(static_cast<char>(b));
}

QByteArray nmea(std::string_view body)
{
    uint8_t checksum = 0;
    for (const char byte : body) {
        checksum ^= static_cast<uint8_t>(byte);
    }
    return '$' + QByteArray(body.data(), static_cast<qsizetype>(body.size())) + '*' +
           QByteArray::number(checksum, 16).rightJustified(2, '0').toUpper() + "\r\n";
}

QByteArray rtcm(const QByteArray& payload)
{
    QByteArray frame;
    frame.append(char(0xd3)).append(static_cast<char>((payload.size() >> 8) & 0x03));
    frame.append(static_cast<char>(payload.size() & 0xff)).append(payload);
    const uint32_t crc =
        QGC::crc24q({reinterpret_cast<const uint8_t*>(frame.constData()), static_cast<size_t>(frame.size())});
    return frame.append(static_cast<char>(crc >> 16))
        .append(static_cast<char>(crc >> 8))
        .append(static_cast<char>(crc));
}

/// RTCM 1005 station coordinates, as published in Quectel's base-station application note.
QByteArray rtcm1005()
{
    return rtcm(QByteArray::fromHex("3ed122033a2266a8ee8b4a8c4d3507a1bddfbf"));
}

/// A short, well-framed 1077 header; decoders pass it through without interpreting observations.
QByteArray rtcm1077()
{
    return rtcm(QByteArray::fromHex("4350000000000000"));
}

QByteArray dataFile(const char* directory, const QString& name)
{
    QFile file(QString::fromUtf8(directory) + u'/' + name);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QByteArray navPvt(uint32_t tow)
{
    return ubx(UBXWire::NAV_PVT, Payload(92)
                                     .set<uint32_t>(0, tow)
                                     .set<uint16_t>(4, 2024)
                                     .set<uint8_t>(6, 1)
                                     .set<uint8_t>(7, 2)
                                     .set<uint8_t>(8, 3)
                                     .set<uint8_t>(9, 4)
                                     .set<uint8_t>(10, static_cast<uint8_t>(5 + tow / 1000))
                                     .set<uint8_t>(11, 0x07)
                                     .set<uint32_t>(12, 20)
                                     .set<uint8_t>(20, 3)
                                     .set<uint8_t>(21, 0x01)
                                     .set<uint8_t>(23, 12)
                                     .set<int32_t>(24, 80000000)
                                     .set<int32_t>(28, 470000000)
                                     .set<int32_t>(32, 500000)
                                     .set<int32_t>(36, 450000)
                                     .set<uint32_t>(40, 1500)
                                     .set<uint32_t>(44, 2500)
                                     .set<int32_t>(48, 100)
                                     .set<int32_t>(52, 200)
                                     .set<int32_t>(56, -50)
                                     .set<int32_t>(60, 224)
                                     .set<int32_t>(64, 6343500)
                                     .set<uint32_t>(68, 300)
                                     .set<uint32_t>(72, 500000)
                                     .set<uint16_t>(76, 150)
                                     .bytes());
}

QByteArray navEoe(uint32_t tow)
{
    return ubx(UBXWire::NAV_EOE, Payload(4).set<uint32_t>(0, tow).bytes());
}

/// Synthetic mean ECEF position (0, 6378237 m, 0): latitude 0, longitude 90.
QByteArray navSvin(uint32_t tow, uint32_t duration, bool valid, bool active)
{
    return ubx(UBXWire::NAV_SVIN, Payload(40)
                                      .set<uint32_t>(4, tow)
                                      .set<uint32_t>(8, duration)
                                      .set<int32_t>(16, 637823700)
                                      .set<uint32_t>(28, 25000)
                                      .set<uint32_t>(32, duration)
                                      .set<uint8_t>(36, valid)
                                      .set<uint8_t>(37, active)
                                      .bytes());
}

QByteArray navSat(uint32_t tow)
{
    Payload payload(8 + 3 * 12);
    payload.set<uint32_t>(0, tow).set<uint8_t>(4, 1).set<uint8_t>(5, 3);

    const struct
    {
        uint8_t gnss;
        uint8_t sv;
        bool used;
    } satellites[] = {{0, 1, true}, {0, 2, false}, {6, 3, true}};

    for (qsizetype index = 0; index < 3; ++index) {
        const qsizetype offset = 8 + index * 12;
        payload.set<uint8_t>(offset, satellites[index].gnss)
            .set<uint8_t>(offset + 1, satellites[index].sv)
            .set<uint8_t>(offset + 2, 40)
            .set<int8_t>(offset + 3, 45)
            .set<int16_t>(offset + 4, 120)
            .set<uint32_t>(offset + 8, satellites[index].used ? 0x08 : 0);
    }
    return ubx(UBXWire::NAV_SAT, payload.bytes());
}

QByteArray monRf()
{
    return ubx(UBXWire::MON_RF, Payload(28)
                                    .set<uint8_t>(1, 1)
                                    .set<uint8_t>(5, 3)
                                    .set<uint8_t>(6, 2)
                                    .set<uint8_t>(7, 1)
                                    .set<uint16_t>(16, 87)
                                    .set<uint16_t>(18, 5432)
                                    .set<uint8_t>(20, 12)
                                    .bytes());
}

QByteArray navStatus()
{
    return ubx(UBXWire::NAV_STATUS, Payload(16).set<uint8_t>(4, 3).set<uint8_t>(5, 1).set<uint8_t>(7, 2 << 3).bytes());
}

/// @a comms adds the transmit-buffer warning that requests MON-COMMS diagnostics.
std::vector<StreamStep> ubxStream(bool comms)
{
    return {
        {navSvin(1000, 30, false, true) + navPvt(1000) + navSat(1000) + navEoe(1000) + monRf() + navStatus(), 1500ms},
        {navSvin(2000, 181, true, false) + navPvt(2000) + navEoe(2000), 1500ms},
        {rtcm1005() + (comms ? ubx(UBXWire::INF_ERROR, "txbuf alloc") : QByteArray()), 3000ms},
    };
}

std::vector<StreamStep> sbfStream()
{
    return {
        {dataFile(GPS_FIXTURE_DIR, QStringLiteral("synthetic-valid.sbf")), 1000ms},
        {rtcm1005(), 1000ms},
        {{}, 6000ms},
    };
}

std::vector<StreamStep> ashtechStream()
{
    return {
        {nmea("GPZDA,114501.00,28,12,2011,00,00") + nmea("GPGST,114501.00,1.0,0.5,0.4,45.0,0.4,0.5,0.9") +
             nmea("PASHR,POS,2,12,114501.00,4700.00000,N,00800.00000,E,500.000,0,90,10,0,1,1,1,1,"),
         1000ms},
        {nmea(GPSTest::ASHTECH_SURVEY_FINISHED), 1000ms},
        {rtcm1005(), 1000ms},
    };
}

std::vector<StreamStep> femtoStream()
{
    return {
        {nmea("GPGGA,123519.00,4700.00000,N,00800.00000,E,1,12,0.9,450.000,M,50.000,M,,"), 1000ms},
        {nmea("GPGGA,123520.00,4700.00000,N,00800.00000,E,7,12,0.9,450.000,M,50.000,M,,"), 1000ms},
        {rtcm1005(), 1000ms},
    };
}

std::vector<StreamStep> unicoreStream()
{
    return {
        {nmea("GPGGA,123519.00,4700.00000,N,00800.00000,E,1,12,0.9,450.000,M,50.000,M,,") +
             nmea("GPGSV,1,1,03,01,40,083,46,02,17,308,41,12,07,344,39"),
         2000ms},
        // Corrections before the averaged base is valid are withheld; later ones pass through.
        {rtcm1005() + rtcm1077(), 5000ms},
        {rtcm1005(), 2000ms},
    };
}

std::vector<StreamStep> quectelStream()
{
    return {
        {{}, 12000ms},
        {rtcm1005(), 2000ms},
    };
}

std::vector<StreamStep> passiveStream()
{
    return {
        {dataFile(GPS_CORPUS_DIR, QStringLiteral("gga.nmea")) + dataFile(GPS_CORPUS_DIR, QStringLiteral("gsv.nmea")) +
             dataFile(GPS_CORPUS_DIR, QStringLiteral("zda.nmea")) +
             dataFile(GPS_FIXTURE_DIR, QStringLiteral("synthetic-gst.nmea")) + rtcm1005(),
         1000ms},
        {{}, 6000ms},
    };
}

// ---------------------------------------------------------------------------------------------------------------
// Request builders.

GPSReceiverConfig surveyIn(double accuracyMeters, int64_t seconds, uint32_t baud = 0)
{
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = accuracyMeters,
                                                            .duration = std::chrono::seconds(seconds)}},
            .baudRate = baud};
}

GPSReceiverConfig fixedBase(double latitude, double longitude, float altitude, float accuracy, uint32_t baud = 0)
{
    return {.base = {.mode = GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = latitude,
                                                                      .longitudeDegrees = longitude,
                                                                      .altitudeMeters = altitude},
                                                         .accuracyMeters = accuracy}},
            .baudRate = baud};
}

GPSReceiverConfig averaging(int64_t seconds, uint32_t baud = 0)
{
    return {.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = std::chrono::seconds(seconds)}},
            .baudRate = baud};
}

GPSReceiverConfig passiveInput(uint32_t baud)
{
    return {.role = GPSReceiverConfig::Role::Passive, .baudRate = baud};
}

GPSReceiverConfig compact(GPSReceiverConfig config)
{
    config.base.compactObservations = true;
    return config;
}

GPSReceiverConfig persistent(GPSReceiverConfig config)
{
    config.allowPersistentChanges = true;
    return config;
}

GPSReceiverConfig withRole(GPSReceiverConfig config, GPSReceiverConfig::Role role)
{
    config.role = role;
    return config;
}

// ---------------------------------------------------------------------------------------------------------------
// Fault injection around any receiver model.

/// Matching commands get a scripted reply (or silence) instead of reaching the model, or reach it and then run a
/// hook. Rules match in order; a rule applies to its first..last matching occurrences.
class ScriptedFaults final : public ScriptedReceiver::Model
{
public:
    struct Rule
    {
        std::function<bool(const QByteArray&)> matches;
        QByteArray reply = {};
        bool forward = false;
        std::function<void(ScriptedReceiver&)> after = {};
        /// Models that answer only the latest command drop earlier queued replies.
        bool clearReplies = false;
        int first = 1;
        int last = std::numeric_limits<int>::max();
        int seen = 0;
    };

    explicit ScriptedFaults(ScriptedReceiver::Model& model)
        : _model(model)
    {}

    std::vector<Rule> rules;

    void reset(ScriptedReceiver& receiver) override { _model.reset(receiver); }

    std::optional<QByteArray> takeCommand(QByteArray& pending) override { return _model.takeCommand(pending); }

    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        for (auto& rule : rules) {
            if (!rule.matches(command)) {
                continue;
            }
            const int occurrence = ++rule.seen;
            if (occurrence < rule.first || occurrence > rule.last) {
                continue;
            }
            if (rule.forward) {
                const auto result = _model.handleCommand(receiver, command, context);
                if (rule.after) {
                    rule.after(receiver);
                }
                return result;
            }
            if (rule.clearReplies) {
                receiver.clearReplies();
            }
            if (!rule.reply.isEmpty()) {
                receiver.queueReply(rule.reply);
            }
            const int size = static_cast<int>(command.size());
            return {GPSWriteStatus::Completed, size, size};
        }
        return _model.handleCommand(receiver, command, context);
    }

    std::optional<bool> handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate) override
    {
        return _model.handleBaudrate(receiver, baudrate);
    }

    void onTransportReadWait(ScriptedReceiver& receiver, std::chrono::milliseconds timeout) override
    {
        _model.onTransportReadWait(receiver, timeout);
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        _model.onProtocolReadWait(receiver, deadline);
    }

    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override
    {
        return _model.readChunkSize(receiver, requested, available);
    }

    bool coalesceReads(const ScriptedReceiver& receiver) const override { return _model.coalesceReads(receiver); }

private:
    ScriptedReceiver::Model& _model;
};

using Rule = ScriptedFaults::Rule;

std::function<bool(const QByteArray&)> startsWith(QByteArray prefix)
{
    return [prefix = std::move(prefix)](const QByteArray& command) { return command.startsWith(prefix); };
}

/// A UBX CFG-VALSET frame that sets @a key.
std::function<bool(const QByteArray&)> valsetWith(uint32_t key)
{
    return [key](const QByteArray& frame) {
        if (frame.size() < 12 || qFromLittleEndian<quint16>(frame.constData() + 2) != UBXWire::CFG_VALSET) {
            return false;
        }
        const qsizetype end = frame.size() - 2;
        for (qsizetype offset = 10; offset + 4 <= end;) {
            const auto candidate = qFromLittleEndian<quint32>(frame.constData() + offset);
            if (candidate == key) {
                return true;
            }
            const unsigned size = (candidate >> 28) & 7;
            offset += 4 + (size <= 2 ? 1 : qsizetype{1} << (size - 2));
        }
        return false;
    };
}

Rule reply(std::function<bool(const QByteArray&)> matches, QByteArray bytes, int first = 1,
           int last = std::numeric_limits<int>::max())
{
    return {.matches = std::move(matches), .reply = std::move(bytes), .first = first, .last = last};
}

Rule silence(QByteArray prefix)
{
    return {.matches = startsWith(std::move(prefix))};
}

Rule afterCommand(QByteArray prefix, std::function<void(ScriptedReceiver&)> hook)
{
    return {.matches = startsWith(std::move(prefix)), .forward = true, .after = std::move(hook)};
}

Rule ubxNak(uint32_t key)
{
    return reply(valsetWith(key), ubx(UBXWire::ACK_NAK, QByteArray::fromHex("068a")));
}

/// Septentrio and Femtomes models answer only the latest command.
Rule latestReply(QByteArray prefix, QByteArray bytes, int first = 1, int last = std::numeric_limits<int>::max())
{
    auto rule = reply(startsWith(std::move(prefix)), std::move(bytes), first, last);
    rule.clearReplies = true;
    return rule;
}

const QByteArray SBF_NAK = "$R? rejected\n";
const QByteArray FEMTO_NAK = "<ERROR\r\n";

// ---------------------------------------------------------------------------------------------------------------
// Receiver benches: a model, optional fault rules, and the transport the protocol drives.

class Bench
{
public:
    virtual ~Bench() = default;

    virtual GPSGolden::Link link() = 0;
};

template <typename Model>
class ModelBench final : public Bench
{
public:
    template <typename... Args>
    explicit ModelBench(GPSTestClock& testClock, Args&&... args)
        : clock(testClock)
        , model(std::forward<Args>(args)...)
        , faults(model)
        , receiver(stop, faults)
    {}

    GPSGolden::Link link() override { return {receiver, clock, wait}; }

    GPSTestClock& clock;
    Model model;
    ScriptedFaults faults;
    std::stop_source stop;
    ScriptedReceiver receiver;
    std::function<bool(std::chrono::microseconds)> wait;
};

using BenchFactory = std::function<std::unique_ptr<Bench>(GPSTestClock&)>;
using UBXBench = ModelBench<UBXReceiverModel>;
using SBFBench = ModelBench<SBFReceiverModel>;
using AshtechBench = ModelBench<GPSTest::AshtechReceiverModel>;
using FemtoBench = ModelBench<FemtoReceiverModel>;
using UnicoreBench = ModelBench<GPSTest::UnicoreReceiver>;
using QuectelBench = ModelBench<GPSTest::QuectelReceiver>;
using PassiveBench = ModelBench<ScriptedReceiver::Model>;

template <typename BenchType, typename... Args>
BenchFactory bench(std::function<void(BenchType&)> setup, Args... args)
{
    return [setup = std::move(setup), args...](GPSTestClock& clock) -> std::unique_ptr<Bench> {
        auto result = std::make_unique<BenchType>(clock, args..., clock);
        if (setup) {
            setup(*result);
        }
        return result;
    };
}

/// A UBXReceiverModel::Receiver profile on its fixed 115200 baud transport, as GPSDriverTest drives it.
BenchFactory ubxProfile(UBXReceiverModel::Receiver receiver, std::function<void(UBXBench&)> setup = {})
{
    return bench<UBXBench>(std::move(setup), receiver);
}

/// UBXReceiverModel's wire-level behaviour on a UART without a fixed baud.
BenchFactory ubxWire(std::function<void(UBXBench&)> setup = {})
{
    return bench<UBXBench>(
        [setup = std::move(setup)](UBXBench& b) {
            b.model.lowLevelProtocolBehavior = true;
            b.receiver.setFixedBaudrate(0);
            if (setup) {
                setup(b);
            }
        },
        UBXReceiverModel::Receiver::F9P);
}

BenchFactory sbf(std::function<void(SBFBench&)> setup = {})
{
    return bench<SBFBench>(std::move(setup));
}

BenchFactory ashtech(std::function<void(AshtechBench&)> setup = {})
{
    return bench<AshtechBench>(std::move(setup));
}

BenchFactory femto(std::function<void(FemtoBench&)> setup = {})
{
    return bench<FemtoBench>(std::move(setup));
}

/// UnicoreReceiver's own services: waits run its receiver events, and its fault flags fail reads.
BenchFactory unicore(std::function<void(UnicoreBench&)> setup = {})
{
    return bench<UnicoreBench>([setup = std::move(setup)](UnicoreBench& b) {
        auto& model = b.model;
        if (setup) {
            setup(b);
        }
        model.startedUs = model.clock.nowUs();
        b.wait = [&model](std::chrono::microseconds delay) {
            model.events.advanceTo(model.clock.nowUs() + static_cast<uint64_t>(delay.count()));
            return !model.cancel;
        };
        b.receiver.setReadHandler([&model](uint8_t*, int, std::chrono::milliseconds) -> std::optional<GPSReadResult> {
            if (model.cancel) {
                return GPSReadResult{GPSReadStatus::Cancelled};
            }
            if (model.readError) {
                return GPSReadResult{GPSReadStatus::Error, 0, QStringLiteral("Unicore test disconnect")};
            }
            return std::nullopt;
        });
    });
}

/// QuectelReceiver's own services: its configured role and base are active and saved at connection time.
BenchFactory quectel(std::function<void(QuectelBench&)> setup = {})
{
    return bench<QuectelBench>([setup = std::move(setup)](QuectelBench& b) {
        auto& model = b.model;
        if (setup) {
            setup(b);
        }
        model.startedUs = model.clock.nowUs();
        model.activeRole = model.savedRole = model.role;
        model.activeBase = model.savedBase = model.base;
        model.savedRates = model.rates;
        const auto cancelled = [&model] {
            return model.failed && model.fault == GPSTest::QuectelReceiver::Fault::Cancel;
        };
        b.wait = [&model, cancelled](std::chrono::microseconds delay) {
            model.events.advanceTo(model.clock.nowUs() + static_cast<uint64_t>(delay.count()));
            return !cancelled();
        };
        b.receiver.setReadHandler(
            [cancelled](uint8_t*, int, std::chrono::milliseconds) -> std::optional<GPSReadResult> {
                if (cancelled()) {
                    return GPSReadResult{GPSReadStatus::Cancelled};
                }
                return std::nullopt;
            });
    });
}

/// A receiver that never answers.
BenchFactory passive(std::function<void(PassiveBench&)> setup = {})
{
    return [setup = std::move(setup)](GPSTestClock& clock) -> std::unique_ptr<Bench> {
        auto result = std::make_unique<PassiveBench>(clock);
        if (setup) {
            setup(*result);
        }
        return result;
    };
}

/// A receiver behind receiver detection: it parses only its own dialect, and answers and streams only while the host
/// runs the link at the receiver's rate.
template <typename Model>
class AutomaticBench final : public Bench
{
public:
    template <typename... Args>
    AutomaticBench(GPSTestClock& testClock, GPSTest::Dialect dialect, Args&&... args)
        : clock(testClock)
        , model(std::forward<Args>(args)...)
        , faults(model)
        , detection(faults, dialect, clock, [this] { return atRate(); })
        , receiver(stop, detection)
    {
        receiver.setFixedBaudrate(0);
        detection.attach(receiver);
    }

    GPSGolden::Link link() override { return {receiver, clock, wait}; }

    GPSTestClock& clock;
    Model model;
    ScriptedFaults faults;
    GPSTest::DetectionReceiver detection;
    std::stop_source stop;
    ScriptedReceiver receiver;
    std::function<bool(std::chrono::microseconds)> wait;
    /// Defaults to the rate setReceiverBaudrate() gave the receiver.
    std::function<bool()> atRate = [this] { return receiver.hostBaudrate() == receiver.receiverBaudrate(); };
};

template <typename Model, typename... Args>
BenchFactory automatic(GPSTest::Dialect dialect, std::function<void(AutomaticBench<Model>&)> setup, Args... args)
{
    return [dialect, setup = std::move(setup), args...](GPSTestClock& clock) -> std::unique_ptr<Bench> {
        auto result = std::make_unique<AutomaticBench<Model>>(clock, dialect, args..., clock);
        if (setup) {
            setup(*result);
        }
        return result;
    };
}

/// The receiver only answers at @a baud.
template <typename BenchType>
void enforceBaud(BenchType& b, unsigned baud)
{
    b.receiver.setReceiverBaudrate(baud);
    b.receiver.setBaudrateEnforced(true);
}

// ---------------------------------------------------------------------------------------------------------------
// Scenario table. Names are golden file names; `about` is part of the golden.

struct ScenarioDef
{
    GPSType type;
    const char* name;
    const char* about;
    BenchFactory bench;
    GPSReceiverConfig config;
    std::vector<StreamStep> stream;
};

struct Table
{
    std::vector<ScenarioDef> rows;

    void add(GPSType type, const char* name, const char* about, BenchFactory factory, GPSReceiverConfig config,
             std::vector<StreamStep> stream = {})
    {
        rows.push_back({type, name, about, std::move(factory), std::move(config), std::move(stream)});
    }
};

void addUblox(Table& table)
{
    using Receiver = UBXReceiverModel::Receiver;
    constexpr GPSType type = GPSType::ublox;
    const auto survey = surveyIn(2.0, 180);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = ubxStream(false);
    const auto wireStream = ubxStream(true);
    const auto wireSurvey = surveyIn(1.25, 60, 115200);
    const auto wireFixed = fixedBase(47, 8, 500, 1, 115200);

    const struct
    {
        const char* name;
        Receiver receiver;
    } profiles[] = {
        {"profile-m8n-survey", Receiver::M8N}, {"profile-m9n-survey", Receiver::M9N},
        {"profile-m10-survey", Receiver::M10}, {"profile-m8p-rover-survey", Receiver::M8PRover},
        {"profile-f9r-survey", Receiver::F9R}, {"profile-unidentified-survey", Receiver::Unidentified},
        {"profile-u6-survey", Receiver::U6},   {"profile-m8n-early-survey", Receiver::M8NEarly},
    };

    table.add(type, "profile-m8p-base-survey", "M8P base profile, survey-in", ubxProfile(Receiver::M8PBase), survey,
              stream);
    table.add(type, "profile-m8p-base-fixed", "M8P base profile, fixed base", ubxProfile(Receiver::M8PBase), fixed,
              stream);
    table.add(type, "profile-m8p-base-fixed-compact", "M8P base profile, fixed base with MSM4",
              ubxProfile(Receiver::M8PBase), compact(fixed), stream);
    table.add(type, "profile-f9p-survey", "F9P profile, survey-in", ubxProfile(Receiver::F9P), survey, stream);
    table.add(type, "profile-f9p-fixed", "F9P profile, fixed base", ubxProfile(Receiver::F9P), fixed, stream);
    table.add(type, "profile-f9p-survey-compact", "F9P profile, survey-in with MSM4", ubxProfile(Receiver::F9P),
              compact(survey), stream);
    table.add(type, "profile-f9p-fixed-compact", "F9P profile, fixed base with MSM4", ubxProfile(Receiver::F9P),
              compact(fixed), stream);
    for (const auto& profile : profiles) {
        table.add(type, profile.name, "Receiver profile without base support", ubxProfile(profile.receiver), survey,
                  stream);
    }
    table.add(type, "profile-f9p-survey-restart", "F9P left surveying by an earlier session",
              ubxProfile(Receiver::F9P,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.retainedSurveyDuration = 329000;
                         }),
              survey, stream);
    table.add(type, "profile-f9p-survey-stop-stuck", "F9P whose earlier survey never stops",
              ubxProfile(Receiver::F9P,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.retainedSurveyDuration = 329000;
                             b.model.surveyStopStuck = true;
                         }),
              survey, stream);
    table.add(type, "profile-m8p-base-survey-stop-stuck", "M8P whose earlier survey never stops",
              ubxProfile(Receiver::M8PBase,
                         [](UBXBench& b) {
                             b.model.timeMode = 1;
                             b.model.surveyStopStuck = true;
                         }),
              survey, stream);
    table.add(type, "profile-f9p-corrupt-version", "F9P with corrupt identity replies around the valid one",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.corruptVersionReplies = true; }), fixed, stream);
    table.add(type, "profile-m8p-base-corrupt-version", "M8P with corrupt identity replies around the valid one",
              ubxProfile(Receiver::M8PBase, [](UBXBench& b) { b.model.corruptVersionReplies = true; }), fixed, stream);
    table.add(
        type, "profile-f9p-disable-nak", "F9P rejects disabling time mode (required)",
        ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.disableReply = UBXReceiverModel::DisableReply::Nak; }),
        survey, stream);
    table.add(
        type, "profile-f9p-disable-timeout", "F9P never acknowledges disabling time mode",
        ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.disableReply = UBXReceiverModel::DisableReply::Timeout; }),
        survey, stream);
    table.add(type, "profile-f9p-stale-disable-ack", "F9P repeats a stale acknowledgement before disabling",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.staleDisableAck = true; }), survey, stream);
    table.add(
        type, "profile-f9p-readback-nak", "F9P rejects the time-mode readback",
        ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.readbackReply = UBXReceiverModel::ReadbackReply::Nak; }),
        survey, stream);
    table.add(type, "profile-f9p-readback-wrong-value", "F9P reads back a different time mode",
              ubxProfile(Receiver::F9P,
                         [](UBXBench& b) { b.model.readbackReply = UBXReceiverModel::ReadbackReply::WrongValue; }),
              survey, stream);
    table.add(type, "profile-f9p-survey-rtcm-rejected", "F9P rejects RTCM activation after the survey completes",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), survey, stream);
    table.add(type, "profile-f9p-fixed-rtcm-rejected", "F9P rejects RTCM activation for a fixed base",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), fixed, stream);
    table.add(type, "profile-m8p-base-fixed-rtcm-rejected", "M8P rejects RTCM activation for a fixed base",
              ubxProfile(Receiver::M8PBase, [](UBXBench& b) { b.model.rejectRtcmActivation = true; }), fixed, stream);
    table.add(type, "profile-f9p-delayed-optional-ack", "F9P acknowledges an optional command late",
              ubxProfile(Receiver::F9P, [](UBXBench& b) { b.model.delayOptionalAck = true; }), survey, stream);
    table.add(type, "profile-f9p-maximum-fixed-accuracy", "F9P fixed base at the largest accuracy value",
              ubxProfile(Receiver::F9P), fixedBase(47, 8, 500, 429496.71875f), stream);
    table.add(type, "profile-m8p-base-maximum-fixed-accuracy", "M8P fixed base at the largest accuracy value",
              ubxProfile(Receiver::M8PBase), fixedBase(47, 8, 500, 429496.71875f), stream);
    table.add(type, "profile-f9p-explicit-baud", "F9P with the transport's own fixed baud requested",
              ubxProfile(Receiver::F9P), surveyIn(2.0, 180, 115200), stream);
    table.add(type, "profile-f9p-explicit-baud-mismatch", "Requested baud differs from the fixed transport baud",
              ubxProfile(Receiver::F9P), surveyIn(2.0, 180, 38400), stream);
    table.add(type, "profile-f9p-persistent", "Persistent changes are not supported", ubxProfile(Receiver::F9P),
              persistent(survey), stream);
    table.add(type, "profile-f9p-averaging", "Receiver averaging is not supported", ubxProfile(Receiver::F9P),
              averaging(60), stream);
    table.add(type, "profile-f9p-passive-role", "Passive input is not a u-blox role", ubxProfile(Receiver::F9P),
              withRole(survey, GPSReceiverConfig::Role::Passive), stream);
    table.add(type, "profile-f9p-invalid-fixed", "Fixed base without a position", ubxProfile(Receiver::F9P),
              {.base = {.mode = GPSBaseStationConfig::Fixed{}}}, stream);

    // Wire-level model (UBXReceiverModel::lowLevelProtocolBehavior).
    const auto module = [](const char* name, const char* hardware = "") {
        return [name, hardware](UBXBench& b) {
            b.model.module = name;
            b.model.hardware = hardware;
        };
    };
    const auto legacyModule = [](const char* name) {
        return [name](UBXBench& b) {
            b.model.legacy = true;
            b.model.module = name;
        };
    };
    table.add(type, "wire-f9p-survey", "ZED-F9P (protocol 27), survey-in", ubxWire(), wireSurvey, wireStream);
    table.add(type, "wire-f9p-fixed", "ZED-F9P (protocol 27), fixed base", ubxWire(), wireFixed, wireStream);
    table.add(type, "wire-f9p-survey-compact", "ZED-F9P, survey-in with MSM4", ubxWire(), compact(wireSurvey),
              wireStream);
    table.add(type, "wire-f9p-fixed-compact", "ZED-F9P, fixed base with MSM4", ubxWire(), compact(wireFixed),
              wireStream);
    table.add(type, "wire-f9p-l1l5-survey", "ZED-F9P L1/L5 firmware (FWVER=HPGL1L5), survey-in",
              ubxWire(module("ZED-F9P FWVER=HPGL1L5")), wireSurvey, wireStream);
    table.add(type, "wire-f9p-l1l5-fixed", "ZED-F9P L1/L5 firmware (FWVER=HPGL1L5), fixed base",
              ubxWire(module("ZED-F9P FWVER=HPGL1L5")), wireFixed, wireStream);
    table.add(type, "wire-x20-survey", "ZED-X20P, survey-in", ubxWire(module("ZED-X20P")), wireSurvey, wireStream);
    table.add(type, "wire-x20-fixed-compact", "ZED-X20P, fixed base with MSM4", ubxWire(module("ZED-X20P")),
              compact(wireFixed), wireStream);
    table.add(type, "wire-m8p-legacy-survey", "NEO-M8P (pre-protocol-27 commands), survey-in",
              ubxWire(legacyModule("NEO-M8P")), wireSurvey, wireStream);
    table.add(type, "wire-m8p-legacy-fixed", "NEO-M8P (pre-protocol-27 commands), fixed base",
              ubxWire(legacyModule("NEO-M8P")), wireFixed, wireStream);
    table.add(type, "wire-m8p-legacy-fixed-compact", "NEO-M8P, fixed base with MSM4", ubxWire(legacyModule("NEO-M8P")),
              compact(wireFixed), wireStream);
    table.add(type, "wire-f9p-legacy-protocol", "ZED-F9P hardware reporting PROTVER 20.30", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.hardware = "00190000";
                  b.model.protocol = "20.30";
              }),
              wireSurvey, wireStream);
    table.add(type, "wire-m8n-legacy", "NEO-M8N cannot be a base", ubxWire(legacyModule("NEO-M8N")), wireSurvey);
    table.add(type, "wire-m9n", "NEO-M9N cannot be a base", ubxWire(module("NEO-M9N")), wireSurvey);
    table.add(type, "wire-m10", "MAX-M10S cannot be a base", ubxWire(module("MAX-M10S", "000A0000")), wireSurvey);
    table.add(type, "wire-dan-f10n", "DAN-F10N (L1/L5 M10) cannot be a base", ubxWire(module("DAN-F10N", "000A0000")),
              wireSurvey);

    const auto uart = [](unsigned rate, std::function<void(UBXReceiverModel&)> extra = {}) {
        return ubxWire([rate, extra](UBXBench& b) {
            b.model.receiverBaud = rate;
            b.model.protocol = "27.31";
            if (extra) {
                extra(b.model);
            }
        });
    };
    table.add(type, "wire-auto-baud-9600", "Detects a receiver at 9600 and raises it to 115200", uart(9600),
              surveyIn(1, 60), wireStream);
    table.add(type, "wire-auto-baud-115200", "Detects a receiver already at 115200", uart(115200), surveyIn(1, 60),
              wireStream);
    table.add(type, "wire-auto-baud-lost-ack", "Baud handoff without an ACK forces a readback",
              uart(9600, [](UBXReceiverModel& m) { m.loseBaudAck = true; }), surveyIn(1, 60), wireStream);
    table.add(type, "wire-auto-baud-lost-ack-jamdet-nak",
              "Baud handoff without an ACK, then CFG-SEC-JAMDET rejected: falls back to CFG-ITFM",
              uart(9600,
                   [](UBXReceiverModel& m) {
                       m.loseBaudAck = true;
                       m.unsupportedKeys = {UBXWire::KEY_SEC_JAMDET_SENSITIVITY_HI};
                   }),
              surveyIn(1, 60), wireStream);
    table.add(type, "wire-auto-baud-usb", "USB port answers at every host rate",
              uart(9600, [](UBXReceiverModel& m) { m.usb = true; }), surveyIn(1, 60), wireStream);
    table.add(type, "wire-explicit-baud-9600", "Explicit 9600 matches the receiver", uart(9600), surveyIn(1, 60, 9600),
              wireStream);
    table.add(type, "wire-explicit-baud-mismatch", "Explicit 38400 while the receiver is at 9600", uart(9600),
              surveyIn(1, 60, 38400));
    table.add(type, "wire-legacy-auto-baud-9600", "NEO-M8P detected at 9600 and raised with CFG-PRT",
              ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.receiverBaud = 9600;
              }),
              surveyIn(1, 60), wireStream);
    table.add(type, "wire-legacy-auto-baud-lost-ack", "NEO-M8P baud handoff without an ACK", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.receiverBaud = 9600;
                  b.model.loseBaudAck = true;
              }),
              surveyIn(1, 60), wireStream);

    const struct
    {
        const char* name;
        const char* about;
        std::function<void(UBXReceiverModel&)> fault;
    } discovery[] = {
        {"wire-discovery-unknown-hardware", "Identity reports unknown hardware",
         [](UBXReceiverModel& m) { m.hardware = "UNKN0WN!"; }},
        {"wire-discovery-invalid-protocol", "Identity reports an invalid protocol version",
         [](UBXReceiverModel& m) { m.protocol = "invalid"; }},
        {"wire-discovery-identity-timeout", "Identity replies are always corrupt",
         [](UBXReceiverModel& m) { m.corruptIdentity = true; }},
        {"wire-discovery-silent-port-configuration", "Port configuration is never acknowledged",
         [](UBXReceiverModel& m) { m.silencePortConfiguration = true; }},
        {"wire-discovery-ignored-baud-change", "Receiver ignores the new baud and the ACK is lost",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.ignoreBaudChange = true;
         }},
        {"wire-discovery-ignored-baud-change-usb", "USB receiver ignores the new baud and the ACK is lost",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.ignoreBaudChange = true;
             m.usb = true;
         }},
        {"wire-discovery-rejected-after-baud-change", "Late ACK arrives, then the next command is dropped",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.rejectAfterBaudChange = true;
         }},
        {"wire-discovery-m9n", "NEO-M9N discovered at 9600",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.module = "NEO-M9N";
         }},
        {"wire-discovery-nak-after-baud-change", "Late ACK arrives, then the next command is rejected",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.nakAfterBaudChange = true;
         }},
        {"wire-discovery-readback-timeout", "Baud readback after a lost ACK times out",
         [](UBXReceiverModel& m) {
             m.loseBaudAck = true;
             m.readbackReply = UBXReceiverModel::ReadbackReply::Timeout;
         }},
    };

    for (const auto& scenario : discovery) {
        table.add(type, scenario.name, scenario.about, uart(9600, scenario.fault), surveyIn(1, 60));
    }

    table.add(type, "wire-f9p-disable-rejected", "Receiver rejects disabling time mode (required)",
              ubxWire([](UBXBench& b) { b.model.rejectDisable = true; }), wireSurvey, wireStream);
    table.add(type, "wire-f9p-start-rejected", "Receiver rejects starting the survey (base mode)",
              ubxWire([](UBXBench& b) { b.model.rejectStart = true; }), wireSurvey, wireStream);
    table.add(type, "wire-m8p-legacy-disable-rejected", "NEO-M8P rejects disabling time mode", ubxWire([](UBXBench& b) {
                  b.model.legacy = true;
                  b.model.module = "NEO-M8P";
                  b.model.rejectDisable = true;
              }),
              wireSurvey, wireStream);
    table.add(type, "wire-f9p-readback-wrong-value", "Time-mode readback differs",
              ubxWire([](UBXBench& b) { b.model.readbackReply = UBXReceiverModel::ReadbackReply::WrongValue; }),
              wireSurvey, wireStream);

    const struct
    {
        const char* name;
        const char* about;
        uint32_t key;
    } rejectedKeys[] = {
        {"wire-f9p-nak-jamming-detection", "Optional CFG-SEC-JAMDET rejected: falls back to CFG-ITFM",
         UBXWire::KEY_SEC_JAMDET_SENSITIVITY_HI},
        {"wire-f9p-nak-correction-status", "Optional RXM-COR rejected: falls back to RXM-RTCM",
         UBXWire::KEY_MSGOUT_RXM_COR_UART1},
        {"wire-f9p-nak-spartn", "Optional SPARTN input rejected", UBXWire::KEY_UART1INPROT_SPARTN},
        {"wire-f9p-nak-epoch-end", "Optional NAV-EOE output rejected", UBXWire::KEY_MSGOUT_NAV_EOE_UART1},
        {"wire-f9p-nak-rate", "Required measurement rate rejected", UBXWire::KEY_RATE_MEAS},
    };

    for (const auto& scenario : rejectedKeys) {
        table.add(type, scenario.name, scenario.about,
                  ubxWire([key = scenario.key](UBXBench& b) { b.faults.rules.push_back(ubxNak(key)); }), wireSurvey,
                  wireStream);
    }
}

void addSeptentrio(Table& table)
{
    constexpr GPSType type = GPSType::septentrio;
    const auto survey = surveyIn(1, 60);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = sbfStream();
    const auto rules = [](std::vector<Rule> faults) { return sbf([faults](SBFBench& b) { b.faults.rules = faults; }); };
    table.add(type, "survey", "USB connection, survey-in", sbf(), survey, stream);
    table.add(type, "fixed", "USB connection, fixed base", sbf(), fixed, stream);
    table.add(type, "survey-explicit-baud", "Explicit 230400 request; the driver selects its own rate", sbf(),
              surveyIn(1, 60, 230400), stream);
    table.add(type, "com-port-survey", "Serial COM1 connection, survey-in", rules({latestReply("\n\r", "COM1>")}),
              survey, stream);
    table.add(type, "com-port-fixed", "Serial COM1 connection, fixed base", rules({latestReply("\n\r", "COM1>")}),
              fixed, stream);
    table.add(type, "port-detection-timeout", "Receiver never answers the port prompt", rules({silence("\n\r")}),
              survey, stream);
    table.add(type, "nak-optional-output-disable", "Receiver rejects disabling a COM output (optional)",
              rules({latestReply("setDataInOut,COM1", SBF_NAK)}), survey, stream);
    table.add(type, "nak-required-reset", "Receiver rejects resetting the SBF output (required)",
              rules({latestReply("setSBFOutput, Stream1, USB1, none", SBF_NAK)}), survey, stream);
    table.add(type, "data-io-retry", "Second data-port application rejected twice, then accepted",
              rules({latestReply("setDataInOut, USB1, Auto, SBF", SBF_NAK, 2, 3)}), survey, stream);
    table.add(type, "data-io-retry-exhausted", "Second data-port application rejected on every attempt",
              rules({latestReply("setDataInOut, USB1, Auto, SBF", SBF_NAK, 2)}), survey, stream);
    table.add(type, "survey-rejected", "Receiver rejects survey-in (base mode)",
              rules({latestReply("setPVTMode", SBF_NAK)}), survey, stream);
    table.add(type, "fixed-rejected", "Receiver rejects the static position (base mode)",
              rules({latestReply("setStaticPosGeodetic", SBF_NAK)}), fixed, stream);
    table.add(type, "persistent", "Persistent changes are not supported", sbf(), persistent(survey), stream);
    table.add(type, "averaging", "Receiver averaging is not supported", sbf(), averaging(60), stream);
    table.add(type, "compact", "MSM4 corrections are not supported", sbf(), compact(survey), stream);
}

void addTrimble(Table& table)
{
    constexpr GPSType type = GPSType::trimble;
    const auto survey = surveyIn(1, 100);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = ashtechStream();
    const QByteArray nak = nmea("PASHR,NAK");
    const auto rules = [](std::vector<Rule> faults) {
        return ashtech([faults](AshtechBench& b) { b.faults.rules = faults; });
    };
    const auto at38400 = [](bool followSpeedChange) {
        return ashtech([followSpeedChange](AshtechBench& b) {
            enforceBaud(b, 38400);
            if (followSpeedChange) {
                b.faults.rules.push_back(afterCommand(
                    "$PASHS,SPD", [](ScriptedReceiver& receiver) { receiver.setReceiverBaudrate(115200); }));
            }
        });
    };
    table.add(type, "survey", "MB-Two, survey-in started by the first position", ashtech(), survey, stream);
    table.add(type, "fixed", "MB-Two, fixed base", ashtech(), fixed, stream);
    table.add(type, "survey-explicit-115200", "Explicit 115200 request", ashtech(), surveyIn(1, 100, 115200), stream);
    table.add(type, "explicit-baud-change", "Receiver at 38400 follows the speed command to 115200", at38400(true),
              surveyIn(1, 100, 38400), stream);
    table.add(type, "explicit-baud-change-ignored", "Receiver at 38400 ignores the speed command", at38400(false),
              surveyIn(1, 100, 38400), stream);
    table.add(type, "explicit-baud-unsupported", "Explicit 4800 is not an Ashtech probe rate", ashtech(),
              surveyIn(1, 100, 4800), stream);
    table.add(type, "port-query-timeout", "Receiver never answers the port query (identity)",
              rules({silence("$PASHQ,PRT")}), survey, stream);
    table.add(type, "port-query-nak", "Receiver rejects the port query", rules({reply(startsWith("$PASHQ,PRT"), nak)}),
              survey, stream);
    table.add(type, "board-query-timeout", "Receiver never answers the board query", rules({silence("$PASHQ,RID")}),
              survey, stream);
    table.add(type, "nak-optional-output", "Receiver rejects the 20 Hz update rate (optional)",
              rules({reply(startsWith("$PASHS,POP"), nak)}), survey, stream);
    table.add(type, "survey-rejected", "Receiver rejects position averaging (base mode)",
              rules({reply(startsWith("$PASHS,POS,AVG"), nak)}), survey, stream);
    table.add(type, "survey-failed-receipt", "Position averaging reports an error receipt",
              ashtech([](AshtechBench& b) { b.model.surveyReply = GPSTest::ASHTECH_SURVEY_FAILED; }), survey, stream);
    table.add(type, "fixed-rejected", "Receiver rejects the fixed position (base mode)",
              rules({reply(startsWith("$PASHS,POS,"), nak)}), fixed, stream);
    table.add(type, "rtcm-output-rejected", "Receiver rejects an RTCM output message",
              rules({reply(startsWith("$PASHS,RT3,1074"), nak)}), fixed, stream);
    table.add(type, "persistent", "Persistent changes are not supported", ashtech(), persistent(survey), stream);
    table.add(type, "averaging", "Receiver averaging is not supported", ashtech(), averaging(60), stream);
    table.add(type, "compact", "MSM4 corrections are not supported", ashtech(), compact(survey), stream);
}

void addFemto(Table& table)
{
    constexpr GPSType type = GPSType::femto;
    const auto survey = surveyIn(1, 60);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = femtoStream();
    const auto rules = [](std::vector<Rule> faults) {
        return femto([faults](FemtoBench& b) { b.faults.rules = faults; });
    };
    table.add(type, "survey", "Survey-in through receiver position averaging", femto(), survey, stream);
    table.add(type, "fixed", "Fixed base", femto(), fixed, stream);
    table.add(type, "survey-explicit-115200", "Explicit 115200 request", femto(), surveyIn(1, 60, 115200), stream);
    table.add(type, "explicit-baud-unsupported", "Explicit 9600 is not supported", femto(), surveyIn(1, 60, 9600),
              stream);
    table.add(type, "version-timeout", "Receiver never answers the version query (identity)",
              rules({silence("VERSION")}), survey, stream);
    table.add(type, "unlog-nak-once", "First UNLOGALL rejected; the second round succeeds",
              rules({latestReply("UNLOGALL", FEMTO_NAK, 1, 1)}), survey, stream);
    table.add(type, "survey-rejected", "Receiver rejects position averaging (base mode)",
              rules({latestReply("POSAVE", FEMTO_NAK)}), survey, stream);
    table.add(type, "fixed-rejected", "Receiver rejects the fixed position (base mode)",
              rules({latestReply("FIX POSITION", FEMTO_NAK)}), fixed, stream);
    table.add(type, "fixed-rtcm-rejected", "Receiver rejects RTCM output for a fixed base",
              rules({latestReply("LOG RTCM", FEMTO_NAK)}), fixed, stream);
    table.add(type, "survey-rtcm-rejected", "Receiver rejects RTCM output after averaging completes",
              rules({latestReply("LOG RTCM", FEMTO_NAK)}), survey, stream);
    table.add(type, "persistent", "Persistent changes are not supported", femto(), persistent(survey), stream);
    table.add(type, "averaging", "Receiver averaging is not supported", femto(), averaging(60), stream);
    table.add(type, "compact", "MSM4 corrections are not supported", femto(), compact(survey), stream);
}

void addUnicore(Table& table)
{
    using Fault = GPSTest::UnicoreReceiver::Fault;
    constexpr GPSType type = GPSType::unicore;
    const auto average = averaging(5);
    const auto fixed = fixedBase(47, 8, 500, 1);
    const auto stream = unicoreStream();
    const auto fault = [](Fault kind, const char* command) {
        return unicore([kind, command](UnicoreBench& b) {
            b.model.fault = kind;
            b.model.faultCommand = command;
        });
    };
    table.add(type, "averaging", "UM982 receiver averaging", unicore(), average, stream);
    table.add(type, "fixed", "UM982 fixed base", unicore(), fixed, stream);
    table.add(type, "averaging-explicit-115200", "Explicit 115200 request", unicore(), averaging(5, 115200), stream);
    table.add(type, "auto-baud-460800", "Detects a receiver at 460800",
              unicore([](UnicoreBench& b) { b.model.availableBaud = 460800; }), average, stream);
    table.add(type, "explicit-baud-mismatch", "Explicit 9600 while the receiver is at 115200", unicore(),
              averaging(5, 9600), stream);
    table.add(type, "version-timeout", "Receiver never answers the version query (identity)",
              fault(Fault::Silence, "VERSIONA"), average, stream);
    table.add(type, "version-rejected", "Receiver rejects the version query", fault(Fault::Reject, "VERSIONA"), average,
              stream);
    table.add(type, "base-rejected", "Receiver rejects base mode", fault(Fault::Reject, "MODE BASE"), average, stream);
    table.add(type, "fixed-rejected", "Receiver rejects the fixed base", fault(Fault::Reject, "MODE BASE"), fixed,
              stream);
    table.add(type, "log-rejected", "Receiver rejects a required NMEA output", fault(Fault::Reject, "GPGSV 1"), average,
              stream);
    table.add(type, "wrong-ack", "Acknowledgement names another command", fault(Fault::WrongAck, "UNLOG"), average,
              stream);
    table.add(type, "corrupt-ack", "Acknowledgement fails its checksum", fault(Fault::Corrupt, "MODE ROVER"), average,
              stream);
    table.add(type, "write-error", "Transport fails while writing", fault(Fault::WriteError, "RTCM1005 1"), average,
              stream);
    table.add(type, "short-write", "Transport accepts a truncated command", fault(Fault::ShortWrite, "RTCM1074 1"),
              average, stream);
    table.add(type, "cancelled", "Stop requested during configuration", fault(Fault::Cancel, "GPGGA 1"), average,
              stream);
    table.add(type, "read-error", "Transport read fails", unicore([](UnicoreBench& b) { b.model.readError = true; }),
              average, stream);
    table.add(type, "mode-mismatch", "Mode readback reports another role",
              unicore([](UnicoreBench& b) { b.model.modeMismatch = true; }), average, stream);
    table.add(type, "fixed-position-mismatch", "Position readback differs from the fixed base",
              unicore([](UnicoreBench& b) { b.model.positionMismatch = true; }), fixed, stream);
    table.add(type, "fixed-position-missing", "Position readback never arrives",
              unicore([](UnicoreBench& b) { b.model.omitPositionReadback = true; }), fixed, stream);
    table.add(type, "averaging-incomplete", "Averaging never reaches a fixed position",
              unicore([](UnicoreBench& b) { b.model.allowAveragingCompletion = false; }), average, stream);
    table.add(type, "survey", "Accuracy-controlled survey-in is not supported", unicore(), surveyIn(1, 60), stream);
    table.add(type, "persistent", "Persistent changes are not supported", unicore(), persistent(average), stream);
    table.add(type, "compact", "MSM4 corrections are not supported", unicore(), compact(average), stream);
}

void addQuectel(Table& table)
{
    using Fault = GPSTest::QuectelReceiver::Fault;
    constexpr GPSType type = GPSType::quectel;
    constexpr const char* SURVEY_BASE = "1,10,15.0,0.0000,0.0000,0.0000,0.0";
    constexpr const char* FIXED_BASE = "2,0,0,0.0000,6378237.0000,0.0000,0";
    constexpr const char* NO_BASE = "0,0,0,0,0,0,0";
    const auto survey = surveyIn(15, 10);
    const auto fixed = fixedBase(0, 90, 100, 0);
    const auto stream = quectelStream();
    const auto receiver = [](unsigned role, const char* base, std::function<void(QuectelBench&)> extra = {}) {
        return quectel([role, base, extra](QuectelBench& b) {
            b.model.role = role;
            b.model.base = base;
            if (extra) {
                extra(b);
            }
        });
    };
    const auto fault = [receiver](const char* base, Fault kind, const char* command) {
        return receiver(2, base, [kind, command](QuectelBench& b) {
            b.model.fault = kind;
            b.model.failure = command;
        });
    };
    table.add(type, "survey", "Saved base role and matching survey settings", receiver(2, SURVEY_BASE), survey, stream);
    table.add(type, "fixed", "Saved base role and matching fixed position", receiver(2, FIXED_BASE), fixed, stream);
    table.add(type, "survey-persistent", "Consent restarts from saved settings first", receiver(2, SURVEY_BASE),
              persistent(survey), stream);
    table.add(type, "rover-role", "Rover role without consent to change it", receiver(1, SURVEY_BASE), survey, stream);
    table.add(type, "rover-role-persistent", "Rover role changed, saved and restarted with consent",
              receiver(1, SURVEY_BASE), persistent(survey), stream);
    table.add(type, "base-mismatch", "Base settings differ, without consent", receiver(2, NO_BASE), fixed, stream);
    table.add(type, "base-mismatch-persistent", "Base settings written, saved and restarted with consent",
              receiver(2, NO_BASE), persistent(fixed), stream);
    table.add(type, "auto-baud-115200", "Detects a receiver at 115200",
              receiver(2, SURVEY_BASE, [](QuectelBench& b) { enforceBaud(b, 115200); }), survey, stream);
    table.add(type, "explicit-460800", "Explicit 460800 request", receiver(2, SURVEY_BASE), surveyIn(15, 10, 460800),
              stream);
    table.add(type, "explicit-baud-mismatch", "Explicit 9600 while the receiver is at 460800",
              receiver(2, SURVEY_BASE, [](QuectelBench& b) { enforceBaud(b, 460800); }), surveyIn(15, 10, 9600),
              stream);
    table.add(type, "identity-timeout", "Receiver never answers the identity query",
              receiver(2, SURVEY_BASE, [](QuectelBench& b) { b.faults.rules.push_back(silence("$PQTMVERNO")); }),
              survey, stream);
    table.add(type, "wrong-module", "Identity reports an unqualified LG580P",
              receiver(2, SURVEY_BASE,
                       [](QuectelBench& b) {
                           b.model.identity =
                               GPSTest::quectelSentence("PQTMVERNO,LG580P03AANR01A03S,2024/04/30,10:53:07");
                       }),
              survey, stream);
    table.add(type, "message-rate-rejected", "Receiver rejects a required NMEA rate",
              fault(SURVEY_BASE, Fault::Reject, "PQTMCFGMSGRATE,W,GGA"), survey, stream);
    table.add(type, "message-rate-readback-mismatch", "NMEA rate readback differs",
              fault(SURVEY_BASE, Fault::Readback, "PQTMCFGMSGRATE,R,GGA"), survey, stream);
    table.add(type, "base-write-rejected", "Receiver rejects the fixed position (base mode)",
              quectel([](QuectelBench& b) {
                  b.model.role = 2;
                  b.model.base = "0,0,0,0,0,0,0";
                  b.model.fault = Fault::Reject;
                  b.model.failure = "PQTMCFGSVIN,W";
              }),
              persistent(fixed), stream);
    table.add(type, "save-rejected", "Receiver rejects saving the role change", quectel([](QuectelBench& b) {
                  b.model.role = 1;
                  b.model.base = "1,10,15.0,0.0000,0.0000,0.0000,0.0";
                  b.model.fault = Fault::Reject;
                  b.model.failure = "PQTMSAVEPAR";
              }),
              persistent(survey), stream);
    table.add(type, "restart-silent", "Receiver is silent after the restart",
              receiver(2, SURVEY_BASE, [](QuectelBench& b) { b.model.silentAfterReset = true; }), survey, stream);
    table.add(type, "restart-checksum", "Restart reply fails its checksum and the receiver never boots",
              fault(SURVEY_BASE, Fault::Checksum, "PQTMSRR"), survey, stream);
    table.add(type, "cancelled", "Stop requested during configuration",
              fault(SURVEY_BASE, Fault::Cancel, "PQTMCFGSVIN,R"), survey, stream);
    table.add(type, "partial-write", "Transport times out part-way through a command",
              fault(SURVEY_BASE, Fault::Partial, "PQTMCFGMSGRATE,W,GSV"), survey, stream);
    table.add(type, "survey-too-long", "Survey-in beyond 24 hours is not supported", receiver(2, SURVEY_BASE),
              surveyIn(15, 90000), stream);
    table.add(type, "averaging", "Receiver averaging is not supported", receiver(2, SURVEY_BASE), averaging(60),
              stream);
    table.add(type, "compact", "MSM4 corrections are not supported", receiver(2, SURVEY_BASE), compact(survey), stream);
}

void addPassive(Table& table)
{
    constexpr GPSType type = GPSType::passive;
    const auto stream = passiveStream();
    table.add(type, "baud-115200", "NMEA and RTCM input at 115200", passive(), passiveInput(115200), stream);
    table.add(type, "baud-9600", "NMEA and RTCM input at 9600", passive(), passiveInput(9600), stream);
    table.add(type, "auto-baud", "Passive input requires an explicit baud", passive(), passiveInput(0), stream);
    table.add(type, "base-settings", "Passive input has no base settings", passive(),
              withRole(surveyIn(1, 60, 115200), GPSReceiverConfig::Role::Passive), stream);
    table.add(type, "persistent", "Passive input makes no persistent changes", passive(),
              persistent(passiveInput(115200)), stream);
    table.add(type, "rtk-base-role", "Passive input cannot be an RTK base", passive(), surveyIn(1, 60, 115200), stream);
    table.add(type, "baud-unsupported", "Transport cannot change its baud",
              passive([](PassiveBench& b) { b.receiver.setBaudrateResult(false); }), passiveInput(115200), stream);
}

void addAutomatic(Table& table)
{
    using GPSTest::Dialect;
    constexpr GPSType type = GPSType::automatic;
    const QByteArray factoryNmea = nmea("GNGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,");
    const auto at = [](unsigned baud) { return [baud](auto& b) { b.receiver.setReceiverBaudrate(baud); }; };
    const auto unicoreAt = [](AutomaticBench<GPSTest::UnicoreReceiver>& b) {
        auto& model = b.model;
        model.startedUs = model.clock.nowUs();
        b.atRate = [&model] { return model.hostBaud == model.availableBaud; };
        b.wait = [&model](std::chrono::microseconds delay) {
            model.events.advanceTo(model.clock.nowUs() + static_cast<uint64_t>(delay.count()));
            return !model.cancel;
        };
    };
    const auto quectelAt = [](unsigned baud, unsigned role = 2) {
        return [baud, role](AutomaticBench<GPSTest::QuectelReceiver>& b) {
            auto& model = b.model;
            model.role = role;
            model.base = "1,10,15.0,0.0000,0.0000,0.0000,0.0";
            model.startedUs = model.clock.nowUs();
            model.activeRole = model.savedRole = model.role;
            model.activeBase = model.savedBase = model.base;
            model.savedRates = model.rates;
            b.receiver.setReceiverBaudrate(baud);
            b.wait = [&model](std::chrono::microseconds delay) {
                model.events.advanceTo(model.clock.nowUs() + static_cast<uint64_t>(delay.count()));
                return true;
            };
        };
    };

    table.add(type, "ublox-factory-38400", "Factory u-blox sending only NMEA at 38400 answers the MON-VER probe",
              automatic<UBXReceiverModel>(
                  Dialect::UBX,
                  [factoryNmea](AutomaticBench<UBXReceiverModel>& b) {
                      b.model.lowLevelProtocolBehavior = true;
                      b.model.receiverBaud = 38400;
                      b.atRate = [&b] { return b.model.hostBaud == b.model.receiverBaud; };
                      b.detection.stream = factoryNmea;
                      // The first CFG command selects UBX output, ending the factory NMEA.
                      b.faults.rules.push_back(afterCommand(QByteArray("\xb5\x62\x06", 3),
                                                            [&b](ScriptedReceiver&) { b.detection.stream.clear(); }));
                  },
                  UBXReceiverModel::Receiver::F9P),
              surveyIn(1.25, 60));
    table.add(type, "ublox-19200",
              "u-blox sending UBX at 19200, which u-blox configuration does not probe, is configured from that rate",
              automatic<UBXReceiverModel>(
                  Dialect::UBX,
                  [](AutomaticBench<UBXReceiverModel>& b) {
                      b.model.lowLevelProtocolBehavior = true;
                      b.model.receiverBaud = 19200;
                      b.atRate = [&b] { return b.model.hostBaud == b.model.receiverBaud; };
                      b.detection.stream = ubx(0x0701, QByteArray(92, '\0'));
                      b.faults.rules.push_back(afterCommand(QByteArray("\xb5\x62\x06", 3),
                                                            [&b](ScriptedReceiver&) { b.detection.stream.clear(); }));
                  },
                  UBXReceiverModel::Receiver::F9P),
              surveyIn(1.25, 60));
    table.add(type, "ublox-fixed-115200", "Fixed-rate link: every family is probed at 115200 only",
              automatic<UBXReceiverModel>(
                  Dialect::UBX,
                  [](AutomaticBench<UBXReceiverModel>& b) {
                      b.receiver.setFixedBaudrate(115200);
                      b.atRate = [] { return true; };
                  },
                  UBXReceiverModel::Receiver::F9P),
              surveyIn(2.0, 180));
    table.add(type, "septentrio-115200", "Septentrio answers the SBF prompt at 115200",
              automatic<SBFReceiverModel>(Dialect::SBF, at(115200)), surveyIn(1, 60));
    table.add(type, "septentrio-averaging-unsupported", "Detected Septentrio cannot average; nothing is configured",
              automatic<SBFReceiverModel>(Dialect::SBF, at(115200)), averaging(60));
    table.add(type, "trimble-38400", "Trimble answers the port query at 38400 and follows the speed command",
              automatic<GPSTest::AshtechReceiverModel>(
                  Dialect::Ashtech,
                  [](AutomaticBench<GPSTest::AshtechReceiverModel>& b) {
                      b.receiver.setReceiverBaudrate(38400);
                      b.faults.rules.push_back(afterCommand(
                          "$PASHS,SPD", [](ScriptedReceiver& receiver) { receiver.setReceiverBaudrate(115200); }));
                  }),
              surveyIn(1, 100));
    table.add(type, "femto-115200", "Femtomes answers the version query at 115200",
              automatic<FemtoReceiverModel>(Dialect::Femto, at(115200)), surveyIn(1, 60));
    table.add(type, "unicore-115200", "Unicore answers the Femtomes version query in its own words",
              automatic<GPSTest::UnicoreReceiver>(Dialect::Unicore, unicoreAt), averaging(5));
    table.add(type, "quectel-460800", "Quectel answers the firmware query at its default 460800",
              automatic<GPSTest::QuectelReceiver>(Dialect::Quectel, quectelAt(460800)), surveyIn(15, 10));
    table.add(type, "quectel-rover-role", "Factory Quectel in its rover role, without consent: nothing is changed",
              automatic<GPSTest::QuectelReceiver>(Dialect::Quectel, quectelAt(460800, 1)), surveyIn(15, 10));
    table.add(type, "quectel-rover-role-persistent",
              "Consent reaches the detected Quectel, which saves its base role and restarts",
              automatic<GPSTest::QuectelReceiver>(Dialect::Quectel, quectelAt(460800, 1)),
              persistent(surveyIn(15, 10)));
    table.add(type, "nothing-found", "Nothing answers at any rate: every probe at every rate, within the time limit",
              automatic<FemtoReceiverModel>(
                  Dialect::Femto, [](AutomaticBench<FemtoReceiverModel>& b) { b.atRate = [] { return false; }; }),
              surveyIn(1, 60));
}

const std::vector<ScenarioDef>& scenarios()
{
    static const std::vector<ScenarioDef> rows = [] {
        Table table;
        addUblox(table);
        addSeptentrio(table);
        addTrimble(table);
        addFemto(table);
        addUnicore(table);
        addQuectel(table);
        addPassive(table);
        addAutomatic(table);
        return std::move(table.rows);
    }();
    return rows;
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
