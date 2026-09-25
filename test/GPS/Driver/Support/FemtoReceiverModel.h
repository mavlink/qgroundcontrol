#pragma once

#include <cstdint>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "GPSTestClock.h"
#include "Support/ScriptedReceiver.h"

class FemtoReceiverModel : public ScriptedReceiver::Model
{
public:
    explicit FemtoReceiverModel(GPSTestClock& clock)
        : _clock(clock)
    {}

    QString idleReadDetail = QStringLiteral("Scripted receiver connection lost");
    bool failIdleReads = false;

private:
    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        Q_UNUSED(context)
        QByteArray reply = '<' + command.split(' ').first().trimmed() + " OK";
        reply.append(char(0));
        receiver.clearReplies();
        receiver.queueReply(reply);
        const int size = command.size();
        return GPSWriteResult{GPSWriteStatus::Completed, size, size};
    }

    void onTransportReadWait(ScriptedReceiver& receiver, int timeoutMs) override
    {
        Q_UNUSED(timeoutMs)
        if (failIdleReads) {
            receiver.failNextRead(GPSReadResult{GPSReadStatus::Error, 0, idleReadDetail});
        }
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        onTransportReadWait(receiver, deadline.remainingMilliseconds(_clock.nowUs()));
        if (!receiver.hasQueuedReadData()) {
            _clock.advanceTo(deadline.untilUs + 1);
        }
    }

    GPSTestClock& _clock;
};
