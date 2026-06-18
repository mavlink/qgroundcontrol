#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QIODevice>
#include <QtCore/QList>
#include <QtCore/QSpan>
#include <QtCore/QString>

#include "QGCSerialPort.h"

QT_FORWARD_DECLARE_CLASS(QObject)

class AndroidSerialPortPrivate;
class QGCSerialPortInfo;

class AndroidSerialPort : public QGCSerialPort
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(AndroidSerialPort)
    friend class AndroidSerialPortPrivate;  // JNI callbacks live in Private; reach back via _dispatch* helpers

public:
    explicit AndroidSerialPort(QObject* parent = nullptr);
    explicit AndroidSerialPort(const QString& systemLocation, QObject* parent = nullptr);
    ~AndroidSerialPort() override;

    static bool initializeNative();  // false if JNI native registration / wire-constant check failed
    static QList<QGCSerialPortInfo> availableDevices();

    bool isSequential() const override { return true; }

    // Streaming device: more data is always possible while open. Base default reports atEnd=true between reads.
    bool atEnd() const override { return false; }

    bool open(QIODeviceBase::OpenMode mode) override;
    void close() override;
    qint64 bytesToWrite() const override;
    bool waitForReadyRead(int msecs) override;
    bool waitForBytesWritten(int msecs) override;

    void setPortName(const QString& name) override { setSystemLocation(name); }

    QString systemLocation() const;
    void setSystemLocation(const QString& location);
    QString portName() const override;  // Display name (last path component).

    // Single source of truth for wire params; stashes while closed, pushes to Java when open.
    bool reconfigure(const SerialPortConfig& cfg) override;
    SerialPortConfig portConfig() const;

    bool setDataTerminalReady(bool set) override;

    bool flush() override;
    bool clear(bool input = true, bool output = true);

    QGCSerialPortError error() const override;
    void clearError() override;

    void setWriteBufferSize(qint64 size) override;
    qint64 writeBufferSize() const override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

private:
    bool _validateOpenMode(QIODeviceBase::OpenMode mode);
};
