#include "JoystickSDL.h"
#include "JoystickManager.h"
#include "SDLJoystick.h"
#include <QtCore/QLoggingCategory>

#include <QtCore/QMetaObject>
#include <QtCore/QThread>

#include <array>

#include <SDL3/SDL.h>

Q_STATIC_LOGGING_CATEGORY(JoystickSDLLog, "Joystick.JoystickSDL")

/// Discovery cache - main thread only, cleared in shutdown()
static QMap<QString, Joystick*> s_discoveryCache;

/// SDL event watcher - uses Qt::QueuedConnection for thread safety
static bool sdlEventWatcher(void *userdata, SDL_Event *event)
{
    Q_UNUSED(userdata);

    JoystickManager *manager = JoystickManager::instance();
    if (!manager) {
        return true;
    }

    switch (event->type) {
    case SDL_EVENT_JOYSTICK_ADDED:
        qCInfo(JoystickSDLLog) << "SDL event: Joystick added, instance ID:" << event->jdevice.which;
        QMetaObject::invokeMethod(manager, "_checkForAddedOrRemovedJoysticks", Qt::QueuedConnection);
        break;
    case SDL_EVENT_JOYSTICK_REMOVED:
        qCInfo(JoystickSDLLog) << "SDL event: Joystick removed, instance ID:" << event->jdevice.which;
        QMetaObject::invokeMethod(manager, "_checkForAddedOrRemovedJoysticks", Qt::QueuedConnection);
        break;
    // Gamepad events ignored - SDL fires both joystick and gamepad events for gamepads
    case SDL_EVENT_GAMEPAD_ADDED:
        qCDebug(JoystickSDLLog) << "SDL event: Gamepad added (ignored, handled via joystick event), instance ID:" << event->gdevice.which;
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        qCDebug(JoystickSDLLog) << "SDL event: Gamepad removed (ignored, handled via joystick event), instance ID:" << event->gdevice.which;
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

bool JoystickSDL::init()
{
    if (!SDLJoystick::init()) {
        return false;
    }

    SDL_AddEventWatch(sdlEventWatcher, nullptr);
    return true;
}

void JoystickSDL::shutdown()
{
    SDL_RemoveEventWatch(sdlEventWatcher, nullptr);
    SDLJoystick::shutdown();

    s_discoveryCache.clear();
}

QMap<QString, Joystick*> JoystickSDL::discover()
{
    Q_ASSERT(QThread::isMainThread());

    QMap<QString, Joystick*> current;

    qCDebug(JoystickSDLLog) << "Discovering joysticks";

    // Required on Android before SDL_GetJoysticks() returns devices
    SDLJoystick::pumpEvents();

    SDLJoystick::JoystickLock lock;

    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    if (!ids) {
        qCWarning(JoystickSDLLog) << "SDL_GetJoysticks failed:" << SDL_GetError();
        return current;
    }

    qCDebug(JoystickSDLLog) << "SDL_GetJoysticks returned" << count << "joysticks";
    for (int n = 0; n < count; ++n) {
        qCDebug(JoystickSDLLog) << "  [" << n << "] ID:" << ids[n]
                                << "Name:" << SDL_GetJoystickNameForID(ids[n])
                                << "IsGamepad:" << SDL_IsGamepad(ids[n]);
    }

    for (int n = 0; n < count; ++n) {
        const SDL_JoystickID jid = ids[n];
        QString baseName = QString::fromUtf8(SDL_GetJoystickNameForID(jid));
        QString name = baseName;

        // Check cache by instance ID (reconnection of same device)
        bool foundInCache = false;
        for (auto it = s_discoveryCache.begin(); it != s_discoveryCache.end(); ++it) {
            auto *cachedJs = static_cast<JoystickSDL*>(it.value());
            if (cachedJs->instanceId() == jid) {
                name = it.key();
                current[name] = cachedJs;
                s_discoveryCache.erase(it);
                foundInCache = true;
                break;
            }
        }
        if (foundInCache) {
            continue;
        }

        // Check cache by name (for joysticks that were disconnected and reconnected)
        if (s_discoveryCache.contains(name) && !current.contains(name)) {
            current[name] = s_discoveryCache[name];
            auto *js = static_cast<JoystickSDL*>(current[name]);
            js->setInstanceId(jid);
            (void) s_discoveryCache.remove(name);
            continue;
        }

        // Handle duplicate names by appending a number
        int duplicateIndex = 2;
        while (current.contains(name) || s_discoveryCache.contains(name)) {
            name = QStringLiteral("%1 #%2").arg(baseName).arg(duplicateIndex++);
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

        qCDebug(JoystickSDLLog) << "Creating JoystickSDL for" << name << "jid:" << jid;

        current[name] = new JoystickSDL(name,
                                        gamepadAxes,
                                        nonGamepadAxes,
                                        qMax(0, buttonCount),
                                        qMax(0, hatCount),
                                        jid);
    }

    SDL_free(ids);

    qCDebug(JoystickSDLLog) << "Discovered" << current.size() << "joysticks:";
    for (auto it = current.begin(); it != current.end(); ++it) {
        auto *js = static_cast<JoystickSDL*>(it.value());
        qCDebug(JoystickSDLLog) << "  " << it.key() << "instanceId:" << js->instanceId();
    }

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
    if (_sdlGamepad) {
        return SDLJoystick::connectionStateToString(SDL_GetGamepadConnectionState(_sdlGamepad));
    }
    if (_sdlJoystick) {
        return SDLJoystick::connectionStateToString(SDL_GetJoystickConnectionState(_sdlJoystick));
    }
    return QString();
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
        return SDLJoystick::gamepadTypeDisplayName(SDL_GetGamepadType(_sdlGamepad));
    }
    return QString();
}

//-----------------------------------------------------------------------------
// Control Labels
//-----------------------------------------------------------------------------

QString JoystickSDL::axisLabel(int axis) const
{
    if (axis < 0) {
        return tr("Axis %1").arg(axis);
    }

    // Check gamepad axes first
    if (_sdlGamepad && axis < _gamepadAxes.length()) {
        const char *label = SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(_gamepadAxes[axis]));
        if (label) {
            return QString::fromUtf8(label);
        }
    }

    // Check non-gamepad axes
    const int nonGamepadIdx = axis - _gamepadAxes.length();
    if (nonGamepadIdx >= 0 && nonGamepadIdx < _nonGamepadAxes.length()) {
        // Non-gamepad axes don't have standard labels, use raw joystick axis number
        return tr("Axis %1").arg(_nonGamepadAxes[nonGamepadIdx]);
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

QVariantMap JoystickSDL::getAxisBinding(int axis) const
{
    QVariantMap result;
    result[QStringLiteral("valid")] = false;

    if (axis < 0) {
        return result;
    }

    // Gamepad axes have bindings
    if (_sdlGamepad && axis < _gamepadAxes.length()) {
        const SDL_GamepadAxis gamepadAxis = static_cast<SDL_GamepadAxis>(_gamepadAxes[axis]);
        return SDLJoystick::findBinding(_sdlGamepad, [gamepadAxis](const SDL_GamepadBinding *binding) {
            return binding->output_type == SDL_GAMEPAD_BINDTYPE_AXIS &&
                   binding->output.axis.axis == gamepadAxis;
        });
    }

    // Non-gamepad axes map directly to joystick axes (no binding indirection)
    const int nonGamepadIdx = axis - _gamepadAxes.length();
    if (nonGamepadIdx >= 0 && nonGamepadIdx < _nonGamepadAxes.length()) {
        result[QStringLiteral("valid")] = true;
        result[QStringLiteral("inputType")] = static_cast<int>(SDL_GAMEPAD_BINDTYPE_AXIS);
        result[QStringLiteral("inputAxis")] = _nonGamepadAxes[nonGamepadIdx];
        result[QStringLiteral("inputAxisMin")] = -32768;
        result[QStringLiteral("inputAxisMax")] = 32767;
        result[QStringLiteral("direct")] = true;  // Indicates no gamepad mapping, direct joystick access
        return result;
    }

    return result;
}

QVariantMap JoystickSDL::getButtonBinding(int button) const
{
    if (!_sdlGamepad || button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT) {
        QVariantMap result;
        result[QStringLiteral("valid")] = false;
        return result;
    }

    const SDL_GamepadButton gamepadButton = static_cast<SDL_GamepadButton>(button);
    return SDLJoystick::findBinding(_sdlGamepad, [gamepadButton](const SDL_GamepadBinding *binding) {
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
    // Use cached data from events if available (more efficient than polling)
    if (_gyroDataCached) {
        return _cachedGyroData;
    }

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
    // Use cached data from events if available (more efficient than polling)
    if (_accelDataCached) {
        return _cachedAccelData;
    }

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
        return SDLJoystick::gamepadTypeDisplayName(SDL_GetRealGamepadType(_sdlGamepad));
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
        qCWarning(JoystickSDLLog) << "Failed to open haptic device:" << SDL_GetError();
        return false;
    }

    if (!SDL_InitHapticRumble(_sdlHaptic)) {
        qCWarning(JoystickSDLLog) << "Failed to init haptic rumble:" << SDL_GetError();
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
        qCWarning(JoystickSDLLog) << "Failed to play haptic rumble:" << SDL_GetError();
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
    return connectionType();
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

    // Validate and clamp touchpad coordinates to valid range [0.0, 1.0]
    const float clampedX = qBound(0.0f, x, 1.0f);
    const float clampedY = qBound(0.0f, y, 1.0f);
    const float clampedPressure = qBound(0.0f, pressure, 1.0f);

    if (x != clampedX || y != clampedY || pressure != clampedPressure) {
        qCDebug(JoystickSDLLog) << "Virtual touchpad coordinates clamped: x" << x << "->" << clampedX
                                << "y" << y << "->" << clampedY
                                << "pressure" << pressure << "->" << clampedPressure;
    }

    if (!SDL_SetJoystickVirtualTouchpad(_sdlJoystick, touchpad, finger, down, clampedX, clampedY, clampedPressure)) {
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
// Cached Sensor Data (Event-Driven Updates)
//-----------------------------------------------------------------------------

void JoystickSDL::updateCachedGyroData(const QVector3D &data)
{
    _cachedGyroData = data;
    _gyroDataCached = true;
    emit gyroscopeDataUpdated(data);
}

void JoystickSDL::updateCachedAccelData(const QVector3D &data)
{
    _cachedAccelData = data;
    _accelDataCached = true;
    emit accelerometerDataUpdated(data);
}

void JoystickSDL::checkConnectionStateChanged()
{
    const QString currentState = connectionState();
    if (currentState != _lastConnectionState && !currentState.isEmpty()) {
        qCDebug(JoystickSDLLog) << "Connection state changed:" << _lastConnectionState << "->" << currentState;
        _lastConnectionState = currentState;
        emit connectionStateChanged(currentState);
    }
}

QVariantList JoystickSDL::detectAxisDrift(int threshold) const
{
    QVariantList driftingAxes;

    const int totalAxes = _gamepadAxes.length() + _nonGamepadAxes.length();
    for (int i = 0; i < totalAxes; ++i) {
        const int currentValue = _getAxisValue(i);

        // Check if axis is significantly off-center (not near 0)
        // Threshold default of 8000 is ~25% of full range
        if (qAbs(currentValue) > threshold) {
            QVariantMap axisInfo;
            axisInfo[QStringLiteral("axis")] = i;
            axisInfo[QStringLiteral("label")] = axisLabel(i);
            axisInfo[QStringLiteral("value")] = currentValue;
            axisInfo[QStringLiteral("percentage")] = qRound(qAbs(currentValue) * 100.0 / 32767.0);
            driftingAxes.append(axisInfo);
        }
    }

    return driftingAxes;
}
