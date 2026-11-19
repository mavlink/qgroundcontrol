#include "SDLJoystick.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QTextStream>
#include <QtCore/QStandardPaths>

#include <SDL3/SDL.h>

QGC_LOGGING_CATEGORY(SDLJoystickLog, "qgc.utilities.sdl.joystick")

namespace SDLJoystick {

// ============================================================================
// Internal Helpers
// ============================================================================

static QHash<int, QString> s_virtualJoystickNames;

static void loadGamepadMappings()
{
    const auto loadMappingsFromFile = [](const QString &path, const char *description) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        qCDebug(SDLJoystickLog) << "Loading gamepad mappings from" << description;
        const int count = addMappingsFromData(file.readAll());
        qCDebug(SDLJoystickLog) << "Loaded" << count << "mappings from" << description;
        return true;
    };

    // Load bundled database
    if (!loadMappingsFromFile(QStringLiteral(":/gamecontrollerdb.txt"), "bundled database")) {
        qCWarning(SDLJoystickLog) << "Couldn't load bundled gamepad mapping database";
    }

    // Load user custom mappings
    const QString userMappingsPath = userMappingsFilePath();
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
    s_virtualJoystickNames.clear();
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
    // Events are handled by event watches when they are generated. Nothing reads them from SDL's queue, which
    // would otherwise fill up and stay full.
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
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
    return QString::fromUtf8(SDL_GetGamepadStringForType(static_cast<SDL_GamepadType>(type)));
}

int gamepadTypeFromString(const QString &str)
{
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
    return QString::fromUtf8(SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(axis)));
}

int gamepadAxisFromString(const QString &str)
{
    return SDL_GetGamepadAxisFromString(qPrintable(str));
}

QString gamepadButtonToString(int button)
{
    return QString::fromUtf8(SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button)));
}

int gamepadButtonFromString(const QString &str)
{
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
    Uint16 vendor = 0, product = 0, version = 0, crc16 = 0;
    if (!guid.isEmpty()) {
        SDL_GetJoystickGUIDInfo(SDL_StringToGUID(qPrintable(guid)), &vendor, &product, &version, &crc16);
    }

    return {
        {QStringLiteral("valid"), !guid.isEmpty()},
        {QStringLiteral("vendor"), static_cast<int>(vendor)},
        {QStringLiteral("product"), static_cast<int>(product)},
        {QStringLiteral("version"), static_cast<int>(version)},
        {QStringLiteral("crc16"), static_cast<int>(crc16)},
    };
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

int addMappingsFromData(const QByteArray& data)
{
    if (data.isEmpty()) {
        return 0;
    }

    // Unlike SDL_AddGamepadMapping(), this skips the entries for other platforms. SDL copies the data and closes
    // the stream before it returns.
    SDL_IOStream* stream = SDL_IOFromConstMem(data.constData(), static_cast<size_t>(data.size()));
    int count = stream ? SDL_AddGamepadMappingsFromIO(stream, true) : -1;
    if (count < 0) {
        qCWarning(SDLJoystickLog) << "Couldn't add gamepad mappings:" << SDL_GetError();
        return -1;
    }

    // It also skips the entries without a platform, which apply to every platform.
    for (const QByteArray& rawLine : data.split('\n')) {
        const QByteArray line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.contains("platform:")) {
            continue;
        }
        const int added = SDL_AddGamepadMapping(line.constData());
        if (added == -1) {
            qCWarning(SDLJoystickLog) << "Couldn't add gamepad mapping:" << SDL_GetError();
        } else {
            count += added;
        }
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
            // Skip if same GUID (we'll add the new mapping)
            if (!line.isEmpty() && (line.startsWith('#') || !line.startsWith(guid + ','))) {
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
        out << line << "\n";
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
    // Finds a gamepad too: SDL looks up the gamepad of a player index through its joystick and has that joystick's id
    SDL_Joystick *joystick = SDL_GetJoystickFromPlayerIndex(playerIndex);
    return joystick ? static_cast<int>(SDL_GetJoystickID(joystick)) : -1;
}

// ============================================================================
// Pre-Open Device Queries
// ============================================================================

// The path, vendor, product, version and player index come from SDL's joystick functions, for a gamepad too:
// SDL_GetGamepad...ForID() only forwards to SDL_GetJoystick...ForID() (SDL 3.4).

QString getNameForInstanceId(int instanceId)
{
    const char *name = SDL_GetGamepadNameForID(static_cast<SDL_JoystickID>(instanceId));
    if (name && *name) {
        return QString::fromUtf8(name);
    }
    name = SDL_GetJoystickNameForID(static_cast<SDL_JoystickID>(instanceId));
    if (name && *name) {
        return QString::fromUtf8(name);
    }

    // SDL may report an empty name for freshly-created virtual devices on some platforms.
    return s_virtualJoystickNames.value(instanceId);
}

QString getPathForInstanceId(int instanceId)
{
    return QString::fromUtf8(SDL_GetJoystickPathForID(static_cast<SDL_JoystickID>(instanceId)));
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
    return SDL_GetJoystickVendorForID(static_cast<SDL_JoystickID>(instanceId));
}

int getProductForInstanceId(int instanceId)
{
    return SDL_GetJoystickProductForID(static_cast<SDL_JoystickID>(instanceId));
}

int getProductVersionForInstanceId(int instanceId)
{
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
    const QByteArray utf8Name = name.toUtf8();
    desc.name = utf8Name.constData();

    SDL_JoystickID instanceId = SDL_AttachVirtualJoystick(&desc);
    if (instanceId == 0) {
        qCWarning(SDLJoystickLog) << "Failed to create virtual joystick:" << SDL_GetError();
        return -1;
    }

    s_virtualJoystickNames.insert(static_cast<int>(instanceId), name);

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

    s_virtualJoystickNames.remove(instanceId);

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

QVariantMap findBinding(SDL_Gamepad *gamepad,
                        const std::function<bool(const SDL_GamepadBinding *)> &matchFunc)
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
            break;
        }
    }

    SDL_free(bindings);
    return result;
}

} // namespace SDLJoystick
