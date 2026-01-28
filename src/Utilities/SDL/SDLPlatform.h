#pragma once

#include <QtCore/QString>

#include <functional>

/**
 * SDL platform utilities
 * General SDL functionality not specific to joysticks
 */
namespace SDLPlatform {

// ============================================================================
// Version Info
// ============================================================================

/// Get SDL version as string (e.g., "3.4.0")
QString getVersion();

/// Get SDL revision string (git hash)
QString getRevision();

// ============================================================================
// Hints Management
// ============================================================================

/// Set an SDL hint
bool setHint(const QString &name, const QString &value);

/// Get an SDL hint value
QString getHint(const QString &name);

/// Get an SDL hint as boolean
bool getHintBoolean(const QString &name, bool defaultValue = false);

/// Reset an SDL hint to default
bool resetHint(const QString &name);

/// Reset all hints to defaults
void resetHints();

// ============================================================================
// Platform / Capability Queries
// ============================================================================

/// Get the current platform name
QString getPlatform();

/// Check if we're running on a specific platform
bool isAndroid();
bool isIOS();
bool isWindows();
bool isMacOS();
bool isLinux();

/// Check if tablet mode is active (Android/Windows)
bool isTablet();

/// Check if running on a TV device
bool isTV();

/// Check if running in DeX mode (Samsung)
bool isDeXMode();

/// Check if running on a Chromebook (Chrome OS)
bool isChromebook();

// ============================================================================
// System Info
// ============================================================================

/// Get number of logical CPU cores
int getNumLogicalCPUCores();

/// Get system RAM in MB
int getSystemRAM();

// ============================================================================
// Android-Specific
// ============================================================================

/// Get Android SDK version (API level)
/// Returns 0 on non-Android platforms
int getAndroidSDKVersion();

/// Get Android internal storage path
/// Returns empty string on non-Android platforms
QString getAndroidInternalStoragePath();

/// Get Android external storage path
/// Returns empty string on non-Android platforms
QString getAndroidExternalStoragePath();

/// Android external storage state flags
enum AndroidStorageState {
    StorageStateNone = 0,
    StorageStateRead = 0x01,
    StorageStateWrite = 0x02
};

/// Get Android external storage state (read/write permissions)
/// Returns StorageStateNone on non-Android platforms
int getAndroidExternalStorageState();

/// Request an Android runtime permission
/// @param permission The permission string (e.g., "android.permission.CAMERA")
/// @param callback Called with (granted) when permission result is available
/// @return true if request was initiated, false on error
/// On non-Android platforms, callback is called immediately with granted=true
bool requestAndroidPermission(const QString &permission, std::function<void(bool granted)> callback);

// ============================================================================
// Device Power Info
// ============================================================================

/// Get device (phone/tablet) power information
/// @param seconds Output: seconds of battery remaining, or -1 if unknown
/// @param percent Output: battery percentage (0-100), or -1 if unknown
/// @return Power state string
QString getDevicePowerInfo(int *seconds = nullptr, int *percent = nullptr);

} // namespace SDLPlatform
