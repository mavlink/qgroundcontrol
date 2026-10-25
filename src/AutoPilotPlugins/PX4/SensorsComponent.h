#pragma once

#include "VehicleComponent.h"

class Fact;

class SensorsComponent : public VehicleComponent
{
    Q_OBJECT

public:
    SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent = nullptr);

    Q_PROPERTY(bool airspeedCalSupported    READ _airspeedCalSupported  STORED false NOTIFY setupCompleteChanged)
    Q_PROPERTY(bool airspeedCalRequired     READ _airspeedCalRequired   STORED false NOTIFY setupCompleteChanged)

    // Virtuals from VehicleComponent
    QStringList setupCompleteChangedTriggerList(void) const override;

    // Virtuals from VehicleComponent
    virtual QString name(void) const override;
    virtual QString description(void) const override;
    QStringList searchKeywords() const override;
    virtual QString iconResource(void) const override;
    virtual bool requiresSetup(void) const override;
    virtual bool setupComplete(void) const override;
    virtual QUrl setupSource(void) const override;
    virtual QUrl summaryQmlSource(void) const override;

private:
    bool _airspeedCalSupported  (void) const;
    bool _airspeedCalRequired   (void) const;
    bool _airspeedVehicle(void) const;
    bool _airspeedLegacyFirmware(void) const;
    bool _airspeedParamExists(const char* name) const;
    Fact* _airspeedParam(const char* name) const;

    const QString   _name;
    QVariantList    _summaryItems;
    QStringList     _deviceIds;

    static constexpr const char* _airspeedCalParam = "SENS_DPRES_OFF";
    static constexpr const char* _airspeedPresentParam = "SYS_HAS_NUM_ASPD";
    static constexpr const char* _airspeedModeParam = "FW_ARSP_MODE";
    static constexpr const char* _airspeedCheckBreakerParam = "CBRK_AIRSPD_CHK";
    static constexpr int _airspeedCheckBreakerDisabled = 162128;
    static constexpr const char* _magEnabledParam = "SYS_HAS_MAG";
    static constexpr const char* _magCalParam = "CAL_MAG0_ID";
};
