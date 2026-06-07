// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QRECURSIONWATCHER_P_H
#define QRECURSIONWATCHER_P_H

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

class QRecursionNode;
class QRecursionNode {
public:
    inline QRecursionNode();
    bool *_r;
};

template<class T, QRecursionNode T::*Node>
class QRecursionWatcher {
public:
    inline QRecursionWatcher(T *);
    inline ~QRecursionWatcher();
    inline bool hasRecursed() const;
private:
    T *_t;
    bool _r;
};

QRecursionNode::QRecursionNode()
: _r(nullptr)
{
}

template<class T, QRecursionNode T::*Node>
QRecursionWatcher<T, Node>::QRecursionWatcher(T *t)
: _t(t), _r(false)
{
    if ((_t->*Node)._r) *(_t->*Node)._r = true;
    (_t->*Node)._r = &_r;
}

template<class T, QRecursionNode T::*Node>
QRecursionWatcher<T, Node>::~QRecursionWatcher()
{
    if ((_t->*Node)._r == &_r) (_t->*Node)._r = nullptr;
}

template<class T, QRecursionNode T::*Node>
bool QRecursionWatcher<T, Node>::hasRecursed() const
{
    return _r;
}

QT_END_NAMESPACE

#endif // QRECURSIONWATCHER_P_H
