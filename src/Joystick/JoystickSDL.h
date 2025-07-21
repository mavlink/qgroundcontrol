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

struct SDL_Joystick;
typedef struct SDL_Joystick SDL_Joystick;

struct SDL_Gamepad;
typedef struct SDL_Gamepad SDL_Gamepad;

Q_DECLARE_LOGGING_CATEGORY(JoystickSDLLog)

class JoystickSDL : public Joystick
{
public:
    explicit JoystickSDL(const QString &name, int axisCount, int buttonCount, int hatCount, int instanceId, bool isGamepad, QObject *parent = nullptr);
    ~JoystickSDL() override;

    int instanceId() const { return _instanceId; }
    void setInstanceId(int instanceId) { _instanceId = instanceId; }

    // bool requiresCalibration() const final { return !_isGamepad; }

    static bool init();
    static QMap<QString, Joystick*> discover();

private:
    bool _open() final;
    void _close() final;
    bool _update() final;

    bool _getButton(int idx) const final;
    int _getAxis(int idx) const final;
    bool _getHat(int hat, int idx) const final;

    static void _loadGamepadMappings();

    bool _isGamepad = false;
    int _instanceId = -1;

    SDL_Joystick *_sdlJoystick = nullptr;
    SDL_Gamepad *_sdlGamepad = nullptr;
};
