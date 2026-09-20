#pragma once

#include <algorithm>
#include <cstring>

#include <QtCore/QByteArray>
#include <QtCore/QThread>
#include <QtCore/QtEndian>

#include "GPSTransport.h"

class ScriptedSBFReceiver : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned) override { return true; }

    GPSWriteResult writeBounded(const uint8_t* bytes, int length, QDeadlineTimer deadline) override
    {
        if (isCancelled()) {
            return {GPSWriteStatus::Cancelled};
        }
        if (deadline.hasExpired()) {
            return {GPSWriteStatus::TimedOut};
        }
        const QByteArray command(reinterpret_cast<const char*>(bytes), length);
        reply = command == "\n\r" ? QByteArray("USB1>") : "$R: " + command;
        return {GPSWriteStatus::Completed, length, length};
    }

    GPSReadResult read(uint8_t* bytes, int length, int timeoutMs) override
    {
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        if (reply.isEmpty() && streaming) {
            if (timeoutMs > 0) {
                QThread::msleep(1);
            }
            ++streamReads;
            reply = streamReads == 8 && sendUsage ? pvt(12, streamReads) : block(4001, 32, streamReads);
        }
        if (reply.isEmpty()) {
            return {GPSReadStatus::TimedOut};
        }
        const auto count = std::min(length, static_cast<int>(reply.size()));
        std::memcpy(bytes, reply.constData(), static_cast<size_t>(count));
        reply.remove(0, count);
        return {GPSReadStatus::Data, count};
    }

    static QByteArray pvt(uint8_t used, uint32_t tow)
    {
        auto bytes = block(4007, 96, tow);
        bytes[14] = 0x41;  // Valid fixed-base solution.
        bytes[74] = static_cast<char>(used);
        checksum(bytes);
        return bytes;
    }

    bool streaming = false;
    bool sendUsage = false;
    int streamReads = 0;
    QByteArray reply;

private:
    static void checksum(QByteArray& bytes)
    {
        uint16_t crc = 0;
        for (qsizetype i = 4; i < bytes.size(); ++i) {
            crc ^= static_cast<uint16_t>(static_cast<uint8_t>(bytes[i])) << 8;
            for (int bit = 0; bit < 8; ++bit) {
                crc = static_cast<uint16_t>((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
            }
        }
        qToLittleEndian<uint16_t>(crc, bytes.data() + 2);
    }

    static QByteArray block(uint16_t id, int length, uint32_t tow)
    {
        QByteArray bytes(length, '\0');
        bytes[0] = '$';
        bytes[1] = '@';
        qToLittleEndian<uint16_t>(id, bytes.data() + 4);
        qToLittleEndian<uint16_t>(static_cast<uint16_t>(length), bytes.data() + 6);
        qToLittleEndian<uint32_t>(tow, bytes.data() + 8);
        qToLittleEndian<uint16_t>(2300, bytes.data() + 12);
        checksum(bytes);
        return bytes;
    }
};
