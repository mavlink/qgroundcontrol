#pragma once

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QThread>

#include "Protocols/ProtocolTestPackets.h"
#include "Support/ScriptedReceiver.h"
#ifdef QGC_GPS_TEST_CLOCK
#include "GPSProtocolTestIO.h"
#endif

class SBFReceiverModel : public ScriptedReceiver::Model
{
public:
    static QByteArray pvt(uint8_t used, uint32_t tow)
    {
        std::vector<uint8_t> payload(82);
        payload[0] = 0x41;
        payload[60] = used;
        return block(4007, payload, tow);
    }

    bool streaming = false;
    bool sendUsage = false;
    int streamReads = 0;
    QByteArray reply;

private:
    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        Q_UNUSED(context)
        receiver.clearReplies();
        reply = command == "\n\r" ? QByteArray("USB1>") : "$R: " + command;
        receiver.queueReply(std::exchange(reply, {}));
        const int length = command.size();
        return GPSWriteResult{GPSWriteStatus::Completed, length, length};
    }

    void onTransportReadWait(ScriptedReceiver& receiver, int timeoutMs) override
    {
        if (reply.isEmpty() && streaming) {
            if (timeoutMs > 0) {
                QThread::msleep(1);
            }
            ++streamReads;
            reply = streamReads == 8 && sendUsage ? pvt(12, streamReads) : block(4001, 32, streamReads);
        }
        if (!reply.isEmpty()) {
            receiver.queueReply(std::exchange(reply, {}));
        }
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
#ifdef QGC_GPS_TEST_CLOCK
        onTransportReadWait(receiver, deadline.remainingMilliseconds(gps_test_time));
        if (!receiver.hasQueuedReadData()) {
            gps_test_time = deadline.untilUs + 1;
        }
#else
        Q_UNUSED(receiver)
        Q_UNUSED(deadline)
#endif
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
};
