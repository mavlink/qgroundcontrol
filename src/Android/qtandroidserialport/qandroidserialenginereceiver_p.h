// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QtTypes>

#include "AndroidSerial.h"

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

// Mirrors QAbstractSocketEngineReceiver (qtbase/src/network/socket/qabstractsocketengine_p.h):
// abstract callback interface decoupling the JNI engine from the QIODevice receiver
// (QSerialPortPrivate). The engine owns thread marshaling internally — it stages bytes from
// the Java IO thread and delivers them on the owner thread via dataReady().
class QAndroidSerialEngineReceiver
{
public:
    virtual ~QAndroidSerialEngineReceiver() = default;

    // All three callbacks run on the receiver's owner thread (the QSerialPort thread).
    // dataReady carries already-staged bytes the engine has drained from its internal queue.
    virtual void dataReady(QByteArray&& bytes) = 0;
    virtual void closeNotification() = 0;
    virtual void exceptionNotification(AndroidSerial::JavaExceptionKind kind, const QString& message) = 0;
};
