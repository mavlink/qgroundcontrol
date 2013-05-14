#ifndef HUD2DATA_H
#define HUD2DATA_H

typedef enum {
    VALUE_IDX_AIRSPEED = 0,
    VALUE_IDX_ALT = 1,
    VALUE_IDX_GROUNDSPEED = 2,
    VALUE_IDX_PITCH = 3,
    VALUE_IDX_ROLL = 4,
    VALUE_IDX_YAW = 5
}value_idx_enum;

class HUD2Data
{
public:
    HUD2Data();
    double roll;
    double pitch;
    double yaw;
    double alt;
    double climb;
    double airspeed;
    double groundspeed;
    double batt_voltage;
    double batt_charge;
    int    batt_time;
    double thrust;
};

#endif // HUD2DATA_H
