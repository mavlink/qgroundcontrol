// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDBUSPENDINGCALL_H
#define QDBUSPENDINGCALL_H

#include <QtDBus/qtdbusglobal.h>
#include <QtDBus/qdbusmessage.h>
#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>

#ifndef QT_NO_DBUS

class tst_QDBusPendingReply;

QT_BEGIN_NAMESPACE


class QDBusConnection;
class QDBusError;
class QDBusPendingCallWatcher;

class QDBusPendingCallPrivate;

QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QDBusPendingCallPrivate)

class Q_DBUS_EXPORT QDBusPendingCall
{
public:
    QDBusPendingCall(const QDBusPendingCall &other);
    QDBusPendingCall(QDBusPendingCall &&other) noexcept = default;
    ~QDBusPendingCall();
    QDBusPendingCall &operator=(QDBusPendingCall &&other) noexcept { swap(other); return *this; }
    QDBusPendingCall &operator=(const QDBusPendingCall &other);

    void swap(QDBusPendingCall &other) noexcept { d.swap(other.d); }

#ifndef Q_QDOC
    // pretend that they aren't here
    bool isFinished() const;
    void waitForFinished();

    bool isError() const;
    bool isValid() const;
    QDBusError error() const;
    QDBusMessage reply() const;
#endif

    static QDBusPendingCall fromError(const QDBusError &error);
    static QDBusPendingCall fromCompletedCall(const QDBusMessage &message);

protected:
    QExplicitlySharedDataPointer<QDBusPendingCallPrivate> d;
    friend class QDBusPendingCallPrivate;
    friend class QDBusPendingCallWatcher;
    friend class QDBusConnection;

    QDBusPendingCall(QDBusPendingCallPrivate *dd);

private:
    QDBusPendingCall();         // not defined

    friend class ::tst_QDBusPendingReply;
};

Q_DECLARE_SHARED(QDBusPendingCall)

class Q_DBUS_EXPORT QDBusPendingCallWatcher: public QObject, public QDBusPendingCall
{
    Q_OBJECT
public:
    explicit QDBusPendingCallWatcher(const QDBusPendingCall &call, QObject *parent = nullptr);
    ~QDBusPendingCallWatcher();

#ifdef Q_QDOC
    // trick qdoc into thinking this method is here
    bool isFinished() const;
#endif
    void waitForFinished();     // non-virtual override

Q_SIGNALS:
    void finished(QDBusPendingCallWatcher *self = nullptr);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
