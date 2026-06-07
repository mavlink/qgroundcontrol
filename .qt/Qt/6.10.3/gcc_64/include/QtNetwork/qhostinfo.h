// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHOSTINFO_H
#define QHOSTINFO_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qhostaddress.h>

QT_BEGIN_NAMESPACE


class QObject;
class QHostInfoPrivate;

class Q_NETWORK_EXPORT QHostInfo
{
    Q_GADGET
public:
    enum HostInfoError {
        NoError,
        HostNotFound,
        UnknownError
    };

    explicit QHostInfo(int lookupId = -1);
    QHostInfo(const QHostInfo &d);
    QHostInfo(QHostInfo &&other) noexcept : d_ptr(std::exchange(other.d_ptr, nullptr)) {}
    QHostInfo &operator=(const QHostInfo &d);
    QHostInfo &operator=(QHostInfo &&other) noexcept { swap(other); return *this; }
    ~QHostInfo();

    void swap(QHostInfo &other) noexcept { qt_ptr_swap(d_ptr, other.d_ptr); }

    QString hostName() const;
    void setHostName(const QString &name);

    QList<QHostAddress> addresses() const;
    void setAddresses(const QList<QHostAddress> &addresses);

    HostInfoError error() const;
    void setError(HostInfoError error);

    QString errorString() const;
    void setErrorString(const QString &errorString);

    void setLookupId(int id);
    int lookupId() const;

#if QT_NETWORK_REMOVED_SINCE(6, 7)
    static int lookupHost(const QString &name, QObject *receiver, const char *member);
#endif
    static int lookupHost(const QString &name, const QObject *receiver, const char *member);
    static void abortHostLookup(int lookupId);

    static QHostInfo fromName(const QString &name);
    static QString localHostName();
    static QString localDomainName();

#ifdef Q_QDOC
    template<typename Functor>
    static int lookupHost(const QString &name, const QObject *context, Functor functor);
#else
    // lookupHost to a callable (with context)
    template <typename Functor>
    static inline int lookupHost(const QString &name,
                                 const typename QtPrivate::ContextTypeForFunctor<Functor>::ContextType *receiver,
                                 Functor &&func)
    {
        using Prototype = void(*)(QHostInfo);
        QtPrivate::AssertCompatibleFunctions<Prototype, Functor>();
        return lookupHostImpl(name, receiver,
                              QtPrivate::makeCallableObject<Prototype>(std::forward<Functor>(func)),
                              nullptr);
    }
#endif // Q_QDOC

#ifndef QT_NO_CONTEXTLESS_CONNECT
    // lookupHost to a callable (without context)
    template <typename Functor>
    static inline int lookupHost(const QString &name, Functor &&slot)
    {
        return lookupHost(name, nullptr, std::forward<Functor>(slot));
    }
#endif // QT_NO_CONTEXTLESS_CONNECT

private:
    QHostInfoPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QHostInfo)

    static int lookupHostImpl(const QString &name,
                              const QObject *receiver,
                              QtPrivate::QSlotObjectBase *slotObj,
                              const char *member);

    friend QHostInfo Q_NETWORK_EXPORT qt_qhostinfo_lookup(const QString &name, QObject *receiver,
                                                          const char *member, bool *valid, int *id);
};

Q_DECLARE_SHARED(QHostInfo)

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QHostInfo, Q_NETWORK_EXPORT)

#endif // QHOSTINFO_H
