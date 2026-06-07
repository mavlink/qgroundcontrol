// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4PROPERTYKEY_H
#define QV4PROPERTYKEY_H

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

#include <private/qv4writebarrier_p.h>
#include <private/qv4global_p.h>
#include <private/qv4staticvalue_p.h>
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

class QString;

namespace QV4 {

struct PropertyKey
{
private:
    // Property keys are Strings, Symbols or unsigned integers.
    // For convenience we derive them from Values, allowing us to store them
    // on the JS stack
    //
    // They do however behave somewhat different than a Value:
    // * If the key is a String, the pointer to the string is stored in the identifier
    // table and thus unique.
    // * If the key is a Symbol it simply points to the referenced symbol object
    // * if the key is an array index (a uint < UINT_MAX), it's encoded as an
    // integer value
    QV4::StaticValue val;

    inline bool isManaged() const { return val.isManaged(); }
    inline quint32 value() const { return val.value(); }

public:
    static PropertyKey invalid()
    {
        PropertyKey key;
        key.val = StaticValue::undefinedValue();
        return key;
    }

    static PropertyKey fromArrayIndex(uint idx)
    {
        PropertyKey key;
        key.val.setInt_32(idx);
        return key;
    }

    bool isStringOrSymbol() const { return isManaged(); }
    uint asArrayIndex() const
    {
        Q_ASSERT(isArrayIndex());
        return value();
    }

    bool isArrayIndex() const { return val.isInteger(); }
    bool isValid() const { return !val.isUndefined(); }

    // We cannot #include the declaration of Heap::StringOrSymbol here.
    // Therefore we do some gymnastics to enforce the type safety.

    template<typename StringOrSymbol = Heap::StringOrSymbol, typename Engine = QV4::EngineBase>
    static PropertyKey fromStringOrSymbol(Engine *engine, StringOrSymbol *b)
    {
        static_assert(std::is_base_of_v<Heap::StringOrSymbol, StringOrSymbol>);
        PropertyKey key;
        QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *stack) {
            if constexpr (QV4::WriteBarrier::isInsertionBarrier) {
                // treat this as an insertion - the StringOrSymbol becomes reachable
                // via the propertykey, so we consequently need to mark it durnig gc
                b->mark(stack);
            }
        });
        key.val.setM(b);
        Q_ASSERT(key.isManaged());
        return key;
    }

    template<typename StringOrSymbol = Heap::StringOrSymbol>
    StringOrSymbol *asStringOrSymbol() const
    {
        static_assert(std::is_base_of_v<Heap::StringOrSymbol, StringOrSymbol>);
        if (!isManaged())
            return nullptr;
        return static_cast<StringOrSymbol *>(val.m());
    }

    Q_QML_EXPORT bool isString() const;
    Q_QML_EXPORT bool isSymbol() const;
    bool isCanonicalNumericIndexString() const;

    Q_QML_EXPORT QString toQString() const;
    Heap::StringOrSymbol *toStringOrSymbol(ExecutionEngine *e);
    quint64 id() const { return val._val; }
    static PropertyKey fromId(quint64 id) {
        PropertyKey key; key.val._val = id; return key;
    }

    enum FunctionNamePrefix {
        None,
        Getter,
        Setter
    };
    Heap::String *asFunctionName(ExecutionEngine *e, FunctionNamePrefix prefix) const;

    bool operator ==(const PropertyKey &other) const { return val._val == other.val._val; }
    bool operator !=(const PropertyKey &other) const { return val._val != other.val._val; }
    bool operator <(const PropertyKey &other) const { return val._val < other.val._val; }
    friend size_t qHash(const PropertyKey &key, size_t seed = 0) { return qHash(key.val._val, seed); }
};

}

QT_END_NAMESPACE

#endif
