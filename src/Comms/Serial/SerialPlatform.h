#pragma once

// Link-time platform seam for serial-port construction; one TU per platform (QGCSerialPortInfo_host/android.cc) so
// callers stay #ifdef-free. Port-enumeration glue lives on QGCSerialPortInfo itself.

#include <QtCore/QObject>
#include <functional>

QT_FORWARD_DECLARE_CLASS(QIODevice)
QT_FORWARD_DECLARE_CLASS(QObject)
class QGCSerialPort;

namespace SerialPlatform {

// Construct the concrete serial port for a location (host: HostSerialPort; Android: USB-host → AndroidSerialPort, "/dev/tty*" → HostSerialPort). Per-platform TU so host never includes AndroidSerialPort.h.
QGCSerialPort* makeSerialPort(const QString& name, QObject* parent);

// Test-only seam: when set, makeSerialPort() returns this factory's port instead of the platform impl,
// letting SerialLink/SerialWorker run against an in-memory MockSerialPort with no hardware. Pass {} to restore.
using SerialPortFactory = std::function<QGCSerialPort*(const QString& name, QObject* parent)>;
void setPortFactoryForTest(SerialPortFactory factory);
const SerialPortFactory& portFactoryOverride();

// QIODevice feeding QNmeaPositionInfoSource for a serial NMEA GPS; host returns the port directly (GUI thread), Android a worker-threaded adapter (its USB backend can't open on the GUI thread).
QIODevice* makeNmeaSerialSource(const QString& systemLocation, qint32 baud, QObject* parent);

// Singleton relay that fires devicesChanged() when the attached serial-device set changes (Android USB attach/permission-grant; host never emits).
// Connect with Qt::QueuedConnection — the receiver's lifetime governs delivery and Qt auto-disconnects on destruction, so no manual teardown.
class SerialDevicesNotifier : public QObject
{
    Q_OBJECT

public:
    static SerialDevicesNotifier* instance()
    {
        static SerialDevicesNotifier s_instance;
        return &s_instance;
    }

    void notifyChanged() { emit devicesChanged(); }

signals:
    void devicesChanged();

private:
    SerialDevicesNotifier() = default;
};

}  // namespace SerialPlatform
