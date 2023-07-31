/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#include "QGCApplication.h"
#include "AndroidInterface.h"
#include <QAndroidJniObject>
#include <QtAndroid>

QString AndroidInterface::getSDCardPath()
{
    QAndroidJniObject value = QAndroidJniObject::callStaticObjectMethod("org/mavlink/qgroundcontrol/QGCActivity", "getSDCardPath",
                            "()Ljava/lang/String;");
    QString sdCardPath = value.toString();

    QString readPermission("android.permission.READ_EXTERNAL_STORAGE");
    QString writePermission("android.permission.WRITE_EXTERNAL_STORAGE");

    if (QtAndroid::checkPermission(readPermission) == QtAndroid::PermissionResult::Denied ||
            QtAndroid::checkPermission(writePermission) == QtAndroid::PermissionResult::Denied) {
        QtAndroid::PermissionResultMap resultHash = QtAndroid::requestPermissionsSync(QStringList({ readPermission, writePermission }));
        if (resultHash[readPermission] == QtAndroid::PermissionResult::Denied ||
                resultHash[writePermission] == QtAndroid::PermissionResult::Denied) {
            return QString();
        }
    }

    return sdCardPath;
}
