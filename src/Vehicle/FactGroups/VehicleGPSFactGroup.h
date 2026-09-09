#pragma once

#include "GPSPositionFactGroup.h"

class VehicleGPSFactGroup : public GPSPositionFactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact* systemErrors           READ systemErrors           CONSTANT)
    Q_PROPERTY(Fact* spoofingState          READ spoofingState          CONSTANT)
    Q_PROPERTY(Fact* jammingState           READ jammingState           CONSTANT)
    Q_PROPERTY(Fact* authenticationState    READ authenticationState    CONSTANT)
    Q_PROPERTY(Fact* correctionsQuality     READ correctionsQuality     CONSTANT)
    Q_PROPERTY(Fact* systemQuality          READ systemQuality          CONSTANT)
    Q_PROPERTY(Fact* gnssSignalQuality      READ gnssSignalQuality      CONSTANT)
    Q_PROPERTY(Fact* postProcessingQuality  READ postProcessingQuality  CONSTANT)

public:
    explicit VehicleGPSFactGroup(QObject *parent = nullptr);

    Fact* systemErrors() { return integrity()->systemErrors(); }

    Fact* spoofingState() { return integrity()->spoofingState(); }

    Fact* jammingState() { return integrity()->jammingState(); }

    Fact* authenticationState() { return integrity()->authenticationState(); }

    Fact* correctionsQuality() { return integrity()->correctionsQuality(); }

    Fact* systemQuality() { return integrity()->systemQuality(); }

    Fact* gnssSignalQuality() { return integrity()->gnssSignalQuality(); }

    Fact* postProcessingQuality() { return integrity()->postProcessingQuality(); }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

signals:
    void gnssIntegrityReceived();

protected:
    void _handleGpsRawInt(const mavlink_message_t &message);
    void _handleHighLatency(const mavlink_message_t &message);
    void _handleHighLatency2(const mavlink_message_t &message);
    void _handleGnssIntegrity(const mavlink_message_t& message);


    uint8_t _gnssIntegrityId {};
};
