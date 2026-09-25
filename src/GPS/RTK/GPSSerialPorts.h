#pragma once

#include <memory>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

/// The serial inventory and exclusive port claims a receiver session needs. The application adapts its serial
/// port manager, so this library does not depend on the application's serial links. Called on the GUI thread.
class GPSSerialPorts : public QObject
{
    Q_OBJECT

public:
    struct Port
    {
        QString systemLocation;
        QString boardName;
        QString description;
        /// Shared by interfaces of one physical device; empty when identity is unavailable.
        QString physicalDeviceId;
        /// The USB identity is a known RTK receiver.
        bool rtkReceiver = false;
        bool bootloader = false;
    };

    /// Holds the port claim until the last copy is released.
    using Reservation = std::shared_ptr<const void>;

    using QObject::QObject;

    /// May rescan and emit portsEnumerated() before returning.
    virtual QList<Port> ports() = 0;
    virtual bool canReserve(const QString& systemLocation) const = 0;
    /// Empty when the port is already claimed.
    [[nodiscard]] virtual Reservation reserve(const QString& systemLocation) = 0;

signals:
    /// Emitted after a fresh scan, with the native names and system locations present.
    void portsEnumerated(const QStringList& availablePorts);
};
