// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSLIST_H
#define QJSLIST_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qjsengine.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#include <algorithm>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version. It will be kept compatible with the intended usage by
// code generated using qmlcachegen.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

struct QJSListIndexClamp
{
    static qsizetype clamp(qsizetype start, qsizetype max, qsizetype min = 0)
    {
        Q_ASSERT(min >= 0);
        Q_ASSERT(min <= max);
        return std::clamp(start < 0 ? max + qsizetype(start) : qsizetype(start), min, max);
    }
};

template<typename List, typename Value = typename List::value_type>
struct QJSList : private QJSListIndexClamp
{
    Q_DISABLE_COPY_MOVE(QJSList)

    QJSList(List *list, QJSEngine *engine) : m_list(list), m_engine(engine) {}

    Value at(qsizetype index) const
    {
        Q_ASSERT(index >= 0 && index < size());
        return *(m_list->cbegin() + index);
    }

    qsizetype size() const { return m_list->size(); }

    void resize(qsizetype size)
    {
        m_list->resize(size);
    }

    bool includes(const Value &value) const
    {
        return std::find(m_list->cbegin(), m_list->cend(), value) != m_list->cend();
    }

    bool includes(const Value &value, qsizetype start) const
    {
        return std::find(m_list->cbegin() + clamp(start, m_list->size()), m_list->cend(), value)
                != m_list->cend();
    }

    QString join(const QString &separator = QStringLiteral(",")) const
    {
        QString result;
        bool atBegin = true;
        std::for_each(m_list->cbegin(), m_list->cend(), [&](const Value &value) {
            if (atBegin)
                atBegin = false;
            else
                result += separator;
            result += m_engine->coerceValue<Value, QString>(value);
        });
        return result;
    }

    List slice() const
    {
        return *m_list;
    }
    List slice(qsizetype start) const
    {
        List result;
        std::copy(m_list->cbegin() + clamp(start, m_list->size()), m_list->cend(),
                  std::back_inserter(result));
        return result;
    }
    List slice(qsizetype start, qsizetype end) const
    {
        const qsizetype size = m_list->size();
        const qsizetype clampedStart = clamp(start, size);
        const qsizetype clampedEnd = clamp(end, size, clampedStart);

        List result;
        std::copy(m_list->cbegin() + clampedStart, m_list->cbegin() + clampedEnd,
                  std::back_inserter(result));
        return result;
    }

    qsizetype indexOf(const Value &value) const
    {
        const auto begin = m_list->cbegin();
        const auto end = m_list->cend();
        const auto it = std::find(begin, end, value);
        if (it == end)
            return -1;
        const qsizetype result = it - begin;
        Q_ASSERT(result >= 0);
        return result;
    }
    qsizetype indexOf(const Value &value, qsizetype start) const
    {
        const auto begin = m_list->cbegin();
        const auto end = m_list->cend();
        const auto it = std::find(begin + clamp(start, m_list->size()), end, value);
        if (it == end)
            return -1;
        const qsizetype result = it - begin;
        Q_ASSERT(result >= 0);
        return result;
    }

    qsizetype lastIndexOf(const Value &value) const
    {
        const auto begin = std::make_reverse_iterator(m_list->cend());
        const auto end = std::make_reverse_iterator(m_list->cbegin());
        const auto it = std::find(begin, end, value);
        return (end - it) - 1;
    }
    qsizetype lastIndexOf(const Value &value, qsizetype start) const
    {
        const qsizetype size = m_list->size();
        if (size == 0)
            return -1;

        // Construct a one-past-end iterator as input.
        const qsizetype clampedStart = std::min(clamp(start, size), size - 1);
        const auto begin = std::make_reverse_iterator(m_list->cbegin() + clampedStart + 1);

        const auto end = std::make_reverse_iterator(m_list->cbegin());
        const auto it = std::find(begin, end, value);
        return (end - it) - 1;
    }

    QString toString() const { return join(); }

private:
    List *m_list = nullptr;
    QJSEngine *m_engine = nullptr;
};

template<>
struct QJSList<QQmlListProperty<QObject>, QObject *>  : private QJSListIndexClamp
{
    Q_DISABLE_COPY_MOVE(QJSList)

    QJSList(QQmlListProperty<QObject> *list, QJSEngine *engine) : m_list(list), m_engine(engine) {}

    QObject *at(qsizetype index) const
    {
        Q_ASSERT(index >= 0 && index < size());
        return m_list->at(m_list, index);
    }

    qsizetype size() const
    {
        return m_list->count(m_list);
    }

    void resize(qsizetype size)
    {
        qsizetype current = m_list->count(m_list);
        if (current < size && m_list->append) {
            do {
                m_list->append(m_list, nullptr);
            } while (++current < size);
        } else if (current > size && m_list->removeLast) {
            do {
                m_list->removeLast(m_list);
            } while (--current > size);
        }
    }

    bool includes(const QObject *value) const
    {
        if (!m_list->count || !m_list->at)
            return false;

        const qsizetype size = m_list->count(m_list);
        for (qsizetype i = 0; i < size; ++i) {
            if (m_list->at(m_list, i) == value)
                return true;
        }

        return false;
    }
    bool includes(const QObject *value, qsizetype start) const
    {
        if (!m_list->count || !m_list->at)
            return false;

        const qsizetype size = m_list->count(m_list);
        for (qsizetype i = clamp(start, size); i < size; ++i) {
            if (m_list->at(m_list, i) == value)
                return true;
        }

        return false;
    }

    QString join(const QString &separator = QStringLiteral(",")) const
    {
        QString result;
        if (!m_list->count || !m_list->at)
            return result;

        for (qsizetype i = 0, end = m_list->count(m_list); i < end; ++i) {
            if (i != 0)
                result += separator;
            result += m_engine->coerceValue<QObject *, QString>(m_list->at(m_list, i));
        }

        return result;
    }

    QObjectList slice() const
    {
        return m_list->toList<QObjectList>();
    }
    QObjectList slice(qsizetype start) const
    {
        QObjectList result;
        if (!m_list->count || !m_list->at)
            return result;

        const qsizetype size = m_list->count(m_list);
        const qsizetype clampedStart = clamp(start, size);
        result.reserve(size - clampedStart);
        for (qsizetype i = clampedStart; i < size; ++i)
            result.append(m_list->at(m_list, i));
        return result;
    }
    QObjectList slice(qsizetype start, qsizetype end) const
    {
        QObjectList result;
        if (!m_list->count || !m_list->at)
            return result;

        const qsizetype size = m_list->count(m_list);
        const qsizetype clampedStart = clamp(start, size);
        const qsizetype clampedEnd = clamp(end, size, clampedStart);
        result.reserve(clampedEnd - clampedStart);
        for (qsizetype i = clampedStart; i < clampedEnd; ++i)
            result.append(m_list->at(m_list, i));
        return result;
    }

    qsizetype indexOf(const QObject *value) const
    {
        if (!m_list->count || !m_list->at)
            return -1;

        const qsizetype end = m_list->count(m_list);
        for (qsizetype i = 0; i < end; ++i) {
            if (m_list->at(m_list, i) == value)
                return i;
        }
        return -1;
    }
    qsizetype indexOf(const QObject *value, qsizetype start) const
    {
        if (!m_list->count || !m_list->at)
            return -1;

        const qsizetype size = m_list->count(m_list);
        for (qsizetype i = clamp(start, size); i < size; ++i) {
            if (m_list->at(m_list, i) == value)
                return i;
        }
        return -1;
    }

    qsizetype lastIndexOf(const QObject *value) const
    {
        if (!m_list->count || !m_list->at)
            return -1;

        for (qsizetype i = m_list->count(m_list) - 1; i >= 0; --i) {
            if (m_list->at(m_list, i) == value)
                return i;
        }
        return -1;
    }
    qsizetype lastIndexOf(const QObject *value, qsizetype start) const
    {
        if (!m_list->count || !m_list->at)
            return -1;

        const qsizetype size = m_list->count(m_list);
        if (size == 0)
            return -1;

        qsizetype clampedStart = std::min(clamp(start, size), size - 1);
        for (qsizetype i = clampedStart; i >= 0; --i) {
            if (m_list->at(m_list, i) == value)
                return i;
        }
        return -1;
    }

    QString toString() const { return join(); }

private:
    QQmlListProperty<QObject> *m_list = nullptr;
    QJSEngine *m_engine = nullptr;
};

struct QJSListForInIterator
{
public:
    using Ptr = QJSListForInIterator *;
    template<typename List, typename Value>
    void init(const QJSList<List, Value> &list)
    {
        m_index = 0;
        m_size = list.size();
    }

    bool hasNext() const { return m_index < m_size; }
    qsizetype next() { return m_index++; }

private:
    qsizetype m_index;
    qsizetype m_size;
};

// QJSListForInIterator must not require initialization so that we can jump over it with goto.
static_assert(std::is_trivially_copyable_v<QJSListForInIterator>);
static_assert(std::is_trivially_default_constructible_v<QJSListForInIterator>);

struct QJSListForOfIterator
{
public:
    using Ptr = QJSListForOfIterator *;
    void init() { m_index = 0; }

    template<typename List, typename Value>
    bool hasNext(const QJSList<List, Value> &list) const { return m_index < list.size(); }

    template<typename List, typename Value>
    Value next(const QJSList<List, Value> &list) { return list.at(m_index++); }

private:
    qsizetype m_index;
};

// QJSListForOfIterator must not require initialization so that we can jump over it with goto.
static_assert(std::is_trivially_copyable_v<QJSListForOfIterator>);
static_assert(std::is_trivially_default_constructible_v<QJSListForOfIterator>);

QT_END_NAMESPACE

#endif // QJSLIST_H
