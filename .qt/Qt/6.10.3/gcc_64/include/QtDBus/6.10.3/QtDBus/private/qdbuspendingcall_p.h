// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//

#ifndef QDBUSPENDINGCALL_P_H
#define QDBUSPENDINGCALL_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include <qlist.h>
#include <qmutex.h>
#include <qpointer.h>
#include <qshareddata.h>
#include <qwaitcondition.h>

#include "qdbusmessage.h"
#include "qdbus_symbols_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusPendingCall;
class QDBusPendingCallWatcher;
class QDBusPendingCallWatcherHelper;
class QDBusConnectionPrivate;

class QDBusPendingCallPrivate: public QSharedData
{
public:
    // {
    //     set only during construction:
    const QDBusMessage sentMessage;
    QDBusConnectionPrivate * const connection;

    // for the callback mechanism (see setReplyCallback and QDBusConnectionPrivate::sendWithReplyAsync)
    QPointer<QObject> receiver;
    QList<QMetaType> metaTypes;
    int methodIdx;

    // }

    mutable QMutex mutex;
    QWaitCondition waitForFinishedCondition;

    // {
    //    protected by the mutex above:
    QDBusPendingCallWatcherHelper *watcherHelper;
    QDBusMessage replyMessage;
    DBusPendingCall *pending;
    QString expectedReplySignature;
    // }

    QDBusPendingCallPrivate(const QDBusMessage &sent, QDBusConnectionPrivate *connection)
        : sentMessage(sent), connection(connection), watcherHelper(nullptr), pending(nullptr)
    { }
    ~QDBusPendingCallPrivate();
    bool setReplyCallback(QObject *target, const char *member);
    void waitForFinished();
    void waitForFinishedWithGui();
    void setMetaTypes(int count, const QMetaType *types);
    void checkReceivedSignature();
};

class QDBusPendingCallWatcherHelper: public QObject
{
    Q_OBJECT
public:
    void add(QDBusPendingCallWatcher *watcher);

    void emitSignals(const QDBusMessage &replyMessage, const QDBusMessage &sentMessage)
    {
        if (replyMessage.type() == QDBusMessage::ReplyMessage)
            emit reply(replyMessage);
        else
            emit error(QDBusError(replyMessage), sentMessage);
        emit finished();
    }

Q_SIGNALS:
    void finished();
    void reply(const QDBusMessage &msg);
    void error(const QDBusError &error, const QDBusMessage &msg);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
