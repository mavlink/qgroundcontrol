#pragma once

#include <algorithm>
#include <atomic>
#include <cstring>
#include <deque>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>

#include "GPSTransport.h"

class ScriptedGPSTransport : public GPSTransport
{
public:
    explicit ScriptedGPSTransport(const std::atomic_bool& requestStop)
        : GPSTransport(requestStop)
    {}

    GPSOpenResult open() override
    {
        if (const auto result = handleOpen()) {
            return *result;
        }
        return {GPSOpenStatus::Opened};
    }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t* buffer, int length, int timeoutMs) override
    {
        if (const auto result = handleRead(buffer, length, timeoutMs)) {
            return *result;
        }
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        return readQueued(buffer, length);
    }

    GPSWriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline) override
    {
        if (length < 0 || (!buffer && length > 0)) {
            return {GPSWriteStatus::InvalidData};
        }
        if (isCancelled()) {
            return {GPSWriteStatus::Cancelled};
        }
        if (deadline.hasExpired()) {
            return {GPSWriteStatus::TimedOut};
        }
        const QByteArray bytes = length > 0 ? QByteArray(reinterpret_cast<const char*>(buffer), length) : QByteArray();
        if (const auto result = handleWrite(bytes, deadline)) {
            return *result;
        }
        return {GPSWriteStatus::Unsupported};
    }

    bool setBaudrate(unsigned baudrate) override
    {
        if (const auto accepted = handleBaudrate(baudrate)) {
            return *accepted;
        }
        return true;
    }

    void queueReadChunk(const QByteArray& bytes)
    {
        if (!bytes.isEmpty()) {
            _readSteps.push_back(ReadStep{.bytes = bytes});
        }
    }

    void clearReadQueue() { _readSteps.clear(); }

    bool hasQueuedReadData() const { return !_readSteps.empty(); }

protected:
    GPSReadResult readQueued(uint8_t* buffer, int length)
    {
        if (_readSteps.empty()) {
            return {GPSReadStatus::TimedOut};
        }
        ReadStep& step = _readSteps.front();
        if (!buffer || length <= 0) {
            return {GPSReadStatus::InvalidData};
        }
        const int remaining = step.bytes.size() - step.offset;
        const int count = std::min(length, remaining);
        std::memcpy(buffer, step.bytes.constData() + step.offset, static_cast<size_t>(count));
        step.offset += count;
        if (step.offset == step.bytes.size()) {
            _readSteps.pop_front();
        }
        return {GPSReadStatus::Data, count};
    }

    virtual std::optional<GPSOpenResult> handleOpen() { return std::nullopt; }

    virtual std::optional<GPSReadResult> handleRead(uint8_t* buffer, int length, int timeoutMs)
    {
        Q_UNUSED(buffer)
        Q_UNUSED(length)
        Q_UNUSED(timeoutMs)
        return std::nullopt;
    }

    virtual std::optional<GPSWriteResult> handleWrite(const QByteArray& bytes, QDeadlineTimer deadline)
    {
        Q_UNUSED(bytes)
        Q_UNUSED(deadline)
        return std::nullopt;
    }

    virtual std::optional<bool> handleBaudrate(unsigned baudrate)
    {
        Q_UNUSED(baudrate)
        return std::nullopt;
    }

private:
    struct ReadStep
    {
        QByteArray bytes = {};
        int offset = 0;
    };

    std::deque<ReadStep> _readSteps;
};
