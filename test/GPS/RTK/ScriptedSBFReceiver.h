#pragma once

#include <algorithm>
#include <cstring>
#include <span>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QThread>

#include "Driver/Protocols/ProtocolTestPackets.h"
#include "ScriptedGPSTransport.h"

class ScriptedSBFReceiver : public ScriptedGPSTransport
{
public:
    using ScriptedGPSTransport::ScriptedGPSTransport;

protected:
    std::optional<GPSWriteResult> handleWrite(const QByteArray& command, QDeadlineTimer) override
    {
        reply = command == "\n\r" ? QByteArray("USB1>") : "$R: " + command;
        const int length = command.size();
        return GPSWriteResult{GPSWriteStatus::Completed, length, length};
    }

    std::optional<GPSReadResult> handleRead(uint8_t* bytes, int length, int timeoutMs) override
    {
        if (isCancelled()) {
            return GPSReadResult{GPSReadStatus::Cancelled};
        }
        if (reply.isEmpty() && streaming) {
            if (timeoutMs > 0) {
                QThread::msleep(1);
            }
            ++streamReads;
            reply = streamReads == 8 && sendUsage ? pvt(12, streamReads) : block(4001, 32, streamReads);
        }
        if (reply.isEmpty()) {
            return GPSReadResult{GPSReadStatus::TimedOut};
        }
        const auto count = std::min(length, static_cast<int>(reply.size()));
        std::memcpy(bytes, reply.constData(), static_cast<size_t>(count));
        reply.remove(0, count);
        return GPSReadResult{GPSReadStatus::Data, count};
    }

public:
    static QByteArray pvt(uint8_t used, uint32_t tow)
    {
        std::vector<uint8_t> payload(82);
        payload[0] = 0x41;  // Valid fixed-base solution.
        payload[60] = used;
        return block(4007, payload, tow);
    }

    bool streaming = false;
    bool sendUsage = false;
    int streamReads = 0;
    QByteArray reply;

private:
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
