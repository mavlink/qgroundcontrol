/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <jni.h>
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>

class AndroidInterface
{
public:
    static void acquireScreenWakeLock();
    static void releaseScreenWakeLock();
    static void showToast(int id);
    static QString getSystemProperty(QString& prop_name, QString& defaultValue);
    static void setSystemProperty(QString& prop_name, QString& value);
};
