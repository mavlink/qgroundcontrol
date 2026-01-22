#include "JoystickSDL.h"
#include "JoystickManager.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QMetaObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>

#include <array>

#include <SDL3/SDL.h>

QGC_LOGGING_CATEGORY(JoystickSDLLog, "Joystick.JoystickSDL")

// Static discovery cache - persists between discover() calls to track joystick lifecycle
static QMap<QString, Joystick*> s_discoveryCache;

static bool sdlEventWatcher(void *userdata, SDL_Event *event)
{
    Q_UNUSED(userdata);

    // Get manager instance once - may be null during shutdown
    JoystickManager *manager = JoystickManager::instance();
    if (!manager) {
        return true;
    }

    switch (event->type) {
    case SDL_EVENT_JOYSTICK_ADDED:
    case SDL_EVENT_JOYSTICK_REMOVED:
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
        qCDebug(JoystickSDLLog) << "SDL device event:" << event->type;
        QMetaObject::invokeMethod(manager, "_checkForAddedOrRemovedJoysticks", Qt::QueuedConnection);
        break;

    case SDL_EVENT_JOYSTICK_BATTERY_UPDATED:
        qCDebug(JoystickSDLLog) << "Battery updated for joystick" << event->jbattery.which
                                << "state:" << event->jbattery.state
                                << "percent:" << event->jbattery.percent;
        QMetaObject::invokeMethod(manager, "_handleBatteryUpdated",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, static_cast<int>(event->jbattery.which)));
        break;

    case SDL_EVENT_GAMEPAD_REMAPPED:
        qCDebug(JoystickSDLLog) << "Gamepad remapped:" << event->gdevice.which;
        QMetaObject::invokeMethod(manager, "_handleGamepadRemapped",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, static_cast<int>(event->gdevice.which)));
        break;

    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
        QMetaObject::invokeMethod(manager, "_handleTouchpadEvent",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, static_cast<int>(event->gtouchpad.which)),
                                  Q_ARG(int, event->gtouchpad.touchpad),
                                  Q_ARG(int, event->gtouchpad.finger),
                                  Q_ARG(bool, event->type != SDL_EVENT_GAMEPAD_TOUCHPAD_UP),
                                  Q_ARG(float, event->gtouchpad.x),
                                  Q_ARG(float, event->gtouchpad.y),
                                  Q_ARG(float, event->gtouchpad.pressure));
        break;

    case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
        QMetaObject::invokeMethod(manager, "_handleSensorUpdate",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, static_cast<int>(event->gsensor.which)),
                                  Q_ARG(int, event->gsensor.sensor),
                                  Q_ARG(float, event->gsensor.data[0]),
                                  Q_ARG(float, event->gsensor.data[1]),
                                  Q_ARG(float, event->gsensor.data[2]));
        break;

    case SDL_EVENT_JOYSTICK_UPDATE_COMPLETE:
    case SDL_EVENT_GAMEPAD_UPDATE_COMPLETE:
        QMetaObject::invokeMethod(manager, "_handleUpdateComplete",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, static_cast<int>(event->common.type == SDL_EVENT_GAMEPAD_UPDATE_COMPLETE
                                                              ? event->gdevice.which : event->jdevice.which)));
        break;

    default:
        break;
    }

    return true;
}

JoystickSDL::JoystickSDL(const QString &name, const QList<int> &gamepadAxes, const QList<int> &nonGamepadAxes, int buttonCount, int hatCount, int instanceId, QObject *parent)
    : Joystick(name, gamepadAxes.length() + nonGamepadAxes.length(), buttonCount, hatCount, parent)
    , _gamepadAxes(gamepadAxes)
    , _nonGamepadAxes(nonGamepadAxes)
    , _instanceId(instanceId)
{
    qCDebug(JoystickSDLLog) << this;
}

JoystickSDL::~JoystickSDL()
{
    qCDebug(JoystickSDLLog) << this;
}

quint64 JoystickSDL::_getProperties() const
{
    if (_sdlGamepad) {
        return SDL_GetGamepadProperties(_sdlGamepad);
    }
    if (_sdlJoystick) {
        return SDL_GetJoystickProperties(_sdlJoystick);
    }
    return 0;
}

bool JoystickSDL::_checkVirtualJoystick(const char *methodName) const
{
    if (!_sdlJoystick) {
        qCWarning(JoystickSDLLog) << methodName << "called with null joystick";
        return false;
    }
    if (!isVirtual()) {
        qCWarning(JoystickSDLLog) << methodName << "called on non-virtual joystick:" << _name;
        return false;
    }
    return true;
}

bool JoystickSDL::_hasGamepadCapability(const char *propertyName) const
{
    if (_sdlGamepad) {
        SDL_PropertiesID props = SDL_GetGamepadProperties(_sdlGamepad);
        return SDL_GetBooleanProperty(props, propertyName, false);
    }
    return false;
}

QString JoystickSDL::_connectionStateToString(int state)
{
    switch (static_cast<SDL_JoystickConnectionState>(state)) {
    case SDL_JOYSTICK_CONNECTION_INVALID:
        return QStringLiteral("Invalid");
    case SDL_JOYSTICK_CONNECTION_UNKNOWN:
        return QStringLiteral("Unknown");
    case SDL_JOYSTICK_CONNECTION_WIRED:
        return QStringLiteral("Wired");
    case SDL_JOYSTICK_CONNECTION_WIRELESS:
        return QStringLiteral("Wireless");
    default:
        return QString();
    }
}

// Returns user-friendly display names for gamepad types (e.g., "Xbox 360", "PlayStation 5").
// For SDL's internal technical names (e.g., "xbox360", "ps5"), use _gamepadTypeToString().
QString JoystickSDL::_gamepadTypeEnumToString(int type)
{
    switch (static_cast<SDL_GamepadType>(type)) {
    case SDL_GAMEPAD_TYPE_STANDARD:
        return QStringLiteral("Standard");
    case SDL_GAMEPAD_TYPE_XBOX360:
        return QStringLiteral("Xbox 360");
    case SDL_GAMEPAD_TYPE_XBOXONE:
        return QStringLiteral("Xbox One");
    case SDL_GAMEPAD_TYPE_PS3:
        return QStringLiteral("PlayStation 3");
    case SDL_GAMEPAD_TYPE_PS4:
        return QStringLiteral("PlayStation 4");
    case SDL_GAMEPAD_TYPE_PS5:
        return QStringLiteral("PlayStation 5");
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
        return QStringLiteral("Nintendo Switch Pro");
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
        return QStringLiteral("Nintendo Joy-Con (L)");
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
        return QStringLiteral("Nintendo Joy-Con (R)");
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
        return QStringLiteral("Nintendo Joy-Con Pair");
    default:
        return QStringLiteral("Unknown");
    }
}

bool JoystickSDL::init()
{
    // Allow joystick events when app is in background (critical for ground stations)
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    // Enable Steam controller support (Steam Input virtual controllers)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK, "1");

    // Enable HIDAPI driver for better PS4/PS5/Xbox controller support
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX_360, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_WIRELESS, "1");

    // Enable HIDAPI for Nintendo controllers
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH_HOME_LED, "1");

    // Enable HIDAPI for other controllers
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_WII, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_WII_PLAYER_LED, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_8BITDO, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_FLYDIGI, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_LUNA, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STADIA, "1");

    // Enable sensor fusion (use device accelerometer/gyro as gamepad sensors)
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_SENSOR_FUSION, "1");

#ifdef Q_OS_WIN
    // Enable Windows raw input for better device support
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "1");
#endif

#ifdef Q_OS_LINUX
    // Linux-specific hints
    SDL_SetHint(SDL_HINT_JOYSTICK_LINUX_DEADZONES, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_LINUX_DIGITAL_HATS, "1");
#endif

    // Enable threaded joystick processing for smoother updates
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");

    // Enable enhanced reports for gyro/accel data on supported controllers
    SDL_SetHint(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "1");

    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
        qCWarning(JoystickSDLLog) << "Failed to initialize SDL:" << SDL_GetError();
        return false;
    }

    SDL_AddEventWatch(sdlEventWatcher, nullptr);

    _loadGamepadMappings();
    return true;
}

void JoystickSDL::shutdown()
{
    SDL_RemoveEventWatch(sdlEventWatcher, nullptr);
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK);

    // Clear discovery cache to avoid stale pointers after shutdown
    s_discoveryCache.clear();
}

void JoystickSDL::pumpEvents()
{
    SDL_PumpEvents();
    SDL_UpdateJoysticks();
}

void JoystickSDL::_loadGamepadMappings()
{
    const auto loadMappingsFromFile = [](const QString &path, const char *description) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        qCDebug(JoystickSDLLog) << "Loading gamepad mappings from" << description;
        int count = 0;
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            if (line.startsWith('#') || line.isEmpty()) {
                continue;
            }
            if (SDL_AddGamepadMapping(qPrintable(line)) == -1) {
                qCWarning(JoystickSDLLog) << "Couldn't add Gamepad mapping:" << SDL_GetError();
            } else {
                ++count;
            }
        }
        qCDebug(JoystickSDLLog) << "Loaded" << count << "mappings from" << description;
        return true;
    };

    // Load bundled database
    if (!loadMappingsFromFile(QStringLiteral(":/gamecontrollerdb.txt"), "bundled database")) {
        qCWarning(JoystickSDLLog) << "Couldn't load bundled Gamepad mapping database.";
    }

    // Load user custom mappings from <AppConfigLocation>/gamecontrollerdb.txt
    // This allows users to add controller support without recompiling.
    // Format: https://github.com/mdqinc/SDL_GameControllerDB
    const QString userMappingsPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                   + QStringLiteral("/gamecontrollerdb.txt");
    if (loadMappingsFromFile(userMappingsPath, "user config")) {
        qCInfo(JoystickSDLLog) << "Loaded user gamepad mappings from" << userMappingsPath;
    }
}

QMap<QString, Joystick*> JoystickSDL::discover()
{
    QMap<QString, Joystick*> current;

    qCDebug(JoystickSDLLog) << "Discovering joysticks";

    // Lock to ensure device list doesn't change during enumeration
    SDL_LockJoysticks();

    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    if (!ids) {
        SDL_UnlockJoysticks();
        qCWarning(JoystickSDLLog) << "SDL_GetJoysticks failed:" << SDL_GetError();
        return current;
    }

    for (int n = 0; n < count; ++n) {
        const SDL_JoystickID jid = ids[n];
        QString name = QString::fromUtf8(SDL_GetJoystickNameForID(jid));

        if (s_discoveryCache.contains(name)) {
            current[name] = s_discoveryCache[name];
            // Safe: all objects in s_discoveryCache are JoystickSDL instances created by this function
            auto *js = static_cast<JoystickSDL*>(current[name]);
            if (js->instanceId() != jid) {
                js->setInstanceId(jid);
            }
            (void) s_discoveryCache.remove(name);
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
            int bindingCount = 0;
            SDL_GamepadBinding **bindings = SDL_GetGamepadBindings(tmpGamepad, &bindingCount);
            if (bindings) {
                for (int i = 0; i < bindingCount; ++i) {
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

        const QString baseName = name;
        int dupIdx = 1;
        while (current.contains(name)) {
            name = QString("%1 %2").arg(baseName).arg(dupIdx++);
        }

        current[name] = new JoystickSDL(name,
                                        gamepadAxes,
                                        nonGamepadAxes,
                                        qMax(0, buttonCount),
                                        qMax(0, hatCount),
                                        jid);
    }

    SDL_free(ids);
    SDL_UnlockJoysticks();

    for (auto *joystick : std::as_const(s_discoveryCache)) {
        joystick->deleteLater();
    }

    s_discoveryCache = current;
    return current;
}

//-----------------------------------------------------------------------------
// Joystick Interface Overrides
//-----------------------------------------------------------------------------

bool JoystickSDL::_open()
{
    if (isGamepad()) {
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

    if (_sdlHaptic) {
        SDL_CloseHaptic(_sdlHaptic);
        _sdlHaptic = nullptr;
    }

    if (_sdlGamepad) {
        SDL_CloseGamepad(_sdlGamepad);
    } else {
        SDL_CloseJoystick(_sdlJoystick);
    }

    _sdlJoystick = nullptr;
    _sdlGamepad = nullptr;
}

bool JoystickSDL::_update()
{
    if (!_sdlJoystick || !SDL_JoystickConnected(_sdlJoystick)) {
        qCWarning(JoystickSDLLog) << "Joystick disconnected during update:" << _name;
        return false;
    }

    if (_sdlGamepad) {
        SDL_UpdateGamepads();
    } else {
        SDL_UpdateJoysticks();
    }

    return true;
}

//-----------------------------------------------------------------------------
// Input State Accessors
//-----------------------------------------------------------------------------

bool JoystickSDL::_getButton(int idx) const
{
    // First try the standardized gamepad set if idx is inside that set
    if (_sdlGamepad && (idx >= 0) && (idx < SDL_GAMEPAD_BUTTON_COUNT)) {
        return SDL_GetGamepadButton(_sdlGamepad, static_cast<SDL_GamepadButton>(idx));
    }

    // Fall back to raw joystick buttons (covers unmapped/extras)
    if (_sdlJoystick && (idx >= 0) && (idx < SDL_GetNumJoystickButtons(_sdlJoystick))) {
        return SDL_GetJoystickButton(_sdlJoystick, idx);
    }

    return false;
}

int JoystickSDL::_getAxisValue(int idx) const
{
    if (idx < 0) {
        return 0;
    }

    if (_sdlGamepad) {
        if (idx < _gamepadAxes.length()) {
            return SDL_GetGamepadAxis(_sdlGamepad, static_cast<SDL_GamepadAxis>(_gamepadAxes[idx]));
        }
        const int nonGamepadIdx = idx - static_cast<int>(_gamepadAxes.length());
        if (nonGamepadIdx < _nonGamepadAxes.length()) {
            return SDL_GetJoystickAxis(_sdlJoystick, _nonGamepadAxes[nonGamepadIdx]);
        }
        return 0;
    }

    return SDL_GetJoystickAxis(_sdlJoystick, idx);
}

bool JoystickSDL::_getHat(int hat, int idx) const
{
    static constexpr std::array<uint8_t, 4> hatButtons = {SDL_HAT_UP, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHT};

    if (idx < 0 || static_cast<size_t>(idx) >= hatButtons.size()) {
        return false;
    }

    return ((SDL_GetJoystickHat(_sdlJoystick, hat) & hatButtons[idx]) != 0);
}

//-----------------------------------------------------------------------------
// Haptic and LED
//-----------------------------------------------------------------------------

bool JoystickSDL::hasRumble() const
{
    const quint64 props = _getProperties();
    return props != 0 && SDL_GetBooleanProperty(props, SDL_PROP_JOYSTICK_CAP_RUMBLE_BOOLEAN, false);
}

bool JoystickSDL::hasRumbleTriggers() const
{
    const quint64 props = _getProperties();
    return props != 0 && SDL_GetBooleanProperty(props, SDL_PROP_JOYSTICK_CAP_TRIGGER_RUMBLE_BOOLEAN, false);
}

bool JoystickSDL::hasLED() const
{
    const quint64 props = _getProperties();
    if (props == 0) {
        return false;
    }
    return SDL_GetBooleanProperty(props, SDL_PROP_JOYSTICK_CAP_RGB_LED_BOOLEAN, false) ||
           SDL_GetBooleanProperty(props, SDL_PROP_JOYSTICK_CAP_MONO_LED_BOOLEAN, false);
}

void JoystickSDL::rumble(quint16 lowFreq, quint16 highFreq, quint32 durationMs)
{
    if (_sdlGamepad) {
        if (!SDL_RumbleGamepad(_sdlGamepad, lowFreq, highFreq, durationMs)) {
            qCDebug(JoystickSDLLog) << "Rumble failed:" << SDL_GetError();
        }
    } else if (_sdlJoystick) {
        if (!SDL_RumbleJoystick(_sdlJoystick, lowFreq, highFreq, durationMs)) {
            qCDebug(JoystickSDLLog) << "Rumble failed:" << SDL_GetError();
        }
    }
}

void JoystickSDL::rumbleTriggers(quint16 left, quint16 right, quint32 durationMs)
{
    if (_sdlGamepad) {
        if (!SDL_RumbleGamepadTriggers(_sdlGamepad, left, right, durationMs)) {
            qCDebug(JoystickSDLLog) << "Trigger rumble failed:" << SDL_GetError();
        }
    }
}

void JoystickSDL::setLED(quint8 red, quint8 green, quint8 blue)
{
    if (_sdlGamepad) {
        if (!SDL_SetGamepadLED(_sdlGamepad, red, green, blue)) {
            qCDebug(JoystickSDLLog) << "Set LED failed:" << SDL_GetError();
        }
    } else if (_sdlJoystick) {
        if (!SDL_SetJoystickLED(_sdlJoystick, red, green, blue)) {
            qCDebug(JoystickSDLLog) << "Set LED failed:" << SDL_GetError();
        }
    }
}

bool JoystickSDL::sendEffect(const QByteArray &data)
{
    if (_sdlGamepad && !data.isEmpty()) {
        if (!SDL_SendGamepadEffect(_sdlGamepad, data.constData(), static_cast<int>(data.size()))) {
            qCDebug(JoystickSDLLog) << "sendEffect failed:" << SDL_GetError();
            return false;
        }
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Device Identity
//-----------------------------------------------------------------------------

QString JoystickSDL::guid() const
{
    if (_sdlJoystick) {
        SDL_GUID sdlGuid = SDL_GetJoystickGUID(_sdlJoystick);
        char guidStr[33];
        SDL_GUIDToString(sdlGuid, guidStr, sizeof(guidStr));
        return QString::fromLatin1(guidStr);
    }
    return QString();
}

quint16 JoystickSDL::vendorId() const
{
    if (_sdlJoystick) {
        return SDL_GetJoystickVendor(_sdlJoystick);
    }
    return 0;
}

quint16 JoystickSDL::productId() const
{
    if (_sdlJoystick) {
        return SDL_GetJoystickProduct(_sdlJoystick);
    }
    return 0;
}

QString JoystickSDL::serial() const
{
    if (_sdlJoystick) {
        const char *serialStr = SDL_GetJoystickSerial(_sdlJoystick);
        if (serialStr) {
            return QString::fromUtf8(serialStr);
        }
    }
    return QString();
}

QString JoystickSDL::deviceType() const
{
    if (_sdlJoystick) {
        SDL_JoystickType type = SDL_GetJoystickType(_sdlJoystick);
        switch (type) {
        case SDL_JOYSTICK_TYPE_GAMEPAD:
            return QStringLiteral("Gamepad");
        case SDL_JOYSTICK_TYPE_WHEEL:
            return QStringLiteral("Wheel");
        case SDL_JOYSTICK_TYPE_ARCADE_STICK:
            return QStringLiteral("Arcade Stick");
        case SDL_JOYSTICK_TYPE_FLIGHT_STICK:
            return QStringLiteral("Flight Stick");
        case SDL_JOYSTICK_TYPE_DANCE_PAD:
            return QStringLiteral("Dance Pad");
        case SDL_JOYSTICK_TYPE_GUITAR:
            return QStringLiteral("Guitar");
        case SDL_JOYSTICK_TYPE_DRUM_KIT:
            return QStringLiteral("Drum Kit");
        case SDL_JOYSTICK_TYPE_ARCADE_PAD:
            return QStringLiteral("Arcade Pad");
        case SDL_JOYSTICK_TYPE_THROTTLE:
            return QStringLiteral("Throttle");
        default:
            return QStringLiteral("Unknown");
        }
    }
    return QString();
}

QString JoystickSDL::path() const
{
    if (_sdlJoystick) {
        const char *devicePath = SDL_GetJoystickPath(_sdlJoystick);
        if (devicePath) {
            return QString::fromUtf8(devicePath);
        }
    }
    return QString();
}

bool JoystickSDL::isVirtual() const
{
    return SDL_IsJoystickVirtual(_instanceId);
}

quint16 JoystickSDL::firmwareVersion() const
{
    if (_sdlJoystick) {
        return SDL_GetJoystickFirmwareVersion(_sdlJoystick);
    }
    return 0;
}

QString JoystickSDL::connectionType() const
{
    if (!_sdlJoystick) {
        return QString();
    }
    return _connectionStateToString(SDL_GetJoystickConnectionState(_sdlJoystick));
}

//-----------------------------------------------------------------------------
// Player Info
//-----------------------------------------------------------------------------

int JoystickSDL::playerIndex() const
{
    if (_sdlGamepad) {
        return SDL_GetGamepadPlayerIndex(_sdlGamepad);
    }
    if (_sdlJoystick) {
        return SDL_GetJoystickPlayerIndex(_sdlJoystick);
    }
    return -1;
}

void JoystickSDL::setPlayerIndex(int index)
{
    bool success = false;
    if (_sdlGamepad) {
        success = SDL_SetGamepadPlayerIndex(_sdlGamepad, index);
    } else if (_sdlJoystick) {
        success = SDL_SetJoystickPlayerIndex(_sdlJoystick, index);
    }

    if (success) {
        emit playerIndexChanged();
    } else {
        qCDebug(JoystickSDLLog) << "Failed to set player index:" << SDL_GetError();
    }
}

//-----------------------------------------------------------------------------
// Power & Battery Status
//-----------------------------------------------------------------------------

int JoystickSDL::batteryPercent() const
{
    if (_sdlJoystick) {
        int percent = -1;
        SDL_GetJoystickPowerInfo(_sdlJoystick, &percent);
        return percent;
    }
    return -1;
}

QString JoystickSDL::powerState() const
{
    if (!_sdlJoystick) {
        return QString();
    }

    switch (SDL_GetJoystickPowerInfo(_sdlJoystick, nullptr)) {
    case SDL_POWERSTATE_ERROR:
        return QStringLiteral("Error");
    case SDL_POWERSTATE_UNKNOWN:
        return QStringLiteral("Unknown");
    case SDL_POWERSTATE_ON_BATTERY:
        return QStringLiteral("On Battery");
    case SDL_POWERSTATE_NO_BATTERY:
        return QStringLiteral("No Battery");
    case SDL_POWERSTATE_CHARGING:
        return QStringLiteral("Charging");
    case SDL_POWERSTATE_CHARGED:
        return QStringLiteral("Charged");
    default:
        return QString();
    }
}

//-----------------------------------------------------------------------------
// Gamepad Type Detection
//-----------------------------------------------------------------------------

bool JoystickSDL::isGamepad() const
{
    return SDL_IsGamepad(_instanceId);
}

QString JoystickSDL::gamepadType() const
{
    if (_sdlGamepad) {
        return _gamepadTypeEnumToString(SDL_GetGamepadType(_sdlGamepad));
    }
    return QString();
}

//-----------------------------------------------------------------------------
// Control Labels
//-----------------------------------------------------------------------------

QString JoystickSDL::axisLabel(int axis) const
{
    if (_sdlGamepad && axis >= 0 && axis < _gamepadAxes.length()) {
        const char *label = SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(_gamepadAxes[axis]));
        if (label) {
            return QString::fromUtf8(label);
        }
    }
    return tr("Axis %1").arg(axis);
}

QString JoystickSDL::buttonLabel(int button) const
{
    if (_sdlGamepad && button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT) {
        const char *label = SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
        if (label) {
            return QString::fromUtf8(label);
        }
    }
    return tr("Button %1").arg(button);
}

QString JoystickSDL::axisSFSymbol(int axis) const
{
#ifdef Q_OS_DARWIN
    if (_sdlGamepad && axis >= 0 && axis < _gamepadAxes.length()) {
        const char *symbol = SDL_GetGamepadAppleSFSymbolsNameForAxis(_sdlGamepad, static_cast<SDL_GamepadAxis>(_gamepadAxes[axis]));
        if (symbol) {
            return QString::fromUtf8(symbol);
        }
    }
#else
    Q_UNUSED(axis);
#endif
    return QString();
}

QString JoystickSDL::buttonSFSymbol(int button) const
{
#ifdef Q_OS_DARWIN
    if (_sdlGamepad && button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT) {
        const char *symbol = SDL_GetGamepadAppleSFSymbolsNameForButton(_sdlGamepad, static_cast<SDL_GamepadButton>(button));
        if (symbol) {
            return QString::fromUtf8(symbol);
        }
    }
#else
    Q_UNUSED(button);
#endif
    return QString();
}

//-----------------------------------------------------------------------------
// Mapping Management
//-----------------------------------------------------------------------------

QString JoystickSDL::getMapping() const
{
    if (_sdlGamepad) {
        char *mapping = SDL_GetGamepadMapping(_sdlGamepad);
        if (mapping) {
            QString result = QString::fromUtf8(mapping);
            SDL_free(mapping);
            return result;
        }
    }
    return QString();
}

bool JoystickSDL::addMapping(const QString &mapping)
{
    if (mapping.isEmpty()) {
        return false;
    }

    int result = SDL_AddGamepadMapping(qPrintable(mapping));
    if (result == -1) {
        qCWarning(JoystickSDLLog) << "Failed to add gamepad mapping:" << SDL_GetError();
        return false;
    }

    qCDebug(JoystickSDLLog) << "Added gamepad mapping, result:" << result;
    return true;
}

bool JoystickSDL::reloadMappings()
{
    if (!SDL_ReloadGamepadMappings()) {
        qCWarning(JoystickSDLLog) << "Failed to reload gamepad mappings:" << SDL_GetError();
        return false;
    }

    _loadGamepadMappings();

    qCDebug(JoystickSDLLog) << "Reloaded gamepad mappings";
    return true;
}

static void populateBindingResult(QVariantMap &result, const SDL_GamepadBinding *binding)
{
    result[QStringLiteral("valid")] = true;
    result[QStringLiteral("inputType")] = static_cast<int>(binding->input_type);
    if (binding->input_type == SDL_GAMEPAD_BINDTYPE_AXIS) {
        result[QStringLiteral("inputAxis")] = binding->input.axis.axis;
        result[QStringLiteral("inputAxisMin")] = binding->input.axis.axis_min;
        result[QStringLiteral("inputAxisMax")] = binding->input.axis.axis_max;
    } else if (binding->input_type == SDL_GAMEPAD_BINDTYPE_BUTTON) {
        result[QStringLiteral("inputButton")] = binding->input.button;
    } else if (binding->input_type == SDL_GAMEPAD_BINDTYPE_HAT) {
        result[QStringLiteral("inputHat")] = binding->input.hat.hat;
        result[QStringLiteral("inputHatMask")] = binding->input.hat.hat_mask;
    }
}

template<typename MatchFunc>
static QVariantMap findBinding(SDL_Gamepad *gamepad, MatchFunc matchFunc)
{
    QVariantMap result;
    result[QStringLiteral("valid")] = false;

    if (!gamepad) {
        return result;
    }

    int bindingCount = 0;
    SDL_GamepadBinding **bindings = SDL_GetGamepadBindings(gamepad, &bindingCount);
    if (!bindings) {
        return result;
    }

    for (int i = 0; i < bindingCount; ++i) {
        SDL_GamepadBinding *binding = bindings[i];
        if (binding && matchFunc(binding)) {
            populateBindingResult(result, binding);
            SDL_free(bindings);
            return result;
        }
    }

    SDL_free(bindings);
    return result;
}

QVariantMap JoystickSDL::getAxisBinding(int axis) const
{
    if (!_sdlGamepad || axis < 0 || axis >= _gamepadAxes.length()) {
        QVariantMap result;
        result[QStringLiteral("valid")] = false;
        return result;
    }

    const SDL_GamepadAxis gamepadAxis = static_cast<SDL_GamepadAxis>(_gamepadAxes[axis]);
    return findBinding(_sdlGamepad, [gamepadAxis](const SDL_GamepadBinding *binding) {
        return binding->output_type == SDL_GAMEPAD_BINDTYPE_AXIS &&
               binding->output.axis.axis == gamepadAxis;
    });
}

QVariantMap JoystickSDL::getButtonBinding(int button) const
{
    if (!_sdlGamepad || button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT) {
        QVariantMap result;
        result[QStringLiteral("valid")] = false;
        return result;
    }

    const SDL_GamepadButton gamepadButton = static_cast<SDL_GamepadButton>(button);
    return findBinding(_sdlGamepad, [gamepadButton](const SDL_GamepadBinding *binding) {
        return binding->output_type == SDL_GAMEPAD_BINDTYPE_BUTTON &&
               binding->output.button == gamepadButton;
    });
}

//-----------------------------------------------------------------------------
// Sensor Support
//-----------------------------------------------------------------------------

bool JoystickSDL::hasGyroscope() const
{
    if (_sdlGamepad) {
        return SDL_GamepadHasSensor(_sdlGamepad, SDL_SENSOR_GYRO);
    }
    return false;
}

bool JoystickSDL::hasAccelerometer() const
{
    if (_sdlGamepad) {
        return SDL_GamepadHasSensor(_sdlGamepad, SDL_SENSOR_ACCEL);
    }
    return false;
}

bool JoystickSDL::setGyroscopeEnabled(bool enabled)
{
    if (_sdlGamepad && hasGyroscope()) {
        if (!SDL_SetGamepadSensorEnabled(_sdlGamepad, SDL_SENSOR_GYRO, enabled)) {
            qCWarning(JoystickSDLLog) << "Failed to" << (enabled ? "enable" : "disable") << "gyroscope:" << SDL_GetError();
            return false;
        }
        return true;
    }
    return false;
}

bool JoystickSDL::setAccelerometerEnabled(bool enabled)
{
    if (_sdlGamepad && hasAccelerometer()) {
        if (!SDL_SetGamepadSensorEnabled(_sdlGamepad, SDL_SENSOR_ACCEL, enabled)) {
            qCWarning(JoystickSDLLog) << "Failed to" << (enabled ? "enable" : "disable") << "accelerometer:" << SDL_GetError();
            return false;
        }
        return true;
    }
    return false;
}

QVector3D JoystickSDL::gyroscopeData() const
{
    if (!_sdlGamepad) {
        return QVector3D();
    }
    std::array<float, 3> data = {0.0f, 0.0f, 0.0f};
    if (!SDL_GetGamepadSensorData(_sdlGamepad, SDL_SENSOR_GYRO, data.data(), 3)) {
        // Only log if sensor is supposed to be enabled - otherwise this is expected
        if (SDL_GamepadSensorEnabled(_sdlGamepad, SDL_SENSOR_GYRO)) {
            qCDebug(JoystickSDLLog) << "Failed to get gyroscope data:" << SDL_GetError();
        }
        return QVector3D();
    }
    return QVector3D(data[0], data[1], data[2]);
}

QVector3D JoystickSDL::accelerometerData() const
{
    if (!_sdlGamepad) {
        return QVector3D();
    }
    std::array<float, 3> data = {0.0f, 0.0f, 0.0f};
    if (!SDL_GetGamepadSensorData(_sdlGamepad, SDL_SENSOR_ACCEL, data.data(), 3)) {
        // Only log if sensor is supposed to be enabled - otherwise this is expected
        if (SDL_GamepadSensorEnabled(_sdlGamepad, SDL_SENSOR_ACCEL)) {
            qCDebug(JoystickSDLLog) << "Failed to get accelerometer data:" << SDL_GetError();
        }
        return QVector3D();
    }
    return QVector3D(data[0], data[1], data[2]);
}

float JoystickSDL::gyroscopeDataRate() const
{
    if (_sdlGamepad) {
        return SDL_GetGamepadSensorDataRate(_sdlGamepad, SDL_SENSOR_GYRO);
    }
    return 0.0f;
}

float JoystickSDL::accelerometerDataRate() const
{
    if (_sdlGamepad) {
        return SDL_GetGamepadSensorDataRate(_sdlGamepad, SDL_SENSOR_ACCEL);
    }
    return 0.0f;
}

//-----------------------------------------------------------------------------
// Touchpad Support
//-----------------------------------------------------------------------------

int JoystickSDL::touchpadCount() const
{
    if (_sdlGamepad) {
        return SDL_GetNumGamepadTouchpads(_sdlGamepad);
    }
    return 0;
}

int JoystickSDL::touchpadFingerCount(int touchpad) const
{
    if (_sdlGamepad) {
        return SDL_GetNumGamepadTouchpadFingers(_sdlGamepad, touchpad);
    }
    return 0;
}

QVariantMap JoystickSDL::getTouchpadFinger(int touchpad, int finger) const
{
    QVariantMap result;
    if (_sdlGamepad) {
        bool down = false;
        float x = 0.0f, y = 0.0f, pressure = 0.0f;
        if (SDL_GetGamepadTouchpadFinger(_sdlGamepad, touchpad, finger, &down, &x, &y, &pressure)) {
            result[QStringLiteral("valid")] = true;
            result[QStringLiteral("down")] = down;
            result[QStringLiteral("x")] = x;
            result[QStringLiteral("y")] = y;
            result[QStringLiteral("pressure")] = pressure;
            return result;
        }
    }
    result[QStringLiteral("valid")] = false;
    return result;
}

//-----------------------------------------------------------------------------
// Trackball Support
//-----------------------------------------------------------------------------

int JoystickSDL::ballCount() const
{
    if (_sdlJoystick) {
        return SDL_GetNumJoystickBalls(_sdlJoystick);
    }
    return 0;
}

QVariantMap JoystickSDL::getBall(int ball) const
{
    QVariantMap result;
    if (_sdlJoystick) {
        int dx = 0, dy = 0;
        if (SDL_GetJoystickBall(_sdlJoystick, ball, &dx, &dy)) {
            result[QStringLiteral("valid")] = true;
            result[QStringLiteral("dx")] = dx;
            result[QStringLiteral("dy")] = dy;
            return result;
        }
    }
    result[QStringLiteral("valid")] = false;
    return result;
}

//-----------------------------------------------------------------------------
// Capability Queries
//-----------------------------------------------------------------------------

bool JoystickSDL::hasButton(int button) const
{
    if (_sdlGamepad && button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT) {
        return SDL_GamepadHasButton(_sdlGamepad, static_cast<SDL_GamepadButton>(button));
    }
    if (_sdlJoystick && button >= 0 && button < SDL_GetNumJoystickButtons(_sdlJoystick)) {
        return true;
    }
    return false;
}

bool JoystickSDL::hasAxis(int axis) const
{
    if (_sdlGamepad && axis >= 0 && axis < _gamepadAxes.length()) {
        return SDL_GamepadHasAxis(_sdlGamepad, static_cast<SDL_GamepadAxis>(_gamepadAxes[axis]));
    }
    if (_sdlJoystick) {
        const int totalAxes = _gamepadAxes.length() + _nonGamepadAxes.length();
        return axis >= 0 && axis < totalAxes;
    }
    return false;
}

QString JoystickSDL::realGamepadType() const
{
    if (_sdlGamepad) {
        return _gamepadTypeEnumToString(SDL_GetRealGamepadType(_sdlGamepad));
    }
    return QString();
}

//-----------------------------------------------------------------------------
// Type-Specific Labels
//-----------------------------------------------------------------------------

QString JoystickSDL::buttonLabelForType(int button) const
{
    if (_sdlGamepad && button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT) {
        SDL_GamepadType type = SDL_GetGamepadType(_sdlGamepad);
        SDL_GamepadButtonLabel label = SDL_GetGamepadButtonLabelForType(type, static_cast<SDL_GamepadButton>(button));
        switch (label) {
        case SDL_GAMEPAD_BUTTON_LABEL_A:
            return QStringLiteral("A");
        case SDL_GAMEPAD_BUTTON_LABEL_B:
            return QStringLiteral("B");
        case SDL_GAMEPAD_BUTTON_LABEL_X:
            return QStringLiteral("X");
        case SDL_GAMEPAD_BUTTON_LABEL_Y:
            return QStringLiteral("Y");
        case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
            return QStringLiteral("Cross");
        case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:
            return QStringLiteral("Circle");
        case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:
            return QStringLiteral("Square");
        case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE:
            return QStringLiteral("Triangle");
        default:
            break;
        }
    }
    return buttonLabel(button);
}

QString JoystickSDL::axisLabelForType(int axis) const
{
    // SDL doesn't have a specific axis label for type function,
    // so we fall back to the standard axis label
    return axisLabel(axis);
}

//-----------------------------------------------------------------------------
// Haptic/Force Feedback Support
//-----------------------------------------------------------------------------

bool JoystickSDL::hasHaptic() const
{
    if (_sdlJoystick) {
        return SDL_IsJoystickHaptic(_sdlJoystick);
    }
    return false;
}

int JoystickSDL::hapticEffectsCount() const
{
    if (_sdlHaptic) {
        return SDL_GetMaxHapticEffects(_sdlHaptic);
    }
    return 0;
}

bool JoystickSDL::hapticRumbleSupported() const
{
    if (_sdlHaptic) {
        const Uint32 features = SDL_GetHapticFeatures(_sdlHaptic);
        return (features & SDL_HAPTIC_LEFTRIGHT) || (features & SDL_HAPTIC_SINE);
    }
    return false;
}

bool JoystickSDL::hapticRumbleInit()
{
    if (_sdlHaptic) {
        return true;
    }

    if (!_sdlJoystick || !SDL_IsJoystickHaptic(_sdlJoystick)) {
        return false;
    }

    _sdlHaptic = SDL_OpenHapticFromJoystick(_sdlJoystick);
    if (!_sdlHaptic) {
        qCDebug(JoystickSDLLog) << "Failed to open haptic device:" << SDL_GetError();
        return false;
    }

    if (!SDL_InitHapticRumble(_sdlHaptic)) {
        qCDebug(JoystickSDLLog) << "Failed to init haptic rumble:" << SDL_GetError();
        SDL_CloseHaptic(_sdlHaptic);
        _sdlHaptic = nullptr;
        return false;
    }

    qCDebug(JoystickSDLLog) << "Haptic rumble initialized";
    return true;
}

bool JoystickSDL::hapticRumblePlay(float strength, quint32 durationMs)
{
    if (!_sdlHaptic) {
        if (!hapticRumbleInit()) {
            return false;
        }
    }

    if (!SDL_PlayHapticRumble(_sdlHaptic, strength, durationMs)) {
        qCDebug(JoystickSDLLog) << "Failed to play haptic rumble:" << SDL_GetError();
        return false;
    }
    return true;
}

void JoystickSDL::hapticRumbleStop()
{
    if (_sdlHaptic) {
        SDL_StopHapticRumble(_sdlHaptic);
    }
}

//-----------------------------------------------------------------------------
// Mapping Utilities
//-----------------------------------------------------------------------------

QString JoystickSDL::getMappingForGUID(const QString &guid) const
{
    if (guid.isEmpty()) {
        return QString();
    }

    SDL_GUID sdlGuid = SDL_StringToGUID(qPrintable(guid));
    char *mapping = SDL_GetGamepadMappingForGUID(sdlGuid);
    if (mapping) {
        QString result = QString::fromUtf8(mapping);
        SDL_free(mapping);
        return result;
    }
    return QString();
}

//-----------------------------------------------------------------------------
// Virtual Joystick Support
//-----------------------------------------------------------------------------

int JoystickSDL::createVirtualJoystick(const QString &name, int axisCount, int buttonCount, int hatCount)
{
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = static_cast<Uint16>(axisCount);
    desc.nbuttons = static_cast<Uint16>(buttonCount);
    desc.nhats = static_cast<Uint16>(hatCount);
    desc.name = qPrintable(name);

    SDL_JoystickID instanceId = SDL_AttachVirtualJoystick(&desc);
    if (instanceId == 0) {
        qCWarning(JoystickSDLLog) << "Failed to create virtual joystick:" << SDL_GetError();
        return -1;
    }

    qCDebug(JoystickSDLLog) << "Created virtual joystick" << name << "with instance ID" << instanceId;
    return static_cast<int>(instanceId);
}

bool JoystickSDL::destroyVirtualJoystick(int instanceId)
{
    if (instanceId <= 0) {
        return false;
    }

    if (!SDL_DetachVirtualJoystick(static_cast<SDL_JoystickID>(instanceId))) {
        qCWarning(JoystickSDLLog) << "Failed to destroy virtual joystick" << instanceId << ":" << SDL_GetError();
        return false;
    }

    qCDebug(JoystickSDLLog) << "Destroyed virtual joystick" << instanceId;
    return true;
}

bool JoystickSDL::setVirtualAxis(int axis, int value)
{
    if (!_checkVirtualJoystick("setVirtualAxis")) {
        return false;
    }

    if (!SDL_SetJoystickVirtualAxis(_sdlJoystick, axis, static_cast<Sint16>(value))) {
        qCDebug(JoystickSDLLog) << "Failed to set virtual axis" << axis << ":" << SDL_GetError();
        return false;
    }
    return true;
}

bool JoystickSDL::setVirtualButton(int button, bool down)
{
    if (!_checkVirtualJoystick("setVirtualButton")) {
        return false;
    }

    if (!SDL_SetJoystickVirtualButton(_sdlJoystick, button, down)) {
        qCDebug(JoystickSDLLog) << "Failed to set virtual button" << button << ":" << SDL_GetError();
        return false;
    }
    return true;
}

bool JoystickSDL::setVirtualHat(int hat, quint8 value)
{
    if (!_checkVirtualJoystick("setVirtualHat")) {
        return false;
    }

    if (!SDL_SetJoystickVirtualHat(_sdlJoystick, hat, value)) {
        qCDebug(JoystickSDLLog) << "Failed to set virtual hat" << hat << ":" << SDL_GetError();
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Properties/Capability Detection
//-----------------------------------------------------------------------------

bool JoystickSDL::hasMonoLED() const
{
    return _hasGamepadCapability(SDL_PROP_GAMEPAD_CAP_MONO_LED_BOOLEAN);
}

bool JoystickSDL::hasRGBLED() const
{
    return _hasGamepadCapability(SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN);
}

bool JoystickSDL::hasPlayerLED() const
{
    return _hasGamepadCapability(SDL_PROP_GAMEPAD_CAP_PLAYER_LED_BOOLEAN);
}

//-----------------------------------------------------------------------------
// Connection State
//-----------------------------------------------------------------------------

QString JoystickSDL::connectionState() const
{
    if (_sdlGamepad) {
        return _connectionStateToString(SDL_GetGamepadConnectionState(_sdlGamepad));
    }
    if (_sdlJoystick) {
        return _connectionStateToString(SDL_GetJoystickConnectionState(_sdlJoystick));
    }
    return QString();
}

//-----------------------------------------------------------------------------
// Initial Axis State (for drift detection)
//-----------------------------------------------------------------------------

QVariantMap JoystickSDL::getAxisInitialState(int axis) const
{
    QVariantMap result;
    result[QStringLiteral("valid")] = false;
    result[QStringLiteral("value")] = 0;

    if (!_sdlJoystick || axis < 0) {
        return result;
    }

    Sint16 initialValue = 0;
    if (SDL_GetJoystickAxisInitialState(_sdlJoystick, axis, &initialValue)) {
        result[QStringLiteral("valid")] = true;
        result[QStringLiteral("value")] = static_cast<int>(initialValue);
    }
    return result;
}

//-----------------------------------------------------------------------------
// Per-Device Custom Mapping
//-----------------------------------------------------------------------------

bool JoystickSDL::setMapping(const QString &mapping)
{
    if (!_sdlGamepad || mapping.isEmpty()) {
        return false;
    }

    if (!SDL_SetGamepadMapping(SDL_GetJoystickID(_sdlJoystick), qPrintable(mapping))) {
        qCDebug(JoystickSDLLog) << "Failed to set gamepad mapping:" << SDL_GetError();
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Static Pre-Open Device Queries
//-----------------------------------------------------------------------------

QString JoystickSDL::_getNameForInstanceId(int instanceId)
{
    const char *name = SDL_GetGamepadNameForID(static_cast<SDL_JoystickID>(instanceId));
    if (name) {
        return QString::fromUtf8(name);
    }
    name = SDL_GetJoystickNameForID(static_cast<SDL_JoystickID>(instanceId));
    return name ? QString::fromUtf8(name) : QString();
}

QString JoystickSDL::_getPathForInstanceId(int instanceId)
{
    const char *path = SDL_GetGamepadPathForID(static_cast<SDL_JoystickID>(instanceId));
    if (path) {
        return QString::fromUtf8(path);
    }
    path = SDL_GetJoystickPathForID(static_cast<SDL_JoystickID>(instanceId));
    return path ? QString::fromUtf8(path) : QString();
}

QString JoystickSDL::_getGUIDForInstanceId(int instanceId)
{
    SDL_GUID guid = SDL_GetJoystickGUIDForID(static_cast<SDL_JoystickID>(instanceId));
    char guidStr[33];
    SDL_GUIDToString(guid, guidStr, sizeof(guidStr));
    return QString::fromUtf8(guidStr);
}

int JoystickSDL::_getVendorForInstanceId(int instanceId)
{
    Uint16 vendor = SDL_GetGamepadVendorForID(static_cast<SDL_JoystickID>(instanceId));
    if (vendor != 0) {
        return vendor;
    }
    return SDL_GetJoystickVendorForID(static_cast<SDL_JoystickID>(instanceId));
}

int JoystickSDL::_getProductForInstanceId(int instanceId)
{
    Uint16 product = SDL_GetGamepadProductForID(static_cast<SDL_JoystickID>(instanceId));
    if (product != 0) {
        return product;
    }
    return SDL_GetJoystickProductForID(static_cast<SDL_JoystickID>(instanceId));
}

int JoystickSDL::_getProductVersionForInstanceId(int instanceId)
{
    Uint16 version = SDL_GetGamepadProductVersionForID(static_cast<SDL_JoystickID>(instanceId));
    if (version != 0) {
        return version;
    }
    return SDL_GetJoystickProductVersionForID(static_cast<SDL_JoystickID>(instanceId));
}

QString JoystickSDL::_getTypeForInstanceId(int instanceId)
{
    SDL_GamepadType type = SDL_GetGamepadTypeForID(static_cast<SDL_JoystickID>(instanceId));
    return _gamepadTypeToString(type);
}

QString JoystickSDL::_getRealTypeForInstanceId(int instanceId)
{
    SDL_GamepadType type = SDL_GetRealGamepadTypeForID(static_cast<SDL_JoystickID>(instanceId));
    return _gamepadTypeToString(type);
}

int JoystickSDL::_getPlayerIndexForInstanceId(int instanceId)
{
    int index = SDL_GetGamepadPlayerIndexForID(static_cast<SDL_JoystickID>(instanceId));
    if (index >= 0) {
        return index;
    }
    return SDL_GetJoystickPlayerIndexForID(static_cast<SDL_JoystickID>(instanceId));
}

//-----------------------------------------------------------------------------
// Static Type/String Conversions
//-----------------------------------------------------------------------------

// Returns SDL's internal technical names (e.g., "xbox360", "ps5").
// For user-friendly display names (e.g., "Xbox 360", "PlayStation 5"), use _gamepadTypeEnumToString().
QString JoystickSDL::_gamepadTypeToString(int type)
{
    const char *str = SDL_GetGamepadStringForType(static_cast<SDL_GamepadType>(type));
    return str ? QString::fromUtf8(str) : QString();
}

int JoystickSDL::_gamepadTypeFromString(const QString &str)
{
    if (str.isEmpty()) {
        return SDL_GAMEPAD_TYPE_UNKNOWN;
    }
    return SDL_GetGamepadTypeFromString(qPrintable(str));
}

QString JoystickSDL::_gamepadAxisToString(int axis)
{
    const char *str = SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(axis));
    return str ? QString::fromUtf8(str) : QString();
}

int JoystickSDL::_gamepadAxisFromString(const QString &str)
{
    if (str.isEmpty()) {
        return SDL_GAMEPAD_AXIS_INVALID;
    }
    return SDL_GetGamepadAxisFromString(qPrintable(str));
}

QString JoystickSDL::_gamepadButtonToString(int button)
{
    const char *str = SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
    return str ? QString::fromUtf8(str) : QString();
}

int JoystickSDL::_gamepadButtonFromString(const QString &str)
{
    if (str.isEmpty()) {
        return SDL_GAMEPAD_BUTTON_INVALID;
    }
    return SDL_GetGamepadButtonFromString(qPrintable(str));
}

//-----------------------------------------------------------------------------
// Static Thread Safety
//-----------------------------------------------------------------------------

void JoystickSDL::_lockJoysticks()
{
    SDL_LockJoysticks();
}

void JoystickSDL::_unlockJoysticks()
{
    SDL_UnlockJoysticks();
}

//-----------------------------------------------------------------------------
// Virtual Joystick Enhancements
//-----------------------------------------------------------------------------

bool JoystickSDL::setVirtualBall(int ball, int dx, int dy)
{
    if (!_checkVirtualJoystick("setVirtualBall")) {
        return false;
    }

    if (!SDL_SetJoystickVirtualBall(_sdlJoystick, ball, static_cast<Sint16>(dx), static_cast<Sint16>(dy))) {
        qCDebug(JoystickSDLLog) << "Failed to set virtual ball" << ball << ":" << SDL_GetError();
        return false;
    }
    return true;
}

bool JoystickSDL::setVirtualTouchpad(int touchpad, int finger, bool down, float x, float y, float pressure)
{
    if (!_checkVirtualJoystick("setVirtualTouchpad")) {
        return false;
    }

    if (!SDL_SetJoystickVirtualTouchpad(_sdlJoystick, touchpad, finger, down, x, y, pressure)) {
        qCDebug(JoystickSDLLog) << "Failed to set virtual touchpad" << touchpad << "finger" << finger << ":" << SDL_GetError();
        return false;
    }
    return true;
}

bool JoystickSDL::sendVirtualSensorData(int sensorType, float x, float y, float z)
{
    if (!_checkVirtualJoystick("sendVirtualSensorData")) {
        return false;
    }

    const float data[3] = {x, y, z};
    if (!SDL_SendJoystickVirtualSensorData(_sdlJoystick, static_cast<SDL_SensorType>(sensorType), SDL_GetTicksNS(), data, 3)) {
        qCDebug(JoystickSDLLog) << "Failed to send virtual sensor data:" << SDL_GetError();
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Static Event/Polling Control
//-----------------------------------------------------------------------------

void JoystickSDL::_setJoystickEventsEnabled(bool enabled)
{
    SDL_SetJoystickEventsEnabled(enabled);
}

bool JoystickSDL::_joystickEventsEnabled()
{
    return SDL_JoystickEventsEnabled();
}

void JoystickSDL::_setGamepadEventsEnabled(bool enabled)
{
    SDL_SetGamepadEventsEnabled(enabled);
}

bool JoystickSDL::_gamepadEventsEnabled()
{
    return SDL_GamepadEventsEnabled();
}

void JoystickSDL::_updateJoysticks()
{
    SDL_UpdateJoysticks();
}

void JoystickSDL::_updateGamepads()
{
    SDL_UpdateGamepads();
}

//-----------------------------------------------------------------------------
// Static Utility
//-----------------------------------------------------------------------------

int JoystickSDL::_getInstanceIdFromPlayerIndex(int playerIndex)
{
    SDL_Gamepad *gamepad = SDL_GetGamepadFromPlayerIndex(playerIndex);
    if (gamepad) {
        SDL_JoystickID id = SDL_GetGamepadID(gamepad);
        return static_cast<int>(id);
    }

    SDL_Joystick *joystick = SDL_GetJoystickFromPlayerIndex(playerIndex);
    if (joystick) {
        SDL_JoystickID id = SDL_GetJoystickID(joystick);
        return static_cast<int>(id);
    }

    return -1;
}

//-----------------------------------------------------------------------------
// GUID Info Decoding
//-----------------------------------------------------------------------------

QVariantMap JoystickSDL::_getGUIDInfo(const QString &guid)
{
    QVariantMap result;
    result[QStringLiteral("valid")] = false;
    result[QStringLiteral("vendor")] = 0;
    result[QStringLiteral("product")] = 0;
    result[QStringLiteral("version")] = 0;
    result[QStringLiteral("crc16")] = 0;

    if (guid.isEmpty()) {
        return result;
    }

    SDL_GUID sdlGuid = SDL_StringToGUID(qPrintable(guid));
    Uint16 vendor = 0, product = 0, version = 0, crc16 = 0;
    SDL_GetJoystickGUIDInfo(sdlGuid, &vendor, &product, &version, &crc16);

    result[QStringLiteral("valid")] = true;
    result[QStringLiteral("vendor")] = static_cast<int>(vendor);
    result[QStringLiteral("product")] = static_cast<int>(product);
    result[QStringLiteral("version")] = static_cast<int>(version);
    result[QStringLiteral("crc16")] = static_cast<int>(crc16);

    return result;
}

//-----------------------------------------------------------------------------
// Bulk Mapping Loading
//-----------------------------------------------------------------------------

int JoystickSDL::_addMappingsFromFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return -1;
    }

    int count = SDL_AddGamepadMappingsFromFile(qPrintable(filePath));
    if (count < 0) {
        qCDebug(JoystickSDLLog) << "Failed to load mappings from" << filePath << ":" << SDL_GetError();
    } else {
        qCDebug(JoystickSDLLog) << "Loaded" << count << "mappings from" << filePath;
    }
    return count;
}
