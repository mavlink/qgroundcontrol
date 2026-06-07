// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QFIELDLIST_P_H
#define QFIELDLIST_P_H

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
#include <QtCore/qtaggedpointer.h>


// QForwardFieldList is a super simple linked list that can only prepend
template<class N, N *N::*nextMember, typename Tag = QtPrivate::TagInfo<N>>
class QForwardFieldList
{
public:
    inline QForwardFieldList();
    inline N *first() const;
    inline N *takeFirst();

    inline void prepend(N *);
    template <typename OtherTag>
    inline void copyAndClearPrepend(QForwardFieldList<N, nextMember, OtherTag> &);

    inline bool isEmpty() const;
    inline bool isOne() const;
    inline bool isMany() const;

    static inline N *next(N *v);

    inline Tag tag() const;
    inline void setTag(Tag t);
private:
    QTaggedPointer<N, Tag> _first;
};

// QFieldList is a simple linked list, that can append and prepend and also
// maintains a count
template<class N, N *N::*nextMember>
class QFieldList
{
public:
    inline QFieldList();
    inline N *first() const;
    inline N *takeFirst();

    inline void append(N *);
    inline void prepend(N *);

    inline bool isEmpty() const;
    inline bool isOne() const;
    inline bool isMany() const;
    inline int count() const;

    inline void append(QFieldList<N, nextMember> &);
    inline void prepend(QFieldList<N, nextMember> &);
    inline void insertAfter(N *, QFieldList<N, nextMember> &);

    inline void copyAndClear(QFieldList<N, nextMember> &);
    template <typename Tag>
    inline void copyAndClearAppend(QForwardFieldList<N, nextMember, Tag> &);
    template <typename Tag>
    inline void copyAndClearPrepend(QForwardFieldList<N, nextMember, Tag> &);

    static inline N *next(N *v);

    inline bool flag() const;
    inline void setFlag();
    inline void clearFlag();
    inline void setFlagValue(bool);
private:
    N *_first;
    N *_last;
    quint32 _flag:1;
    quint32 _count:31;
};

template<class N, N *N::*nextMember, typename Tag>
QForwardFieldList<N, nextMember, Tag>::QForwardFieldList()
{
}

template<class N, N *N::*nextMember, typename Tag>
N *QForwardFieldList<N, nextMember, Tag>::first() const
{
    return _first.data();
}

template<class N, N *N::*nextMember, typename Tag>
N *QForwardFieldList<N, nextMember, Tag>::takeFirst()
{
    N *value = _first.data();
    if (value) {
        _first = next(value);
        value->*nextMember = nullptr;
    }
    return value;
}

template<class N, N *N::*nextMember, typename Tag>
void QForwardFieldList<N, nextMember, Tag>::prepend(N *v)
{
    Q_ASSERT(v->*nextMember == nullptr);
    v->*nextMember = _first.data();
    _first = v;
}

template<class N, N *N::*nextMember, typename Tag>
template <typename OtherTag>
void QForwardFieldList<N, nextMember, Tag>::copyAndClearPrepend(QForwardFieldList<N, nextMember, OtherTag> &o)
{
    _first = nullptr;
    while (N *n = o.takeFirst()) prepend(n);
}

template<class N, N *N::*nextMember, typename Tag>
bool QForwardFieldList<N, nextMember, Tag>::isEmpty() const
{
    return _first.isNull();
}

template<class N, N *N::*nextMember, typename Tag>
bool QForwardFieldList<N, nextMember, Tag>::isOne() const
{
    return _first.data() && _first->*nextMember == 0;
}

template<class N, N *N::*nextMember, typename Tag>
bool QForwardFieldList<N, nextMember, Tag>::isMany() const
{
    return _first.data() && _first->*nextMember != 0;
}

template<class N, N *N::*nextMember, typename Tag>
N *QForwardFieldList<N, nextMember, Tag>::next(N *v)
{
    Q_ASSERT(v);
    return v->*nextMember;
}

template<class N, N *N::*nextMember, typename Tag>
Tag QForwardFieldList<N, nextMember, Tag>::tag() const
{
    return _first.tag();
}

template<class N, N *N::*nextMember, typename Tag>
void QForwardFieldList<N, nextMember, Tag>::setTag(Tag t)
{
    _first.setTag(t);
}

template<class N, N *N::*nextMember>
QFieldList<N, nextMember>::QFieldList()
: _first(nullptr), _last(nullptr), _flag(0), _count(0)
{
}

template<class N, N *N::*nextMember>
N *QFieldList<N, nextMember>::first() const
{
    return _first;
}

template<class N, N *N::*nextMember>
N *QFieldList<N, nextMember>::takeFirst()
{
    N *value = _first;
    if (value) {
        _first = next(value);
        if (_last == value) {
            Q_ASSERT(_first == nullptr);
            _last = nullptr;
        }
        value->*nextMember = nullptr;
        --_count;
    }
    return value;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::append(N *v)
{
    Q_ASSERT(v->*nextMember == nullptr);
    if (isEmpty()) {
        _first = v;
        _last = v;
    } else {
        _last->*nextMember = v;
        _last = v;
    }
    ++_count;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::prepend(N *v)
{
    Q_ASSERT(v->*nextMember == nullptr);
    if (isEmpty()) {
        _first = v;
        _last = v;
    } else {
        v->*nextMember = _first;
        _first = v;
    }
    ++_count;
}

template<class N, N *N::*nextMember>
bool QFieldList<N, nextMember>::isEmpty() const
{
    return _count == 0;
}

template<class N, N *N::*nextMember>
bool QFieldList<N, nextMember>::isOne() const
{
    return _count == 1;
}

template<class N, N *N::*nextMember>
bool QFieldList<N, nextMember>::isMany() const
{
    return _count > 1;
}

template<class N, N *N::*nextMember>
int QFieldList<N, nextMember>::count() const
{
    return _count;
}

template<class N, N *N::*nextMember>
N *QFieldList<N, nextMember>::next(N *v)
{
    Q_ASSERT(v);
    return v->*nextMember;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::append(QFieldList<N, nextMember> &o)
{
    if (!o.isEmpty()) {
        if (isEmpty()) {
            _first = o._first;
            _last = o._last;
            _count = o._count;
        } else {
            _last->*nextMember = o._first;
            _last = o._last;
            _count += o._count;
        }
        o._first = o._last = 0; o._count = 0;
    }
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::prepend(QFieldList<N, nextMember> &o)
{
    if (!o.isEmpty()) {
        if (isEmpty()) {
            _first = o._first;
            _last = o._last;
            _count = o._count;
        } else {
            o._last->*nextMember = _first;
            _first = o._first;
            _count += o._count;
        }
        o._first = o._last = 0; o._count = 0;
    }
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::insertAfter(N *after, QFieldList<N, nextMember> &o)
{
    if (after == 0) {
        prepend(o);
    } else if (after == _last) {
        append(o);
    } else if (!o.isEmpty()) {
        if (isEmpty()) {
            _first = o._first;
            _last = o._last;
            _count = o._count;
        } else {
            o._last->*nextMember = after->*nextMember;
            after->*nextMember = o._first;
            _count += o._count;
        }
        o._first = o._last = 0; o._count = 0;
    }
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::copyAndClear(QFieldList<N, nextMember> &o)
{
    _first = o._first;
    _last = o._last;
    _count = o._count;
    o._first = o._last = nullptr;
    o._count = 0;
}

template<class N, N *N::*nextMember>
template <typename Tag>
void QFieldList<N, nextMember>::copyAndClearAppend(QForwardFieldList<N, nextMember, Tag> &o)
{
    _first = 0;
    _last = 0;
    _count = 0;
    while (N *n = o.takeFirst()) append(n);
}

template<class N, N *N::*nextMember>
template <typename Tag>
void QFieldList<N, nextMember>::copyAndClearPrepend(QForwardFieldList<N, nextMember, Tag> &o)
{
    _first = nullptr;
    _last = nullptr;
    _count = 0;
    while (N *n = o.takeFirst()) prepend(n);
}

template<class N, N *N::*nextMember>
bool QFieldList<N, nextMember>::flag() const
{
    return _flag;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::setFlag()
{
    _flag = true;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::clearFlag()
{
    _flag = false;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::setFlagValue(bool v)
{
    _flag = v;
}

#endif // QFIELDLIST_P_H
