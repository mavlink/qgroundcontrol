/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AndroidInterface.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QJniObject>
#include <QtCore/QJniEnvironment>
#include <QtCore/private/qandroidextras_p.h>

QGC_LOGGING_CATEGORY(AndroidInterfaceLog, "qgc.android.src.androidinterface")

namespace AndroidInterface
{

bool cleanJavaException()
{
    QJniEnvironment jniEnv;
    const bool result = jniEnv.checkAndClearExceptions();
    return result;
}

void setNativeMethods()
{
    qCDebug(AndroidInterfaceLog) << "Registering Native Functions";

    JNINativeMethod javaMethods[] {
        {"qgcLogDebug",   "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniLogDebug)},
        {"qgcLogWarning", "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniLogWarning)}
    };

    (void) AndroidInterface::cleanJavaException();

    jclass objectClass = AndroidInterface::getActivityClass();
    if(!objectClass) {
        qCWarning(AndroidInterfaceLog) << "Couldn't find class:" << objectClass;
        return;
    }

    QJniEnvironment jniEnv;
    jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));

    if (val < 0) {
        qCWarning(AndroidInterfaceLog) << "Error registering methods:" << val;
    } else {
        qCDebug(AndroidInterfaceLog) << "Native Functions Registered";
    }

    (void) AndroidInterface::cleanJavaException();
}

void jniLogDebug(JNIEnv *envA, jobject thizA, jstring messageA)
{
    Q_UNUSED(thizA);

    const char * const stringL = envA->GetStringUTFChars(messageA, nullptr);
    const QString logMessage = QString::fromUtf8(stringL);
    envA->ReleaseStringUTFChars(messageA, stringL);
    (void) QJniEnvironment::checkAndClearExceptions(envA);
    qCDebug(AndroidInterfaceLog) << logMessage;
}

void jniLogWarning(JNIEnv *envA, jobject thizA, jstring messageA)
{
    Q_UNUSED(thizA);

    const char * const stringL = envA->GetStringUTFChars(messageA, nullptr);
    const QString logMessage = QString::fromUtf8(stringL);
    envA->ReleaseStringUTFChars(messageA, stringL);
    (void) QJniEnvironment::checkAndClearExceptions(envA);
    qCWarning(AndroidInterfaceLog) << logMessage;
}

bool checkStoragePermissions()
{
    const QString readPermission("android.permission.READ_EXTERNAL_STORAGE");
    const QString writePermission("android.permission.WRITE_EXTERNAL_STORAGE");

    const QStringList permissions = { readPermission, writePermission };
    for (const auto& permission: permissions) {
        QFuture<QtAndroidPrivate::PermissionResult> futurePermissionResult = QtAndroidPrivate::checkPermission(permission);
        QtAndroidPrivate::PermissionResult permissionResult = futurePermissionResult.result();
        if (permissionResult == QtAndroidPrivate::PermissionResult::Denied) {
            futurePermissionResult = QtAndroidPrivate::requestPermission(permission);
            permissionResult = futurePermissionResult.result();
            if (permissionResult == QtAndroidPrivate::PermissionResult::Denied) {
                return false;
            }
        }
    }

    return true;
}

QString getSDCardPath()
{
    if (!checkStoragePermissions()) {
        qCWarning(AndroidInterfaceLog) << "Storage Permission Denied";
        return QString();
    }

    const QJniObject result = QJniObject::callStaticObjectMethod(kJniQGCActivityClassName, "getSDCardPath", "()Ljava/lang/String;");
    if (!result.isValid()) {
        qCWarning(AndroidInterfaceLog) << "Invalid Result";
        return QString();
    }

    return result.toString();
}

jclass getActivityClass()
{
    static jclass javaClass = nullptr;

    if (!javaClass) {
        QJniEnvironment env;
        if (!env.isValid()) {
            qCWarning(AndroidInterfaceLog) << "Invalid QJniEnvironment";
            return nullptr;
        }

        if (!QJniObject::isClassAvailable(kJniQGCActivityClassName)) {
            qCWarning(AndroidInterfaceLog) << "Class Not Available";
            return nullptr;
        }

        javaClass = env.findClass(kJniQGCActivityClassName);
        if (!javaClass) {
            qCWarning(AndroidInterfaceLog) << "Class Not Found";
            return nullptr;
        }

        env.checkAndClearExceptions();
    }

    return javaClass;
}

} // namespace AndroidInterface
