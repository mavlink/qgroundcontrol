#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

#include <QtCore/QString>

#include "GPSDeviceTransport.h"

class QSerialPort;

/// GPSTransport backed by a QSerialPort. Owns the port and must be constructed on
/// the thread that pumps the driver — QSerialPort has thread affinity.
class SerialGPSTransport : public GPSDeviceTransport
{
public:
    /// How long open() keeps retrying a device it is not permitted to open yet.
    static constexpr std::chrono::milliseconds OPEN_TIMEOUT{30000};

    SerialGPSTransport(QString device, GPSCancelToken cancelToken,
                       std::chrono::milliseconds openTimeout = OPEN_TIMEOUT);
    ~SerialGPSTransport() override;

    /// Open the device, retrying briefly while it settles after startup. Aborts the
    /// retry promptly once a stop is requested, so a disconnect can't be stalled by it.
    GPSOpenResult open() override;

    /// True once the port hits an error the receive loop should stop retrying past.
    bool fatalError() const override;

    bool setBaudrate(unsigned baudrate) override;

protected:
    QIODevice* device() const override;
    bool readUnavailable() const override;
    bool waitReadable(QDeadlineTimer deadline) override;
    GPSReadResult readFailure() const override;
    void waitWritten(QDeadlineTimer& deadline) override;
    qint64 confirmedWritten(int accepted, qint64 drained) override;

private:
    /// Write waits run in slices this long to observe a stop: waitForBytesWritten() blocks on the port alone, and an
    /// event wait cannot replace it, as QSerialPort hands a block to the device without a signal (its bytesWritten()
    /// reports it only once the next block starts) and Android's backend sends queued writes under its own timeout.
    /// Read and open waits dispatch the port's events and wake at once.
    static constexpr std::chrono::milliseconds CANCELLATION_POLL{50};
    static constexpr std::chrono::milliseconds OPEN_RETRY{500};

    QString _errorDetail() const;
    bool _inputBudgetExhausted() const;

    QString _device;
    std::chrono::milliseconds _openTimeout;
    std::unique_ptr<QSerialPort> _serial;
    bool _inputOverflow = false;
    qint64 _acceptedTotal = 0;
    qint64 _writtenTotal = 0;
};
