#pragma once

#include <memory>

#include "GPSRecordingBuffer.h"
#include "GPSTransport.h"

/// Records the transport contract without changing return values, cancellation or worker ownership.
class GPSRecordingTransport : public GPSTransport
{
public:
    GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, const std::atomic_bool& stop,
                          std::shared_ptr<GPSRecordingStream> recording);
    ~GPSRecordingTransport() override;
    bool open() override;
    bool fatalError() const override;
    unsigned fixedBaudrate() const override;
    std::chrono::milliseconds correctionWriteTimeout(int length) const override;
    int read(uint8_t* buffer, int length, int timeoutMs) override;
    int write(const uint8_t* buffer, int length) override;
    WriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;
    bool setBaudrate(unsigned baudrate) override;

private:
    std::unique_ptr<GPSTransport> _transport;
    std::shared_ptr<GPSRecordingStream> _recording;
};
