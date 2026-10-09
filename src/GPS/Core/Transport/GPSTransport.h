#pragma once

#include <chrono>
#include <cstdint>
#include <span>

#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#include "GPSCancellation.h"

enum class GPSOpenStatus
{
    Opened,
    TimedOut,
    Cancelled,
    Error,
    Unsupported
};

enum class GPSReadStatus
{
    Data,
    TimedOut,
    Cancelled,
    Closed,
    Error,
    Overflow
};

enum class GPSWriteStatus
{
    Completed,
    TimedOut,
    Cancelled,
    Error,
    Unsupported
};

struct [[nodiscard]] GPSOpenResult
{
    GPSOpenStatus status = GPSOpenStatus::Unsupported;
    QString detail = {};
};

struct [[nodiscard]] GPSReadResult
{
    GPSReadStatus status = GPSReadStatus::TimedOut;
    int bytesRead = 0;
    QString detail = {};
};

struct [[nodiscard]] GPSWriteResult
{
    GPSWriteStatus status = GPSWriteStatus::Unsupported;
    int acceptedBytes = 0;
    int writtenBytes = 0;
    QString detail = {};

    /// Invalid counts return -1 rather than masking inconsistent evidence.
    [[nodiscard]] int uncertainBytes() const
    {
        return acceptedBytes >= 0 && writtenBytes >= 0 && writtenBytes <= acceptedBytes ? acceptedBytes - writtenBytes
                                                                                        : -1;
    }
};

/// Byte link the GPS driver reads and writes through.
/// Implemented by the owner of the physical connection and consumed by GPSDriver,
/// keeping the native protocol drivers decoupled from the concrete transport.
/// Construct, use, and destroy on one owner thread. Blocking waits dispatch events:
/// callbacks must not reenter, reopen, or destroy the transport during a call.
/// Other threads cancel through the GPSCancelSource that issued the construction token.
class GPSTransport
{
public:
    explicit GPSTransport(GPSCancelToken cancelToken);
    virtual ~GPSTransport();

    virtual GPSOpenResult open() = 0;
    virtual bool fatalError() const = 0;

    bool isCancelled() const { return _cancelToken.isCancelled(); }

    const GPSCancelToken& cancelToken() const { return _cancelToken; }

    /// Nonzero when the link cannot follow baud-rate changes (for example, a serial bridge).
    virtual unsigned fixedBaudrate() const { return 0; }

    /// Line rate that TCP and UDP links report. Drivers still program the receiver's serial port with it (UBX CFG-PRT
    /// sets UART1 to it), so the serial side of a network bridge and its receiver must run at this rate.
    static constexpr unsigned BRIDGE_BAUDRATE = 115200;
    /// Received bytes a link buffers before it stops taking more (TCP), retires the session (serial) or drops the
    /// oldest (UDP).
    static constexpr qint64 READ_BUFFER_BYTES = 64 * 1024;
    /// Bytes a link queues for writing at once.
    static constexpr qint64 WRITE_BUFFER_BYTES = 4 * 1024;

    /// A nonpositive timeout polls immediately available input. Failures never carry usable stream bytes. An empty
    /// @a buffer reports the failure of a failed link without reading.
    virtual GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) = 0;

    virtual std::chrono::milliseconds configurationWriteTimeout() const;

    /// Receiver configuration write under the command deadline, capped by configurationWriteTimeout().
    /// Counts describe transport progress, never receiver acknowledgement.
    /// A failed operation that accepted bytes retires the connection; open a new session before writing again.
    GPSWriteResult write(QByteArrayView bytes, QDeadlineTimer deadline);

    /// Sets the link baud rate. Returns true on success. By default only a healthy link's fixed rate succeeds, as a
    /// network bridge cannot change it. A refused rate leaves the link usable unless fatalError() reports otherwise.
    virtual bool setBaudrate(unsigned baudrate);

protected:
    /// Receives non-empty bytes and an unexpired, capped deadline that implementations must honor.
    virtual GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) = 0;

private:
    GPSCancelToken _cancelToken;
};
