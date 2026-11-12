#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <span>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GPSDriver.h"
#include "GPSTransport.h"
#include "Protocols/Support/AshtechReceiverModel.h"
#include "Protocols/Support/FemtoReceiverModel.h"
#include "Protocols/Support/GPSModelViolations.h"
#include "Protocols/Support/GPSProtocolTestData.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/SBFReceiverModel.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "Protocols/Support/UnicoreReceiverModel.h"
#include "ReplyFuzz.h"

// The seeds of the reply fuzzer, and its smoke test: every seed, shortened and damaged copies of it, and random
// inputs go through the harness without libFuzzer. --write-corpus writes the seeds for a libFuzzer run.

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

using Input = std::vector<uint8_t>;

struct Seed
{
    QByteArray name;
    Input input;
    /// Recorded while the driver configured a receiver model, so the driver must configure again from the seed.
    bool recorded = false;
};

/// Receive cycles recorded after the configuration.
constexpr int RECORDED_RECEIVE_CYCLES = 4;
/// Bytes of one record of a seed made from a recording, which a receiver sends unasked.
constexpr qsizetype STREAM_RECORD_SIZE = 64;

[[noreturn]] void fail(const QByteArray& seed, const QString& reason)
{
    std::fprintf(stderr, "GPS reply fuzz smoke: %s: %s\n", seed.constData(), qPrintable(reason));
    std::exit(1);
}

/// The link to a receiver model, which records every read as the fuzzer's link replays it.
class RecordingLink final : public GPSTransport
{
public:
    RecordingLink(ReceiverBench& receiver, Input& input)
        : GPSTransport(GPSCancelToken{})
        , _receiver(receiver)
        , _input(input)
    {}

    GPSOpenResult open() override { return _receiver.open(); }

    bool fatalError() const override { return _receiver.fatalError(); }

    unsigned fixedBaudRate() const override { return _receiver.fixedBaudRate(); }

    bool setBaudRate(unsigned baudRate) override { return _receiver.setBaudRate(baudRate); }

    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override
    {
        const GPSReadResult result = _receiver.read(buffer, timeout);
        if (buffer.empty()) {
            return result;
        }
        if (result.status != GPSReadStatus::Data || result.bytesRead <= 0) {
            GPSReplyFuzz::appendRecord(_input, {});
            return result;
        }
        auto bytes = std::span<const uint8_t>(buffer).first(static_cast<size_t>(result.bytesRead));
        while (!bytes.empty()) {
            const size_t count = std::min(bytes.size(), GPSReplyFuzz::MAX_RECORD_SIZE);
            GPSReplyFuzz::appendRecord(_input, bytes.first(count));
            bytes = bytes.subspan(count);
        }
        _receiver.clock.advanceBy(GPSReplyFuzz::READ_DURATION);
        return result;
    }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override
    {
        return _receiver.write(bytes, deadline);
    }

private:
    ReceiverBench& _receiver;
    Input& _input;
};

/// The replies of a receiver model to the driver that configures it for @a type with @a options, then receives.
template <typename Model, typename... Args>
Seed recorded(const char* name, GPSType type, uint8_t options,
              const std::function<void(ModelReceiver<Model>&)>& setup = {}, Args&&... args)
{
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<Model> receiver(clock, std::forward<Args>(args)...);
    if (setup) {
        setup(receiver);
    }
    receiver.startSession();
    Seed seed{name, {static_cast<uint8_t>(type), options}, true};
    RecordingLink link(receiver, seed.input);
    GPSClock driverClock = clock.source();
    if (receiver.wait) {
        driverClock.wait = receiver.wait;
    }
    GPSDriver driver(type, link, GPSReplyFuzz::receiverConfig(type, options), {}, driverClock);
    if (!driver.configure()) {
        fail(seed.name, QStringLiteral("the receiver model was not configured: ") + driver.configurationError());
    }
    for (int cycle = 0; cycle < RECORDED_RECEIVE_CYCLES; ++cycle) {
        if (driver.receiveOutcome(1000ms).error != GPSProtocolError::None) {
            break;
        }
    }
    if (const auto violations = ModelViolations::take(); !violations.empty()) {
        fail(seed.name, QString::fromStdString(violations.front()));
    }
    return seed;
}

/// A receiver model behind receiver detection at @a baud.
template <typename Model>
std::function<void(ModelReceiver<Model>&)> detected(Dialect dialect, unsigned baud, QByteArray stream = {})
{
    return [dialect, baud, stream = std::move(stream)](ModelReceiver<Model>& receiver) {
        receiver.detect(dialect).stream = stream;
        receiver.setReceiverBaudRate(baud);
    };
}

/// @a stream as what a receiver sends unasked to the driver of each receiver type, over a network link.
void appendStreams(std::vector<Seed>& seeds, const QString& name, const QByteArray& stream)
{
    for (uint8_t target = 0; target < GPSReplyFuzz::TARGETS; ++target) {
        Seed seed{QStringLiteral("%1-as-%2").arg(name).arg(target).toLatin1(),
                  {target, GPSReplyFuzz::NetworkLink | GPSReplyFuzz::SelectedRate}};
        for (qsizetype offset = 0; offset < stream.size(); offset += STREAM_RECORD_SIZE) {
            const QByteArrayView record =
                QByteArrayView(stream).sliced(offset, std::min(STREAM_RECORD_SIZE, stream.size() - offset));
            GPSReplyFuzz::appendRecord(
                seed.input, {reinterpret_cast<const uint8_t*>(record.data()), static_cast<size_t>(record.size())});
        }
        seeds.push_back(std::move(seed));
    }
}

std::vector<Seed> seeds()
{
    using GPSReplyFuzz::FixedBase;
    using GPSReplyFuzz::ReceiverAveraging;
    const auto serial = [](ModelReceiver<UBXReceiverModel>& receiver) { receiver.setFixedBaudRate(0); };
    const auto quectelBase = [](ModelReceiver<QuectelReceiverModel>& receiver) { receiver.model.role = 2; };
    std::vector<Seed> result{
        recorded<UBXReceiverModel>("ublox-survey", GPSType::UBlox, 0, serial),
        recorded<UBXReceiverModel>("ublox-fixed", GPSType::UBlox, FixedBase, serial),
        recorded<UBXReceiverModel>("ublox-m8p-survey", GPSType::UBlox, 0,
                                   [](ModelReceiver<UBXReceiverModel>& receiver) {
                                       receiver.setFixedBaudRate(0);
                                       receiver.model.legacy = true;
                                       receiver.model.module = "NEO-M8P";
                                   }),
        recorded<SBFReceiverModel>("septentrio-survey", GPSType::Septentrio, 0),
        recorded<SBFReceiverModel>("septentrio-fixed", GPSType::Septentrio, FixedBase),
        recorded<AshtechReceiverModel>("trimble-survey", GPSType::Trimble, 0),
        recorded<AshtechReceiverModel>("trimble-fixed", GPSType::Trimble, FixedBase),
        recorded<FemtoReceiverModel>("femto-fixed", GPSType::Femto, FixedBase),
        recorded<UnicoreReceiverModel>("unicore-averaging", GPSType::Unicore, FixedBase | ReceiverAveraging),
        recorded<UnicoreReceiverModel>("unicore-fixed", GPSType::Unicore, FixedBase),
        recorded<QuectelReceiverModel>("quectel-survey", GPSType::Quectel, 0, quectelBase),
        recorded<QuectelReceiverModel>("quectel-fixed", GPSType::Quectel, FixedBase,
                                       [](ModelReceiver<QuectelReceiverModel>& receiver) {
                                           receiver.model.role = 2;
                                           receiver.model.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
                                       }),
        // The u-blox model keeps the rates of its own port.
        recorded<UBXReceiverModel>("automatic-ublox", GPSType::Automatic, 0,
                                   [](ModelReceiver<UBXReceiverModel>& receiver) {
                                       receiver.detect(Dialect::UBX);
                                       receiver.model.receiverBaud = 38400;
                                       receiver.atRate = [&receiver] {
                                           return receiver.model.hostBaud == receiver.model.receiverBaud;
                                       };
                                   }),
        recorded<SBFReceiverModel>("automatic-septentrio", GPSType::Automatic, 0,
                                   detected<SBFReceiverModel>(Dialect::SBF, 115200)),
        recorded<AshtechReceiverModel>("automatic-trimble", GPSType::Automatic, 0,
                                       detected<AshtechReceiverModel>(Dialect::Ashtech, 115200)),
        recorded<FemtoReceiverModel>("automatic-femto", GPSType::Automatic, FixedBase,
                                     detected<FemtoReceiverModel>(Dialect::Femto, 115200)),
    };
    for (const auto& [label, directory] : {std::pair{"corpus", CORPUS_DIR}, std::pair{"fixtures", FIXTURE_DIR}}) {
        const QStringList suffixes{QStringLiteral("ascii"), QStringLiteral("bin"),  QStringLiteral("cap"),
                                   QStringLiteral("gps"),   QStringLiteral("nmea"), QStringLiteral("sbf"),
                                   QStringLiteral("ubx")};
        QStringList names = QDir(QString::fromUtf8(directory)).entryList(QDir::Files, QDir::Name);
        for (const QString& name : std::as_const(names)) {
            if (suffixes.contains(QFileInfo(name).suffix())) {
                appendStreams(result, QLatin1String(label) + u'-' + name, dataFile(directory, name));
            }
        }
    }
    return result;
}

void exercise(std::span<const uint8_t> input)
{
    LLVMFuzzerTestOneInput(input.data(), input.size());
}

}  // namespace

int main(int argc, char* argv[])
{
    const auto corpus = seeds();
    if (argc == 3 && QByteArray(argv[1]) == "--write-corpus") {
        const QDir directory(QString::fromLocal8Bit(argv[2]));
        if (!QDir().mkpath(directory.path())) {
            return 1;
        }
        for (const auto& seed : corpus) {
            QFile file(directory.filePath(QString::fromLatin1(seed.name)));
            const auto size = static_cast<qint64>(seed.input.size());
            if (!file.open(QIODevice::WriteOnly) ||
                file.write(reinterpret_cast<const char*>(seed.input.data()), size) != size) {
                return 1;
            }
        }
        return 0;
    }
    if (argc != 1) {
        std::fputs("Usage: QGCGPSReplyFuzzSmoke [--write-corpus DIRECTORY]\n", stderr);
        return 1;
    }
    uint32_t state = 0x47505346;
    const auto random = [&state] {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    size_t recordings = 0;
    for (const auto& seed : corpus) {
        if (seed.recorded) {
            ++recordings;
            if (!GPSReplyFuzz::run(seed.input).configured) {
                fail(seed.name, QStringLiteral("the driver did not configure from the recorded replies"));
            }
        } else {
            exercise(seed.input);
        }
        // The peer falls silent or closes the connection early, and sends damaged replies.
        for (const size_t divisor : {4, 2}) {
            Input shortened(seed.input.begin(), seed.input.begin() + seed.input.size() / divisor);
            exercise(shortened);
            if (shortened.size() > 1) {
                shortened[1] |= GPSReplyFuzz::ClosesAtEnd;
                exercise(shortened);
            }
        }
        for (int damage = 1; damage <= 4; ++damage) {
            Input damaged = seed.input;
            for (int byte = 0; byte < damage * damage && damaged.size() > GPSReplyFuzz::HEADER_SIZE; ++byte) {
                damaged[GPSReplyFuzz::HEADER_SIZE + random() % (damaged.size() - GPSReplyFuzz::HEADER_SIZE)] ^=
                    static_cast<uint8_t>(1u << (random() % 8));
            }
            exercise(damaged);
        }
    }
    for (const size_t length : std::array<size_t, 8>{0, 1, 2, 3, 64, 1024, 4096, 16384}) {
        for (uint8_t target = 0; target < GPSReplyFuzz::TARGETS; ++target) {
            Input bytes(length);
            for (uint8_t& byte : bytes) {
                byte = static_cast<uint8_t>(random());
            }
            if (!bytes.empty()) {
                bytes[0] = target;
            }
            exercise(bytes);
        }
    }
    std::printf(
        "GPS reply fuzz smoke: %zu seeds, %zu of them recorded configurations, their shortened and damaged "
        "copies, and 64 random inputs passed\n",
        corpus.size(), recordings);
    return 0;
}
