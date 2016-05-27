#include "JoystickSDL.h"

#include "QGCApplication.h"

#include <QQmlEngine>

JoystickSDL::JoystickSDL(const QString& name, int axisCount, int buttonCount, int hatCount, int index, MultiVehicleManager* multiVehicleManager)
    : Joystick(name,axisCount,buttonCount,hatCount,multiVehicleManager)
    , _index(index)
{
}

QMap<QString, Joystick*> JoystickSDL::discover(MultiVehicleManager* _multiVehicleManager) {
    static QMap<QString, Joystick*> ret;

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0) {
        qWarning() << "Couldn't initialize SimpleDirectMediaLayer:" << SDL_GetError();
        return ret;
    }

    // Load available joysticks

    qCDebug(JoystickLog) << "Available joysticks";

    for (int i=0; i<SDL_NumJoysticks(); i++) {
        QString name = SDL_JoystickName(i);

        if (!ret.contains(name)) {
            int axisCount, buttonCount, hatCount;

            SDL_Joystick* sdlJoystick = SDL_JoystickOpen(i);
            axisCount = SDL_JoystickNumAxes(sdlJoystick);
            buttonCount = SDL_JoystickNumButtons(sdlJoystick);
            hatCount = SDL_JoystickNumHats(sdlJoystick);
            SDL_JoystickClose(sdlJoystick);

            qCDebug(JoystickLog) << "\t" << name << "axes:" << axisCount << "buttons:" << buttonCount << "hats:" << hatCount;
            ret[name] = new JoystickSDL(name, axisCount, buttonCount, hatCount, i, _multiVehicleManager);
        } else {
            qCDebug(JoystickLog) << "\tSkipping duplicate" << name;
        }
    }
    return ret;
}

bool JoystickSDL::_open(void) {
    sdlJoystick = SDL_JoystickOpen(_index);

    if (!sdlJoystick) {
        qCWarning(JoystickLog) << "SDL_JoystickOpen failed:" << SDL_GetError();
        return false;
    }

    return true;
}

void JoystickSDL::_close(void) {
    SDL_JoystickClose(sdlJoystick);
}

bool JoystickSDL::_update(void)
{
    SDL_JoystickUpdate();
    return true;
}

bool JoystickSDL::_getButton(int i) {
    return !!SDL_JoystickGetButton(sdlJoystick, i);
}

int JoystickSDL::_getAxis(int i) {
    return SDL_JoystickGetAxis(sdlJoystick, i);
}

uint8_t JoystickSDL::_getHat(int hat,int i) {
    uint8_t hatButtons[] = {SDL_HAT_UP,SDL_HAT_DOWN,SDL_HAT_LEFT,SDL_HAT_RIGHT};

    if ( i < int(sizeof(hatButtons)) ) {
        return !!(SDL_JoystickGetHat(sdlJoystick, hat) & hatButtons[i]);
    }
    return 0;
}

