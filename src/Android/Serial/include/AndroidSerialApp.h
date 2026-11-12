#pragma once

#include <QtCore/QObject>

/// What the app itself uses of AndroidSerial, the USB serial ports of the Android device as Java provides them:
/// it binds Java's callbacks as the library loads, tests that binding at boot, and hears when the devices change.
/// AndroidSerialJni.h declares the rest of the namespace, which is private to the serial port library.
namespace AndroidSerial {
/// Tells when the ports QGC may open may have changed: a device was attached or detached, or the user granted
/// or denied permission. An Android thread emits the signal, so connect a receiver object to it, whose thread
/// then runs the slot. The watcher itself belongs to the application's thread.
class DeviceWatcher : public QObject
{
    Q_OBJECT

public:
    static DeviceWatcher* instance();

signals:
    void devicesChanged();

private:
    DeviceWatcher();
};

/// Registers the callbacks that Java calls on its threads, and finds the Java methods that C++ calls: call it
/// as the library loads, before anything else of the namespace. Returns false, after a warning, if Java does
/// not declare the callbacks as they are registered here: a callback would then throw on its Java thread.
[[nodiscard]] bool setNativeMethods();
/// Proves, for QGC's boot test, that C++ and Java reach each other in the app as it is packaged: Java has every
/// method and field that C++ binds, passes arguments that C++ gave it to each callback, and answers calls about
/// a port that does not exist. Returns false after it has logged what failed, as critical messages of the
/// category Android.AndroidSerial.SelfTest.
[[nodiscard]] bool selfTest();
}  // namespace AndroidSerial
