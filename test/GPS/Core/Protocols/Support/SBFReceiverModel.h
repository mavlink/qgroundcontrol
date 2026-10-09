#pragma once

#include <algorithm>
#include <chrono>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>

#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ScriptedReceiver.h"

namespace GPSTest {

class SBFReceiverModel : public ScriptedReceiver::Model
{
public:
    explicit SBFReceiverModel(GPSTestClock& clock)
        : _clock(clock)
    {}

    static QByteArray pvt(uint8_t used, uint32_t tow)
    {
        std::vector<uint8_t> payload(82);
        payload[0] = 0x41;
        payload[60] = used;
        return block(4007, payload, tow);
    }

    static constexpr std::chrono::milliseconds STREAM_INTERVAL{1};

    bool streaming = false;
    bool sendUsage = false;
    int streamReads = 0;
    QByteArray reply;
    /// Command replies leave out their line ending.
    bool unterminatedReply = false;
    /// The most bytes one read returns.
    int readChunk = std::numeric_limits<int>::max();

private:
    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        Q_UNUSED(context)
        receiver.clearReplies();
        reply = command == "\n\r" ? QByteArray("USB1>") : "$R: " + command;
        while (unterminatedReply && (reply.endsWith('\r') || reply.endsWith('\n'))) {
            reply.chop(1);
        }
        receiver.queueReply(std::exchange(reply, {}));
        const int length = command.size();
        return GPSWriteResult{GPSWriteStatus::Completed, length, length};
    }

    /// Without a clock the transport path never waits, so a streaming receiver sends a block on every read.
    void onTransportReadWait(ScriptedReceiver& receiver, std::chrono::milliseconds timeout) override
    {
        Q_UNUSED(timeout)
        _deliver(receiver);
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        if (reply.isEmpty() && streaming) {
            // A streaming receiver sends one block every STREAM_INTERVAL.
            const uint64_t blockAtUs = (std::max) (_clock.nowUs(), _nextBlockUs);
            if (blockAtUs > deadline.untilUs) {
                _clock.advanceTo(deadline.untilUs + 1);
                return;
            }
            _clock.advanceTo(blockAtUs);
            _nextBlockUs = blockAtUs + static_cast<uint64_t>(std::chrono::microseconds(STREAM_INTERVAL).count());
        }
        _deliver(receiver);
        if (!receiver.hasQueuedReadData()) {
            _clock.advanceTo(deadline.untilUs + 1);
        }
    }

    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override
    {
        Q_UNUSED(receiver)
        return (std::min) ({requested, available, readChunk});
    }

    void _deliver(ScriptedReceiver& receiver)
    {
        if (reply.isEmpty() && streaming) {
            ++streamReads;
            reply = streamReads == 8 && sendUsage ? pvt(12, streamReads) : block(4001, 32, streamReads);
        }
        if (!reply.isEmpty()) {
            receiver.queueReply(std::exchange(reply, {}));
        }
    }

    static QByteArray block(uint16_t id, int length, uint32_t tow)
    {
        return block(id, std::vector<uint8_t>(static_cast<size_t>(length - 14)), tow);
    }

    static QByteArray block(uint16_t id, const std::vector<uint8_t>& payload, uint32_t tow)
    {
        const auto bytes = sbfBlock(id, std::span<const uint8_t>(payload.data(), payload.size()), tow, 2300);
        return QByteArray(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));
    }

    GPSTestClock& _clock;
    uint64_t _nextBlockUs = 0;
};

}  // namespace GPSTest
