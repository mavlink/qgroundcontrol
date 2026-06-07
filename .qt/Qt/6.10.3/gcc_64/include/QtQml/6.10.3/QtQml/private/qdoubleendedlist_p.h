// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QDOUBLEENDEDLIST_P_H
#define QDOUBLEENDEDLIST_P_H

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

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QInheritedListNode
{
public:
    ~QInheritedListNode() { remove(); }
    bool isInList() const
    {
        Q_ASSERT((m_prev && m_next) || (!m_prev && !m_next));
        return m_prev != nullptr;
    }

private:
    template<class N>
    friend class QDoubleEndedList;

    void remove()
    {
        Q_ASSERT((m_prev && m_next) || (!m_prev && !m_next));
        if (!m_prev)
            return;

        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
        m_prev = nullptr;
        m_next = nullptr;
    }

    QInheritedListNode *m_next = nullptr;
    QInheritedListNode *m_prev = nullptr;
};

template<class N>
class QDoubleEndedList
{
public:
    QDoubleEndedList()
    {
        m_head.m_next = &m_head;
        m_head.m_prev = &m_head;
        assertHeadConsistent();
    }

    ~QDoubleEndedList()
    {
        assertHeadConsistent();
        while (!isEmpty())
            m_head.m_next->remove();
        assertHeadConsistent();
    }

    bool isEmpty() const
    {
        assertHeadConsistent();
        return m_head.m_next == &m_head;
    }

    void prepend(N *n)
    {
        assertHeadConsistent();
        QInheritedListNode *nnode = n;
        nnode->remove();

        nnode->m_next = m_head.m_next ? m_head.m_next : &m_head;
        nnode->m_next->m_prev = nnode;

        m_head.m_next = nnode;
        nnode->m_prev = &m_head;
        assertHeadConsistent();
    }

    void append(N *n)
    {
        assertHeadConsistent();
        QInheritedListNode *nnode = n;
        nnode->remove();

        nnode->m_prev = m_head.m_prev ? m_head.m_prev : &m_head;
        nnode->m_prev->m_next = nnode;

        m_head.m_prev = nnode;
        nnode->m_next = &m_head;
        assertHeadConsistent();
    }

    void remove(N *n) {
        Q_ASSERT(contains(n));
        QInheritedListNode *nnode = n;
        nnode->remove();
        assertHeadConsistent();
    }

    bool contains(const N *n) const
    {
        assertHeadConsistent();
        for (const QInheritedListNode *nnode = m_head.m_next;
             nnode != &m_head; nnode = nnode->m_next) {
            if (nnode == n)
                return true;
        }

        return false;
    }

    template<typename T, typename Node>
    class base_iterator {
    public:
        T *operator*() const { return QDoubleEndedList<N>::nodeToN(m_node); }
        T *operator->() const { return QDoubleEndedList<N>::nodeToN(m_node); }

        bool operator==(const base_iterator &other) const { return other.m_node == m_node; }
        bool operator!=(const base_iterator &other) const { return other.m_node != m_node; }

        base_iterator &operator++()
        {
            m_node = m_node->m_next;
            return *this;
        }

        base_iterator operator++(int)
        {
            const base_iterator self(m_node);
            m_node = m_node->m_next;
            return self;
        }

    private:
        friend class QDoubleEndedList<N>;

        base_iterator(Node *node) : m_node(node)
        {
            Q_ASSERT(m_node != nullptr);
        }

        Node *m_node = nullptr;
    };

    using iterator = base_iterator<N, QInheritedListNode>;
    using const_iterator = base_iterator<const N, const QInheritedListNode>;

    const N *first() const { return checkedNodeToN(m_head.m_next); }
    N *first() { return checkedNodeToN(m_head.m_next); }

    const N *last() const { return checkedNodeToN(m_head.m_prev); }
    N *last() { return checkedNodeToN(m_head.m_prev); }

    const N *next(const N *current) const
    {
        Q_ASSERT(contains(current));
        const QInheritedListNode *nnode = current;
        return checkedNodeToN(nnode->m_next);
    }

    N *next(N *current)
    {
        Q_ASSERT(contains(current));
        const QInheritedListNode *nnode = current;
        return checkedNodeToN(nnode->m_next);
    }

    const N *prev(const N *current) const
    {
        Q_ASSERT(contains(current));
        const QInheritedListNode *nnode = current;
        return checkedNodeToN(nnode->m_prev);
    }

    N *prev(N *current)
    {
        Q_ASSERT(contains(current));
        const QInheritedListNode *nnode = current;
        return checkedNodeToN(nnode->m_prev);
    }

    iterator begin()
    {
        assertHeadConsistent();
        return iterator(m_head.m_next);
    }

    iterator end()
    {
        assertHeadConsistent();
        return iterator(&m_head);
    }

    const_iterator begin() const
    {
        assertHeadConsistent();
        return const_iterator(m_head.m_next);
    }

    const_iterator end() const
    {
        assertHeadConsistent();
        return const_iterator(&m_head);
    }

    qsizetype count() const
    {
        assertHeadConsistent();
        qsizetype result = 0;
        for (const auto *node = m_head.m_next; node != &m_head; node = node->m_next)
            ++result;
        return result;
    }

private:
    static N *nodeToN(QInheritedListNode *node)
    {
        return static_cast<N *>(node);
    }

    static const N *nodeToN(const QInheritedListNode *node)
    {
        return static_cast<const N *>(node);
    }

    N *checkedNodeToN(QInheritedListNode *node) const
    {
        assertHeadConsistent();
        return (!node || node == &m_head) ? nullptr : nodeToN(node);
    }

    void assertHeadConsistent() const
    {
        Q_ASSERT(m_head.m_next != nullptr);
        Q_ASSERT(m_head.m_prev != nullptr);
        Q_ASSERT(m_head.m_next != &m_head || m_head.m_prev == &m_head);
    }

    QInheritedListNode m_head;
};

QT_END_NAMESPACE

#endif // QDOUBLEENDEDLIST_P_H
