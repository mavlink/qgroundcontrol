#include "SDLJoystick.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QStandardPaths>

#include <SDL3/SDL.h>

QGC_LOGGING_CATEGORY(SDLJoystickLog, "qgc.utilities.sdl.joystick")

namespace SDLJoystick {

// ============================================================================
// Internal Helpers
// ============================================================================

static void loadGamepadMappings()
{
    const auto loadMappingsFromFile = [](const QString &path, const char *description) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        qCDebug(SDLJoystickLog) << "Loading gamepad mappings from" << description;
        int count = 0;
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            if (line.startsWith('#') || line.isEmpty()) {
                continue;
            }
            if (SDL_AddGamepadMapping(qPrintable(line)) == -1) {
                qCWarning(SDLJoystickLog) << "Couldn't add gamepad mapping:" << SDL_GetError();
            } else {
                ++count;
            }
        }
        qCDebug(SDLJoystickLog) << "Loaded" << count << "mappings from" << description;
        return true;
    };

    // Load bundled database
    if (!loadMappingsFromFile(QStringLiteral(":/gamecontrollerdb.txt"), "bundled database")) {
        qCWarning(SDLJoystickLog) << "Couldn't load bundled gamepad mapping database";
    }

    // Load user custom mappings
    const QString userMappingsPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                   + QStringLiteral("/gamecontrollerdb.txt");
    if (loadMappingsFromFile(userMappingsPath, "user config")) {
        qCInfo(SDLJoystickLog) << "Loaded user gamepad mappings from" << userMappingsPath;
    }
}

// ============================================================================
// Initialization
// ============================================================================

bool init()
{
    qCDebug(SDLJoystickLog) << "Initializing SDL joystick subsystem";

    // Allow joystick events when app is in background (critical for ground stations)
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

#ifdef Q_OS_ANDROID
    // On Android, prevent SDL from trying to load the internal gamepad config file
    SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE, "");
#endif

    // Enable Steam controller support
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK, "1");

    // Enable HIDAPI driver for better controller support
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

    // Enable sensor fusion
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_SENSOR_FUSION, "1");

#ifdef Q_OS_WIN
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "1");
#endif

#ifdef Q_OS_LINUX
    SDL_SetHint(SDL_HINT_JOYSTICK_LINUX_DEADZONES, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_LINUX_DIGITAL_HATS, "1");
#endif

    // Enable threaded joystick processing
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");

    // Enable enhanced reports for gyro/accel data
    SDL_SetHint(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "1");

    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
        qCWarning(SDLJoystickLog) << "Failed to initialize SDL:" << SDL_GetError();
        return false;
    }

    loadGamepadMappings();

    qCDebug(SDLJoystickLog) << "SDL joystick subsystem initialized successfully";
    return true;
}

void shutdown()
{
    qCDebug(SDLJoystickLog) << "Shutting down SDL joystick subsystem";
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK);
}

bool isInitialized()
{
    return SDL_WasInit(SDL_INIT_JOYSTICK) != 0;
}

// ============================================================================
// Event Control
// ============================================================================

void pumpEvents()
{
    SDL_PumpEvents();
    SDL_UpdateJoysticks();
}

void setJoystickEventsEnabled(bool enabled)
{
    SDL_SetJoystickEventsEnabled(enabled);
}

bool joystickEventsEnabled()
{
    return SDL_JoystickEventsEnabled();
}

void setGamepadEventsEnabled(bool enabled)
{
    SDL_SetGamepadEventsEnabled(enabled);
}

bool gamepadEventsEnabled()
{
    return SDL_GamepadEventsEnabled();
}

void updateJoysticks()
{
    SDL_UpdateJoysticks();
}

void updateGamepads()
{
    SDL_UpdateGamepads();
}

// ============================================================================
// Thread Safety
// ============================================================================

void lockJoysticks()
{
    SDL_LockJoysticks();
}

void unlockJoysticks()
{
    SDL_UnlockJoysticks();
}

// ============================================================================
// Type/String Conversions
// ============================================================================

QString gamepadTypeToString(int type)
{
    const char *str = SDL_GetGamepadStringForType(static_cast<SDL_GamepadType>(type));
    return str ? QString::fromUtf8(str) : QString();
}

int gamepadTypeFromString(const QString &str)
{
    if (str.isEmpty()) {
        return SDL_GAMEPAD_TYPE_UNKNOWN;
    }
    return SDL_GetGamepadTypeFromString(qPrintable(str));
}

QString gamepadTypeDisplayName(int type)
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

QString gamepadAxisToString(int axis)
{
    const char *str = SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(axis));
    return str ? QString::fromUtf8(str) : QString();
}

int gamepadAxisFromString(const QString &str)
{
    if (str.isEmpty()) {
        return SDL_GAMEPAD_AXIS_INVALID;
    }
    return SDL_GetGamepadAxisFromString(qPrintable(str));
}

QString gamepadButtonToString(int button)
{
    const char *str = SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
    return str ? QString::fromUtf8(str) : QString();
}

int gamepadButtonFromString(const QString &str)
{
    if (str.isEmpty()) {
        return SDL_GAMEPAD_BUTTON_INVALID;
    }
    return SDL_GetGamepadButtonFromString(qPrintable(str));
}

QString connectionStateToString(int state)
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

// ============================================================================
// GUID Utilities
// ============================================================================

QVariantMap getGUIDInfo(const QString &guid)
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

QString getMappingForGUID(const QString &guid)
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

// ============================================================================
// Mapping Management
// ============================================================================

int addMappingsFromFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return -1;
    }

    int count = SDL_AddGamepadMappingsFromFile(qPrintable(filePath));
    if (count < 0) {
        qCDebug(SDLJoystickLog) << "Failed to load mappings from" << filePath << ":" << SDL_GetError();
    } else {
        qCDebug(SDLJoystickLog) << "Loaded" << count << "mappings from" << filePath;
    }
    return count;
}

bool addMapping(const QString &mapping)
{
    if (mapping.isEmpty()) {
        return false;
    }

    int result = SDL_AddGamepadMapping(qPrintable(mapping));
    if (result == -1) {
        qCWarning(SDLJoystickLog) << "Failed to add gamepad mapping:" << SDL_GetError();
        return false;
    }

    qCDebug(SDLJoystickLog) << "Added gamepad mapping, result:" << result;
    return true;
}

QString userMappingsFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/gamecontrollerdb.txt");
}

bool addMappingPersistent(const QString &mapping)
{
    if (mapping.isEmpty()) {
        return false;
    }

    // First add to SDL's runtime mapping database
    if (!addMapping(mapping)) {
        return false;
    }

    // Extract GUID from mapping (format: "GUID,name,mapping...")
    const int commaPos = mapping.indexOf(',');
    if (commaPos <= 0) {
        qCWarning(SDLJoystickLog) << "Invalid mapping format, cannot extract GUID";
        return false;
    }
    const QString guid = mapping.left(commaPos);

    // Now persist to user's config file
    const QString filePath = userMappingsFilePath();
    QFile file(filePath);

    // Read existing mappings, filtering out any with the same GUID
    QStringList existingMappings;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.isEmpty() || line.startsWith('#')) {
                existingMappings.append(line);
                continue;
            }
            // Skip if same GUID (we'll add the new mapping)
            if (!line.startsWith(guid + ',')) {
                existingMappings.append(line);
            }
        }
        file.close();
    }

    // Write back with new mapping
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCWarning(SDLJoystickLog) << "Failed to open user mappings file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out << "# QGroundControl User Gamepad Mappings\n";
    out << "# Generated automatically - edit with caution\n\n";
    for (const QString &line : existingMappings) {
        if (!line.isEmpty()) {
            out << line << "\n";
        }
    }
    out << mapping << "\n";
    file.close();

    qCInfo(SDLJoystickLog) << "Persisted gamepad mapping for GUID:" << guid << "to" << filePath;
    return true;
}

bool reloadMappings()
{
    // SDL_ReloadGamepadMappings() reloads built-in mappings only.
    // We need to re-apply our bundled and user mappings afterwards.
    // Note: This does NOT call SDL_ReloadGamepadMappings() first because
    // that would reset to SDL defaults, and loadGamepadMappings() adds
    // our mappings on top. If we need a full reset, call both explicitly.
    loadGamepadMappings();

    qCDebug(SDLJoystickLog) << "Reloaded gamepad mappings";
    return true;
}

// ============================================================================
// Player Index
// ============================================================================

int getInstanceIdFromPlayerIndex(int playerIndex)
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

// ============================================================================
// Pre-Open Device Queries
// ============================================================================

QString getNameForInstanceId(int instanceId)
{
    const char *name = SDL_GetGamepadNameForID(static_cast<SDL_JoystickID>(instanceId));
    if (name) {
        return QString::fromUtf8(name);
    }
    name = SDL_GetJoystickNameForID(static_cast<SDL_JoystickID>(instanceId));
    return name ? QString::fromUtf8(name) : QString();
}

QString getPathForInstanceId(int instanceId)
{
    const char *path = SDL_GetGamepadPathForID(static_cast<SDL_JoystickID>(instanceId));
    if (path) {
        return QString::fromUtf8(path);
    }
    path = SDL_GetJoystickPathForID(static_cast<SDL_JoystickID>(instanceId));
    return path ? QString::fromUtf8(path) : QString();
}

QString getGUIDForInstanceId(int instanceId)
{
    SDL_GUID guid = SDL_GetJoystickGUIDForID(static_cast<SDL_JoystickID>(instanceId));
    char guidStr[33];
    SDL_GUIDToString(guid, guidStr, sizeof(guidStr));
    return QString::fromUtf8(guidStr);
}

int getVendorForInstanceId(int instanceId)
{
    Uint16 vendor = SDL_GetGamepadVendorForID(static_cast<SDL_JoystickID>(instanceId));
    if (vendor != 0) {
        return vendor;
    }
    return SDL_GetJoystickVendorForID(static_cast<SDL_JoystickID>(instanceId));
}

int getProductForInstanceId(int instanceId)
{
    Uint16 product = SDL_GetGamepadProductForID(static_cast<SDL_JoystickID>(instanceId));
    if (product != 0) {
        return product;
    }
    return SDL_GetJoystickProductForID(static_cast<SDL_JoystickID>(instanceId));
}

int getProductVersionForInstanceId(int instanceId)
{
    Uint16 version = SDL_GetGamepadProductVersionForID(static_cast<SDL_JoystickID>(instanceId));
    if (version != 0) {
        return version;
    }
    return SDL_GetJoystickProductVersionForID(static_cast<SDL_JoystickID>(instanceId));
}

QString getTypeForInstanceId(int instanceId)
{
    SDL_GamepadType type = SDL_GetGamepadTypeForID(static_cast<SDL_JoystickID>(instanceId));
    return gamepadTypeToString(static_cast<int>(type));
}

QString getRealTypeForInstanceId(int instanceId)
{
    SDL_GamepadType type = SDL_GetRealGamepadTypeForID(static_cast<SDL_JoystickID>(instanceId));
    return gamepadTypeToString(static_cast<int>(type));
}

int getPlayerIndexForInstanceId(int instanceId)
{
    int index = SDL_GetGamepadPlayerIndexForID(static_cast<SDL_JoystickID>(instanceId));
    if (index >= 0) {
        return index;
    }
    return SDL_GetJoystickPlayerIndexForID(static_cast<SDL_JoystickID>(instanceId));
}

QString getConnectionStateForInstanceId(int instanceId)
{
    // Try gamepad first, then joystick
    SDL_JoystickConnectionState state = SDL_GetGamepadConnectionState(SDL_GetGamepadFromID(static_cast<SDL_JoystickID>(instanceId)));
    if (state == SDL_JOYSTICK_CONNECTION_INVALID) {
        state = SDL_GetJoystickConnectionState(SDL_GetJoystickFromID(static_cast<SDL_JoystickID>(instanceId)));
    }
    return connectionStateToString(static_cast<int>(state));
}

// ============================================================================
// Virtual Joystick
// ============================================================================

int createVirtualJoystick(const QString &name, int axisCount, int buttonCount, int hatCount)
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
        qCWarning(SDLJoystickLog) << "Failed to create virtual joystick:" << SDL_GetError();
        return -1;
    }

    qCDebug(SDLJoystickLog) << "Created virtual joystick" << name << "with instance ID" << instanceId;
    return static_cast<int>(instanceId);
}

bool destroyVirtualJoystick(int instanceId)
{
    if (instanceId <= 0) {
        return false;
    }

    if (!SDL_DetachVirtualJoystick(static_cast<SDL_JoystickID>(instanceId))) {
        qCWarning(SDLJoystickLog) << "Failed to destroy virtual joystick" << instanceId << ":" << SDL_GetError();
        return false;
    }

    qCDebug(SDLJoystickLog) << "Destroyed virtual joystick" << instanceId;
    return true;
}

bool isVirtualJoystick(int instanceId)
{
    return SDL_IsJoystickVirtual(static_cast<SDL_JoystickID>(instanceId));
}

// ============================================================================
// Gamepad Binding Utilities
// ============================================================================

void populateBindingResult(QVariantMap &result, const SDL_GamepadBinding *binding)
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

} // namespace SDLJoystick
