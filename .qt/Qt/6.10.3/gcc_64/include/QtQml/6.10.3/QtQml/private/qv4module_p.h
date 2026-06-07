// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4MODULE
#define QV4MODULE

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

#include "qv4object_p.h"
#include "qv4context_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

#define ModuleMembers(class, Member) \
    Member(class, NoMark, ExecutableCompilationUnit *, unit) \
    Member(class, Pointer, CallContext *, scope) \
    Member(class, HeapValue, HeapValue, self) \
    Member(class, NoMark, bool, evaluated)

DECLARE_EXPORTED_HEAP_OBJECT(Module, Object) {
    DECLARE_MARKOBJECTS(Module)

    void init(ExecutionEngine *engine, ExecutableCompilationUnit *moduleUnit);
};

}

struct Q_QML_EXPORT Module : public Object {
    V4_OBJECT2(Module, Object)

    void evaluate();
    const Value *resolveExport(PropertyKey key) const;

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
    static bool virtualHasProperty(const Managed *m, PropertyKey id);
    static bool virtualPreventExtensions(Managed *);
    static bool virtualDefineOwnProperty(Managed *, PropertyKey, const Property *, PropertyAttributes);
    static bool virtualPut(Managed *, PropertyKey, const Value &, Value *);
    static bool virtualDeleteProperty(Managed *m, PropertyKey id);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
    static Heap::Object *virtualGetPrototypeOf(const Managed *);
    static bool virtualSetPrototypeOf(Managed *, const Object *proto);
    static bool virtualIsExtensible(const Managed *);
};

}

QT_END_NAMESPACE

#endif // QV4MODULE
