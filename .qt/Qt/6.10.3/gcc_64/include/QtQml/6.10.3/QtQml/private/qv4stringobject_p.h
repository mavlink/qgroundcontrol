// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4STRINGOBJECT_P_H
#define QV4STRINGOBJECT_P_H

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
#include "qv4functionobject_p.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

#define StringObjectMembers(class, Member) \
    Member(class, Pointer, String *, string)

DECLARE_HEAP_OBJECT(StringObject, Object) {
    DECLARE_MARKOBJECTS(StringObject)

    enum {
        LengthPropertyIndex = 0
    };

    void init(bool /*don't init*/)
    { Object::init(); }
    void init();
    void init(const QV4::String *string);

    Heap::String *getIndex(uint index) const;
    uint length() const;
};

struct StringCtor : FunctionObject {
    void init(QV4::ExecutionEngine *engine);
};

}

struct StringObject: Object {
    V4_OBJECT2(StringObject, Object)
    Q_MANAGED_TYPE(StringObject)
    V4_INTERNALCLASS(StringObject)
    V4_PROTOTYPE(stringPrototype)

    Heap::String *getIndex(uint index) const {
        return d()->getIndex(index);
    }
    uint length() const {
        return d()->length();
    }

    using Object::getOwnProperty;
protected:
    static bool virtualDeleteProperty(Managed *m, PropertyKey id);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
};

struct StringCtor: FunctionObject
{
    V4_OBJECT2(StringCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_fromCharCode(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_fromCodePoint(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_raw(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct StringPrototype: StringObject
{
    V4_PROTOTYPE(objectPrototype)
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_charAt(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_charCodeAt(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_codePointAt(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_concat(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_endsWith(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_indexOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_includes(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_lastIndexOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_localeCompare(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_match(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_normalize(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_padEnd(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_padStart(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_repeat(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_replace(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_search(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_split(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_startsWith(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_substr(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_substring(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLowerCase(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLocaleLowerCase(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toUpperCase(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toLocaleUpperCase(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_trim(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_iterator(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
