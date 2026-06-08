#pragma once

// Unified serial interface; makeSerialPort() picks impl per platform (no caller #ifdef needed).

#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include "QGCSerialPortTypes.h"

class QIODevicePrivate;

class QGCSerialPort : public QIODevice
{
    Q_OBJECT

public:
    explicit QGCSerialPort(QObject* parent = nullptr) : QIODevice(parent) {}

    // Host: a QSerialPort portName. Android: a systemLocation() (USB) or "/dev/tty*" path.
    virtual void setPortName(const QString& name) = 0;
    virtual QString portName() const = 0;

    // Atomic wire-parameter apply (one JNI hop on Android USB; wraps QSerialPort termios calls on host).
    virtual bool reconfigure(const SerialPortConfig& cfg) = 0;

    // Rolls back to closed on either failure; false always leaves device closed.
    bool openConfigured(QIODevice::OpenMode mode, const SerialPortConfig& cfg)
    {
        if (!open(mode)) {
            return false;
        }
        if (!reconfigure(cfg)) {
            close();
            return false;
        }
        return true;
    }

    virtual bool setDataTerminalReady(bool on) = 0;
    virtual bool flush() = 0;
    virtual void setWriteBufferSize(qint64 size) = 0;
    virtual qint64 writeBufferSize() const = 0;
    virtual QGCSerialPortError error() const = 0;
    virtual void clearError() = 0;

signals:
    void errorOccurred(QGCSerialPortError error);

protected:
    // d-pointer ctor for subclasses with a custom QIODevicePrivate (AndroidSerialPort); keeps the type incomplete here.
    QGCSerialPort(QIODevicePrivate& dd, QObject* parent) : QIODevice(dd, parent) {}
};
