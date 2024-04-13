#include "AndroidInterface.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>
#include <QtCore/private/qandroidextras_p.h>

Q_LOGGING_CATEGORY(AndroidInterfaceLog, "qgc.android.interface");

void AndroidInterface::cleanJavaException()
{
    QJniEnvironment env;
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

const char* AndroidInterface::getQGCActivityClassName()
{
    static const char* kJniQGCActivityClassName = "org/mavlink/qgroundcontrol/QGCActivity";
    return kJniQGCActivityClassName;
}

bool AndroidInterface::checkStoragePermissions()
{
    qCDebug(AndroidInterfaceLog) << Q_FUNC_INFO;

    bool result = true;

    QString readPermission("android.permission.READ_EXTERNAL_STORAGE");
    QString writePermission("android.permission.WRITE_EXTERNAL_STORAGE");

    QStringList permissions = { readPermission, writePermission };
    for (const auto& permission: permissions)
    {
        auto futurePermissionResult = QtAndroidPrivate::checkPermission(permission);
        auto permissionResult = futurePermissionResult.result();
        if (permissionResult == QtAndroidPrivate::PermissionResult::Denied)
        {
            futurePermissionResult = QtAndroidPrivate::requestPermission(permission);
            permissionResult = futurePermissionResult.result();
            if (permissionResult == QtAndroidPrivate::PermissionResult::Denied)
            {
                result = false;
                break;
            }
        }
    }

    return result;
}

QString AndroidInterface::getSDCardPath()
{
    qCDebug(AndroidInterfaceLog) << Q_FUNC_INFO;

    QString result = QString();

    if (checkStoragePermissions())
    {
        auto value = QJniObject::callStaticObjectMethod(getQGCActivityClassName(), "getSDCardPath", "()Ljava/lang/String;");
        result = value.toString();
    }

    return result;
}
