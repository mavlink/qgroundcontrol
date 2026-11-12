#pragma once

#include <memory>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSReceiverWorker.h"

class SerialPortManager;

/// A serial device the receiver settings offer.
struct GPSSerialPortEntry
{
    Q_GADGET
    QML_VALUE_TYPE(gpsSerialPortEntry)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(QString value MEMBER value FINAL)
    Q_PROPERTY(QString label MEMBER label FINAL)

public:
    /// The system location, which the device setting stores.
    QString value;
    /// What the picker shows.
    QString label;

    bool operator==(const GPSSerialPortEntry&) const = default;
};

/// Holds a serial device for one receiver session until the last copy is released.
using GPSSerialClaim = std::shared_ptr<const void>;

/// The serial side of the receiver service: the devices a receiver may connect or discover, their claims, and the
/// transport of a serial session. This class is the whole serial side of a build without serial links, which has none:
/// it lists, finds and claims no device, so a serial selection connects over TCP and nothing is discovered.
/// GPSReceiverSystemSerialPorts is the serial side of every other build, and create() gives the build's.
class GPSReceiverSerialPorts : public QObject
{
    Q_OBJECT

public:
    explicit GPSReceiverSerialPorts(QObject* parent = nullptr);

    /// The build's serial side, over @a manager where the build has serial links.
    [[nodiscard]] static GPSReceiverSerialPorts* create(SerialPortManager* manager, QObject* parent);

    /// Receivers connect over serial devices here.
    virtual bool supported() const { return false; }

    /// The application's serial rates that a receiver accepts as an explicit rate.
    virtual QList<int> baudRates() const { return {}; }

    /// The serial device picker's entries, of the devices the last scan listed: "value" is the system location,
    /// "label" the device's description and path, marked in front when another QGroundControl connection, such as a
    /// vehicle link, holds the port. Where only one serial device can be open at a time, the other ports are marked
    /// too while such a connection holds one, since none of them can be opened until then.
    QList<GPSSerialPortEntry> entries() const { return _entries; }

    /// Scans the devices when a scan is due. A scan notifies, as scanned() describes.
    virtual void scanIfDue() {}

    /// Marks the entries as other connections hold ports now. Connections claim and release their ports without a
    /// scan, so the receiver asks for this as often as it polls. Notifies when a mark changed.
    virtual void refreshEntries() {}

    /// The device is plugged in and not in its bootloader.
    virtual bool available([[maybe_unused]] const QString& device) const { return false; }

    /// The device discovery may connect with the saved manufacturer at @a nowUs, microseconds of one monotonic clock:
    /// the first free device whose USB identity is a known RTK receiver adapter and which has been listed for as long
    /// as a newly plugged device needs before it is opened. Empty while there is none.
    virtual QString settledRtkReceiver([[maybe_unused]] quint64 nowUs) { return {}; }

    /// An RTK receiver that discovery would connect is listed and stays closed: only one serial device can be open at
    /// a time, and another QGroundControl connection, such as a vehicle link, has one open.
    virtual bool rtkReceiverKeptClosed() const { return false; }

    /// Forgets since when the RTK receivers have been listed, so each waits again from its next listing.
    virtual void forgetRtkReceivers() {}

    /// Claims the device until the last copy is released. Empty when it is in use, and where only one serial device
    /// can be open at a time, as with Android's USB serial driver, while any other is: held() tells the two apart.
    virtual GPSSerialClaim claim([[maybe_unused]] const QString& device) { return {}; }

    /// A connection holds the device: another one, or a session of this receiver whose worker has not stopped.
    virtual bool held([[maybe_unused]] const QString& device) const { return false; }

    /// Opens @a device for a session, which holds @a claim until its worker has stopped. Empty where no device opens.
    virtual GPSReceiverWorker::TransportFactory transportFactory([[maybe_unused]] const QString& device,
                                                                 [[maybe_unused]] GPSSerialClaim claim) const
    {
        return {};
    }

signals:
    /// A scan listed @a devices, by name and by path: a session on a device that is not among them was unplugged.
    /// Emitted before the entries follow the scan. Observers may connect, disconnect or change the configuration.
    void scanned(const QStringList& devices);
    void entriesChanged();

protected:
    void setEntries(QList<GPSSerialPortEntry> entries);

private:
    QList<GPSSerialPortEntry> _entries;
};
