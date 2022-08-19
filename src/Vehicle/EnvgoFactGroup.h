#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"

class EnvgoFactGroup : public FactGroup
{
    Q_OBJECT

public:
    EnvgoFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* curTemperature         READ curTemperature         CONSTANT)
    Q_PROPERTY(Fact* motorTemperature       READ motorTemperature       CONSTANT)
    Q_PROPERTY(Fact* motorCtrlTemperature   READ motorCtrlTemperature   CONSTANT)
    Q_PROPERTY(Fact* batteryTemperature     READ batteryTemperature     CONSTANT)
    Q_PROPERTY(Fact* servoTemperature       READ servoTemperature       CONSTANT)
    
    // Q_PROPERTY(Fact* simultaneousSpeed      READ simultaneousSpeed      CONSTANT)
    Q_PROPERTY(Fact* averageSpeed           READ averageSpeed           CONSTANT)
    
    Q_PROPERTY(Fact* travelDirection        READ travelDirection        CONSTANT)
    Q_PROPERTY(Fact* distanceTravelled      READ distanceTravelled      CONSTANT)
    Q_PROPERTY(Fact* remainBattery          READ remainBattery          CONSTANT)
    Q_PROPERTY(Fact* timeTravelled          READ timeTravelled          CONSTANT)
    Q_PROPERTY(Fact* heightAboveWater       READ heightAboveWater       CONSTANT)
    Q_PROPERTY(Fact* flightMode             READ flightMode             CONSTANT)
    Q_PROPERTY(Fact* gear                   READ gear                   CONSTANT)
    

    Fact* curTemperature        () { return &_curTemperatureFact; }
    Fact* motorTemperature      () { return &_motorTemperatureFact; }
    Fact* motorCtrlTemperature  () { return &_motorCtrlTemperatureFact; }
    Fact* batteryTemperature    () { return &_batteryTemperatureFact; }
    Fact* servoTemperature      () { return &_servoTemperatureFact; }
    
    // Fact* simultaneousSpeed     () { return &_simultaneousSpeedFact; }
    Fact* averageSpeed          () { return &_averageSpeedFact; }
    
    Fact* travelDirection       () { return &_travelDirectionFact; }
    Fact* distanceTravelled     () { return &_distanceTravelledFact; }
    Fact* remainBattery         () { return &_remainBatteryFact; }
    Fact* timeTravelled         () { return &_timeTravelledFact; }
    Fact* heightAboveWater      () { return &_heightAboveWaterFact; }
    Fact* flightMode            () { return &_flightModeFact; }
    Fact* gear                  () { return &_gearFact; }

    // Overrides from FactGroup
    // void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static const char* _curTemperatureFactName;
    static const char* _motorTemperatureFactName;
    static const char* _motorCtrlTemperatureFactName;
    static const char* _batteryTemperatureFactName;
    static const char* _servoTemperatureFactName;

    // static const char* _simultaneousSpeedFactName;
    static const char* _averageSpeedFactName;
    
    static const char* _travelDirectionFactName;
    static const char* _distanceTravelledFactName;
    static const char* _remainBatteryFactName;
    static const char* _timeTravelledFactName;
    static const char* _heightAboveWaterFactName;
    static const char* _flightModeFactName;
    static const char* _gearFactName;
    
private:
    Fact            _curTemperatureFact;
    Fact            _motorTemperatureFact;
    Fact            _motorCtrlTemperatureFact;
    Fact            _batteryTemperatureFact;
    Fact            _servoTemperatureFact;
    
    // Fact            _simultaneousSpeedFact;
    Fact            _averageSpeedFact;
    
    Fact            _travelDirectionFact;
    Fact            _distanceTravelledFact;
    Fact            _remainBatteryFact;
    Fact            _timeTravelledFact;
    Fact            _heightAboveWaterFact;
    Fact            _flightModeFact;
    Fact            _gearFact;
};
