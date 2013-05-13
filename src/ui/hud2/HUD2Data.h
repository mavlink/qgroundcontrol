#ifndef HUD2DATA_H
#define HUD2DATA_H

typedef enum {
    VALUE_IDX_AIRSPEED = 0,
    VALUE_IDX_ALT,
    VALUE_IDX_GROUNDSPEED,
    VALUE_IDX_PITCH,
    VALUE_IDX_ROLL,
    VALUE_IDX_YAW
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
//    int   fps; // repaints per second
};

#endif // HUD2DATA_H
