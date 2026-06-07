// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QANIMATIONJOBUTIL_P_H
#define QANIMATIONJOBUTIL_P_H

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

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtconfigmacros.h>

#include <type_traits>

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU_ONLY >= 1300
#  define ACTION_IF_DISABLE_DANGLING_POINTER_WARNING    QT_WARNING_DISABLE_GCC("-Wdangling-pointer")
#else
#  define ACTION_IF_DISABLE_DANGLING_POINTER_WARNING
#endif

// SelfDeletable is used for self-destruction detection along with
// ACTION_IF_DELETED and RETURN_IF_DELETED macros. While using, the objects
// under test should have a member m_selfDeletable of type SelfDeletable
struct SelfDeletable {
    ~SelfDeletable() {
        if (m_wasDeleted)
            *m_wasDeleted = true;
    }
    bool *m_wasDeleted = nullptr;
};

// \param p pointer to object under test, which should have a member m_selfDeletable of type SelfDeletable
// \param func statements or functions that to be executed under test.
// \param action post process if p was deleted under test.
#define ACTION_IF_DELETED(p, func, action) \
do { \
    QT_WARNING_PUSH \
    ACTION_IF_DISABLE_DANGLING_POINTER_WARNING \
    static_assert(std::is_same<decltype((p)->m_selfDeletable), SelfDeletable>::value, "m_selfDeletable must be SelfDeletable");\
    bool *prevWasDeleted = (p)->m_selfDeletable.m_wasDeleted; \
    bool wasDeleted = false; \
    (p)->m_selfDeletable.m_wasDeleted = &wasDeleted; \
    {func;} \
    if (wasDeleted) { \
        if (prevWasDeleted) \
            *prevWasDeleted = true; \
        {action;} \
    } \
    (p)->m_selfDeletable.m_wasDeleted = prevWasDeleted; \
    QT_WARNING_POP \
} while (false)

#define RETURN_IF_DELETED(func) \
ACTION_IF_DELETED(this, func, return)

QT_END_NAMESPACE

#endif // QANIMATIONJOBUTIL_P_H
