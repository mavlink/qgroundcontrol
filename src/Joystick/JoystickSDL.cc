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

QGC_LOGGING_CATEGORY(JoystickSDLLog, "qgc.joystick.joysticksdl")

JoystickSDL::JoystickSDL(const QString &name, int axisCount, int buttonCount, int hatCount, int instanceId, bool isGamepad, QObject *parent)
    : Joystick(name, axisCount, buttonCount, hatCount, parent)
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

        SDL_Joystick *tmpJoy = SDL_OpenJoystick(jid);
        if (!tmpJoy) {
            qCWarning(JoystickSDLLog) << "Failed to open joystick" << jid << SDL_GetError();
            continue;
        }

        const int axisCount = SDL_GetNumJoystickAxes(tmpJoy);
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
                                        qMax(0, axisCount),
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

bool JoystickSDL::_getButton(int idx) const
{
    int button = -1;

    if (_isGamepad) {
        button = SDL_GetGamepadButton(_sdlGamepad, static_cast<SDL_GamepadButton>(idx));
    } else {
        button = SDL_GetJoystickButton(_sdlJoystick, idx);
    }

    return (button == 1);
}

int JoystickSDL::_getAxis(int idx) const
{
    int axis = -1;

    if (_isGamepad) {
        axis = SDL_GetGamepadAxis(_sdlGamepad, static_cast<SDL_GamepadAxis>(idx));
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
