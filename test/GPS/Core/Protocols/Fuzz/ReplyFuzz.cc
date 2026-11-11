#include "ReplyFuzz.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtLogging>

#include "GPSDriver.h"
#include "GPSReceiverDetector.h"
#include "GPSTransport.h"
#include "Protocols/Support/GPSTestClock.h"
#include "RTCMFramer.h"

using namespace std::chrono_literals;

namespace GPSReplyFuzz {

namespace {

/// Reads, writes and rate changes of one input. Every wait of the driver ends at a deadline on the virtual clock, so
/// far fewer happen; more means that the driver spins without time passing.
constexpr unsigned MAX_OPERATIONS = 1000000;
/// Sessions of one input: the second is a reconnect with the replies the first one left.
constexpr int MAX_SESSIONS = 2;
/// Receive cycles of one session, and those that still run once the peer has nothing left to send, which is long
/// enough for every report to expire.
constexpr int MAX_RECEIVE_CYCLES = 64;
constexpr int SILENT_RECEIVE_CYCLES = 8;
constexpr std::chrono::milliseconds RECEIVE_TIMEOUT{1000};
/// What receiver detection may take beyond its limit: the reads in flight when the limit passes.
constexpr std::chrono::microseconds DETECTION_SLACK{50000};

void require(bool condition)
{
    if (!condition) {
        std::abort();
    }
}

/// The peer: its replies are the input's records, and it accepts every write.
class ReplyLink final : public GPSTransport
{
public:
    ReplyLink(std::span<const uint8_t> records, uint8_t options, GPSTest::GPSTestClock& clock)
        : GPSTransport(GPSCancelToken{})
        , _records(records)
        , _options(options)
        , _clock(clock)
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return _closed; }

    unsigned fixedBaudRate() const override { return (_options & NetworkLink) ? BRIDGE_BAUD_RATE : 0; }

    bool setBaudRate(unsigned baudRate) override
    {
        _count();
        return !_closed && (!(_options & NetworkLink) || baudRate == BRIDGE_BAUD_RATE);
    }

    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override
    {
        _count();
        if (_closed) {
            return {GPSReadStatus::Closed, 0, QStringLiteral("The peer closed the connection")};
        }
        if (buffer.empty()) {
            return {GPSReadStatus::TimedOut};
        }
        if (_pending.empty() && !_records.empty()) {
            const size_t size = std::min<size_t>(_records.front(), _records.size() - 1);
            _pending = _records.subspan(1, size);
            _records = _records.subspan(1 + size);
        } else if (_pending.empty() && (_options & ClosesAtEnd)) {
            _closed = true;
            return {GPSReadStatus::Closed, 0, QStringLiteral("The peer closed the connection")};
        }
        if (_pending.empty()) {
            _clock.advanceBy(std::max<std::chrono::microseconds>(timeout, 0us));
            return {GPSReadStatus::TimedOut};
        }
        const size_t count = std::min(buffer.size(), _pending.size());
        std::copy_n(_pending.begin(), count, buffer.begin());
        _pending = _pending.subspan(count);
        _clock.advanceBy(READ_DURATION);
        return {GPSReadStatus::Data, static_cast<int>(count)};
    }

    /// Nothing is left for the driver to read.
    [[nodiscard]] bool silent() const { return _closed || (_pending.empty() && _records.empty()); }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override
    {
        Q_UNUSED(deadline)
        _count();
        if (_closed) {
            return {GPSWriteStatus::Error};
        }
        const int length = static_cast<int>(bytes.size());
        return {GPSWriteStatus::Completed, length, length};
    }

private:
    void _count() { require(++_operations <= MAX_OPERATIONS); }

    std::span<const uint8_t> _records;
    std::span<const uint8_t> _pending;
    const uint8_t _options;
    GPSTest::GPSTestClock& _clock;
    unsigned _operations = 0;
    bool _closed = false;
};

/// Reads every character here, in instrumented code, so that a sanitizer sees text built from memory that is gone.
size_t checksum(const QString& text)
{
    size_t sum = 0;
    for (const QChar character : text) {
        sum += character.unicode();
    }
    return sum;
}

/// Keeps what the sinks read from being optimized away.
volatile size_t s_touched = 0;

void dropMessage(QtMsgType, const QMessageLogContext&, const QString&) {}

}  // namespace

GPSReceiverConfig receiverConfig(GPSType type, uint8_t options)
{
    using Base = GPSBaseStationConfig;
    // The survey and the fixed position the Unicore and Quectel receiver models report, so that seeds recorded from
    // them configure.
    const Base::Mode survey = Base::SurveyIn{.accuracyMeters = 15.0, .duration = 60s};
    const Base::Mode fixed =
        Base::Fixed{.position = {.latitudeDegrees = 0, .longitudeDegrees = 90, .altitudeMeters = 100}};
    const Base::Mode averaging = Base::ReceiverAveraging{.maximumDuration = 60s};
    const bool wantsFixed = (options & FixedBase) && !(options & ReceiverAveraging);
    const bool wantsAveraging = (options & FixedBase) && (options & ReceiverAveraging);
    GPSReceiverConfig request;
    request.base.mode = wantsFixed ? fixed : wantsAveraging ? averaging : survey;
    request.base.compactObservations = options & CompactObservations;
    request.allowPersistentChanges = options & PersistentChanges;
    request.baudRate = (options & SelectedRate) ? GPSTransport::BRIDGE_BAUD_RATE : 0;
    // The request as it is, then without what the type may lack, then in each other base mode.
    for (const auto& mode : {request.base.mode, survey, fixed, averaging}) {
        for (const bool reduced : {false, true}) {
            GPSReceiverConfig candidate = request;
            candidate.base.mode = mode;
            if (reduced) {
                candidate.base.compactObservations = false;
                candidate.allowPersistentChanges = false;
            }
            if (gpsValidateReceiverConfig(type, candidate) == GPSReceiverConfigError::None) {
                return candidate;
            }
        }
    }
    return request;
}

void appendRecord(std::vector<uint8_t>& input, std::span<const uint8_t> bytes)
{
    require(bytes.size() <= MAX_RECORD_SIZE);
    input.push_back(static_cast<uint8_t>(bytes.size()));
    input.insert(input.end(), bytes.begin(), bytes.end());
}

Outcome run(std::span<const uint8_t> input)
{
    // The driver warns about every failed step; only what it does to build the message matters here.
    static const QtMessageHandler previous = qInstallMessageHandler(dropMessage);
    Q_UNUSED(previous)

    const auto type = static_cast<GPSType>(input.empty() ? 0 : input[0] % TARGETS);
    const uint8_t options = input.size() > 1 ? input[1] : 0;
    GPSTest::GPSTestClock clock(GPSTest::GPSTestClock::START_US);
    ReplyLink link(input.size() > HEADER_SIZE ? input.subspan(HEADER_SIZE) : std::span<const uint8_t>{}, options,
                   clock);

    Outcome outcome;
    size_t touched = 0;
    GPSDriverSinks sinks;
    sinks.onPosition = [&](const GPSPositionReport& report) {
        ++outcome.positions;
        touched += report.navigation.satellitesUsed.value_or(0) + static_cast<size_t>(report.navigation.fixType);
    };
    sinks.onSatelliteInfo = [&](const GPSSatelliteReport& report) {
        for (const auto& constellation : report.constellationsInView()) {
            touched += static_cast<size_t>(constellation.inView);
        }
    };
    sinks.onRTCM = [&](const QByteArray& frame) {
        ++outcome.corrections;
        // One complete frame, as the sink is promised.
        require(RTCMFramer::isValidFrame(QByteArrayView(frame)));
    };
    sinks.onBaseStatus = [&](const GPSBaseStatus& status) { touched += static_cast<size_t>(status.state); };
    std::optional<uint64_t> detectedAtUs;
    sinks.onReceiverDetected = [&](GPSType detected) {
        require(detected != GPSType::Automatic);
        if (!detectedAtUs) {
            detectedAtUs = clock.nowUs();
        }
    };

    GPSDriver driver(type, link, receiverConfig(type, options), sinks, clock.source());
    for (int session = 0; session < MAX_SESSIONS; ++session) {
        const uint64_t startedAtUs = clock.nowUs();
        detectedAtUs.reset();
        const bool configured = driver.configure();
        if (type == GPSType::Automatic) {
            // Detection ends when it reports the receiver, or with the configuration when it finds none.
            const auto limit = std::chrono::microseconds(GPSReceiverDetector::TIMEOUT) + DETECTION_SLACK;
            require(detectedAtUs.value_or(clock.nowUs()) - startedAtUs <= static_cast<uint64_t>(limit.count()));
        }
        // A failed configuration says why, and a successful one leaves no diagnostic.
        require(configured == driver.configurationError().isEmpty());
        touched += checksum(driver.configurationError()) + checksum(driver.receiverIdentity());
        for (const auto& evidence : driver.configurationEvidence()) {
            for (const char character : evidence.command) {
                touched += static_cast<unsigned char>(character);
            }
        }
        if (session == 0) {
            outcome.configured = configured;
        }
        int silentCycles = 0;
        for (int cycle = 0; configured && cycle < MAX_RECEIVE_CYCLES && silentCycles < SILENT_RECEIVE_CYCLES; ++cycle) {
            const GPSReceiveResult result = driver.receiveOutcome(RECEIVE_TIMEOUT);
            touched += checksum(result.detail);
            if (result.error != GPSProtocolError::None) {
                break;
            }
            silentCycles += link.silent();
        }
        if (link.silent()) {
            break;
        }
    }
    s_touched = touched;
    return outcome;
}

}  // namespace GPSReplyFuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    (void) GPSReplyFuzz::run({data, size});
    return 0;
}
