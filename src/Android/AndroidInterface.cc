#include "AndroidInterface.h"

#include <QAndroidScreen.h>
#include <QtAndroidHelpers/QAndroidPartialWakeLocker.h>
#include <QtAndroidHelpers/QAndroidWiFiLocker.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QMetaObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QStandardPaths>
#include <QtCore/private/qandroidextras_p.h>

#include "AppSettings.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsFact.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(AndroidInterfaceLog, "Android.AndroidInterface")
// What the Java side of the USB serial ports logs, so that it is switched on with the other serial categories.
QGC_LOGGING_CATEGORY(AndroidSerialJavaLog, "Android.AndroidSerial.Java")

namespace AndroidInterface {

static std::function<void(const QString&)> s_importCallback;

/// The category of a message from Java, which starts with the tag that Java logged it under. The classes of the
/// USB serial ports have tags that start alike.
static const QLoggingCategory& javaLogCategory(const QString& message)
{
    return message.startsWith(QLatin1String("QGCUsbSerial")) ? AndroidSerialJavaLog() : AndroidInterfaceLog();
}

static void jniLogDebug(JNIEnv*, jobject, jstring message)
{
    const QString text = QJniObject(message).toString();
    qCDebug(javaLogCategory(text)).noquote() << text;
}

static void jniLogWarning(JNIEnv*, jobject, jstring message)
{
    const QString text = QJniObject(message).toString();
    qCWarning(javaLogCategory(text)).noquote() << text;
}

static void jniStoragePermissionsResult(JNIEnv*, jobject, jboolean granted)
{
    if (!qgcApp()) {
        return;
    }

    if (!granted) {
        qCWarning(AndroidInterfaceLog) << "Storage permission request denied; disabling save to SD card";

        (void)QMetaObject::invokeMethod(
            qgcApp(),
            []() {
                SettingsManager* const settingsManager = SettingsManager::instance();
                if (!settingsManager) {
                    return;
                }

                AppSettings* const appSettings = settingsManager->appSettings();
                if (!appSettings) {
                    return;
                }

                if (!appSettings->androidDontSaveToSDCard()->rawValue().toBool()) {
                    appSettings->androidDontSaveToSDCard()->setRawValue(true);
                }
            },
            Qt::QueuedConnection);
        return;
    }

    (void)QMetaObject::invokeMethod(
        qgcApp(),
        []() {
            SettingsManager* const settingsManager = SettingsManager::instance();
            if (!settingsManager) {
                return;
            }

            AppSettings* const appSettings = settingsManager->appSettings();
            if (!appSettings || appSettings->androidDontSaveToSDCard()->rawValue().toBool()) {
                return;
            }

            SettingsFact* const savePathFact = qobject_cast<SettingsFact*>(appSettings->savePath());
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
        },
        Qt::QueuedConnection);
}

static void jniOnImportResult(JNIEnv* env, jobject, jstring filePathA)
{
    const char* const filePathCStr = env->GetStringUTFChars(filePathA, nullptr);
    const QString filePath = QString::fromUtf8(filePathCStr);
    env->ReleaseStringUTFChars(filePathA, filePathCStr);
    (void)QJniEnvironment::checkAndClearExceptions(env);
    auto callback = std::move(s_importCallback);
    if (!callback) {
        return;
    }
    callback(filePath);
}

void setNativeMethods()
{
    qCDebug(AndroidInterfaceLog) << "Registering Native Functions";

    const JNINativeMethod javaMethods[]{
        {"qgcLogDebug", "(Ljava/lang/String;)V", reinterpret_cast<void*>(jniLogDebug)},
        {"qgcLogWarning", "(Ljava/lang/String;)V", reinterpret_cast<void*>(jniLogWarning)},
        {"nativeStoragePermissionsResult", "(Z)V", reinterpret_cast<void*>(jniStoragePermissionsResult)},
        {"onImportResult", "(Ljava/lang/String;)V", reinterpret_cast<void*>(jniOnImportResult)}};

    QJniEnvironment env;
    if (!env.registerNativeMethods(kJniQGCActivityClassName, javaMethods, std::size(javaMethods))) {
        qCWarning(AndroidInterfaceLog) << "Failed to register native methods for" << kJniQGCActivityClassName;
    } else {
        qCDebug(AndroidInterfaceLog) << "Native Functions Registered";
    }
}

bool checkStoragePermissions()
{
    const bool hasPermission =
        QJniObject::callStaticMethod<jboolean>(kJniQGCActivityClassName, "checkStoragePermissions", "()Z");
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

    const QJniObject result =
        QJniObject::callStaticObjectMethod(kJniQGCActivityClassName, "getSDCardPath", "()Ljava/lang/String;");
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

void openFileImportDialog(const QString& destPath, std::function<void(const QString&)> callback)
{
    s_importCallback = std::move(callback);

    const QJniObject jDestPath = QJniObject::fromString(destPath);
    QJniObject::callStaticMethod<void>(
        kJniQGCActivityClassName,
        "openFileImportDialog",
        "(Ljava/lang/String;)V",
        jDestPath.object<jstring>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidInterfaceLog) << "Exception in openFileImportDialog";
        if (s_importCallback) {
            auto cb = std::move(s_importCallback);
            cb(QString());
        }
    }
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

void setBackgroundWork(BackgroundWork work, bool active)
{
    static int s_activeWork = 0;
    static QSharedPointer<QLocks::QLockBase> s_backgroundWakeLock;
    static QSharedPointer<QLocks::QLockBase> s_backgroundWifiLock;

    const bool wasActive = (s_activeWork != 0);
    if (active) {
        s_activeWork |= static_cast<int>(work);
    } else {
        s_activeWork &= ~static_cast<int>(work);
    }
    const bool isActive = (s_activeWork != 0);
    if (isActive == wasActive) {
        return;
    }

    if (isActive) {
        s_backgroundWakeLock = QAndroidPartialWakeLocker::instance().getLock();
        s_backgroundWifiLock = QAndroidWiFiLocker::instance().getLock();
        const QJniObject channelName =
            QJniObject::fromString(QCoreApplication::translate("AndroidInterface", "Background activity"));
        const QJniObject title = QJniObject::fromString(
            QCoreApplication::translate("AndroidInterface", "%1 is running").arg(QCoreApplication::applicationName()));
        const QJniObject text = QJniObject::fromString(QCoreApplication::translate(
            "AndroidInterface", "Active in the background while a vehicle is connected or video is recording."));
        QJniObject::callStaticMethod<void>(kJniQGCActivityClassName, "startBackgroundWork",
                                           "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                                           channelName.object<jstring>(), title.object<jstring>(),
                                           text.object<jstring>());
    } else {
        s_backgroundWakeLock.reset();
        s_backgroundWifiLock.reset();
        QJniObject::callStaticMethod<void>(kJniQGCActivityClassName, "stopBackgroundWork", "()V");
    }
    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidInterfaceLog) << "Exception while" << (isActive ? "starting" : "stopping")
                                       << "the foreground service";
    }

    // Android 13+ shows the service's notification only once the user has allowed notifications; the service runs
    // either way. Asked once a run: Android shows its dialog only until the user has answered it.
    static bool s_askedForNotifications = false;
    if (isActive && !s_askedForNotifications && (QNativeInterface::QAndroidApplication::sdkVersion() >= 33)) {
        s_askedForNotifications = true;
        (void) QtAndroidPrivate::requestPermission(QStringLiteral("android.permission.POST_NOTIFICATIONS"));
    }
}

}  // namespace AndroidInterface
