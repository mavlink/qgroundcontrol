#pragma once

#include <QtCore/QString>

#include <atomic>
#include <cstdint>
#include <memory>

#include "GPSTransport.h"

class QSerialPort;

/// GPSTransport backed by a QSerialPort. Owns the port and must be constructed on
/// the thread that pumps the driver — QSerialPort has thread affinity.
class SerialGPSTransport : public GPSTransport
{
public:
    static constexpr qint64 kWriteBufferBytes = 4 * 1024;
    static constexpr qint64 kReadBufferBytes = 64 * 1024;

    SerialGPSTransport(QString device, const std::atomic_bool& requestStop);
    ~SerialGPSTransport() override;

    /// Open the device, retrying briefly while it settles after startup. Aborts the
    /// retry promptly if requestStop is set, so a disconnect can't be stalled by it.
    OpenResult open() override;

    /// True once the port hits an error the receive loop should stop retrying past.
    bool fatalError() const override;

    ReadResult read(uint8_t* buffer, int length, int timeoutMs) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;
    WriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;
    std::chrono::milliseconds correctionWriteTimeout(int length) const override;
    bool setBaudrate(unsigned baudrate) override;

private:
    static constexpr int kOpenTimeoutMs = 30000;
    static constexpr int kOpenRetryMs = 500;
    static constexpr int kWriteTimeoutMs = 500;

    QString _errorDetail() const;
    bool _inputBudgetExhausted() const;

    QString _device;
    std::unique_ptr<QSerialPort> _serial;
    bool _inputOverflow = false;
    qint64 _acceptedTotal = 0;
    qint64 _writtenTotal = 0;
};
