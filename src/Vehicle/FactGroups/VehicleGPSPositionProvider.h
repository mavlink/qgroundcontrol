#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "GPSObservation.h"

class Vehicle;

/// Independent original message receipts for the selected vehicle's raw GPS and fused position.
class VehicleGPSPositionProvider : public QObject
{
    Q_OBJECT
    friend class NTRIPGgaProviderTest;

public:
    explicit VehicleGPSPositionProvider(QObject* parent = nullptr);
    ~VehicleGPSPositionProvider() override;
    void setVehicle(Vehicle* vehicle);
    GPSObservation gpsPosition() const;
    GPSObservation ekfPosition() const;

private:
    QPointer<Vehicle> _vehicle;
};
