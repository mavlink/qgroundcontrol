// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLGUARD_P_H
#define QQMLGUARD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmldata_p.h>
#include <private/qqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlGuardImpl
{
public:
    using ObjectDestroyedFn = void(*)(QQmlGuardImpl *);

    inline QQmlGuardImpl();
    inline QQmlGuardImpl(QObject *);
    inline QQmlGuardImpl(const QQmlGuardImpl &);
protected:
    inline ~QQmlGuardImpl();

public: // ### make so it can be private
    QObject *o = nullptr;
    QQmlGuardImpl  *next = nullptr;
    QQmlGuardImpl **prev = nullptr;
    ObjectDestroyedFn objectDestroyed = nullptr;

    inline void addGuard();
    inline void remGuard();

    inline void setObject(QObject *g);
    bool isNull() const noexcept { return !o; }
};

class QObject;
template<class T>
class QQmlGuard : protected QQmlGuardImpl
{
    friend class QQmlData;
public:
    Q_NODISCARD_CTOR inline QQmlGuard();
    Q_NODISCARD_CTOR inline QQmlGuard(ObjectDestroyedFn objectDestroyed, T *);
    Q_NODISCARD_CTOR inline QQmlGuard(T *);
    Q_NODISCARD_CTOR inline QQmlGuard(const QQmlGuard<T> &);

    inline QQmlGuard<T> &operator=(const QQmlGuard<T> &o);
    inline QQmlGuard<T> &operator=(T *);

    T *object() const noexcept { return static_cast<T *>(o); }
    void setObject(T *g) { QQmlGuardImpl::setObject(g); }

    using QQmlGuardImpl::isNull;

    T *operator->() const noexcept { return object(); }
    T &operator*() const { return *object(); }
    operator T *() const noexcept { return object(); }
    T *data() const noexcept { return object(); }
};

/* used in QQmlStrongJSQObjectReference to indicate that the
 * object has JS ownership
 * We save it in objectDestroyFn to save space
 * (implemented in qqmlengine.cpp)
 */
void Q_QML_EXPORT hasJsOwnershipIndicator(QQmlGuardImpl *);

template <typename T>
class QQmlStrongJSQObjectReference final : protected QQmlGuardImpl
{
public:
    T *object() const noexcept { return static_cast<T *>(o); }

    using QQmlGuardImpl::isNull;

    T *operator->() const noexcept { return object(); }
    T &operator*() const { return *object(); }
    operator T *() const noexcept { return object(); }
    T *data() const noexcept { return object(); }

    void setObject(T *obj, QObject *parent) {
        T *old = object();
        if (obj == old)
            return;

        if (hasJsOwnership() && old && old->parent() == parent)
            QQml_setParent_noEvent(old, nullptr);

        QQmlGuardImpl::setObject(obj);

        if (obj && !obj->parent() && !QQmlData::keepAliveDuringGarbageCollection(obj)) {
            setJsOwnership(true);
            QQml_setParent_noEvent(obj, parent);
        } else {
            setJsOwnership(false);
        }
    }

private:
    bool hasJsOwnership() {
        return objectDestroyed == hasJsOwnershipIndicator;
    }

    void setJsOwnership(bool itHasOwnership) {
        objectDestroyed = itHasOwnership ? hasJsOwnershipIndicator : nullptr;
    }
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlGuard<QObject>)

QT_BEGIN_NAMESPACE

QQmlGuardImpl::QQmlGuardImpl()
{
}

QQmlGuardImpl::QQmlGuardImpl(QObject *g)
: o(g)
{
    if (o) addGuard();
}

/*
    \internal
    Copying a QQmlGuardImpl leaves the old one in the intrinsic linked list of guards.
    The fresh copy does not contain the list pointer of the existing guard; instead
    only the object and objectDestroyed pointers are copied, and if there is an object
    we add the new guard to the object's list of guards.
 */
QQmlGuardImpl::QQmlGuardImpl(const QQmlGuardImpl &g)
: o(g.o), objectDestroyed(g.objectDestroyed)
{
    if (o) addGuard();
}

QQmlGuardImpl::~QQmlGuardImpl()
{
    if (prev) remGuard();
    o = nullptr;
}

void QQmlGuardImpl::addGuard()
{
    Q_ASSERT(!prev);

    if (QObjectPrivate::get(o)->wasDeleted)
        return;

    QQmlData *data = QQmlData::get(o, true);
    next = data->guards;
    if (next) next->prev = &next;
    data->guards = this;
    prev = &data->guards;
}

void QQmlGuardImpl::remGuard()
{
    Q_ASSERT(prev);

    if (next) next->prev = prev;
    *prev = next;
    next = nullptr;
    prev = nullptr;
}

template<class T>
QQmlGuard<T>::QQmlGuard()
{
}

template<class T>
QQmlGuard<T>::QQmlGuard(ObjectDestroyedFn objDestroyed, T *obj)
    : QQmlGuardImpl(obj)
{
    objectDestroyed = objDestroyed;
}

template<class T>
QQmlGuard<T>::QQmlGuard(T *g)
: QQmlGuardImpl(g)
{
}

template<class T>
QQmlGuard<T>::QQmlGuard(const QQmlGuard<T> &g)
: QQmlGuardImpl(g)
{
}

template<class T>
QQmlGuard<T> &QQmlGuard<T>::operator=(const QQmlGuard<T> &g)
{
    objectDestroyed = g.objectDestroyed;
    setObject(g.object());
    return *this;
}

template<class T>
QQmlGuard<T> &QQmlGuard<T>::operator=(T *g)
{
    /* this does not touch objectDestroyed, as operator= is only a convenience
     * for setObject. All logic involving objectDestroyed is (sub-)class specific
     * and remains unaffected.
     */
    setObject(g);
    return *this;
}

void QQmlGuardImpl::setObject(QObject *g)
{
    if (g != o) {
        if (prev) remGuard();
        o = g;
        if (o) addGuard();
    }
}

QT_END_NAMESPACE

#endif // QQMLGUARD_P_H
