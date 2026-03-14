#include "SerialGPSTransport.h"
#include "QGCLoggingCategory.h"

#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif

#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(SerialGPSTransportLog, "GPS.SerialGPSTransport")

SerialGPSTransport::SerialGPSTransport(const QString &device, QObject *parent)
    : GPSTransport(parent)
    , _device(device)
{
}

SerialGPSTransport::~SerialGPSTransport()
{
    close();
}

bool SerialGPSTransport::open()
{
    _serial = new QSerialPort(this);
    _serial->setPortName(_device);
    if (!_serial->open(QIODevice::ReadWrite)) {
        uint32_t retries = 60;
        while ((retries-- > 0) && (_serial->error() == QSerialPort::PermissionError)) {
            qCDebug(SerialGPSTransportLog) << "Cannot open device... retrying";
            QThread::msleep(500);
            if (_serial->open(QIODevice::ReadWrite)) {
                _serial->clearError();
                break;
            }
        }

        if (_serial->error() != QSerialPort::NoError) {
            qCWarning(SerialGPSTransportLog) << "Failed to open Serial Device" << _device << _serial->errorString();
            delete _serial;
            _serial = nullptr;
            return false;
        }
    }

    (void) _serial->setBaudRate(QSerialPort::Baud9600);
    (void) _serial->setDataBits(QSerialPort::Data8);
    (void) _serial->setParity(QSerialPort::NoParity);
    (void) _serial->setStopBits(QSerialPort::OneStop);
    (void) _serial->setFlowControl(QSerialPort::NoFlowControl);

    return true;
}

void SerialGPSTransport::close()
{
    if (_serial) {
        _serial->close();
        delete _serial;
        _serial = nullptr;
    }
}

bool SerialGPSTransport::isOpen() const
{
    return _serial && _serial->isOpen();
}

qint64 SerialGPSTransport::read(char *data, qint64 maxSize)
{
    return _serial ? _serial->read(data, maxSize) : -1;
}

qint64 SerialGPSTransport::write(const char *data, qint64 size)
{
    return _serial ? _serial->write(data, size) : -1;
}

bool SerialGPSTransport::waitForReadyRead(int msecs)
{
    return _serial && _serial->waitForReadyRead(msecs);
}

bool SerialGPSTransport::waitForBytesWritten(int msecs)
{
    return _serial && _serial->waitForBytesWritten(msecs);
}

qint64 SerialGPSTransport::bytesAvailable() const
{
    return _serial ? _serial->bytesAvailable() : 0;
}

bool SerialGPSTransport::setBaudRate(qint32 baudRate)
{
    return _serial && _serial->setBaudRate(baudRate);
}
