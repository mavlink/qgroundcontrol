#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ScriptedReceiver.h"

namespace GPSTest {

class FemtoReceiverModel : public ScriptedReceiver::Model
{
public:
    explicit FemtoReceiverModel(GPSTestClock& clock)
        : _clock(clock)
    {}

    QString idleReadDetail = QStringLiteral("Scripted receiver connection lost");
    bool failIdleReads = false;
    /// NUL bytes ahead of each acknowledgement.
    int noiseBytes = 0;
    /// The most bytes one read returns.
    int readChunk = std::numeric_limits<int>::max();

private:
    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        Q_UNUSED(context)
        QByteArray reply = QByteArray(noiseBytes, '\0') + '<' + command.split(' ').first().trimmed() + " OK";
        reply.append(char(0));
        receiver.clearReplies();
        receiver.queueReply(reply);
        const int size = command.size();
        return GPSWriteResult{GPSWriteStatus::Completed, size, size};
    }

    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override
    {
        Q_UNUSED(receiver)
        return (std::min) ({requested, available, readChunk});
    }

    void onTransportReadWait(ScriptedReceiver& receiver, std::chrono::milliseconds timeout) override
    {
        Q_UNUSED(timeout)
        if (failIdleReads) {
            receiver.failNextRead(GPSReadResult{GPSReadStatus::Error, 0, idleReadDetail});
        }
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        onTransportReadWait(receiver, deadline.remaining(_clock.nowUs()));
        if (!receiver.hasQueuedReadData()) {
            _clock.advanceTo(deadline.untilUs + 1);
        }
    }

    GPSTestClock& _clock;
};

}  // namespace GPSTest
