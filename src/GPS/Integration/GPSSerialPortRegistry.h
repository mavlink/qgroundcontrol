#pragma once
#include <QtCore/QPointer>

#include "GPSSerialDiscovery.h"
#include "SerialPortManager.h"

class GPSSerialPortRegistry final : public GPSSerialDiscovery
{
public:
    explicit GPSSerialPortRegistry(SerialPortManager* ports, QObject* parent = nullptr);
    ~GPSSerialPortRegistry() override;
    QList<Port> availablePorts() override;
    ReservationPtr reservePort(const QString& device) override;
    ReservationPtr excludeFromAutoConnect(const QString& device) override;
    bool canReservePort(const QString& device) const override;
    bool canAutoConnectPort(const QString& device) const override;
    bool isAutoConnectExcluded(const QString& device) const override;

private:
    QPointer<SerialPortManager> _ports;
};
