#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <span>

#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#include "GPSTransport.h"

class QIODevice;

/// GPSTransport over a QIODevice its subclass owns. Implements the bounded read and write contract once; subclasses
/// adapt it through the protected hooks. Waits dispatch the owner thread's events and wake at once on a stop.
class GPSDeviceTransport : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override;

protected:
    /// The open device, or null before open().
    virtual QIODevice* device() const = 0;

    /// Whether read() fails without waiting; by default, when there is no device.
    virtual bool readUnavailable() const;
    /// Waits until input is buffered. Input reported buffered is delivered even after the link fails.
    virtual bool waitReadable(QDeadlineTimer deadline);
    /// Takes buffered input. @return the bytes taken, or -1 on failure.
    virtual qint64 take(std::span<uint8_t> buffer);
    /// Describes a failed link; by default, a closed connection with the device's error.
    virtual GPSReadResult readFailure() const;

    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;
    /// Waits for queued bytes to drain. May expire @a deadline to end the write.
    virtual void waitWritten(QDeadlineTimer& deadline);
    /// Bytes of this write the link confirmed, given @a accepted bytes accepted and @a drained bytes reported by the
    /// device's bytesWritten() during the write; by default @a drained.
    virtual qint64 confirmedWritten(int accepted, qint64 drained);
    /// Retires the connection after a failed write that accepted bytes; by default closes the device.
    virtual void retire();

    /// Dispatches events until @a done holds, @a deadline expires or a stop is requested, also woken by the
    /// device's input, output, error and state signals. @return whether @a done holds and no stop was requested.
    bool waitUntil(const std::function<bool()>& done, QDeadlineTimer deadline);
    /// waitUntil() for @a ready or a failed link. Short waitForConnected() calls would abort DNS and connection
    /// progress on timeout. @return whether @a ready holds on a healthy link.
    bool waitForDevice(const std::function<bool()>& ready, QDeadlineTimer deadline);
};
