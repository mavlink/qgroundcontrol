#pragma once

#include <functional>
#include <memory>

#include <QtCore/QHash>
#include <QtCore/QPointer>

#include "GPSReceiverSerialPorts.h"
#include "SerialPortManager.h"

/// The receiver's serial side in a build with serial links: the application's serial devices and their claims. A
/// session holds its claim until its worker has stopped.
class GPSReceiverSystemSerialPorts : public GPSReceiverSerialPorts
{
    Q_OBJECT

    friend class GPSReceiverTest;

public:
    /// @a manager lists and claims the devices; without it none is available.
    explicit GPSReceiverSystemSerialPorts(SerialPortManager* manager, QObject* parent = nullptr);

    bool supported() const override { return true; }

    QList<int> baudRates() const override;
    void scanIfDue() override;
    void refreshEntries() override;
    bool available(const QString& device) const override;
    QString settledRtkReceiver(quint64 nowUs) override;

    void forgetRtkReceivers() override { _rtkReceivers.clear(); }

    GPSSerialClaim claim(const QString& device) override;
    GPSReceiverWorker::TransportFactory transportFactory(const QString& device, GPSSerialClaim claim) const override;

private:
    /// Free devices whose USB identity is a known RTK receiver adapter, one per physical device.
    QStringList _freeRtkReceivers() const;

    const QPointer<SerialPortManager> _manager;
    /// The devices of the last scan, which the entries are made of.
    QList<SerialPortManager::Port> _scannedPorts;
    /// The claims made here, which never mark their port as in use, even while a retired session's worker still holds
    /// one.
    QHash<QString, std::weak_ptr<const SerialPortManager::Reservation>> _claims;
    /// Since when each free RTK receiver has been listed.
    SerialPortSettleTracker _rtkReceivers;
    /// Opens a serial device; tests open scripted transports instead.
    std::function<std::unique_ptr<GPSTransport>(const QString&, GPSCancelToken)> _openDevice;
};
