#pragma once

#include <QtCore/QString>
#include <QtCore/QVariantMap>

#include <functional>

// Forward-declare SDL types so consumers don't pull <SDL3/SDL.h> through
// this header. Both are typedef'd from struct tags of the same name.
struct SDL_Gamepad;
struct SDL_GamepadBinding;

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
void shutdown();
bool isInitialized();

// ============================================================================
// Event Control
// ============================================================================

/// Pump SDL events, then discard the queued ones: event watches have already handled them (call periodically)
void pumpEvents();

void setJoystickEventsEnabled(bool enabled);
bool joystickEventsEnabled();

void setGamepadEventsEnabled(bool enabled);
bool gamepadEventsEnabled();

void updateJoysticks();
void updateGamepads();

// ============================================================================
// Thread Safety
// ============================================================================

void lockJoysticks();
void unlockJoysticks();

class JoystickLock {
public:
    JoystickLock() { lockJoysticks(); }
    ~JoystickLock() { unlockJoysticks(); }
    Q_DISABLE_COPY(JoystickLock)
};

// ============================================================================
// Type/String Conversions
// ============================================================================

QString gamepadTypeToString(int type);
int gamepadTypeFromString(const QString &str);
QString gamepadTypeDisplayName(int type);

QString gamepadAxisToString(int axis);
int gamepadAxisFromString(const QString &str);

QString gamepadButtonToString(int button);
int gamepadButtonFromString(const QString &str);

QString connectionStateToString(int state);

// ============================================================================
// GUID Utilities
// ============================================================================

/// Parse GUID info (vendor, product, version, crc16)
QVariantMap getGUIDInfo(const QString &guid);

QString getMappingForGUID(const QString &guid);

// ============================================================================
// Mapping Management
// ============================================================================

int addMappingsFromFile(const QString &filePath);

/// Add gamepad mappings from the contents of a mapping database, skipping the entries for other platforms.
/// @return Number of mappings added, or -1 on error.
int addMappingsFromData(const QByteArray& data);

bool addMapping(const QString &mapping);

/// Add a mapping and persist it to user's config file
bool addMappingPersistent(const QString &mapping);

bool reloadMappings();

QString userMappingsFilePath();

// ============================================================================
// Player Index
// ============================================================================

int getInstanceIdFromPlayerIndex(int playerIndex);

// ============================================================================
// Pre-Open Device Queries (query device info without opening)
// ============================================================================

QString getNameForInstanceId(int instanceId);
QString getPathForInstanceId(int instanceId);
QString getGUIDForInstanceId(int instanceId);
int getVendorForInstanceId(int instanceId);
int getProductForInstanceId(int instanceId);
int getProductVersionForInstanceId(int instanceId);
QString getTypeForInstanceId(int instanceId);
QString getRealTypeForInstanceId(int instanceId);
int getPlayerIndexForInstanceId(int instanceId);

/// Get connection state for instance ID (Wired/Wireless/Unknown)
/// Note: Requires the device to be opened first, returns "Invalid" otherwise
QString getConnectionStateForInstanceId(int instanceId);

// ============================================================================
// Virtual Joystick
// ============================================================================

int createVirtualJoystick(const QString &name, int axisCount, int buttonCount, int hatCount);
bool destroyVirtualJoystick(int instanceId);
bool isVirtualJoystick(int instanceId);

// ============================================================================
// Gamepad Binding Utilities
// ============================================================================

void populateBindingResult(QVariantMap &result, const SDL_GamepadBinding *binding);

/// Find a gamepad binding matching the given predicate.
/// @return QVariantMap with binding info if found, or {"valid": false} if not.
QVariantMap findBinding(SDL_Gamepad *gamepad,
                        const std::function<bool(const SDL_GamepadBinding *)> &matchFunc);

} // namespace SDLJoystick
