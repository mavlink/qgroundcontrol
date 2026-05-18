// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

//
// QGCSerialPortAdapter — single serial-port type used across QGC.
//
// Inherits QIODevice so consumers (SerialLink, Bootloader, GPSProvider, NMEA)
// can do polymorphic read/write/wait* and pass instances to APIs that take
// QIODevice* (e.g. setNmeaSourceDevice). Owns one of:
//   - host:    QSerialPort
//   - Android: QGCAndroidSerialPort
// The Q_OS_ANDROID branch lives entirely inside the .cc — consumers never
// include qserialport.h or qserialport_android.cpp.
//
// Enums mirror QSerialPort::* numeric values exactly so existing call sites
// that used QSerialPort::Baud115200 / Data8 / NoParity migrate by symbol
// substitution; no value changes.
//

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include <memory>

class QGCSerialBackend;

class QGCSerialPortAdapter : public QIODevice
{
    Q_OBJECT

public:
    // Mirrors QSerialPort::SerialPortError values that SerialWorker switches on.
    // Full enum has more entries; we collapse the rest into OtherError because
    // SerialWorker's default branch already treats them uniformly.
    enum Error {
        NoError          = 0,
        ResourceError    = 9,
        PermissionError  = 2,
        TimeoutError     = 12,  // matches QSerialPort::TimeoutError
        OtherError       = 99,
    };
    Q_ENUM(Error)

    // Numeric values match QSerialPort::DataBits.
    enum DataBits { Data5 = 5, Data6 = 6, Data7 = 7, Data8 = 8 };
    Q_ENUM(DataBits)

    // Numeric values match QSerialPort::Parity — note the gap at 1 (no value).
    enum Parity {
        NoParity    = 0,
        EvenParity  = 2,
        OddParity   = 3,
        SpaceParity = 4,
        MarkParity  = 5,
    };
    Q_ENUM(Parity)

    // Numeric values match QSerialPort::StopBits.
    enum StopBits { OneStop = 1, OneAndHalfStop = 3, TwoStop = 2 };
    Q_ENUM(StopBits)

    // Numeric values match QSerialPort::FlowControl.
    enum FlowControl {
        NoFlowControl       = 0,
        HardwareControl     = 1,
        SoftwareControl     = 2,
    };
    Q_ENUM(FlowControl)

    // Common baud rates — numeric values are the wire baud rates themselves
    // (also match QSerialPort::BaudRate where they exist).
    enum BaudRate {
        Baud1200   = 1200,
        Baud2400   = 2400,
        Baud4800   = 4800,
        Baud9600   = 9600,
        Baud19200  = 19200,
        Baud38400  = 38400,
        Baud57600  = 57600,
        Baud115200 = 115200,
    };
    Q_ENUM(BaudRate)

    explicit QGCSerialPortAdapter(QObject *parent = nullptr);
    ~QGCSerialPortAdapter() override;

    // Test-only override: forces the adapter to use the QGCAndroidSerialPort backend
    // even on host builds. Set BEFORE constructing any QGCSerialPortAdapter instances.
    static void setForceAndroidBackendForTests(bool force);
    static bool forceAndroidBackendForTests();

    void setPortName(const QString &name);
    QString portName() const;

    bool open(QIODevice::OpenMode mode) override;
    void close() override;

    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;
    bool waitForReadyRead(int msecs) override;
    bool waitForBytesWritten(int msecs) override;
    bool flush();

    void setWriteBufferSize(qint64 size);
    Error error() const;
    void clearError();

    bool setBaudRate(qint32 baud);
    bool setDataBits(int bits);     // accepts DataBits enum or raw int
    bool setParity(int parity);     // accepts Parity enum or raw int
    bool setStopBits(int bits);     // accepts StopBits enum or raw int
    bool setFlowControl(int flow);  // accepts FlowControl enum or raw int
    // Batched setter — Android collapses 4 JNI hops into 1; host falls through
    // to the four QSerialPort setters (Qt has no batched API).
    bool setSerialParameters(qint32 baud, int dataBits, int stopBits, int parity);
    bool setDataTerminalReady(bool on);

signals:
    void errorOccurred(QGCSerialPortAdapter::Error error);

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 size) override;

private:
    // Strategy-pattern backend. Concrete types (HostSerialBackend / AndroidSerialBackend)
    // live entirely in the .cc — the adapter calls only the virtual interface, so
    // platform branching collapses to the single factory call in the constructor.
    // Pattern reference: Qt's QNetworkAccessBackend (qtbase/src/network/access/).
    std::unique_ptr<QGCSerialBackend> _backend;
};
