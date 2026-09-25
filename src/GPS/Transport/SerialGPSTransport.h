#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <stop_token>

#include <QtCore/QString>

#include "GPSTransport.h"

class QSerialPort;

/// GPSTransport backed by a QSerialPort. Owns the port and must be constructed on
/// the thread that pumps the driver — QSerialPort has thread affinity.
class SerialGPSTransport : public GPSTransport
{
public:
    static constexpr qint64 kWriteBufferBytes = 4 * 1024;
    static constexpr qint64 kReadBufferBytes = 64 * 1024;

    SerialGPSTransport(QString device, std::stop_token stopToken);
    ~SerialGPSTransport() override;

    /// Open the device, retrying briefly while it settles after startup. Aborts the
    /// retry promptly once a stop is requested, so a disconnect can't be stalled by it.
    GPSOpenResult open() override;

    /// True once the port hits an error the receive loop should stop retrying past.
    bool fatalError() const override;

    GPSReadResult read(uint8_t* buffer, int length, std::chrono::milliseconds timeout) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;
    bool setBaudrate(unsigned baudrate) override;

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;

private:
    /// QSerialPort waits block on the port alone (Qt's single-descriptor poll, or the Android backend's own wait
    /// condition) and offer no cross-thread wake-up, so serial waits run in slices this long to observe a stop.
    static constexpr std::chrono::milliseconds kCancellationPoll{50};
    static constexpr std::chrono::milliseconds kOpenTimeout{30000};
    static constexpr std::chrono::milliseconds kOpenRetry{500};
    static constexpr std::chrono::milliseconds kWriteTimeout{500};

    QString _errorDetail() const;
    bool _inputBudgetExhausted() const;

    QString _device;
    std::unique_ptr<QSerialPort> _serial;
    bool _inputOverflow = false;
    qint64 _acceptedTotal = 0;
    qint64 _writtenTotal = 0;
};
