#ifndef JOYSTICKSDL_H
#define JOYSTICKSDL_H

#include "Joystick.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"

#ifdef Q_OS_MAC
    #include <SDL.h>
#else
    #include <SDL/SDL.h>
#endif


class JoystickSDL : public Joystick
{
public:
    JoystickSDL(const QString& name, int axisCount, int buttonCount, int hatCount, int index, MultiVehicleManager* multiVehicleManager);

    static QMap<QString, Joystick*> discover(MultiVehicleManager* _multiVehicleManager); 

private:
    bool _open() final;
    void _close() final;
    bool _update() final;

    bool _getButton(int i) final;
    int _getAxis(int i) final;
    uint8_t _getHat(int hat,int i) final;

    SDL_Joystick *sdlJoystick;
    int     _index;      ///< Index for SDL_JoystickOpen
};

#endif // JOYSTICKSDL_H
