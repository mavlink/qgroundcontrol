/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickSDL.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QTextStream>

#include <SDL3/SDL.h>

QGC_LOGGING_CATEGORY(JoystickSDLLog, "Joystick.joysticksdl")

JoystickSDL::JoystickSDL(const QString &name, QList<int> gamepadAxes, QList<int> nonGamepadAxes, int buttonCount, int hatCount, int instanceId, bool isGamepad, QObject *parent)
    : Joystick(name, gamepadAxes.length() + nonGamepadAxes.length(), buttonCount, hatCount, parent)
    , _gamepadAxes(gamepadAxes)
    , _nonGamepadAxes(nonGamepadAxes)
    , _isGamepad(isGamepad)
    , _instanceId(instanceId)
{
    qCDebug(JoystickSDLLog) << this;

    if (_isGamepad) {
        _setDefaultCalibration();
    }
}

JoystickSDL::~JoystickSDL()
{
    qCDebug(JoystickSDLLog) << this;
}

bool JoystickSDL::init()
{
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
        SDL_SetJoystickEventsEnabled(false);
        qCWarning(JoystickSDLLog) << "Failed to initialize SDL:" << SDL_GetError();
        return false;
    }

    _loadGamepadMappings();
    return true;
}

QMap<QString, Joystick*> JoystickSDL::discover()
{
    static QMap<QString, Joystick*> previous;
    QMap<QString, Joystick*> current;

    qCDebug(JoystickSDLLog) << "Discovering joysticks";

    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    if (!ids) {
        qCWarning(JoystickSDLLog) << "SDL_GetJoysticks failed:" << SDL_GetError();
        return current;
    }

    for (int n = 0; n < count; ++n) {
        const SDL_JoystickID jid = ids[n];
        QString name = QString::fromUtf8(SDL_GetJoystickNameForID(jid));

        if (previous.contains(name)) {
            current[name] = previous[name];
            JoystickSDL *js = static_cast<JoystickSDL*>(current[name]);
            if (js->instanceId() != jid) {
                js->setInstanceId(jid);
            }
            (void) previous.remove(name);
            continue;
        }

        QList<int> gamepadAxes;
        QSet<int> joyAxesMappedToGamepad;

        if (SDL_IsGamepad(jid)) {
            auto tmpGamepad = SDL_OpenGamepad(jid);
            if (!tmpGamepad) {
                qCWarning(JoystickSDLLog) << "Failed to open gamepad" << jid << SDL_GetError();
                continue;
            }

            // Determine if this gamepad axis is one we should show to the user
            for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++) {
                if (SDL_GamepadHasAxis(tmpGamepad, static_cast<SDL_GamepadAxis>(i))) {
                    gamepadAxes.append(i);
                }
            }

            // If a sdlJoystick axis is mapped to a sdlGamepad axis, then the axis is represented
            // by both the sdlJoystick interface and the sdlGamepad interface. If this is the case,
            // We'll only show the sdlGamepad interface version of the axis to the user.
            int count = 0;
            SDL_GamepadBinding **bindings = SDL_GetGamepadBindings(tmpGamepad, &count);
            if (bindings) {
                for (int i = 0; i < count; ++i) {
                    SDL_GamepadBinding *binding = bindings[i];
                    if (binding && binding->input_type == SDL_GAMEPAD_BINDTYPE_AXIS && binding->output_type == SDL_GAMEPAD_BINDTYPE_AXIS) {
                        joyAxesMappedToGamepad.insert(binding->input.axis.axis);
                    }
                }
                SDL_free(bindings);
            } else {
                qCWarning(JoystickSDLLog) << "Failed to get bindings for" << name << "error:" << SDL_GetError();
            }

            SDL_CloseGamepad(tmpGamepad);
        }

        SDL_Joystick *tmpJoy = SDL_OpenJoystick(jid);
        if (!tmpJoy) {
            qCWarning(JoystickSDLLog) << "Failed to open joystick" << jid << SDL_GetError();
            continue;
        }

        QList<int> nonGamepadAxes;
        const int axisCount = SDL_GetNumJoystickAxes(tmpJoy);
        for (int i = 0; i < axisCount; i++) {
            if (!joyAxesMappedToGamepad.contains(i)) {
                nonGamepadAxes.append(i);
            }
        }

        const int buttonCount = SDL_GetNumJoystickButtons(tmpJoy);
        const int hatCount = SDL_GetNumJoystickHats(tmpJoy);
        SDL_CloseJoystick(tmpJoy);

        const bool isGamepad = SDL_IsGamepad(jid);

        const QString baseName = name;
        quint8 dupIdx = 1;
        while (current.contains(name)) {
            name = QString("%1 %2").arg(baseName).arg(dupIdx++);
        }

        current[name] = new JoystickSDL(name,
                                        gamepadAxes,
                                        nonGamepadAxes,
                                        qMax(0, buttonCount),
                                        qMax(0, hatCount),
                                        jid,
                                        isGamepad);
    }

    SDL_free(ids);
    previous = current;
    return current;
}

bool JoystickSDL::_open()
{
    if (_isGamepad) {
        _sdlGamepad = SDL_OpenGamepad(_instanceId);
        if (!_sdlGamepad) {
            qCWarning(JoystickSDLLog) << "SDL_OpenGamepad failed:" << SDL_GetError();
            return false;
        }
        _sdlJoystick = SDL_GetGamepadJoystick(_sdlGamepad);
    } else {
        _sdlJoystick = SDL_OpenJoystick(_instanceId);
    }

    if (!_sdlJoystick) {
        qCWarning(JoystickSDLLog) << "SDL_JoystickOpen failed:" << SDL_GetError();
        return false;
    }

    qCDebug(JoystickSDLLog) << "Opened" << SDL_GetJoystickName(_sdlJoystick) << "joystick at" << _sdlJoystick;

    return true;
}

void JoystickSDL::_close()
{
    if (!_sdlJoystick) {
        qCWarning(JoystickSDLLog) << "Attempt to close null joystick!";
        return;
    }

    qCDebug(JoystickSDLLog) << "Closing" << SDL_GetJoystickName(_sdlJoystick) << "joystick at" << _sdlJoystick;

    if (_isGamepad) {
        SDL_CloseGamepad(_sdlGamepad);
    } else {
        SDL_CloseJoystick(_sdlJoystick);
    }

    _sdlJoystick = nullptr;
    _sdlGamepad = nullptr;
}

bool JoystickSDL::_update()
{
    if (_isGamepad) {
        SDL_UpdateGamepads();
    } else {
        SDL_UpdateJoysticks();
    }

    return true;
}

// dev
bool JoystickSDL::_getButton(int idx) const
{
    if (idx < 0) {
        return false;
    }

    // 1) Standardized gamepad buttons (SDL3)
    if (_sdlGamepad) {
        if (idx < SDL_GAMEPAD_BUTTON_COUNT) {
            return SDL_GetGamepadButton(
                       _sdlGamepad,
                       static_cast<SDL_GamepadButton>(idx)
                   ) != 0;
        }
    }

    if (!_sdlJoystick) {
        return false;
    }

    const int rawCount = SDL_GetNumJoystickButtons(_sdlJoystick);
    if (idx < rawCount) {
        return SDL_GetJoystickButton(_sdlJoystick, idx) != 0;
    }

    return false;
}
// dev end

int JoystickSDL::_getAxis(int idx) const
{
    int axis = -1;

    if (_isGamepad) {
        if (idx < _gamepadAxes.length()) {
            axis = SDL_GetGamepadAxis(_sdlGamepad, static_cast<SDL_GamepadAxis>(_gamepadAxes[idx]));
        }
        else {
            axis = SDL_GetJoystickAxis(_sdlJoystick, _nonGamepadAxes[idx - _gamepadAxes.length()]);
        }
    } else {
        axis = SDL_GetJoystickAxis(_sdlJoystick, idx);
    }

    return axis;
}

bool JoystickSDL::_getHat(int hat, int idx) const
{
    static constexpr uint8_t hatButtons[] = {SDL_HAT_UP, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHT};

    if (idx >= std::size(hatButtons)) {
        return false;
    }

    return ((SDL_GetJoystickHat(_sdlJoystick, hat) & hatButtons[idx]) != 0);
}

void JoystickSDL::_loadGamepadMappings()
{
    QFile file(QStringLiteral(":/gamecontrollerdb.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(JoystickSDLLog) << "Couldn't load Gamepad mapping database.";
        return;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (line.startsWith('#') || line.isEmpty()) {
            continue;
        }
        if (SDL_AddGamepadMapping(qPrintable(line)) == -1) {
            qCWarning(JoystickSDLLog) << "Couldn't add Gamepad mapping:" << SDL_GetError();
        }
    }

#ifdef SDL_GAMECONTROLLERCONFIG
    const QString mappingsStr = QStringLiteral(SDL_GAMECONTROLLERCONFIG);
    const QStringList mappingList = mappingsStr.split(u'\n', Qt::SkipEmptyParts);
    for (const QString &mapping : mappingList) {
        if (SDL_AddGamepadMapping(qPrintable(mapping)) == -1) {
            qCWarning(JoystickSDLLog) << "Couldn't add Gamepad mapping:" << mapping << "Error:" << SDL_GetError();
        }
    }
#endif
}
