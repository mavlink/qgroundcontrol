// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QV4MEMBERDATA_H
#define QV4MEMBERDATA_H

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

#include "qv4global_p.h"
#include "qv4managed_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

#define MemberDataMembers(class, Member) \
    Member(class, ValueArray, ValueArray, values)

DECLARE_HEAP_OBJECT(MemberData, Base) {
    DECLARE_MARKOBJECTS(MemberData)
};
static_assert(std::is_trivially_copyable_v<MemberData>);
static_assert(std::is_trivially_default_constructible_v<MemberData>);

}

struct MemberData : Managed
{
    V4_MANAGED(MemberData, Managed)
    V4_INTERNALCLASS(MemberData)

    const Value &operator[] (uint idx) const { return d()->values[idx]; }
    const Value *data() const { return d()->values.data(); }
    void set(EngineBase *e, uint index, Value v) { d()->values.set(e, index, v); }
    void set(EngineBase *e, uint index, Heap::Base *b) { d()->values.set(e, index, b); }

    inline uint size() const { return d()->values.size; }

    static Heap::MemberData *allocate(QV4::ExecutionEngine *e, uint n, Heap::MemberData *old = nullptr);
};

}

QT_END_NAMESPACE

#endif
