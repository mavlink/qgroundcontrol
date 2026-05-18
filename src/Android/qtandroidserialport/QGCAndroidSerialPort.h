// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

//
// QGCAndroidSerialPort
//
// Sibling QIODevice that replaces QGC's QSerialPortPrivate override on Android.
//
// Background. Until task fix/android-serial-overhaul, QGC shipped a custom
// QSerialPortPrivate that shadowed Qt's private API (`<QtCore/private/qiodevice_p.h>`,
// `Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS`, etc.) and forced an Android-only divergence
// in SerialLink.h via `#ifdef Q_OS_ANDROID #include "qserialport.h"`. That design:
//   (1) silently breaks on every Qt minor bump that touches QSerialPort internals,
//   (2) blocks host-side testing — Qt's QtSerialPort module already provides
//       QSerialPort + private impl on host, so linking our copy collides,
//   (3) forces every consumer to think in QSerialPort::Foo enums even on Android
//       where the underlying engine is JNI-backed and has no termios semantics.
//
// This class replaces that pattern. It is a regular QIODevice subclass owned by
// SerialLink (via QGCSerialPortFactory), with its own writeBuffer / readBuffer /
// error state / configuration getters. Zero dependency on Qt-private headers.
//
// Construction lifecycle:
//   1. SerialLink (or its worker) instantiates QGCAndroidSerialPort.
//   2. Caller wires Q_PROPERTY-equivalent setters (setBaudRate, setDataBits, ...)
//      while state == Closed. Setters cache to local fields; no engine call yet.
//   3. Caller invokes open(QIODevice::ReadWrite). open() constructs the engine
//      via QGCAndroidSerialPortFactory::createEngine() (default: QAndroidSerialEngine;
//      tests inject MockQSerialPortEngine), pushes cached config to the engine, and
//      starts the read thread.
//   4. Engine callbacks (dataReady / closeNotification / exceptionNotification)
//      are received via the inherited QAndroidSerialEngineReceiver interface and
//      marshaled to the owner thread before reaching this object.
//   5. close() emits aboutToClose() *first* (matches QIODevice contract — fixes
//      the inversion noted by qt-qml-expert finding #4 in fix/android-serial-overhaul),
//      then tears down the engine, then emits readChannelFinished().
//
// Migration plan (multi-session):
//   - Session 1: this header (API contract).
//   - Session 2: implementation that wraps existing QAndroidSerialEngine + reuses
//                the writeBuffer drain / _expectClosure / re-entrancy guard logic
//                lifted verbatim from QSerialPortPrivate.
//   - Session 3: QGCSerialPortFactory + SerialLink polymorphism (replace the
//                `#ifdef Q_OS_ANDROID` in SerialLink.h with runtime dispatch).
//   - Session 4: migrate QGCSerialPortInfo Android path to enumerate this type.
//   - Session 5: host-side QGCAndroidSerialPortTest exercising the production
//                logic via injected MockQSerialPortEngine (task #1, was task #2).
//   - Session 6+: delete src/Android/qtandroidserialport/qserialport*.* and the
//                `#ifdef Q_OS_ANDROID` branches in consumers.
//

#include <QtCore/QByteArray>
#include <QtCore/QChronoTimer>
#include <QtCore/QIODevice>
#include <QtCore/QString>

#include <functional>
#include <memory>

#include "AndroidSerial.h"
#include "iqserialportengine_p.h"
#include "qandroidserialenginereceiver_p.h"

QT_FORWARD_DECLARE_CLASS(QObject)

// Error classification. Intentionally NOT QSerialPort::SerialPortError — we don't want
// to leak QtSerialPort enums into a class that has no termios concept. Translation to
// QSerialPort errors (if SerialLink chooses to expose them) happens at the boundary.
enum class QGCSerialPortError {
    NoError,
    NotOpen,
    OpenFailed,
    PermissionDenied,    // USB permission denied or device in use
    ResourceUnavailable, // Device disappeared (cable yanked, runner stopped)
    Read,
    Write,
    Timeout,
    UnsupportedOperation,
    Unknown,
};

// Data bits / parity / stop bits / flow control — own enums, not QSerialPort::*.
// Values match standard wire encodings so engine impls don't need translation tables.
enum class QGCDataBits : uint8_t { Data5 = 5, Data6 = 6, Data7 = 7, Data8 = 8 };
enum class QGCParity   : uint8_t { None = 0, Odd = 1, Even = 2, Mark = 3, Space = 4 };
enum class QGCStopBits : uint8_t { One = 1, OneAndHalf = 3, Two = 2 };

enum class QGCFlowControl : uint8_t {
    None         = 0,
    HardwareRtsCts = 1,
    SoftwareXonXoff = 2,
};

// Pinout signal bitmask. Matches QSerialPort::PinoutSignals values exactly so the
// SerialLink-layer translation is a static_cast, not a switch.
struct QGCPinoutSignals
{
    enum Signal : quint32 {
        None  = 0x000,
        TxD   = 0x001,
        RxD   = 0x002,
        DTR   = 0x004,
        DSR   = 0x008,
        DCD   = 0x010,
        RNG   = 0x020,
        RTS   = 0x040,
        CTS   = 0x080,
        SecTxD = 0x100,
        SecRxD = 0x200,
    };
};

// Process-wide engine factory. Defaults to QAndroidSerialEngine. Tests can swap to
// MockQSerialPortEngine via setEngineFactory(). Mirrors QSerialPortPrivate::EngineFactory
// from the legacy implementation — kept here so the public-facing class owns it.
class QGCAndroidSerialPortFactory
{
public:
    using EngineFactory = std::function<std::unique_ptr<IQSerialPortEngine>(
        QAndroidSerialEngineReceiver*, QObject*)>;

    // Setter is process-wide and NOT thread-safe. Call from test setup before
    // constructing any QGCAndroidSerialPort instance.
    static void setEngineFactory(EngineFactory factory);
    static void resetEngineFactory();  // Restore default (QAndroidSerialEngine).

private:
    friend class QGCAndroidSerialPort;
    static EngineFactory& _engineFactory();
};

// QIODevice replacement for QSerialPort on Android. No Qt-private dependencies.
class QGCAndroidSerialPort : public QIODevice, private QAndroidSerialEngineReceiver
{
    Q_OBJECT

public:
    explicit QGCAndroidSerialPort(QObject* parent = nullptr);
    explicit QGCAndroidSerialPort(const QString& systemLocation, QObject* parent = nullptr);
    ~QGCAndroidSerialPort() override;

    // --- QIODevice ---
    bool isSequential() const override { return true; }
    bool open(QIODeviceBase::OpenMode mode) override;
    void close() override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;
    bool canReadLine() const override;
    bool waitForReadyRead(int msecs) override;
    bool waitForBytesWritten(int msecs) override;

    // --- Port identification ---
    QString systemLocation() const { return _systemLocation; }
    void setSystemLocation(const QString& location);
    QString portName() const;  // Display name (last path component).

    // --- Configuration (callable while open or closed) ---
    // Closed: cached for the next open(). Open: applied immediately, returns false
    // on engine rejection.
    bool setBaudRate(qint32 baudRate);
    qint32 baudRate() const { return _baudRate; }

    bool setDataBits(QGCDataBits dataBits);
    QGCDataBits dataBits() const { return _dataBits; }

    bool setParity(QGCParity parity);
    QGCParity parity() const { return _parity; }

    bool setStopBits(QGCStopBits stopBits);
    QGCStopBits stopBits() const { return _stopBits; }

    // Batched parameter setter — collapses what would otherwise be four
    // sequential setSerialParameters JNI calls (one per setter) into one.
    // Emits only the *Changed signals whose values actually moved.
    bool setSerialParameters(qint32 baudRate, QGCDataBits dataBits,
                             QGCStopBits stopBits, QGCParity parity);

    bool setFlowControl(QGCFlowControl flowControl);
    QGCFlowControl flowControl() const { return _flowControl; }

    // --- Control lines (open only) ---
    bool setDataTerminalReady(bool set);
    bool setRequestToSend(bool set);
    bool isDataTerminalReady();
    bool isRequestToSend();
    quint32 pinoutSignals();  // Bitmask of QGCPinoutSignals::Signal values.

    bool setBreakEnabled(bool set);
    bool isBreakEnabled() const { return _breakEnabled; }
    bool sendBreak(int duration);

    bool flush();
    bool clear(bool input = true, bool output = true);

    // --- Error reporting ---
    QGCSerialPortError error() const { return _error; }
    void clearError();

    // --- Buffer sizing ---
    qint64 readBufferSize() const { return _readBufferMaxSize; }
    void setReadBufferSize(qint64 size);
    qint64 writeBufferSize() const { return _writeBufferMaxSize; }
    void setWriteBufferSize(qint64 size);

signals:
    void errorOccurred(QGCSerialPortError error);
    void baudRateChanged(qint32 baudRate);
    void dataBitsChanged(QGCDataBits dataBits);
    void parityChanged(QGCParity parity);
    void stopBitsChanged(QGCStopBits stopBits);
    void flowControlChanged(QGCFlowControl flowControl);
    void breakEnabledChanged(bool set);
    void dataTerminalReadyChanged(bool set);
    void requestToSendChanged(bool set);

protected:
    // --- QIODevice ---
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 readLineData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

    // --- QAndroidSerialEngineReceiver ---
    // Engine has already marshaled these to the owner thread before invoking.
    void dataReady(QByteArray&& bytes) override;
    void closeNotification() override;
    void exceptionNotification(AndroidSerial::JavaExceptionKind kind, const QString& message) override;

private:
    void _setError(QGCSerialPortError errorCode, const QString& errorString = {});
    bool _startAsyncRead();
    bool _stopAsyncRead();
    void _drainWriteBuffer();
    qint64 _writeToEngine(const char* data, qint64 maxSize, int timeout, bool async);

    // Engine instance. Created in open(), reset in close(). Typed as the interface
    // so the factory can return MockQSerialPortEngine for tests.
    std::unique_ptr<IQSerialPortEngine> _engine;

    // Owner-thread write-buffer drain. Created in open(), reset in close().
    // QChronoTimer is thread-affine; start() from a foreign thread is not safe.
    std::unique_ptr<QChronoTimer> _writeTimer;

    // Internal buffers (no QIODevicePrivate dependency).
    //
    // _readOffset is QIODevicePrivate's trick (see qiodevice_p.h): consume from the
    // front by bumping the offset, only memmove-compact when the dead prefix is large.
    // Avoids the O(n) QByteArray::remove(0, n) on every readData() at high baud.
    QByteArray _readBuffer;
    qsizetype  _readOffset = 0;
    QByteArray _writeBuffer;
    qint64 _readBufferMaxSize = 0;
    qint64 _writeBufferMaxSize = 0;

    // Cached configuration. Pushed to engine in open(); also applied via setters
    // while open.
    QString _systemLocation;
    qint32 _baudRate = 57600;
    QGCDataBits _dataBits = QGCDataBits::Data8;
    QGCParity _parity = QGCParity::None;
    QGCStopBits _stopBits = QGCStopBits::One;
    QGCFlowControl _flowControl = QGCFlowControl::None;
    bool _breakEnabled = false;

    // Error state (mirrors QSerialPort::SerialPortError pattern but native enum).
    QGCSerialPortError _error = QGCSerialPortError::NoError;

    // Re-entrancy guards. Mirrors Qt upstream qserialport_unix.cpp pattern: a slot
    // connected to readyRead() may call write() which schedules a drain; without
    // these flags a slot connected to bytesWritten() that calls readAll() (or
    // vice versa) can recurse. Reset in close() so reopen doesn't swallow the
    // first emit.
    bool _emittedReadyRead = false;
    bool _emittedBytesWritten = false;

    // Pre-close flag. Mirrors Qt BluetoothSocket InputStreamThread::expectClosure:
    // set before tearing down _engine so the racing Java IOException from the
    // forced close is reported as expected, not as ResourceUnavailable.
    bool _expectClosure = false;
};
