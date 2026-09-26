#pragma once

#include <algorithm>
#include <chrono>
#include <cstring>
#include <stop_token>
#include <thread>

#include <QtCore/QByteArray>

#include "GPSTransport.h"

/// Serves its input in fixed chunks and then idles until each read times out. Writes are kept; acceptLimit caps
/// the bytes one write accepts, which then times out.
class MemoryGPSTransport final : public GPSTransport
{
public:
    explicit MemoryGPSTransport(QByteArray input = {}, int chunkBytes = 64, std::stop_token stopToken = {})
        : GPSTransport(std::move(stopToken))
        , _input(std::move(input))
        , _chunkBytes(chunkBytes)
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t* buffer, int length, std::chrono::milliseconds timeout) override
    {
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        if (drained()) {
            std::this_thread::sleep_for((std::max) (timeout, std::chrono::milliseconds::zero()));
            return {GPSReadStatus::TimedOut};
        }
        const int count = static_cast<int>(std::min<qsizetype>({length, _chunkBytes, _input.size() - _offset}));
        std::memcpy(buffer, _input.constData() + _offset, static_cast<size_t>(count));
        _offset += count;
        return {GPSReadStatus::Data, count};
    }

    bool setBaudrate(unsigned) override { return true; }

    bool drained() const { return _offset >= _input.size(); }

    const QByteArray& written() const { return _written; }

    int acceptLimit = -1;

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer) override
    {
        const int accepted = acceptLimit >= 0 ? std::min(length, acceptLimit) : length;
        _written.append(reinterpret_cast<const char*>(buffer), accepted);
        return {accepted == length ? GPSWriteStatus::Completed : GPSWriteStatus::TimedOut, accepted, accepted};
    }

private:
    QByteArray _input;
    QByteArray _written;
    qsizetype _offset = 0;
    int _chunkBytes;
};
