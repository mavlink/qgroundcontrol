#include "VehicleYachtHudFactGroup.h"
//#include "Vehicle.h"
class Vehicle;

const char* VehicleYachtHudFactGroup::_time_boot_msFactName = "time_boot_ms";
const char* VehicleYachtHudFactGroup::_rollFactName = "roll";
const char* VehicleYachtHudFactGroup::_pitchFactName = "pitch";
const char* VehicleYachtHudFactGroup::_headingFactName = "heading";
const char* VehicleYachtHudFactGroup::_heading_spFactName = "heading_sp";
const char* VehicleYachtHudFactGroup::_rtkYawFactName = "rtkYaw";
const char* VehicleYachtHudFactGroup::_magYawFactName = "magYaw";
const char* VehicleYachtHudFactGroup::_courseFactName = "course";
const char* VehicleYachtHudFactGroup::_speedFactName = "speed";
const char* VehicleYachtHudFactGroup::_vxFactName = "vx";
const char* VehicleYachtHudFactGroup::_vyFactName = "vy";
const char* VehicleYachtHudFactGroup::_vx_spFactName = "vx_sp";
const char* VehicleYachtHudFactGroup::_vy_spFactName = "vy_sp";
const char* VehicleYachtHudFactGroup::_yawspeedFactName = "yawspeed";
const char* VehicleYachtHudFactGroup::_yawspeed_spFactName = "yawspeed_sp";
const char* VehicleYachtHudFactGroup::_vin1FactName = "vin1";
const char* VehicleYachtHudFactGroup::_pout1FactName = "pout1";
const char* VehicleYachtHudFactGroup::_iout1FactName = "iout1";
const char* VehicleYachtHudFactGroup::_rpm1FactName = "rpm1";
const char* VehicleYachtHudFactGroup::_motortemp1FactName = "motortemp1";
const char* VehicleYachtHudFactGroup::_vin2FactName = "vin2";
const char* VehicleYachtHudFactGroup::_pout2FactName = "pout2";
const char* VehicleYachtHudFactGroup::_iout2FactName = "iout2";
const char* VehicleYachtHudFactGroup::_rpm2FactName = "rpm2";
const char* VehicleYachtHudFactGroup::_motortemp2FactName = "motortemp2";
const char* VehicleYachtHudFactGroup::_vin3FactName = "vin3";
const char* VehicleYachtHudFactGroup::_pout3FactName = "pout3";
const char* VehicleYachtHudFactGroup::_iout3FactName = "iout3";
const char* VehicleYachtHudFactGroup::_rpm3FactName = "rpm3";
const char* VehicleYachtHudFactGroup::_motortemp3FactName = "motortemp3";
const char* VehicleYachtHudFactGroup::_vin4FactName = "vin4";
const char* VehicleYachtHudFactGroup::_pout4FactName = "pout4";
const char* VehicleYachtHudFactGroup::_iout4FactName = "iout4";
const char* VehicleYachtHudFactGroup::_rpm4FactName = "rpm4";
const char* VehicleYachtHudFactGroup::_motortemp4FactName = "motortemp4";
const char* VehicleYachtHudFactGroup::_vin5FactName = "vin5";
const char* VehicleYachtHudFactGroup::_pout5FactName = "pout5";
const char* VehicleYachtHudFactGroup::_iout5FactName = "iout5";
const char* VehicleYachtHudFactGroup::_rpm5FactName = "rpm5";
const char* VehicleYachtHudFactGroup::_motortemp5FactName = "motortemp5";
const char* VehicleYachtHudFactGroup::_vin6FactName = "vin6";
const char* VehicleYachtHudFactGroup::_pout6FactName = "pout6";
const char* VehicleYachtHudFactGroup::_iout6FactName = "iout6";
const char* VehicleYachtHudFactGroup::_rpm6FactName = "rpm6";
const char* VehicleYachtHudFactGroup::_motortemp6FactName = "motortemp6";
const char* VehicleYachtHudFactGroup::_angleLeftFactName = "angleLeft";
const char* VehicleYachtHudFactGroup::_angleRightFactName = "angleRight";
const char* VehicleYachtHudFactGroup::_angleLeftSPFactName = "angleLeftSP";
const char* VehicleYachtHudFactGroup::_angleRightSPFactName = "angleRightSP";
const char* VehicleYachtHudFactGroup::_imuTempFactName = "imuTemp";

VehicleYachtHudFactGroup::VehicleYachtHudFactGroup(QObject *parent)
    : FactGroup{100, ":/json/YachtHudFact.json", parent}
      , _time_boot_msFact (0, _time_boot_msFactName, FactMetaData::valueTypeUint32)
      , _rollFact(0,  _rollFactName, FactMetaData::valueTypeFloat)
      , _pitchFact(0,  _pitchFactName, FactMetaData::valueTypeFloat)
      , _headingFact(0,  _headingFactName, FactMetaData::valueTypeFloat)
      , _heading_spFact(0,  _heading_spFactName, FactMetaData::valueTypeFloat)
      , _rtkYawFact(0,  _rtkYawFactName, FactMetaData::valueTypeFloat)
      , _magYawFact(0,  _magYawFactName, FactMetaData::valueTypeFloat)
      , _courseFact(0,  _courseFactName, FactMetaData::valueTypeFloat)
      , _speedFact(0,  _speedFactName, FactMetaData::valueTypeFloat)
      , _vxFact(0,  _vxFactName, FactMetaData::valueTypeFloat)
      , _vyFact(0,  _vyFactName, FactMetaData::valueTypeFloat)
      , _vx_spFact(0,  _vx_spFactName, FactMetaData::valueTypeFloat)
      , _vy_spFact(0,  _vy_spFactName, FactMetaData::valueTypeFloat)
      , _yawspeedFact(0,  _yawspeedFactName, FactMetaData::valueTypeFloat)
      , _yawspeed_spFact(0,  _yawspeed_spFactName, FactMetaData::valueTypeFloat)
      , _vin1Fact(0,  _vin1FactName, FactMetaData::valueTypeFloat)
      , _pout1Fact(0,  _pout1FactName, FactMetaData::valueTypeFloat)
      , _iout1Fact(0,  _iout1FactName, FactMetaData::valueTypeFloat)
      , _rpm1Fact(0,  _rpm1FactName, FactMetaData::valueTypeFloat)
      , _motortemp1Fact(0,  _motortemp1FactName, FactMetaData::valueTypeFloat)
      , _vin2Fact(0,  _vin2FactName, FactMetaData::valueTypeFloat)
      , _pout2Fact(0,  _pout2FactName, FactMetaData::valueTypeFloat)
      , _iout2Fact(0,  _iout2FactName, FactMetaData::valueTypeFloat)
      , _rpm2Fact(0,  _rpm2FactName, FactMetaData::valueTypeFloat)
      , _motortemp2Fact(0,  _motortemp2FactName, FactMetaData::valueTypeFloat)
      , _vin3Fact(0,  _vin3FactName, FactMetaData::valueTypeFloat)
      , _pout3Fact(0,  _pout3FactName, FactMetaData::valueTypeFloat)
      , _iout3Fact(0,  _iout3FactName, FactMetaData::valueTypeFloat)
      , _rpm3Fact(0,  _rpm3FactName, FactMetaData::valueTypeFloat)
      , _motortemp3Fact(0,  _motortemp3FactName, FactMetaData::valueTypeFloat)
      , _vin4Fact(0,  _vin4FactName, FactMetaData::valueTypeFloat)
      , _pout4Fact(0,  _pout4FactName, FactMetaData::valueTypeFloat)
      , _iout4Fact(0,  _iout4FactName, FactMetaData::valueTypeFloat)
      , _rpm4Fact(0,  _rpm4FactName, FactMetaData::valueTypeFloat)
      , _motortemp4Fact(0,  _motortemp4FactName, FactMetaData::valueTypeFloat)
      , _vin5Fact(0,  _vin5FactName, FactMetaData::valueTypeFloat)
      , _pout5Fact(0,  _pout5FactName, FactMetaData::valueTypeFloat)
      , _iout5Fact(0,  _iout5FactName, FactMetaData::valueTypeFloat)
      , _rpm5Fact(0,  _rpm5FactName, FactMetaData::valueTypeFloat)
      , _motortemp5Fact(0,  _motortemp5FactName, FactMetaData::valueTypeFloat)
      , _vin6Fact(0,  _vin6FactName, FactMetaData::valueTypeFloat)
      , _pout6Fact(0,  _pout6FactName, FactMetaData::valueTypeFloat)
      , _iout6Fact(0,  _iout6FactName, FactMetaData::valueTypeFloat)
      , _rpm6Fact(0,  _rpm6FactName, FactMetaData::valueTypeFloat)
      , _motortemp6Fact(0,  _motortemp6FactName, FactMetaData::valueTypeFloat)
      , _angleLeftFact(0,  _angleLeftFactName, FactMetaData::valueTypeFloat)
      , _angleRightFact(0,  _angleRightFactName, FactMetaData::valueTypeFloat)
      , _angleLeftSPFact(0,  _angleLeftSPFactName, FactMetaData::valueTypeFloat)
      , _angleRightSPFact(0,  _angleRightSPFactName, FactMetaData::valueTypeFloat)
      , _imuTempFact(0,  _imuTempFactName, FactMetaData::valueTypeFloat)
{
    _addFact(&_time_boot_msFact,  _time_boot_msFactName);
    _addFact(&_rollFact,  _rollFactName);
    _addFact(&_pitchFact,  _pitchFactName);
    _addFact(&_headingFact,  _headingFactName);
    _addFact(&_heading_spFact,  _heading_spFactName);
    _addFact(&_rtkYawFact,  _rtkYawFactName);
    _addFact(&_magYawFact,  _magYawFactName);
    _addFact(&_courseFact,  _courseFactName);
    _addFact(&_speedFact,  _speedFactName);
    _addFact(&_vxFact,  _vxFactName);
    _addFact(&_vyFact,  _vyFactName);
    _addFact(&_vx_spFact,  _vx_spFactName);
    _addFact(&_vy_spFact,  _vy_spFactName);
    _addFact(&_yawspeedFact,  _yawspeedFactName);
    _addFact(&_yawspeed_spFact,  _yawspeed_spFactName);
    _addFact(&_vin1Fact,  _vin1FactName);
    _addFact(&_pout1Fact,  _pout1FactName);
    _addFact(&_iout1Fact,  _iout1FactName);
    _addFact(&_rpm1Fact,  _rpm1FactName);
    _addFact(&_motortemp1Fact,  _motortemp1FactName);
    _addFact(&_vin2Fact,  _vin2FactName);
    _addFact(&_pout2Fact,  _pout2FactName);
    _addFact(&_iout2Fact,  _iout2FactName);
    _addFact(&_rpm2Fact,  _rpm2FactName);
    _addFact(&_motortemp2Fact,  _motortemp2FactName);
    _addFact(&_vin3Fact,  _vin3FactName);
    _addFact(&_pout3Fact,  _pout3FactName);
    _addFact(&_iout3Fact,  _iout3FactName);
    _addFact(&_rpm3Fact,  _rpm3FactName);
    _addFact(&_motortemp3Fact,  _motortemp3FactName);
    _addFact(&_vin4Fact,  _vin4FactName);
    _addFact(&_pout4Fact,  _pout4FactName);
    _addFact(&_iout4Fact,  _iout4FactName);
    _addFact(&_rpm4Fact,  _rpm4FactName);
    _addFact(&_motortemp4Fact,  _motortemp4FactName);
    _addFact(&_vin5Fact,  _vin5FactName);
    _addFact(&_pout5Fact,  _pout5FactName);
    _addFact(&_iout5Fact,  _iout5FactName);
    _addFact(&_rpm5Fact,  _rpm5FactName);
    _addFact(&_motortemp5Fact,  _motortemp5FactName);
    _addFact(&_vin6Fact,  _vin6FactName);
    _addFact(&_pout6Fact,  _pout6FactName);
    _addFact(&_iout6Fact,  _iout6FactName);
    _addFact(&_rpm6Fact,  _rpm6FactName);
    _addFact(&_motortemp6Fact,  _motortemp6FactName);
    _addFact(&_angleLeftFact,  _angleLeftFactName);
    _addFact(&_angleRightFact,  _angleRightFactName);
    _addFact(&_angleLeftSPFact,  _angleLeftSPFactName);
    _addFact(&_angleRightSPFact,  _angleRightSPFactName);
    _addFact(&_imuTempFact,  _imuTempFactName);
}

void VehicleYachtHudFactGroup::handleMessage(Vehicle* vehicle, const mavlink_message_t& message){
    if(message.msgid != MAVLINK_MSG_ID_YACHT_HUD)
        return;
    mavlink_yacht_hud_t hud_msg;
    mavlink_msg_yacht_hud_decode(&message, &hud_msg);

    _time_boot_msFact.setRawValue(hud_msg.time_boot_ms);
    _rollFact        .setRawValue(hud_msg.roll        / 100.f);
    _pitchFact       .setRawValue(hud_msg.pitch       / 100.f);
    _headingFact     .setRawValue(hud_msg.heading     / 100.f);
    _heading_spFact  .setRawValue(hud_msg.heading_sp  / 100.f);
    _rtkYawFact      .setRawValue(hud_msg.RTKYaw      / 100.f);
    _magYawFact      .setRawValue(hud_msg.magYaw      / 100.f);
    _courseFact      .setRawValue(hud_msg.course      / 100.f);
    _speedFact       .setRawValue(hud_msg.speed       / 100.f);
    _vxFact          .setRawValue(hud_msg.vx          / 100.f);
    _vyFact          .setRawValue(hud_msg.vy          / 100.f);
    _vx_spFact       .setRawValue(hud_msg.vx_sp       / 100.f);
    _vy_spFact       .setRawValue(hud_msg.vy_sp       / 100.f);
    _yawspeedFact    .setRawValue(hud_msg.yawspeed    / 100.f);
    _yawspeed_spFact .setRawValue(hud_msg.yawspeed_sp / 100.f);
    _vin1Fact        .setRawValue(hud_msg.vin1        );
    _pout1Fact       .setRawValue(hud_msg.pout1       / 100.f);
    _iout1Fact       .setRawValue(hud_msg.iout1       / 10.f);
    _rpm1Fact        .setRawValue(hud_msg.rpm1        );
    _motortemp1Fact  .setRawValue(hud_msg.motortemp1  / 100.f);
    _vin2Fact        .setRawValue(hud_msg.vin2        );
    _pout2Fact       .setRawValue(hud_msg.pout2       / 100.f);
    _iout2Fact       .setRawValue(hud_msg.iout2       / 10.f);
    _rpm2Fact        .setRawValue(hud_msg.rpm2        );
    _motortemp2Fact  .setRawValue(hud_msg.motortemp2  / 100.f);
    _vin3Fact        .setRawValue(hud_msg.vin3        );
    _pout3Fact       .setRawValue(hud_msg.pout3       / 100.f);
    _iout3Fact       .setRawValue(hud_msg.iout3       / 10.f);
    _rpm3Fact        .setRawValue(hud_msg.rpm3        );
    _motortemp3Fact  .setRawValue(hud_msg.motortemp3  / 100.f);
    _vin4Fact        .setRawValue(hud_msg.vin4        );
    _pout4Fact       .setRawValue(hud_msg.pout4       / 100.f);
    _iout4Fact       .setRawValue(hud_msg.iout4       / 10.f);
    _rpm4Fact        .setRawValue(hud_msg.rpm4        );
    _motortemp4Fact  .setRawValue(hud_msg.motortemp4  / 100.f);
    _vin5Fact        .setRawValue(hud_msg.vin5        );
    _pout5Fact       .setRawValue(hud_msg.pout5       / 100.f);
    _iout5Fact       .setRawValue(hud_msg.iout5       / 10.f);
    _rpm5Fact        .setRawValue(hud_msg.rpm5        );
    _motortemp5Fact  .setRawValue(hud_msg.motortemp5  / 100.f);
    _vin6Fact        .setRawValue(hud_msg.vin6        );
    _pout6Fact       .setRawValue(hud_msg.pout6       / 100.f);
    _iout6Fact       .setRawValue(hud_msg.iout6       / 10.f);
    _rpm6Fact        .setRawValue(hud_msg.rpm6        );
    _motortemp6Fact  .setRawValue(hud_msg.motortemp6  / 100.f);
    _angleLeftFact   .setRawValue(hud_msg.angleLeft   * 0.00549316406f);
    _angleRightFact  .setRawValue(hud_msg.angleRight  * 0.00549316406f);
    _angleLeftSPFact .setRawValue(hud_msg.angleLeftSP * 0.00549316406f);
    _angleRightSPFact.setRawValue(hud_msg.angleRightSP* 0.00549316406f);
    _imuTempFact     .setRawValue(hud_msg.imuTemp     / 100.f);
}
