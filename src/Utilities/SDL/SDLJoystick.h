#pragma once

#include <QtCore/QString>
#include <QtCore/QVariantMap>

#include <SDL3/SDL.h>

/**
 * SDL joystick/gamepad utilities
 * Joystick-specific SDL functionality
 */
namespace SDLJoystick {

// ============================================================================
// Initialization
// ============================================================================

/// Initialize SDL joystick/gamepad subsystems with QGC-specific hints
bool init();

/// Shutdown SDL joystick/gamepad subsystems
void shutdown();

/// Check if SDL joystick subsystem is initialized
bool isInitialized();

// ============================================================================
// Event Control
// ============================================================================

/// Pump SDL events (call periodically)
void pumpEvents();

/// Enable/disable joystick event processing
void setJoystickEventsEnabled(bool enabled);
bool joystickEventsEnabled();

/// Enable/disable gamepad event processing
void setGamepadEventsEnabled(bool enabled);
bool gamepadEventsEnabled();

/// Update joystick/gamepad state
void updateJoysticks();
void updateGamepads();

// ============================================================================
// Thread Safety
// ============================================================================

/// Lock joystick access for thread-safe operations
void lockJoysticks();
void unlockJoysticks();

/// RAII lock guard for joysticks
class JoystickLock {
public:
    JoystickLock() { lockJoysticks(); }
    ~JoystickLock() { unlockJoysticks(); }
    JoystickLock(const JoystickLock&) = delete;
    JoystickLock& operator=(const JoystickLock&) = delete;
};

// ============================================================================
// Type/String Conversions
// ============================================================================

/// Gamepad type conversions
QString gamepadTypeToString(int type);
int gamepadTypeFromString(const QString &str);
QString gamepadTypeDisplayName(int type);

/// Gamepad axis conversions
QString gamepadAxisToString(int axis);
int gamepadAxisFromString(const QString &str);

/// Gamepad button conversions
QString gamepadButtonToString(int button);
int gamepadButtonFromString(const QString &str);

/// Connection state to string
QString connectionStateToString(int state);

// ============================================================================
// GUID Utilities
// ============================================================================

/// Parse GUID info (vendor, product, version, crc16)
QVariantMap getGUIDInfo(const QString &guid);

/// Get mapping string for a GUID
QString getMappingForGUID(const QString &guid);

// ============================================================================
// Mapping Management
// ============================================================================

/// Add gamepad mappings from a file
int addMappingsFromFile(const QString &filePath);

/// Add a single mapping string
bool addMapping(const QString &mapping);

/// Add a mapping and persist it to user's config file
bool addMappingPersistent(const QString &mapping);

/// Reload all gamepad mappings
bool reloadMappings();

/// Get path to user's custom mappings file
QString userMappingsFilePath();

// ============================================================================
// Player Index
// ============================================================================

/// Get instance ID from player index
int getInstanceIdFromPlayerIndex(int playerIndex);

// ============================================================================
// Pre-Open Device Queries (query device info without opening)
// ============================================================================

/// Get device name for instance ID
QString getNameForInstanceId(int instanceId);

/// Get device path for instance ID
QString getPathForInstanceId(int instanceId);

/// Get GUID string for instance ID
QString getGUIDForInstanceId(int instanceId);

/// Get vendor ID for instance ID
int getVendorForInstanceId(int instanceId);

/// Get product ID for instance ID
int getProductForInstanceId(int instanceId);

/// Get product version for instance ID
int getProductVersionForInstanceId(int instanceId);

/// Get gamepad type string for instance ID
QString getTypeForInstanceId(int instanceId);

/// Get real (underlying) gamepad type string for instance ID
QString getRealTypeForInstanceId(int instanceId);

/// Get player index for instance ID
int getPlayerIndexForInstanceId(int instanceId);

/// Get connection state for instance ID (Wired/Wireless/Unknown)
/// Note: Requires the device to be opened first, returns "Invalid" otherwise
QString getConnectionStateForInstanceId(int instanceId);

// ============================================================================
// Virtual Joystick
// ============================================================================

/// Create a virtual joystick
int createVirtualJoystick(const QString &name, int axisCount, int buttonCount, int hatCount);

/// Destroy a virtual joystick
bool destroyVirtualJoystick(int instanceId);

/// Check if a joystick is virtual
bool isVirtualJoystick(int instanceId);

// ============================================================================
// Gamepad Binding Utilities
// ============================================================================

/// Populate a QVariantMap with binding information from SDL_GamepadBinding
void populateBindingResult(QVariantMap &result, const SDL_GamepadBinding *binding);

/// Find a gamepad binding matching the given predicate
/// @param gamepad The SDL gamepad to query
/// @param matchFunc A callable that takes (const SDL_GamepadBinding*) and returns bool
/// @return QVariantMap with binding info if found, or {"valid": false} if not
template<typename MatchFunc>
inline QVariantMap findBinding(SDL_Gamepad *gamepad, MatchFunc matchFunc)
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

} // namespace SDLJoystick
