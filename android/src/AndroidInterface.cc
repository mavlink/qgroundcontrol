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

QString AndroidInterface::getSDCardPath()
{
    QAndroidJniObject value = QAndroidJniObject::callStaticObjectMethod("org/mavlink/qgroundcontrol/QGCActivity", "getSDCardPath",
                            "()Ljava/lang/String;");
    return value.toString();
}
