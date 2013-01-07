#ifndef HUD2DATA_H
#define HUD2DATA_H

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

    int   fps; // repaints per second
};

#endif // HUD2DATA_H
