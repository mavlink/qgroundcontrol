/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief SDL Joystick Interface

#pragma once

#include "Joystick.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"

#include <SDL.h>

/// @brief SDL Joystick Interface
class JoystickSDL : public Joystick
{
public:
    JoystickSDL(const QString& name, int axisCount, int buttonCount, int hatCount, int index, bool isGameController, MultiVehicleManager* multiVehicleManager);

    static QMap<QString, Joystick*> discover(MultiVehicleManager* _multiVehicleManager); 
    static bool init(void);

    int index(void) const { return _index; }
    void setIndex(int index) { _index = index; }

    // This can be uncommented to hide the calibration buttons for gamecontrollers in the future
    // bool requiresCalibration(void) final { return !_isGameController; }

private:
    static void _loadGameControllerMappings();

    bool _open      () final;
    void _close     () final;
    bool _update    () final;

    bool _getButton (int i) final;
    int  _getAxis   (int i) final;
    bool _getHat    (int hat,int i) final;

    SDL_Joystick*       sdlJoystick;
    SDL_GameController* sdlController;

    bool    _isGameController;
    int     _index;      ///< Index for SDL_JoystickOpen

};
