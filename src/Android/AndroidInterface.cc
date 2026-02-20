#include "AndroidInterface.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "SettingsFact.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJniObject>
#include <QtCore/QJniEnvironment>
#include <QtCore/QMetaObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QStandardPaths>
#include <QtAndroidHelpers/QAndroidPartialWakeLocker.h>
#include <QtAndroidHelpers/QAndroidWiFiLocker.h>
#include <QAndroidScreen.h>

QGC_LOGGING_CATEGORY(AndroidInterfaceLog, "Android.AndroidInterface")

namespace AndroidInterface
{

static void jniLogDebug(JNIEnv *, jobject, jstring message)
{
    qCDebug(AndroidInterfaceLog) << QJniObject(message).toString();
}

static void jniLogWarning(JNIEnv *, jobject, jstring message)
{
    qCWarning(AndroidInterfaceLog) << QJniObject(message).toString();
}

static void jniStoragePermissionsResult(JNIEnv *, jobject, jboolean granted)
{
    if (!granted) {
        qCWarning(AndroidInterfaceLog) << "Storage permission request denied";
        return;
    }

    if (!qgcApp()) {
        return;
    }

    (void) QMetaObject::invokeMethod(qgcApp(), []() {
        SettingsManager *const settingsManager = SettingsManager::instance();
        if (!settingsManager) {
            return;
        }

        AppSettings *const appSettings = settingsManager->appSettings();
        if (!appSettings || appSettings->androidDontSaveToSDCard()->rawValue().toBool()) {
            return;
        }

        SettingsFact *const savePathFact = qobject_cast<SettingsFact *>(appSettings->savePath());
        if (!savePathFact) {
            return;
        }

        const QString appName = QCoreApplication::applicationName();
        const QString currentSavePath = savePathFact->rawValue().toString();
        const QString internalBasePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        const QString internalSavePath = QDir(internalBasePath).filePath(appName);

        if (!currentSavePath.isEmpty() && (currentSavePath != internalSavePath)) {
            return;
        }

        const QString sdCardRootPath = getSDCardPath();
        if (sdCardRootPath.isEmpty() || !QDir(sdCardRootPath).exists() || !QFileInfo(sdCardRootPath).isWritable()) {
            return;
        }

        const QString sdSavePath = QDir(sdCardRootPath).filePath(appName);
        if (currentSavePath != sdSavePath) {
            qCDebug(AndroidInterfaceLog) << "Applying SD card save path after permission grant:" << sdSavePath;
            savePathFact->setRawValue(sdSavePath);
        }
    }, Qt::QueuedConnection);
}

void setNativeMethods()
{
    qCDebug(AndroidInterfaceLog) << "Registering Native Functions";

    const JNINativeMethod javaMethods[] {
        {"qgcLogDebug",   "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniLogDebug)},
        {"qgcLogWarning", "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniLogWarning)},
        {"nativeStoragePermissionsResult", "(Z)V", reinterpret_cast<void *>(jniStoragePermissionsResult)}
    };

    QJniEnvironment env;
    if (!env.registerNativeMethods(kJniQGCActivityClassName, javaMethods, std::size(javaMethods))) {
        qCWarning(AndroidInterfaceLog) << "Failed to register native methods for" << kJniQGCActivityClassName;
    } else {
        qCDebug(AndroidInterfaceLog) << "Native Functions Registered";
    }
}

bool checkStoragePermissions()
{
    const bool hasPermission = QJniObject::callStaticMethod<jboolean>(
        kJniQGCActivityClassName,
        "checkStoragePermissions",
        "()Z"
    );
    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidInterfaceLog) << "Exception in checkStoragePermissions";
        return false;
    }

    if (hasPermission) {
        qCDebug(AndroidInterfaceLog) << "Storage permissions granted";
    } else {
        qCWarning(AndroidInterfaceLog) << "Storage permissions not granted";
    }

    return hasPermission;
}

QString getSDCardPath()
{
    if (!checkStoragePermissions()) {
        qCWarning(AndroidInterfaceLog) << "Storage Permission Denied";
        return QString();
    }

    const QJniObject result = QJniObject::callStaticObjectMethod(kJniQGCActivityClassName, "getSDCardPath", "()Ljava/lang/String;");
    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidInterfaceLog) << "Exception in getSDCardPath";
        return QString();
    }
    if (!result.isValid()) {
        qCWarning(AndroidInterfaceLog) << "Call to java getSDCardPath failed: Invalid Result";
        return QString();
    }

    return result.toString();
}

static QSharedPointer<QLocks::QLockBase> s_partialWakeLock;
static QSharedPointer<QLocks::QLockBase> s_wifiLock;

void setKeepScreenOn(bool on)
{
    if (!QAndroidScreen::instance()) {
        new QAndroidScreen(QCoreApplication::instance());
    }
    QAndroidScreen::instance()->keepScreenOn(on);

    if (on) {
        s_partialWakeLock = QAndroidPartialWakeLocker::instance().getLock();
        s_wifiLock = QAndroidWiFiLocker::instance().getLock();
    } else {
        s_partialWakeLock.reset();
        s_wifiLock.reset();
    }
}

} // namespace AndroidInterface
