// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QINTRUSIVELIST_P_H
#define QINTRUSIVELIST_P_H

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

class QIntrusiveListNode
{
public:
    ~QIntrusiveListNode() { remove(); }

    void remove()
    {
        if (_prev) *_prev = _next;
        if (_next) _next->_prev = _prev;
        _prev = nullptr;
        _next = nullptr;
    }

    bool isInList() const { return _prev != nullptr; }

private:
    template<class N, QIntrusiveListNode N::*member>
    friend class QIntrusiveList;

    QIntrusiveListNode *_next = nullptr;
    QIntrusiveListNode**_prev = nullptr;
};

template<class N, QIntrusiveListNode N::*member>
class QIntrusiveList
{
private:
    template<typename O>
    class iterator_impl {
    public:
        iterator_impl() = default;
        iterator_impl(O value) : _value(value) {}

        O operator*() const { return _value; }
        O operator->() const { return _value; }
        bool operator==(const iterator_impl &other) const { return other._value == _value; }
        bool operator!=(const iterator_impl &other) const { return other._value != _value; }
        iterator_impl &operator++()
        {
            _value = QIntrusiveList<N, member>::next(_value);
            return *this;
        }

    protected:
        O _value = nullptr;
    };

public:
    class iterator : public iterator_impl<N *>
    {
    public:
        iterator() = default;
        iterator(N *value) : iterator_impl<N *>(value) {}

        iterator &erase()
        {
            N *old = this->_value;
            this->_value = QIntrusiveList<N, member>::next(this->_value);
            (old->*member).remove();
            return *this;
        }
    };

    using const_iterator = iterator_impl<const N *>;

    using Iterator = iterator;
    using ConstIterator = const_iterator;

    ~QIntrusiveList() { while (__first) __first->remove(); }

    bool isEmpty() const { return __first == nullptr; }

    void insert(N *n)
    {
        QIntrusiveListNode *nnode = &(n->*member);
        nnode->remove();

        nnode->_next = __first;
        if (nnode->_next) nnode->_next->_prev = &nnode->_next;
        __first = nnode;
        nnode->_prev = &__first;
    }

    void remove(N *n)
    {
        QIntrusiveListNode *nnode = &(n->*member);
        nnode->remove();
    }

    bool contains(const N *n) const
    {
        QIntrusiveListNode *nnode = __first;
        while (nnode) {
            if (nodeToN(nnode) == n)
                return true;
            nnode = nnode->_next;
        }
        return false;
    }

    const N *first() const { return __first ? nodeToN(__first) : nullptr; }
    N *first() { return __first ? nodeToN(__first) : nullptr; }

    template<typename O>
    static O next(O current)
    {
        QIntrusiveListNode *nextnode = (current->*member)._next;
        return nextnode ? nodeToN(nextnode) : nullptr;
    }

    iterator begin() { return __first ? iterator(nodeToN(__first)) : iterator(); }
    iterator end() { return iterator(); }

    const_iterator begin() const
    {
        return __first ? const_iterator(nodeToN(__first)) : const_iterator();
    }

    const_iterator end() const { return const_iterator(); }

private:

    static N *nodeToN(QIntrusiveListNode *node)
    {
        QT_WARNING_PUSH
#if defined(Q_CC_CLANG) && Q_CC_CLANG >= 1300
        QT_WARNING_DISABLE_CLANG("-Wnull-pointer-subtraction")
#endif
        return (N *)((char *)node - ((char *)&(((N *)nullptr)->*member) - (char *)nullptr));
        QT_WARNING_POP
    }

    QIntrusiveListNode *__first = nullptr;
};

QT_END_NAMESPACE

#endif // QINTRUSIVELIST_P_H
