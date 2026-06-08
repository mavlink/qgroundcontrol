#include "SerialGPSTransport.h"

#include "QGCLoggingCategory.h"
#include "QGCSerialPort.h"
#include "SerialPlatform.h"

#include <QtCore/QThread>

#include <utility>

QGC_LOGGING_CATEGORY(SerialGPSTransportLog, "GPS.SerialGPSTransport")

SerialGPSTransport::SerialGPSTransport(QString device, const std::atomic_bool &requestStop)
    : _device(std::move(device))
    , _requestStop(requestStop)
{
}

SerialGPSTransport::~SerialGPSTransport() = default;

bool SerialGPSTransport::open()
{
    _serial.reset(SerialPlatform::makeSerialPort(_device, nullptr));
    if (!_serial) {
        qCWarning(SerialGPSTransportLog) << "GPS: Failed to create Serial Device" << _device;
        return false;
    }
    if (!_serial->open(QIODevice::ReadWrite)) {
        // Device can take 10-20s to become accessible after startup.
        uint32_t retries = 60;
        while ((retries-- > 0) && !_requestStop && (_serial->error() == QGCSerialPortError::PermissionDenied)) {
            qCDebug(SerialGPSTransportLog) << "Cannot open device... retrying";
            QThread::msleep(500);
            _serial->clearError();
            if (_serial->open(QIODevice::ReadWrite)) {
                break;
            }
        }

        if (_serial->error() != QGCSerialPortError::NoError) {
            qCWarning(SerialGPSTransportLog) << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
            return false;
        }
    }

    _config.baud = 9600;
    _config.dataBits = QGCDataBits::Data8;
    _config.parity = QGCParity::None;
    _config.stopBits = QGCStopBits::OneStop;
    _config.flowControl = QGCFlowControl::None;
    if (!_serial->reconfigure(_config)) {
        qCWarning(SerialGPSTransportLog) << "GPS: Failed to configure Serial Device" << _device << _serial->errorString();
        _serial->close();
        return false;
    }

    return true;
}

bool SerialGPSTransport::fatalError() const
{
    return _serial
        && (_serial->error() != QGCSerialPortError::NoError)
        && (_serial->error() != QGCSerialPortError::Timeout);
}

int SerialGPSTransport::read(uint8_t *buffer, int length, int timeoutMs)
{
    if (_requestStop) {
        return -1; // abort an in-flight configure/receive so disconnect joins promptly
    }
    if (_serial->bytesAvailable() == 0) {
        if (!_serial->waitForReadyRead(timeoutMs)) {
            return 0;
        }
    }
    return _serial->read(reinterpret_cast<char *>(buffer), length);
}

int SerialGPSTransport::write(const uint8_t *buffer, int length)
{
    if (_requestStop) {
        return -1;
    }
    int written = 0;
    while (written < length) {
        const qint64 n = _serial->write(reinterpret_cast<const char *>(buffer) + written, length - written);
        if (n < 0) {
            return -1;
        }
        written += static_cast<int>(n);
        if (written < length && !_serial->waitForBytesWritten(kWriteTimeoutMs)) {
            return -1;
        }
    }
    return written;
}

bool SerialGPSTransport::setBaudrate(unsigned baudrate)
{
    _config.baud = static_cast<qint32>(baudrate);
    return _serial->reconfigure(_config);
}
