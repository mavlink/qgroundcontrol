#pragma once

// Receiver wire builders, request builders, scripted faults, benches and the scenario table of the golden
// transcripts. Each receiver family registers its scenarios in its own GoldenScenarios<Family>.cc.

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
#include "Protocols/Support/AshtechReceiverModel.h"
#include "Protocols/Support/DetectionReceiver.h"
#include "Protocols/Support/FemtoReceiverModel.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/GoldenTranscript.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/SBFReceiverModel.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "Protocols/Support/UnicoreReceiverModel.h"

namespace GPSGoldenScenario {

using namespace std::chrono_literals;
using GPSGolden::StreamStep;

// Nonzero, so a zero receipt timestamp always means "never received".
inline constexpr uint64_t START_US = 1000000000;

// ---------------------------------------------------------------------------------------------------------------
// Receiver wire builders, independent of the protocol implementation.

namespace UBXWire {
inline constexpr uint16_t NAV_STATUS = 0x0301;
inline constexpr uint16_t NAV_PVT = 0x0701;
inline constexpr uint16_t NAV_SAT = 0x3501;
inline constexpr uint16_t NAV_SVIN = 0x3b01;
inline constexpr uint16_t NAV_EOE = 0x6101;
inline constexpr uint16_t INF_ERROR = 0x0004;
inline constexpr uint16_t ACK_NAK = 0x0005;
inline constexpr uint16_t CFG_VALSET = 0x8a06;
inline constexpr uint16_t MON_RF = 0x380a;
inline constexpr uint32_t KEY_RATE_MEAS = 0x30210001;
inline constexpr uint32_t KEY_SEC_JAMDET_SENSITIVITY_HI = 0x10f60051;
inline constexpr uint32_t KEY_UART1INPROT_SPARTN = 0x10730005;
inline constexpr uint32_t KEY_MSGOUT_RXM_COR_UART1 = 0x209106b7;
inline constexpr uint32_t KEY_MSGOUT_NAV_EOE_UART1 = 0x20910160;
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

inline QByteArray ubx(uint16_t message, const QByteArray& payload)
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

inline QByteArray nmea(std::string_view body)
{
    uint8_t checksum = 0;
    for (const char byte : body) {
        checksum ^= static_cast<uint8_t>(byte);
    }
    return '$' + QByteArray(body.data(), static_cast<qsizetype>(body.size())) + '*' +
           QByteArray::number(checksum, 16).rightJustified(2, '0').toUpper() + "\r\n";
}

inline QByteArray rtcm(const QByteArray& payload)
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
inline QByteArray rtcm1005()
{
    return rtcm(QByteArray::fromHex("3ed122033a2266a8ee8b4a8c4d3507a1bddfbf"));
}

/// A short, well-framed 1077 header; decoders pass it through without interpreting observations.
inline QByteArray rtcm1077()
{
    return rtcm(QByteArray::fromHex("4350000000000000"));
}

inline QByteArray dataFile(const char* directory, const QString& name)
{
    QFile file(QString::fromUtf8(directory) + u'/' + name);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

inline QByteArray navPvt(uint32_t tow)
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

inline QByteArray navEoe(uint32_t tow)
{
    return ubx(UBXWire::NAV_EOE, Payload(4).set<uint32_t>(0, tow).bytes());
}

/// Synthetic mean ECEF position (0, 6378237 m, 0): latitude 0, longitude 90.
inline QByteArray navSvin(uint32_t tow, uint32_t duration, bool valid, bool active)
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

inline QByteArray navSat(uint32_t tow)
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

inline QByteArray monRf()
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

inline QByteArray navStatus()
{
    return ubx(UBXWire::NAV_STATUS, Payload(16).set<uint8_t>(4, 3).set<uint8_t>(5, 1).set<uint8_t>(7, 2 << 3).bytes());
}

/// @a comms adds the transmit-buffer warning that requests MON-COMMS diagnostics.
inline std::vector<StreamStep> ubxStream(bool comms)
{
    return {
        {navSvin(1000, 30, false, true) + navPvt(1000) + navSat(1000) + navEoe(1000) + monRf() + navStatus(), 1500ms},
        {navSvin(2000, 181, true, false) + navPvt(2000) + navEoe(2000), 1500ms},
        {rtcm1005() + (comms ? ubx(UBXWire::INF_ERROR, "txbuf alloc") : QByteArray()), 3000ms},
    };
}

inline std::vector<StreamStep> sbfStream()
{
    return {
        {dataFile(GPS_FIXTURE_DIR, QStringLiteral("synthetic-valid.sbf")), 1000ms},
        {rtcm1005(), 1000ms},
        {{}, 6000ms},
    };
}

inline std::vector<StreamStep> ashtechStream()
{
    return {
        {nmea("GPZDA,114501.00,28,12,2011,00,00") + nmea("GPGST,114501.00,1.0,0.5,0.4,45.0,0.4,0.5,0.9") +
             nmea("PASHR,POS,2,12,114501.00,4700.00000,N,00800.00000,E,500.000,0,90,10,0,1,1,1,1,"),
         1000ms},
        {nmea(GPSTest::ASHTECH_SURVEY_FINISHED), 1000ms},
        {rtcm1005(), 1000ms},
    };
}

inline std::vector<StreamStep> femtoStream()
{
    return {
        {nmea("GPGGA,123519.00,4700.00000,N,00800.00000,E,1,12,0.9,450.000,M,50.000,M,,"), 1000ms},
        {nmea("GPGGA,123520.00,4700.00000,N,00800.00000,E,7,12,0.9,450.000,M,50.000,M,,"), 1000ms},
        {rtcm1005(), 1000ms},
    };
}

inline std::vector<StreamStep> unicoreStream()
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

inline std::vector<StreamStep> quectelStream()
{
    return {
        {{}, 12000ms},
        {rtcm1005(), 2000ms},
    };
}

inline std::vector<StreamStep> passiveStream()
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

inline GPSReceiverConfig surveyIn(double accuracyMeters, int64_t seconds, uint32_t baud = 0)
{
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = accuracyMeters,
                                                            .duration = std::chrono::seconds(seconds)}},
            .baudRate = baud};
}

inline GPSReceiverConfig fixedBase(double latitude, double longitude, float altitude, float accuracy, uint32_t baud = 0)
{
    return {.base = {.mode = GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = latitude,
                                                                      .longitudeDegrees = longitude,
                                                                      .altitudeMeters = altitude},
                                                         .accuracyMeters = accuracy}},
            .baudRate = baud};
}

inline GPSReceiverConfig averaging(int64_t seconds, uint32_t baud = 0)
{
    return {.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = std::chrono::seconds(seconds)}},
            .baudRate = baud};
}

inline GPSReceiverConfig passiveInput(uint32_t baud)
{
    return {.role = GPSReceiverConfig::Role::Passive, .baudRate = baud};
}

inline GPSReceiverConfig compact(GPSReceiverConfig config)
{
    config.base.compactObservations = true;
    return config;
}

inline GPSReceiverConfig persistent(GPSReceiverConfig config)
{
    config.allowPersistentChanges = true;
    return config;
}

inline GPSReceiverConfig withRole(GPSReceiverConfig config, GPSReceiverConfig::Role role)
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

inline std::function<bool(const QByteArray&)> startsWith(QByteArray prefix)
{
    return [prefix = std::move(prefix)](const QByteArray& command) { return command.startsWith(prefix); };
}

/// A UBX CFG-VALSET frame that sets @a key.
inline std::function<bool(const QByteArray&)> valsetWith(uint32_t key)
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

inline Rule reply(std::function<bool(const QByteArray&)> matches, QByteArray bytes, int first = 1,
                  int last = std::numeric_limits<int>::max())
{
    return {.matches = std::move(matches), .reply = std::move(bytes), .first = first, .last = last};
}

inline Rule silence(QByteArray prefix)
{
    return {.matches = startsWith(std::move(prefix))};
}

inline Rule afterCommand(QByteArray prefix, std::function<void(ScriptedReceiver&)> hook)
{
    return {.matches = startsWith(std::move(prefix)), .forward = true, .after = std::move(hook)};
}

inline Rule ubxNak(uint32_t key)
{
    return reply(valsetWith(key), ubx(UBXWire::ACK_NAK, QByteArray::fromHex("068a")));
}

/// Septentrio and Femtomes models answer only the latest command.
inline Rule latestReply(QByteArray prefix, QByteArray bytes, int first = 1, int last = std::numeric_limits<int>::max())
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
inline BenchFactory bench(std::function<void(BenchType&)> setup, Args... args)
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
inline BenchFactory ubxProfile(UBXReceiverModel::Receiver receiver, std::function<void(UBXBench&)> setup = {})
{
    return bench<UBXBench>(std::move(setup), receiver);
}

/// UBXReceiverModel's wire-level behaviour on a UART without a fixed baud.
inline BenchFactory ubxWire(std::function<void(UBXBench&)> setup = {})
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

inline BenchFactory sbf(std::function<void(SBFBench&)> setup = {})
{
    return bench<SBFBench>(std::move(setup));
}

inline BenchFactory ashtech(std::function<void(AshtechBench&)> setup = {})
{
    return bench<AshtechBench>(std::move(setup));
}

inline BenchFactory femto(std::function<void(FemtoBench&)> setup = {})
{
    return bench<FemtoBench>(std::move(setup));
}

/// UnicoreReceiver's own services: waits run its receiver events, and its fault flags fail reads.
inline BenchFactory unicore(std::function<void(UnicoreBench&)> setup = {})
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
inline BenchFactory quectel(std::function<void(QuectelBench&)> setup = {})
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
inline BenchFactory passive(std::function<void(PassiveBench&)> setup = {})
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
inline BenchFactory automatic(GPSTest::Dialect dialect, std::function<void(AutomaticBench<Model>&)> setup, Args... args)
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
inline void enforceBaud(BenchType& b, unsigned baud)
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

}  // namespace GPSGoldenScenario
