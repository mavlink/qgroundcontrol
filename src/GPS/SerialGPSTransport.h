#pragma once

#include "GPSTransport.h"

#include <QtCore/QString>

#include <atomic>
#include <cstdint>
#include <memory>

class QSerialPort;

/// GPSTransport backed by a QSerialPort. Owns the port and must be constructed on
/// the thread that pumps the driver — QSerialPort has thread affinity.
class SerialGPSTransport : public GPSTransport
{
public:
    SerialGPSTransport(QString device, const std::atomic_bool &requestStop);
    ~SerialGPSTransport() override;

    /// Open the device, retrying briefly while it settles after startup. Aborts the
    /// retry promptly if requestStop is set, so a disconnect can't be stalled by it.
    bool open();

    /// True once the port hits an error the receive loop should stop retrying past.
    bool fatalError() const;

    int read(uint8_t *buffer, int length, int timeoutMs) override;
    int write(const uint8_t *buffer, int length) override;
    bool setBaudrate(unsigned baudrate) override;

private:
    static constexpr int kWriteTimeoutMs = 500;

    QString _device;
    const std::atomic_bool &_requestStop;
    std::unique_ptr<QSerialPort> _serial;
};
