/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief SDL Joystick Interface

#pragma once

#include "Joystick.h"

#define SDL_MAIN_HANDLED

#include <SDL.h>

class MultiVehicleManager;

/// @brief SDL Joystick Interface
class JoystickSDL : public Joystick
{
public:
    JoystickSDL(const QString& name, int axisCount, int buttonCount, int hatCount, int index, bool isGameController);
    ~JoystickSDL();

    static QMap<QString, Joystick*> discover();
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

    bool _getButton (int i) const final;
    int  _getAxis   (int i) const final;
    bool _getHat    (int hat,int i) const final;

    SDL_Joystick*       sdlJoystick;
    SDL_GameController* sdlController;

    bool    _isGameController;
    int     _index;      ///< Index for SDL_JoystickOpen

};
