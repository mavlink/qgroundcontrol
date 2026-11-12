#pragma once

#include <functional>
#include <optional>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "AndroidSerial.h"

/// The Java side of the USB serial ports, simulated. AndroidSerialFake.cc defines the AndroidSerial functions
/// that call Java, in place of AndroidSerial.cc, and they act as this state says. The registry of ports that
/// Java reports to is the real one.
struct AndroidSerialFake
{
    /// A name as Java gives a USB port, which the serial port therefore opens through Java.
    static inline const QString usbPortName = QStringLiteral("/dev/bus/usb/001/002");

    /// The state that every simulated call reads and counts in.
    static AndroidSerialFake& instance();
    /// Back to a Java side on which every call succeeds.
    static void reset();

    /// A port that Java has open. A test calls these to report input, written output and errors to C++ as
    /// Java's threads would.
    struct OpenPort
    {
        AndroidSerial::PortHandle handle = AndroidSerial::kNoPort;

        /// Returns what the port answers Java: whether it has room for more input.
        bool newDataArrived(const char* bytes, int length) const;
        void bytesWritten(int count) const;
        void resourceErrorArrived(AndroidSerial::ResourceErrorReason reason, const QString& detail = QString()) const;
    };

    /// The USB ports that Java lists.
    QList<QSerialPortInfo> ports;
    /// The port that Java opened last, until it is closed.
    std::optional<OpenPort> port;
    /// The name by which a port was last opened.
    QString openedLocation;
    /// The names of the ports whose device QGC may open.
    QStringList permittedPorts;
    /// The names of the ports whose device the user was to be asked for, in the order of the requests.
    QStringList permissionRequests;
    /// Whether Java finds an attached device to ask for.
    bool requestPermissionResult = true;

    AndroidSerial::Result openResult = AndroidSerial::Result::Ok;
    AndroidSerial::Result closeResult = AndroidSerial::Result::Ok;
    int fileDescriptor = 1;
    AndroidSerial::Result setParametersResult = AndroidSerial::Result::Ok;
    AndroidSerial::Result startReadingResult = AndroidSerial::Result::Ok;
    AndroidSerial::Result dtrResult = AndroidSerial::Result::Ok;
    AndroidSerial::Result rtsResult = AndroidSerial::Result::Ok;
    /// What Java's failure says of itself to a port that asks after a failed open, command or write.
    QString lastErrorDetail;
    /// What becomes of this many bytes handed over to write: how many of them Java reports written at once, or
    /// a negative AndroidSerial::Result if the hand-over fails. The rest stays with a device that takes no more,
    /// until a test reports it through the listener.
    std::function<int(int)> writeStep = [](int length) { return length; };
    /// Called whenever the port asks Java to start reading, on the port's thread.
    std::function<void()> startReadingHook;
    /// Called whenever the port asks Java to discard what the device has buffered, on the port's thread.
    std::function<void()> purgeBuffersHook;

    int openCalls = 0;
    int closeCalls = 0;
    int setParametersCalls = 0;
    int startReadingCalls = 0;
    int stopReadingCalls = 0;
    int writeCalls = 0;
};
