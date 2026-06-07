// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4STRING_H
#define QV4STRING_H

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

#include <QtCore/qstring.h>
#include "qv4managed_p.h"
#include <QtCore/private/qnumeric_p.h>
#include "qv4enginebase_p.h"
#include <private/qv4stringtoarrayindex_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ExecutionEngine;
struct PropertyKey;

namespace Heap {

struct Q_QML_EXPORT StringOrSymbol : Base
{
    enum StringType {
        StringType_Symbol,
        StringType_Regular,
        StringType_ArrayIndex,
        StringType_Unknown,
        StringType_AddedString,
        StringType_SubString,
        StringType_Complex = StringType_AddedString
    };

    void init() {
        Base::init();
        new (&textStorage) QStringPrivate;
    }

    void init(QStringPrivate text)
    {
        Base::init();
        new (&textStorage) QStringPrivate(std::move(text));
    }

    mutable struct { alignas(QStringPrivate) unsigned char data[sizeof(QStringPrivate)]; } textStorage;
    mutable PropertyKey identifier;
    mutable uint subtype;
    mutable uint stringHash;

    static void markObjects(Heap::Base *that, MarkStack *markStack);
    void destroy();

    QStringPrivate &text() const { return *reinterpret_cast<QStringPrivate *>(&textStorage); }

    inline QString toQString() const {
        QStringPrivate dd = text();
        return QString(std::move(dd));
    }
    void createHashValue() const;
    inline unsigned hashValue() const {
        if (subtype >= StringType_Unknown)
            createHashValue();
        Q_ASSERT(subtype < StringType_Complex);

        return stringHash;
    }
};

struct Q_QML_EXPORT String : StringOrSymbol {
    static void markObjects(Heap::Base *that, MarkStack *markStack);

    const VTable *vtable() const {
        return internalClass->vtable;
    }

    void init(const QString &text);
    void simplifyString() const;
    int length() const;
    std::size_t retainedTextSize() const {
        return subtype >= StringType_Complex ? 0 : (std::size_t(text().size) * sizeof(QChar));
    }
    inline QString toQString() const {
        if (subtype >= StringType_Complex)
            simplifyString();
        return StringOrSymbol::toQString();
    }
    inline bool isEqualTo(const String *other) const {
        if (this == other)
            return true;
        if (hashValue() != other->hashValue())
            return false;
        Q_ASSERT(subtype < StringType_Complex);
        if (identifier.isValid() && identifier == other->identifier)
            return true;
        if (subtype == Heap::String::StringType_ArrayIndex && other->subtype == Heap::String::StringType_ArrayIndex)
            return true;

        return toQString() == other->toQString();
    }

    bool startsWithUpper() const;

private:
    static void append(const String *data, QChar *ch);
};
static_assert(std::is_trivially_copyable_v<String>);
static_assert(std::is_trivially_default_constructible_v<String>);

struct ComplexString : String {
    void init(String *l, String *n);
    void init(String *ref, int from, int len);
    mutable String *left;
    mutable String *right;
    union {
        mutable int largestSubLength;
        int from;
    };
    int len;
};
static_assert(std::is_trivially_copyable_v<ComplexString>);
static_assert(std::is_trivially_default_constructible_v<ComplexString>);

inline
int String::length() const {
    // TODO: ensure that our strings never actually grow larger than INT_MAX
    return subtype < StringType_AddedString ? int(text().size) : static_cast<const ComplexString *>(this)->len;
}

}

struct Q_QML_EXPORT StringOrSymbol : public Managed {
    V4_MANAGED(StringOrSymbol, Managed)
    V4_NEEDS_DESTROY
    enum {
        IsStringOrSymbol = true
    };

private:
    inline void createPropertyKey() const;
public:
    PropertyKey propertyKey() const { Q_ASSERT(d()->identifier.isValid()); return d()->identifier; }
    PropertyKey toPropertyKey() const;


    inline QString toQString() const {
        return d()->toQString();
    }
};

struct Q_QML_EXPORT String : public StringOrSymbol {
    V4_MANAGED(String, StringOrSymbol)
    Q_MANAGED_TYPE(String)
    V4_INTERNALCLASS(String)
    enum {
        IsString = true
    };

    uchar subtype() const { return d()->subtype; }
    void setSubtype(uchar subtype) const { d()->subtype = subtype; }

    bool equals(String *other) const {
        return d()->isEqualTo(other->d());
    }
    inline bool isEqualTo(const String *other) const {
        return d()->isEqualTo(other->d());
    }

    inline bool lessThan(const String *other) {
        return toQString() < other->toQString();
    }

    inline QString toQString() const {
        return d()->toQString();
    }

    inline unsigned hashValue() const {
        return d()->hashValue();
    }
    uint toUInt(bool *ok) const;

    // slow path
    Q_NEVER_INLINE void createPropertyKeyImpl() const;

    static uint createHashValue(const QChar *ch, int length, uint *subtype)
    {
        const QChar *end = ch + length;
        return calculateHashValue(ch, end, subtype);
    }

    static uint createHashValueDisallowingArrayIndex(const QChar *ch, int length, uint *subtype)
    {
        const QChar *end = ch + length;
        return calculateHashValue<String::DisallowArrayIndex>(ch, end, subtype);
    }

    static uint createHashValue(const char *ch, int length, uint *subtype)
    {
        const char *end = ch + length;
        return calculateHashValue(ch, end, subtype);
    }

    bool startsWithUpper() const { return d()->startsWithUpper(); }

protected:
    static bool virtualIsEqualTo(Managed *that, Managed *o);
    static qint64 virtualGetLength(const Managed *m);

public:
    enum IndicesBehavior {Default, DisallowArrayIndex};
    template <IndicesBehavior Behavior = Default, typename T>
    static inline uint calculateHashValue(const T *ch, const T* end, uint *subtype)
    {
        // array indices get their number as hash value
        uint h = UINT_MAX;
        if constexpr (Behavior != DisallowArrayIndex) {
            h = stringToArrayIndex(ch, end);
            if (h != UINT_MAX) {
                if (subtype)
                    *subtype = Heap::StringOrSymbol::StringType_ArrayIndex;
                return h;
            }
        }

        while (ch < end) {
            h = 31 * h + charToUInt(ch);
            ++ch;
        }

        if (subtype)
            *subtype = (ch != end && charToUInt(ch) == '@') ? Heap::StringOrSymbol::StringType_Symbol : Heap::StringOrSymbol::StringType_Regular;
        return h;
    }
};

struct ComplexString : String {
    typedef QV4::Heap::ComplexString Data;
    QV4::Heap::ComplexString *d_unchecked() const { return static_cast<QV4::Heap::ComplexString *>(m()); }
    QV4::Heap::ComplexString *d() const {
        QV4::Heap::ComplexString *dptr = d_unchecked();
        dptr->_checkIsInitialized();
        return dptr;
    }
};

inline
void StringOrSymbol::createPropertyKey() const {
    Q_ASSERT(!d()->identifier.isValid());
    Q_ASSERT(isString());
    static_cast<const String *>(this)->createPropertyKeyImpl();
}

inline PropertyKey StringOrSymbol::toPropertyKey() const {
    if (!d()->identifier.isValid())
        createPropertyKey();
    return d()->identifier;
}

template<>
inline const StringOrSymbol *Value::as() const {
    return isManaged() && m()->internalClass->vtable->isStringOrSymbol ? static_cast<const String *>(this) : nullptr;
}

template<>
inline const String *Value::as() const {
    return isManaged() && m()->internalClass->vtable->isString ? static_cast<const String *>(this) : nullptr;
}

template<>
inline ReturnedValue value_convert<String>(ExecutionEngine *e, const Value &v)
{
    return v.toString(e)->asReturnedValue();
}

}

QT_END_NAMESPACE

#endif
