#include "SDLPlatform.h"

#include <functional>

#ifdef Q_OS_ANDROID
#include <QtCore/QJniObject>
#endif

#include <SDL3/SDL.h>

namespace SDLPlatform {

// ============================================================================
// Version Info
// ============================================================================

QString getVersion()
{
    int version = SDL_GetVersion();
    int major = SDL_VERSIONNUM_MAJOR(version);
    int minor = SDL_VERSIONNUM_MINOR(version);
    int micro = SDL_VERSIONNUM_MICRO(version);
    return QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(micro);
}

QString getRevision()
{
    const char *rev = SDL_GetRevision();
    return rev ? QString::fromUtf8(rev) : QString();
}

// ============================================================================
// Hints Management
// ============================================================================

bool setHint(const QString &name, const QString &value)
{
    return SDL_SetHint(qPrintable(name), qPrintable(value));
}

QString getHint(const QString &name)
{
    const char *value = SDL_GetHint(qPrintable(name));
    return value ? QString::fromUtf8(value) : QString();
}

bool getHintBoolean(const QString &name, bool defaultValue)
{
    return SDL_GetHintBoolean(qPrintable(name), defaultValue);
}

bool resetHint(const QString &name)
{
    return SDL_ResetHint(qPrintable(name));
}

void resetHints()
{
    SDL_ResetHints();
}

// ============================================================================
// Platform / Capability Queries
// ============================================================================

QString getPlatform()
{
    return QString::fromUtf8(SDL_GetPlatform());
}

bool isAndroid()
{
#ifdef Q_OS_ANDROID
    return true;
#else
    return false;
#endif
}

bool isIOS()
{
#ifdef Q_OS_IOS
    return true;
#else
    return false;
#endif
}

bool isWindows()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool isMacOS()
{
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

bool isLinux()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    return true;
#else
    return false;
#endif
}

bool isTablet()
{
#ifdef Q_OS_ANDROID
    return QJniObject::callStaticMethod<jboolean>("org/libsdl/app/SDLActivity", "isTablet", "()Z");
#else
    return false;
#endif
}

bool isTV()
{
#ifdef Q_OS_ANDROID
    return QJniObject::callStaticMethod<jboolean>("org/libsdl/app/SDLActivity", "isAndroidTV", "()Z");
#else
    return false;
#endif
}

bool isDeXMode()
{
#ifdef Q_OS_ANDROID
    return QJniObject::callStaticMethod<jboolean>("org/libsdl/app/SDLActivity", "isDeXMode", "()Z");
#else
    return false;
#endif
}

bool isChromebook()
{
#ifdef Q_OS_ANDROID
    return QJniObject::callStaticMethod<jboolean>("org/libsdl/app/SDLActivity", "isChromebook", "()Z");
#else
    return false;
#endif
}

// ============================================================================
// System Info
// ============================================================================

int getNumLogicalCPUCores()
{
    return SDL_GetNumLogicalCPUCores();
}

int getSystemRAM()
{
    return SDL_GetSystemRAM();
}

// ============================================================================
// Android-Specific
// ============================================================================

int getAndroidSDKVersion()
{
#ifdef Q_OS_ANDROID
    return SDL_GetAndroidSDKVersion();
#else
    return 0;
#endif
}

QString getAndroidInternalStoragePath()
{
#ifdef Q_OS_ANDROID
    const char *path = SDL_GetAndroidInternalStoragePath();
    return path ? QString::fromUtf8(path) : QString();
#else
    return QString();
#endif
}

QString getAndroidExternalStoragePath()
{
#ifdef Q_OS_ANDROID
    const char *path = SDL_GetAndroidExternalStoragePath();
    return path ? QString::fromUtf8(path) : QString();
#else
    return QString();
#endif
}

int getAndroidExternalStorageState()
{
#ifdef Q_OS_ANDROID
    Uint32 state = SDL_GetAndroidExternalStorageState();
    int result = StorageStateNone;
    if (state & SDL_ANDROID_EXTERNAL_STORAGE_READ) {
        result |= StorageStateRead;
    }
    if (state & SDL_ANDROID_EXTERNAL_STORAGE_WRITE) {
        result |= StorageStateWrite;
    }
    return result;
#else
    return StorageStateNone;
#endif
}

#ifdef Q_OS_ANDROID
static void androidPermissionCallback(void *userdata, const char * /*permission*/, bool granted)
{
    auto *callback = static_cast<std::function<void(bool)>*>(userdata);
    if (callback) {
        (*callback)(granted);
        delete callback;
    }
}
#endif

bool requestAndroidPermission(const QString &permission, std::function<void(bool granted)> callback)
{
#ifdef Q_OS_ANDROID
    if (!callback) {
        return false;
    }
    auto *callbackPtr = new std::function<void(bool)>(std::move(callback));
    if (!SDL_RequestAndroidPermission(qPrintable(permission), androidPermissionCallback, callbackPtr)) {
        delete callbackPtr;
        return false;
    }
    return true;
#else
    Q_UNUSED(permission);
    if (callback) {
        callback(true);
    }
    return true;
#endif
}

// ============================================================================
// Device Power Info
// ============================================================================

QString getDevicePowerInfo(int *seconds, int *percent)
{
    int sec = -1;
    int pct = -1;
    SDL_PowerState state = SDL_GetPowerInfo(&sec, &pct);

    if (seconds) {
        *seconds = sec;
    }
    if (percent) {
        *percent = pct;
    }

    switch (state) {
    case SDL_POWERSTATE_ON_BATTERY:
        return QStringLiteral("On Battery");
    case SDL_POWERSTATE_NO_BATTERY:
        return QStringLiteral("No Battery");
    case SDL_POWERSTATE_CHARGING:
        return QStringLiteral("Charging");
    case SDL_POWERSTATE_CHARGED:
        return QStringLiteral("Charged");
    case SDL_POWERSTATE_ERROR:
        return QStringLiteral("Error");
    case SDL_POWERSTATE_UNKNOWN:
    default:
        return QStringLiteral("Unknown");
    }
}

} // namespace SDLPlatform
