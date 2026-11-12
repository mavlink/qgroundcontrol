#pragma once

#include <QtCore/QString>

/// What the app decides about the serial ports of the Android device: which of them
/// QSerialPortInfo::availablePorts() lists, and when the user is asked again for a USB permission. Defined with
/// that list, in qserialportinfo_android.cpp.
namespace AndroidSerialPorts {
// Whether the list has, after the USB ports, the serial device nodes too: the /dev/tty* that some devices, such
// as rooted ones, let an app open (POSIX). The app sets it once at startup, from its androidUsePosixSerial
// setting. How a port opens depends on its name alone.
void setUsePosixSerial(bool use);
bool usePosixSerial();
/// Whether the device has a serial port node that QGC may read and write.
bool hasPosixSerialPorts();
/// Whether the port is a USB one whose device QGC may open.
bool hasUsbPermission(const QString& portName);
/// Asks the user to let QGC open the USB device of this port, and returns whether it did. It does not for a port
/// whose device QGC may open already or is not attached, nor for one that is not a USB port. The user answers
/// later, and opening the port fails until then. Opening the port asks once for each time the device is
/// attached; this asks whenever it is called, also after a denial, so call it only for a connection that the
/// user has asked for.
bool requestUsbPermission(const QString& portName);
}  // namespace AndroidSerialPorts
