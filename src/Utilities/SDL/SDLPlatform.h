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

bool setHint(const QString &name, const QString &value);
QString getHint(const QString &name);
bool getHintBoolean(const QString &name, bool defaultValue = false);
bool resetHint(const QString &name);
void resetHints();

// ============================================================================
// Platform / Capability Queries
// ============================================================================

QString getPlatform();

bool isAndroid();
bool isIOS();
bool isWindows();
bool isMacOS();
bool isLinux();

/// Check if tablet mode is active (Android/Windows)
bool isTablet();

bool isTV();

/// Check if running in DeX mode (Samsung)
bool isDeXMode();

bool isChromebook();

// ============================================================================
// System Info
// ============================================================================

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
