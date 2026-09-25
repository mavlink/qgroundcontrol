#pragma once

#include <QtCore/QPointer>

#include "GPSSerialPorts.h"

class SerialPortManager;

/// Serves the RTK receiver from the application's serial port manager, sharing its reservations.
class GPSSerialPortManagerAdapter : public GPSSerialPorts
{
    Q_OBJECT

public:
    explicit GPSSerialPortManagerAdapter(SerialPortManager* manager, QObject* parent = nullptr);

    QList<Port> ports() override;
    bool canReserve(const QString& systemLocation) const override;
    Reservation reserve(const QString& systemLocation) override;

private:
    QPointer<SerialPortManager> _manager;
};
