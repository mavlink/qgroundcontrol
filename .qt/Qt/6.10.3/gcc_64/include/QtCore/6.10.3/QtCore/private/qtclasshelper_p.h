// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTCLASSHELPER_P_H
#define QTCLASSHELPER_P_H

#include <QtCore/private/qglobal_p.h>

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

#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

/*
    Helper for setters overloaded on lvalue/rvalue. If the setter is doing a
    lot of work around the actual assignment, a common pattern is to create a
    private helper method

        void doSetFoo(const Foo &lvalue, Foo *rvalue);

    and implement the two setters inline by calling that function:

        void setFoo(const Foo &f) { doSetFoo(f, nullptr); }
        void setFoo(Foo &&f) { doSetFoo(f, &f); }

    Then, in doSetFoo(), when assigning the argument to the member, use this
    function:

        q_choose_assign(m_foo, lvalue, rvalue);

    or, when appending to a container,

        q_choose_append(m_container, lvalue, rvalue);

    The functions mandate (in the C++ sense) that all arguments are the same
    type, so they deduce each argument separately and then static_assert that
    they're the same. If we need std::exchange()-like mixed types later, it's
    easy to relax. For now, avoid being overly general.
*/
template <typename T, typename U, typename V>
decltype(auto) q_choose_assign(T &var, const U &lvalue, V *rvalue)
{
    static_assert(std::is_same_v<T, U>, "all arguments must be of the same type");
    static_assert(std::is_same_v<U, V>, "all arguments must be of the same type");
    if (rvalue)
        return var = std::move(*rvalue);
    else
        return var = lvalue;
}

template <typename Container, typename U, typename V>
decltype(auto) q_choose_append(Container &c, const U &lvalue, V *rvalue)
{
    static_assert(std::is_same_v<typename Container::value_type, U>, "arguments must match container");
    static_assert(std::is_same_v<U, V>, "all arguments must be of the same type");
    if (rvalue)
        c.push_back(std::move(*rvalue));
    else
        c.push_back(lvalue);
    return c.back();
}

QT_END_NAMESPACE

#endif // QTCLASSHELPER_P_H
