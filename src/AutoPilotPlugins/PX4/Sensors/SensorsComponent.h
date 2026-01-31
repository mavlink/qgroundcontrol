#pragma once

#include "SensorsComponentBase.h"

class SensorsComponent : public SensorsComponentBase
{
    Q_OBJECT

public:
    SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    Q_PROPERTY(bool airspeedCalSupported    READ _airspeedCalSupported  STORED false NOTIFY setupCompleteChanged)
    Q_PROPERTY(bool airspeedCalRequired     READ _airspeedCalRequired   STORED false NOTIFY setupCompleteChanged)

    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList() const override;
    bool setupComplete() const override;
    QUrl setupSource() const override;
    QUrl summaryQmlSource() const override;

private:
    bool _airspeedCalSupported() const;
    bool _airspeedCalRequired() const;

    QVariantList    _summaryItems;
    QStringList     _deviceIds;
    QStringList     _airspeedCalTriggerParams;

    static constexpr const char* _airspeedBreakerParam = "CBRK_AIRSPD_CHK";
    static constexpr const char* _airspeedDisabledParam = "FW_ARSP_MODE";
    static constexpr const char* _airspeedCalParam = "SENS_DPRES_OFF";
    static constexpr const char* _magEnabledParam = "SYS_HAS_MAG";
    static constexpr const char* _magCalParam = "CAL_MAG0_ID";
};
