#pragma once

#include <QtCore/QObject>

#include <array>

#include "GPSIntegrityStore.h"
#include "VehicleGPSObservation.h"
#include "development/mavlink_msg_gnss_integrity.h"

/// One MAVLink ingestion path per vehicle, shared by Facts and position consumers.
class VehicleGPSObservationStream : public QObject
{
    Q_OBJECT
    friend class NTRIPGgaProviderTest;

public:
    explicit VehicleGPSObservationStream(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~VehicleGPSObservationStream() override;
    void handleMessage(const mavlink_message_t& message, int systemId = 0, int defaultComponentId = 0);
    void reset();
    VehicleGPSObservation gps(int receiver = 0) const;

    GPSObservation fusedPosition() const { return _fused; }

    GPSIntegrityStore* integrity(int receiver);

signals:
    void gpsReceived(int receiver, const VehicleGPSObservation& observation);
    void fusedPositionReceived(const GPSObservation& observation);
    void integrityReceived(int receiver);

private:
    void _publish(int receiver, const VehicleGPSObservation& observation);
    void _integrityReceived(const mavlink_gnss_integrity_t& message);

    QPointer<RuntimeScheduler> _scheduler;
    std::array<VehicleGPSObservation, 2> _gps;
    GPSObservation _fused;
    GPSIntegrityStore _integrity1;
    GPSIntegrityStore _integrity2;
    quint64 _revision = 0;
};
