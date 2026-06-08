#pragma once

// Platform-neutral enums shared by QGCSerialPort and its HostSerialPort / AndroidSerialPort impls.

#include <QtCore/QLatin1StringView>
#include <QtCore/QObject>
#include <QtCore/QtTypes>
#include <QtSerialPort/QSerialPort>
#include <cstdint>

// makeSerialPort() routing prefixes: "/dev/tty*" → kernel TTY via QSerialPort; else (Android) → AndroidSerialPort.
inline constexpr QLatin1StringView kDirectUartPathPrefix{"/dev/tty"};
inline constexpr QLatin1StringView kDevPrefix{"/dev/"};

// RX cap: HostSerialPort pauses at this fill (lossless); AndroidSerialPort drops over-cap (lossy — no pause).
inline constexpr qint64 kSerialRxBufferCapBytes = 512 * 1024;

// TX cap: write() returns 0 at fill; SerialLink blocks on waitForBytesWritten until a chunk drains.
inline constexpr qint64 kSerialWriteBufferCapBytes = 2 * 1024 * 1024;

namespace QGCSerial {
Q_NAMESPACE

enum class QGCSerialPortError
{
    NoError,
    NotOpen,
    OpenFailed,
    PermissionDenied,
    ResourceUnavailable,
    Read,
    Write,
    Timeout,
    UnsupportedOperation,
    Unknown,
};
Q_ENUM_NS(QGCSerialPortError)

// DataBits/StopBits are value-identical to QSerialPort's and cross the JNI wire by value; alias rather than
// duplicate. Parity is NOT aliased: QGCParity is contiguous (Odd=1,Even=2,Mark=3,Space=4) and its ordinals are
// the Android JNI wire values, whereas QSerialPort::Parity is gapped (Even=2,Odd=3,Space=4,Mark=5).
enum class QGCParity : uint8_t
{
    None = 0,
    Odd = 1,
    Even = 2,
    Mark = 3,
    Space = 4
};
Q_ENUM_NS(QGCParity)

enum class QGCFlowControl : uint8_t
{
    None = 0,
    HardwareRtsCts = 1,
    SoftwareXonXoff = 2,
    DtrDsr = 3,
    XonXoffInline = 4,
};
Q_ENUM_NS(QGCFlowControl)

}  // namespace QGCSerial

using QGCSerial::QGCFlowControl;
using QGCSerial::QGCParity;
using QGCSerial::QGCSerialPortError;

// 1:1 with QSerialPort; same underlying integers cross the Android JNI wire.
using QGCDataBits = QSerialPort::DataBits;
using QGCStopBits = QSerialPort::StopBits;

// Bundled wire params, carried by value so each impl applies the set atomically (one JNI hop on Android USB).
struct SerialPortConfig
{
    qint32 baud = 57600;
    QGCDataBits dataBits = QGCDataBits::Data8;
    QGCStopBits stopBits = QGCStopBits::OneStop;
    QGCParity parity = QGCParity::None;
    QGCFlowControl flowControl = QGCFlowControl::None;

    bool isValid() const noexcept
    {
        return (baud > 0) && (dataBits >= QGCDataBits::Data5) && (dataBits <= QGCDataBits::Data8) &&
               (stopBits == QGCStopBits::OneStop || stopBits == QGCStopBits::OneAndHalfStop ||
                stopBits == QGCStopBits::TwoStop) &&
               (parity <= QGCParity::Space) && (flowControl <= QGCFlowControl::XonXoffInline);
    }

    friend bool operator==(const SerialPortConfig&, const SerialPortConfig&) = default;
};
