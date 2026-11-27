#include <QtCore/QCoreApplication>
#include <QtCore/QJniEnvironment>
#include <QtCore/QLoggingCategory>

#include "AndroidInterface.h"
#include "AndroidSerialApp.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AndroidInitLog, "Android.AndroidInit");

jint JNI_OnLoad(JavaVM* vm, void*)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    AndroidInterface::setNativeMethods();

    // Without them the first USB serial callback would throw on a Java thread, and take the app down then. Also
    // in a build without serial links: its Java side tracks the USB serial devices all the same, and tells of
    // each that is attached or detached.
    if (!AndroidSerial::setNativeMethods()) {
        return JNI_ERR;
    }

    QNativeInterface::QAndroidApplication::hideSplashScreen(333);

    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM*, void*)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;
}
