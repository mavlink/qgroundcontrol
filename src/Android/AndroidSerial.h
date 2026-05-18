#pragma once

#include <QtCore/QList>
#include <QtCore/QString>

class QGCSerialPortInfo;

namespace AndroidSerial {

// Wire-format values matching upstream usb-serial-for-android. DataBits and StopBits
// are intentionally absent: their numeric values match QSerialPort::DataBits /
// QSerialPort::StopBits exactly, so callers static_cast directly.
enum Parity      { NoParity = 0, OddParity, EvenParity, MarkParity, SpaceParity };
enum ControlLine { RtsControlLine = 0, CtsControlLine, DtrControlLine, DsrControlLine, CdControlLine, RiControlLine };
enum FlowControl {
    NoFlowControl = 0, RtsCtsFlowControl, DtrDsrFlowControl, XonXoffFlowControl, XonXoffInlineFlowControl
};

// Mirrors SerialConstants.EXC_* in Java. Drives QSerialPort::SerialPortError mapping
// in qserialport_android.cpp::exceptionNotification — keep in sync with Java side.
enum class JavaExceptionKind : int {
    Unknown    = 0,
    Resource   = 1,
    Permission = 2,
    OpenFailed = 3,
};

constexpr qint64 MAX_READ_SIZE = 16 * 1024;
constexpr int INVALID_DEVICE_ID = 0;

struct SerialParameters {
    qint32 baudRate;
    int dataBits;
    int stopBits;
    int parity;
};

void initialize();
QList<QGCSerialPortInfo> availableDevices();

}  // namespace AndroidSerial
