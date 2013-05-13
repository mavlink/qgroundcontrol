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
    float roll;
    float pitch;
    float yaw;
    float lat;
    float lon;
    float alt;
    float airspeed;
    float groundspeed;
    float batt_voltage;
    float batt_charge;
    int   batt_time;
    float thrust;
};

#endif // HUD2DATA_H
