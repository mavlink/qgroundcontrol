/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>

#include "Joystick.h"

struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;

struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;

Q_DECLARE_LOGGING_CATEGORY(JoystickSDLLog)

class JoystickSDL : public Joystick
{
public:
    JoystickSDL(const QString &name, int axisCount, int buttonCount, int hatCount, int index, bool isGameController, QObject *parent = nullptr);
    ~JoystickSDL();

    int index() const { return _index; }
    void setIndex(int index) { _index = index; }

    // bool requiresCalibration() final { return !_isGameController; }

    static bool init();
    static QMap<QString, Joystick*> discover();

private:
    bool _open() final;
    void _close() final;
    bool _update() final;

    bool _getButton(int i) const final;
    int  _getAxis(int i) const final;
    bool _getHat(int hat, int i) const final;

    static void _loadGameControllerMappings();

    bool _isGameController = false;
    int _index = -1;

    SDL_Joystick *_sdlJoystick = nullptr;
    SDL_GameController *_sdlController = nullptr;
};
