#pragma once

#include <cstdint>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "Support/ScriptedReceiver.h"
#ifdef QGC_GPS_TEST_CLOCK
#include "GPSProtocolTestIO.h"
#endif

class FemtoReceiverModel : public ScriptedReceiver::Model
{
public:
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
#ifdef QGC_GPS_TEST_CLOCK
        const uint64_t now = gps_test_time;
#else
        const uint64_t now = 0;
#endif
        onTransportReadWait(receiver, deadline.remainingMilliseconds(now));
#ifdef QGC_GPS_TEST_CLOCK
        if (!receiver.hasQueuedReadData()) {
            gps_test_time = deadline.untilUs + 1;
        }
#else
        Q_UNUSED(deadline)
#endif
    }
};
