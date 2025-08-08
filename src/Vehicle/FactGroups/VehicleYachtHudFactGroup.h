#ifndef VEHICLEYACHTHUDFACTGROUP_H
#define VEHICLEYACHTHUDFACTGROUP_H
#pragma once

#include "FactSystem/FactGroup.h"
#include "MAVLink/QGCMAVLink.h"

class VehicleYachtHudFactGroup : public FactGroup
{
    Q_OBJECT
   public:
    VehicleYachtHudFactGroup(QObject *parent = nullptr);

    Q_PROPERTY(Fact* time_boot_ms READ time_boot_ms CONSTANT)
    Q_PROPERTY(Fact* roll         READ roll         CONSTANT)
    Q_PROPERTY(Fact* pitch        READ pitch        CONSTANT)
    Q_PROPERTY(Fact* heading      READ heading      CONSTANT)
    Q_PROPERTY(Fact* heading_sp   READ heading_sp   CONSTANT)
    Q_PROPERTY(Fact* rtkYaw       READ rtkYaw       CONSTANT)
    Q_PROPERTY(Fact* magYaw       READ magYaw       CONSTANT)
    Q_PROPERTY(Fact* course       READ course       CONSTANT)
    Q_PROPERTY(Fact* speed        READ speed        CONSTANT)
    Q_PROPERTY(Fact* vx           READ vx           CONSTANT)
    Q_PROPERTY(Fact* vy           READ vy           CONSTANT)
    Q_PROPERTY(Fact* vx_sp        READ vx_sp        CONSTANT)
    Q_PROPERTY(Fact* vy_sp        READ vy_sp        CONSTANT)
    Q_PROPERTY(Fact* yawspeed     READ yawspeed     CONSTANT)
    Q_PROPERTY(Fact* yawspeed_sp  READ yawspeed_sp  CONSTANT)
    Q_PROPERTY(Fact* vin1         READ vin1         CONSTANT)
    Q_PROPERTY(Fact* pout1        READ pout1        CONSTANT)
    Q_PROPERTY(Fact* iout1        READ iout1        CONSTANT)
    Q_PROPERTY(Fact* rpm1         READ rpm1         CONSTANT)
    Q_PROPERTY(Fact* motortemp1   READ motortemp1   CONSTANT)
    Q_PROPERTY(Fact* vin2         READ vin2         CONSTANT)
    Q_PROPERTY(Fact* pout2        READ pout2        CONSTANT)
    Q_PROPERTY(Fact* iout2        READ iout2        CONSTANT)
    Q_PROPERTY(Fact* rpm2         READ rpm2         CONSTANT)
    Q_PROPERTY(Fact* motortemp2   READ motortemp2   CONSTANT)
    Q_PROPERTY(Fact* vin3         READ vin3         CONSTANT)
    Q_PROPERTY(Fact* pout3        READ pout3        CONSTANT)
    Q_PROPERTY(Fact* iout3        READ iout3        CONSTANT)
    Q_PROPERTY(Fact* rpm3         READ rpm3         CONSTANT)
    Q_PROPERTY(Fact* motortemp3   READ motortemp3   CONSTANT)
    Q_PROPERTY(Fact* vin4         READ vin4         CONSTANT)
    Q_PROPERTY(Fact* pout4        READ pout4        CONSTANT)
    Q_PROPERTY(Fact* iout4        READ iout4        CONSTANT)
    Q_PROPERTY(Fact* rpm4         READ rpm4         CONSTANT)
    Q_PROPERTY(Fact* motortemp4   READ motortemp4   CONSTANT)
    Q_PROPERTY(Fact* vin5         READ vin5         CONSTANT)
    Q_PROPERTY(Fact* pout5        READ pout5        CONSTANT)
    Q_PROPERTY(Fact* iout5        READ iout5        CONSTANT)
    Q_PROPERTY(Fact* rpm5         READ rpm5         CONSTANT)
    Q_PROPERTY(Fact* motortemp5   READ motortemp5   CONSTANT)
    Q_PROPERTY(Fact* vin6         READ vin6         CONSTANT)
    Q_PROPERTY(Fact* pout6        READ pout6        CONSTANT)
    Q_PROPERTY(Fact* iout6        READ iout6        CONSTANT)
    Q_PROPERTY(Fact* rpm6         READ rpm6         CONSTANT)
    Q_PROPERTY(Fact* motortemp6   READ motortemp6   CONSTANT)
    Q_PROPERTY(Fact* angleLeft    READ angleLeft    CONSTANT)
    Q_PROPERTY(Fact* angleRight   READ angleRight   CONSTANT)
    Q_PROPERTY(Fact* angleLeftSP  READ angleLeftSP  CONSTANT)
    Q_PROPERTY(Fact* angleRightSP READ angleRightSP CONSTANT)
    Q_PROPERTY(Fact* imuTemp      READ imuTemp      CONSTANT)

    Fact* time_boot_ms () {return &_time_boot_msFact;}
    Fact* roll         () {return &_rollFact;}
    Fact* pitch        () {return &_pitchFact;}
    Fact* heading      () {return &_headingFact;}
    Fact* heading_sp   () {return &_heading_spFact;}
    Fact* rtkYaw       () {return &_rtkYawFact;}
    Fact* magYaw       () {return &_magYawFact;}
    Fact* course       () {return &_courseFact;}
    Fact* speed        () {return &_speedFact;}
    Fact* vx           () {return &_vxFact;}
    Fact* vy           () {return &_vyFact;}
    Fact* vx_sp        () {return &_vx_spFact;}
    Fact* vy_sp        () {return &_vy_spFact;}
    Fact* yawspeed     () {return &_yawspeedFact;}
    Fact* yawspeed_sp  () {return &_yawspeed_spFact;}
    Fact* vin1         () {return &_vin1Fact;}
    Fact* pout1        () {return &_pout1Fact;}
    Fact* iout1        () {return &_iout1Fact;}
    Fact* rpm1         () {return &_rpm1Fact;}
    Fact* motortemp1   () {return &_motortemp1Fact;}
    Fact* vin2         () {return &_vin2Fact;}
    Fact* pout2        () {return &_pout2Fact;}
    Fact* iout2        () {return &_iout2Fact;}
    Fact* rpm2         () {return &_rpm2Fact;}
    Fact* motortemp2   () {return &_motortemp2Fact;}
    Fact* vin3         () {return &_vin3Fact;}
    Fact* pout3        () {return &_pout3Fact;}
    Fact* iout3        () {return &_iout3Fact;}
    Fact* rpm3         () {return &_rpm3Fact;}
    Fact* motortemp3   () {return &_motortemp3Fact;}
    Fact* vin4         () {return &_vin4Fact;}
    Fact* pout4        () {return &_pout4Fact;}
    Fact* iout4        () {return &_iout4Fact;}
    Fact* rpm4         () {return &_rpm4Fact;}
    Fact* motortemp4   () {return &_motortemp4Fact;}
    Fact* vin5         () {return &_vin5Fact;}
    Fact* pout5        () {return &_pout5Fact;}
    Fact* iout5        () {return &_iout5Fact;}
    Fact* rpm5         () {return &_rpm5Fact;}
    Fact* motortemp5   () {return &_motortemp5Fact;}
    Fact* vin6         () {return &_vin6Fact;}
    Fact* pout6        () {return &_pout6Fact;}
    Fact* iout6        () {return &_iout6Fact;}
    Fact* rpm6         () {return &_rpm6Fact;}
    Fact* motortemp6   () {return &_motortemp6Fact;}
    Fact* angleLeft    () {return &_angleLeftFact;}
    Fact* angleRight   () {return &_angleRightFact;}
    Fact* angleLeftSP  () {return &_angleLeftSPFact;}
    Fact* angleRightSP () {return &_angleRightSPFact;}
    Fact* imuTemp      () {return &_imuTempFact;}


            // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, const mavlink_message_t& message) override;

    static const char* _time_boot_msFactName;
    static const char* _rollFactName;
    static const char* _pitchFactName;
    static const char* _headingFactName;
    static const char* _heading_spFactName;
    static const char* _rtkYawFactName;
    static const char* _magYawFactName;
    static const char* _courseFactName;
    static const char* _speedFactName;
    static const char* _vxFactName;
    static const char* _vyFactName;
    static const char* _vx_spFactName;
    static const char* _vy_spFactName;
    static const char* _yawspeedFactName;
    static const char* _yawspeed_spFactName;
    static const char* _vin1FactName;
    static const char* _pout1FactName;
    static const char* _iout1FactName;
    static const char* _rpm1FactName;
    static const char* _motortemp1FactName;
    static const char* _vin2FactName;
    static const char* _pout2FactName;
    static const char* _iout2FactName;
    static const char* _rpm2FactName;
    static const char* _motortemp2FactName;
    static const char* _vin3FactName;
    static const char* _pout3FactName;
    static const char* _iout3FactName;
    static const char* _rpm3FactName;
    static const char* _motortemp3FactName;
    static const char* _vin4FactName;
    static const char* _pout4FactName;
    static const char* _iout4FactName;
    static const char* _rpm4FactName;
    static const char* _motortemp4FactName;
    static const char* _vin5FactName;
    static const char* _pout5FactName;
    static const char* _iout5FactName;
    static const char* _rpm5FactName;
    static const char* _motortemp5FactName;
    static const char* _vin6FactName;
    static const char* _pout6FactName;
    static const char* _iout6FactName;
    static const char* _rpm6FactName;
    static const char* _motortemp6FactName;
    static const char* _angleLeftFactName;
    static const char* _angleRightFactName;
    static const char* _angleLeftSPFactName;
    static const char* _angleRightSPFactName;
    static const char* _imuTempFactName;

   private:
    Fact _time_boot_msFact;
    Fact _rollFact;
    Fact _pitchFact;
    Fact _headingFact;
    Fact _heading_spFact;
    Fact _rtkYawFact;
    Fact _magYawFact;
    Fact _courseFact;
    Fact _speedFact;
    Fact _vxFact;
    Fact _vyFact;
    Fact _vx_spFact;
    Fact _vy_spFact;
    Fact _yawspeedFact;
    Fact _yawspeed_spFact;
    Fact _vin1Fact;
    Fact _pout1Fact;
    Fact _iout1Fact;
    Fact _rpm1Fact;
    Fact _motortemp1Fact;
    Fact _vin2Fact;
    Fact _pout2Fact;
    Fact _iout2Fact;
    Fact _rpm2Fact;
    Fact _motortemp2Fact;
    Fact _vin3Fact;
    Fact _pout3Fact;
    Fact _iout3Fact;
    Fact _rpm3Fact;
    Fact _motortemp3Fact;
    Fact _vin4Fact;
    Fact _pout4Fact;
    Fact _iout4Fact;
    Fact _rpm4Fact;
    Fact _motortemp4Fact;
    Fact _vin5Fact;
    Fact _pout5Fact;
    Fact _iout5Fact;
    Fact _rpm5Fact;
    Fact _motortemp5Fact;
    Fact _vin6Fact;
    Fact _pout6Fact;
    Fact _iout6Fact;
    Fact _rpm6Fact;
    Fact _motortemp6Fact;
    Fact _angleLeftFact;
    Fact _angleRightFact;
    Fact _angleLeftSPFact;
    Fact _angleRightSPFact;
    Fact _imuTempFact;
};
#endif  // VEHICLEYACHTHUDFACTGROUP_H
