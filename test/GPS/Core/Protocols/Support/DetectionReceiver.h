#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ScriptedReceiver.h"

namespace GPSTest {

/// The command language of one receiver family.
enum class Dialect : uint8_t
{
    UBX,
    Ashtech,
    SBF,
    Femto,
    Unicore,
    Quectel,
};

/// Printable text with a line ending, as every text command is written.
inline bool textCommand(QByteArrayView bytes)
{
    bool ended = false;
    for (const char ch : bytes) {
        ended |= ch == '\r' || ch == '\n';
        if (ch != '\r' && ch != '\n' && (ch < ' ' || ch > '~')) {
            return false;
        }
    }
    return ended;
}

/// Whether a receiver speaking @a dialect parses @a write, one write as the families send it. Receiver detection sends
/// every family's identity query, and a real receiver ignores those in other dialects, which the models would answer.
inline bool parsesCommand(Dialect dialect, QByteArrayView write)
{
    switch (dialect) {
        case Dialect::UBX:
            // A UBX frame is written in parts; only whole text commands are foreign.
            return !textCommand(write);
        case Dialect::Ashtech:
            return write.startsWith("$PASH");
        case Dialect::SBF:
            return textCommand(write);
        case Dialect::Femto:
            for (const QByteArrayView command : {"UNLOGALL ", "VERSION\r", "LOG ", "POSAVE ", "FIX "}) {
                if (write.startsWith(command)) {
                    return textCommand(write);
                }
            }
            return false;
        case Dialect::Unicore:
            return textCommand(write) && write.endsWith("\r\n") && write.front() >= 'A' && write.front() <= 'Z';
        case Dialect::Quectel:
            return write.startsWith("$PQTM");
    }
    return false;
}

/// One receiver of receiver-detection tests, around its family's model: it parses only its own dialect, answers and
/// streams only while the host runs the link at the receiver's rate, and records every write.
class DetectionReceiver final : public ForwardingModel
{
public:
    static constexpr uint64_t STREAM_INTERVAL_US = 100000;

    /// @a atRate reports whether the host link runs at the receiver's current rate.
    DetectionReceiver(ScriptedReceiver::Model& model, Dialect dialect, GPSTestClock& clock,
                      std::function<bool()> atRate)
        : ForwardingModel(model)
        , _dialect(dialect)
        , _clock(clock)
        , _atRate(std::move(atRate))
    {}

    /// Starts filtering the writes of @a receiver, which must already use this model.
    void attach(ScriptedReceiver& receiver)
    {
        receiver.setWriteHandler(
            [this](const QByteArray& bytes, const ScriptedReceiver::WriteContext&) -> std::optional<GPSWriteResult> {
                writes.push_back(bytes);
                if (_atRate() && parsesCommand(_dialect, bytes)) {
                    return std::nullopt;
                }
                const int length = static_cast<int>(bytes.size());
                return GPSWriteResult{GPSWriteStatus::Completed, length, length};
            });
    }

    /// Output at the receiver's rate, one copy per STREAM_INTERVAL_US while nothing else is queued.
    QByteArray stream;
    /// Every host write, parsed or not.
    std::vector<QByteArray> writes;

    std::optional<bool> handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate) override
    {
        // Output queued at the previous rate is lost with the rate change.
        receiver.clearReplies();
        return _model.handleBaudrate(receiver, baudrate);
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        if (!_atRate()) {
            _model.onProtocolReadWait(receiver, deadline);
            // What the receiver sends at another rate is noise, left out here.
            receiver.clearReplies();
            _clock.advanceTo(deadline.untilUs);
            return;
        }
        if (stream.isEmpty()) {
            _model.onProtocolReadWait(receiver, deadline);
            return;
        }
        _model.onProtocolReadWait(receiver, {std::min(deadline.untilUs, _clock.nowUs() + STREAM_INTERVAL_US)});
        if (!receiver.hasQueuedReadData()) {
            receiver.queueReply(stream);
        }
    }

private:
    Dialect _dialect;
    GPSTestClock& _clock;
    std::function<bool()> _atRate;
};

}  // namespace GPSTest
