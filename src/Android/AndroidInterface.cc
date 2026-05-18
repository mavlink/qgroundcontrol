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
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QStandardPaths>
#include <QtCore/qjnitypes.h>

#include "AppSettings.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsFact.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(AndroidInterfaceLog, "Android.AndroidInterface")

Q_DECLARE_JNI_CLASS(QGCActivity, "org/mavlink/qgroundcontrol/QGCActivity")

namespace AndroidInterface {
static std::function<void(const QString&)> s_importCallback;
}  // namespace AndroidInterface

// File-static (not in namespace) so Q_DECLARE_JNI_NATIVE_METHOD can name the function via
// QT_PREPEND_NAMESPACE. Mirrors Qt BLE jni_android.cpp.
static void jniStoragePermissionsResult(JNIEnv*, jobject, jboolean granted)
{
    if (!granted) {
        qCWarning(AndroidInterfaceLog) << "Storage permission request denied";
        return;
    }

    QPointer<QGCApplication> app = qgcApp();
    if (!app) {
        return;
    }

    (void)QMetaObject::invokeMethod(
        app.data(),
        [app]() {
            if (!app) {
                return;
            }
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

            const QString sdCardRootPath = AndroidInterface::getSDCardPath();
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
Q_DECLARE_JNI_NATIVE_METHOD(jniStoragePermissionsResult, nativeStoragePermissionsResult)

static void jniOnImportResult(JNIEnv* env, jobject, jstring filePathA)
{
    const char* const filePathCStr = env->GetStringUTFChars(filePathA, nullptr);
    const QString filePath = QString::fromUtf8(filePathCStr);
    env->ReleaseStringUTFChars(filePathA, filePathCStr);
    (void)QJniEnvironment::checkAndClearExceptions(env);
    auto callback = std::move(AndroidInterface::s_importCallback);
    if (!callback) {
        return;
    }
    callback(filePath);
}
Q_DECLARE_JNI_NATIVE_METHOD(jniOnImportResult, onImportResult)

namespace AndroidInterface {

void setNativeMethods()
{
    QJniEnvironment env;
    if (!env.registerNativeMethods<QtJniTypes::QGCActivity>({
            Q_JNI_NATIVE_METHOD(jniStoragePermissionsResult),
            Q_JNI_NATIVE_METHOD(jniOnImportResult),
        })) {
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

}  // namespace AndroidInterface
