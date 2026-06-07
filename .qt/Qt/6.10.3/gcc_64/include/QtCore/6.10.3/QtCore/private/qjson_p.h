// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QJSON_P_H
#define QJSON_P_H

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

#include <qjsonvalue.h>
#include <qcborvalue.h>
#include <private/qcborvalue_p.h>

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
#  include <qjsonarray.h>
#  include <qjsonobject.h>
#endif

QT_BEGIN_NAMESPACE

namespace QJsonPrivate {

template<typename Element, typename ElementsIterator>
struct ObjectIterator
{
    using pointer = Element *;

    struct value_type;
    struct reference {
        reference(Element &ref) : m_key(&ref) {}

        reference() = delete;
        ~reference() = default;

        reference(const reference &other) = default;
        reference(reference &&other) = default;

        reference &operator=(const value_type &value);
        reference &operator=(const reference &other)
        {
            if (m_key != other.m_key) {
                key() = other.key();
                value() = other.value();
            }
            return *this;
        }

        reference &operator=(reference &&other)
        {
            key() = other.key();
            value() = other.value();
            return *this;
        }

        Element &key() { return *m_key; }
        Element &value() { return *(m_key + 1); }

        const Element &key() const { return *m_key; }
        const Element &value() const { return *(m_key + 1); }


    private:
        Element *m_key;
    };

    struct value_type {
        value_type(reference ref) : m_key(ref.key()), m_value(ref.value()) {}

        Element key() const { return m_key; }
        Element value() const { return m_value; }
    private:
        Element m_key;
        Element m_value;
    };

    using difference_type = typename QList<Element>::difference_type;
    using iterator_category = std::random_access_iterator_tag;

    ObjectIterator() = default;
    ObjectIterator(ElementsIterator it) : it(it) {}
    ElementsIterator elementsIterator() { return it; }

    ObjectIterator operator++(int) { ObjectIterator ret(it); it += 2; return ret; }
    ObjectIterator &operator++() { it += 2; return *this; }
    ObjectIterator &operator+=(difference_type n) { it += 2 * n; return *this; }

    ObjectIterator operator--(int) { ObjectIterator ret(it); it -= 2; return ret; }
    ObjectIterator &operator--() { it -= 2; return *this; }
    ObjectIterator &operator-=(difference_type n) { it -= 2 * n; return *this; }

    reference operator*() const { return *it; }
    reference operator[](qsizetype n) const { return it[n * 2]; }

    bool operator<(ObjectIterator other) const { return it < other.it; }
    bool operator>(ObjectIterator other) const { return it > other.it; }
    bool operator<=(ObjectIterator other) const { return it <= other.it; }
    bool operator>=(ObjectIterator other) const { return it >= other.it; }

    ElementsIterator it;
};

template<typename Element, typename ElementsIterator>
inline ObjectIterator<Element, ElementsIterator> operator+(
        ObjectIterator<Element, ElementsIterator> a,
        typename ObjectIterator<Element, ElementsIterator>::difference_type n)
{
    return {a.elementsIterator() + 2 * n};
}
template<typename Element, typename ElementsIterator>
inline ObjectIterator<Element, ElementsIterator> operator+(
        qsizetype n, ObjectIterator<Element, ElementsIterator> a)
{
    return {a.elementsIterator() + 2 * n};
}
template<typename Element, typename ElementsIterator>
inline ObjectIterator<Element, ElementsIterator> operator-(
        ObjectIterator<Element, ElementsIterator> a,
        typename ObjectIterator<Element, ElementsIterator>::difference_type n)
{
    return {a.elementsIterator() - 2 * n};
}
template<typename Element, typename ElementsIterator>
inline qsizetype operator-(
        ObjectIterator<Element, ElementsIterator> a,
        ObjectIterator<Element, ElementsIterator> b)
{
    return (a.elementsIterator() - b.elementsIterator()) / 2;
}
template<typename Element, typename ElementsIterator>
inline bool operator!=(
        ObjectIterator<Element, ElementsIterator> a,
        ObjectIterator<Element, ElementsIterator> b)
{
    return a.elementsIterator() != b.elementsIterator();
}
template<typename Element, typename ElementsIterator>
inline bool operator==(
        ObjectIterator<Element, ElementsIterator> a,
        ObjectIterator<Element, ElementsIterator> b)
{
    return a.elementsIterator() == b.elementsIterator();
}

using KeyIterator = ObjectIterator<QtCbor::Element, QList<QtCbor::Element>::iterator>;
using ConstKeyIterator = ObjectIterator<const QtCbor::Element, QList<QtCbor::Element>::const_iterator>;

template<>
inline KeyIterator::reference &KeyIterator::reference::operator=(const KeyIterator::value_type &value)
{
    *m_key = value.key();
    *(m_key + 1) = value.value();
    return *this;
}

inline void swap(KeyIterator::reference a, KeyIterator::reference b)
{
    KeyIterator::value_type t = a;
    a = b;
    b = t;
}

class Value
{
public:
    static qint64 valueHelper(const QCborValue &v) { return v.n; }
    static QCborContainerPrivate *container(const QCborValue &v) { return v.container; }
    static const QCborContainerPrivate *container(QJsonValueConstRef r) noexcept
    {
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
        return (r.is_object ? r.o->o : r.a->a).data();
#else
        return r.d;
#endif
    }
    static QCborContainerPrivate *container(QJsonValueRef r) noexcept
    {
        return const_cast<QCborContainerPrivate *>(container(QJsonValueConstRef(r)));
    }
    static qsizetype indexHelper(QJsonValueConstRef r) noexcept
    {
        qsizetype index = r.index;
        if (r.is_object)
            index = index * 2 + 1;
        return index;
    }
    static const QtCbor::Element &elementHelper(QJsonValueConstRef r) noexcept
    {
        return container(r)->elements.at(indexHelper(r));
    }

    static QJsonValue fromTrustedCbor(const QCborValue &v)
    {
        QJsonValue result;
        result.value = v;
        return result;
    }
};

class Variant
{
public:
    static QJsonObject toJsonObject(const QVariantMap &map);
    static QJsonArray toJsonArray(const QVariantList &list);
};

} // namespace QJsonPrivate

QT_END_NAMESPACE

#endif // QJSON_P_H
