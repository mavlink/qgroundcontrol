// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLIST_P_H
#define QQMLLIST_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmllist.h"
#include "qqmlmetaobject_p.h"
#include "qqmlmetatype_p.h"
#include <QtQml/private/qbipointer_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQmlListReferencePrivate
{
public:
    QQmlListReferencePrivate();

    static QQmlListReference init(const QQmlListProperty<QObject> &, QMetaType);

    QPointer<QObject> object;
    QQmlListProperty<QObject> property;
    QMetaType propertyType;

    void addref();
    void release();
    int refCount;

    static inline QQmlListReferencePrivate *get(QQmlListReference *ref) {
        return ref->d;
    }

    const QMetaObject *elementType()
    {
        if (!m_elementType) {
            m_elementType = QQmlMetaType::rawMetaObjectForType(
                        QQmlMetaType::listValueType(propertyType)).metaObject();
        }

        return m_elementType;
    }

private:
    const QMetaObject *m_elementType = nullptr;
};

template<typename T>
class QQmlListIterator {
public:
    using difference_type = qsizetype;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T*;

    class reference
    {
    public:
        explicit reference(const QQmlListIterator *iter) : m_iter(iter) {}
        reference(const reference &) = default;
        reference(reference &&) = default;
        ~reference() = default;

        operator T *() const
        {
            if (m_iter == nullptr)
                return nullptr;
            return m_iter->m_list->at(m_iter->m_list, m_iter->m_i);
        }

        reference &operator=(T *value) {
            m_iter->m_list->replace(m_iter->m_list, m_iter->m_i, value);
            return *this;
        }

        reference &operator=(const reference &value) { return operator=((T *)(value)); }
        reference &operator=(reference &&value) { return operator=((T *)(value)); }

        friend void swap(reference a, reference b)
        {
            T *tmp = a;
            a = b;
            b = std::move(tmp);
        }
    private:
        const QQmlListIterator *m_iter;
    };

    class pointer
    {
    public:
        explicit pointer(const QQmlListIterator *iter) : m_iter(iter) {}
        reference operator*() const { return reference(m_iter); }
        QQmlListIterator operator->() const { return *m_iter; }

    private:
        const QQmlListIterator *m_iter;
    };

    QQmlListIterator() = default;
    QQmlListIterator(QQmlListProperty<T> *list, qsizetype i) : m_list(list), m_i(i) {}

    QQmlListIterator &operator++()
    {
        ++m_i;
        return *this;
    }

    QQmlListIterator operator++(int)
    {
        QQmlListIterator result = *this;
        ++m_i;
        return result;
    }

    QQmlListIterator &operator--()
    {
        --m_i;
        return *this;
    }

    QQmlListIterator operator--(int)
    {
        QQmlListIterator result = *this;
        --m_i;
        return result;
    }

    QQmlListIterator &operator+=(qsizetype j)
    {
        m_i += j;
        return *this;
    }

    QQmlListIterator &operator-=(qsizetype j)
    {
        m_i -= j;
        return *this;
    }

    QQmlListIterator operator+(qsizetype j)
    {
        return QQmlListIterator(m_list, m_i + j);
    }

    QQmlListIterator operator-(qsizetype j)
    {
        return QQmlListIterator(m_list, m_i - j);
    }

    reference operator*() const
    {
        return reference(this);
    }

    pointer operator->() const
    {
        return pointer(this);
    }

private:
    friend inline bool operator==(const QQmlListIterator &a, const QQmlListIterator &b)
    {
        return a.m_list == b.m_list && a.m_i == b.m_i;
    }

    friend inline bool operator!=(const QQmlListIterator &a, const QQmlListIterator &b)
    {
        return a.m_list != b.m_list || a.m_i != b.m_i;
    }

    friend inline bool operator<(const QQmlListIterator &i, const QQmlListIterator &j)
    {
        return i - j < 0;
    }

    friend inline bool operator>=(const QQmlListIterator &i, const QQmlListIterator &j)
    {
        return !(i < j);
    }

    friend inline bool operator>(const QQmlListIterator &i, const QQmlListIterator &j)
    {
        return i - j > 0;
    }

    friend inline bool operator<=(const QQmlListIterator &i, const QQmlListIterator &j)
    {
        return !(i > j);
    }

    friend inline QQmlListIterator operator+(qsizetype i, const QQmlListIterator &j)
    {
        return j + i;
    }

    friend inline qsizetype operator-(const QQmlListIterator &i, const QQmlListIterator &j)
    {
        return i.m_i - j.m_i;
    }

    QQmlListProperty<T> *m_list = nullptr;
    qsizetype m_i = 0;
};

template<typename T>
QQmlListIterator<T> begin(QQmlListProperty<T> &list)
{
    return QQmlListIterator<T>(&list, 0);
}

template<typename T>
QQmlListIterator<T> end(QQmlListProperty<T> &list)
{
    return QQmlListIterator<T>(&list, list.count(&list));
}

QT_END_NAMESPACE

#endif // QQMLLIST_P_H
