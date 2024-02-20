/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AndroidInterface.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>
#include <QtCore/private/qandroidextras_p.h>

static Q_LOGGING_CATEGORY(AndroidInterfaceLog, "qgc.android.interface");
static const char kJniQGCActivityClassName[] {"org/mavlink/qgroundcontrol/QGCActivity"};

bool AndroidInterface::checkStoragePermissions()
{
    QString readPermission("android.permission.READ_EXTERNAL_STORAGE");
    QString writePermission("android.permission.WRITE_EXTERNAL_STORAGE");

    QStringList permissions = { readPermission, writePermission };
    for (const auto& permission: permissions) {
        auto futurePermissionResult = QtAndroidPrivate::checkPermission(permission);
        auto permissionResult = futurePermissionResult.result();
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

QString AndroidInterface::getSDCardPath()
{
    if (!checkStoragePermissions()) {
        return QString();
    } else {
        auto value = QJniObject::callStaticObjectMethod(kJniQGCActivityClassName, "getSDCardPath", "()Ljava/lang/String;");
        return value.toString();
    }
}
