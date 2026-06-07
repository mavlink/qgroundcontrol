// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QCBORVALUE_P_H
#define QCBORVALUE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.
// This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qcborvalue.h"

#if QT_CONFIG(cborstreamreader)
#  include "qcborstreamreader.h"
#endif

#include <private/qglobal_p.h>
#include <private/qstringconverter_p.h>

#include <math.h>

QT_BEGIN_NAMESPACE

namespace QtCbor {
enum class Comparison {
    ForEquality,
    ForOrdering,
};

struct Undefined {};
struct Element
{
    enum ValueFlag : quint32 {
        IsContainer                 = 0x0001,
        HasByteData                 = 0x0002,
        StringIsUtf16               = 0x0004,
        StringIsAscii               = 0x0008
    };
    Q_DECLARE_FLAGS(ValueFlags, ValueFlag)

    union {
        qint64 value;
        QCborContainerPrivate *container;
    };
    QCborValue::Type type;
    ValueFlags flags = {};

    Element(qint64 v = 0, QCborValue::Type t = QCborValue::Undefined, ValueFlags f = {})
        : value(v), type(t), flags(f)
    {}

    Element(QCborContainerPrivate *d, QCborValue::Type t, ValueFlags f = {})
        : container(d), type(t), flags(f | IsContainer)
    {}

    double fpvalue() const
    {
        double d;
        memcpy(&d, &value, sizeof(d));
        return d;
    }
};
Q_DECLARE_OPERATORS_FOR_FLAGS(Element::ValueFlags)
static_assert(sizeof(Element) == 16);

struct ByteData
{
    QByteArray::size_type len;

    const char *byte() const        { return reinterpret_cast<const char *>(this + 1); }
    char *byte()                    { return reinterpret_cast<char *>(this + 1); }
    const QChar *utf16() const      { return reinterpret_cast<const QChar *>(this + 1); }
    QChar *utf16()                  { return reinterpret_cast<QChar *>(this + 1); }

    QByteArray toByteArray() const  { return QByteArray(byte(), len); }
    QString toString() const        { return QString(utf16(), len / 2); }
    QString toUtf8String() const    { return QString::fromUtf8(byte(), len); }

    QByteArray asByteArrayView() const { return QByteArray::fromRawData(byte(), len); }
    QLatin1StringView asLatin1() const  { return {byte(), len}; }
    QUtf8StringView asUtf8StringView() const { return QUtf8StringView(byte(), len); }
    QStringView asStringView() const{ return QStringView(utf16(), len / 2); }
    QString asQStringRaw() const    { return QString::fromRawData(utf16(), len / 2); }
};
static_assert(std::is_trivially_default_constructible<ByteData>::value);
static_assert(std::is_trivially_copyable<ByteData>::value);
static_assert(std::is_standard_layout<ByteData>::value);
} // namespace QtCbor

Q_DECLARE_TYPEINFO(QtCbor::Element, Q_PRIMITIVE_TYPE);

class QCborContainerPrivate : public QSharedData
{
    friend class QExplicitlySharedDataPointer<QCborContainerPrivate>;
    ~QCborContainerPrivate();

public:
    QCborContainerPrivate() = default;
    QCborContainerPrivate(const QCborContainerPrivate &) = default;
    QCborContainerPrivate(QCborContainerPrivate &&) = default;
    QCborContainerPrivate &operator=(const QCborContainerPrivate &) = delete;
    QCborContainerPrivate &operator=(QCborContainerPrivate &&) = delete;

    enum ContainerDisposition { CopyContainer, MoveContainer };

    QByteArray::size_type usedData = 0;
    QByteArray data;
    QList<QtCbor::Element> elements;

    void deref() { if (!ref.deref()) delete this; }
    void compact();
    static QCborContainerPrivate *clone(QCborContainerPrivate *d, qsizetype reserved = -1);
    static QCborContainerPrivate *detach(QCborContainerPrivate *d, qsizetype reserved);
    static QCborContainerPrivate *grow(QCborContainerPrivate *d, qsizetype index);

    static qptrdiff addByteDataImpl(QByteArray &target, QByteArray::size_type &targetUsed,
                                    const char *block, qsizetype len)
    {
        // This function does not do overflow checking, since the len parameter
        // is expected to be trusted. There's another version of this function
        // in decodeStringFromCbor(), which checks.

        qptrdiff offset = target.size();

        // align offset
        offset += alignof(QtCbor::ByteData) - 1;
        offset &= ~(alignof(QtCbor::ByteData) - 1);

        qptrdiff increment = qptrdiff(sizeof(QtCbor::ByteData)) + len;

        targetUsed += increment;
        target.resize(offset + increment);

        char *ptr = target.begin() + offset;
        auto b = new (ptr) QtCbor::ByteData;
        b->len = len;
        if (block)
            memcpy(b->byte(), block, len);

        return offset;
    }

    qptrdiff addByteData(const char *block, qsizetype len)
    {
        return addByteDataImpl(data, usedData, block, len);
    }

    const QtCbor::ByteData *byteData(QtCbor::Element e) const
    {
        if ((e.flags & QtCbor::Element::HasByteData) == 0)
            return nullptr;

        size_t offset = size_t(e.value);
        Q_ASSERT((offset % alignof(QtCbor::ByteData)) == 0);
        Q_ASSERT(offset + sizeof(QtCbor::ByteData) <= size_t(data.size()));

        auto b = reinterpret_cast<const QtCbor::ByteData *>(data.constData() + offset);
        Q_ASSERT(offset + sizeof(*b) + size_t(b->len) <= size_t(data.size()));
        return b;
    }
    const QtCbor::ByteData *byteData(qsizetype idx) const
    {
        return byteData(elements.at(idx));
    }

    QCborContainerPrivate *containerAt(qsizetype idx, QCborValue::Type type) const
    {
        const QtCbor::Element &e = elements.at(idx);
        if (e.type != type || (e.flags & QtCbor::Element::IsContainer) == 0)
            return nullptr;
        return e.container;
    }

    void replaceAt_complex(QtCbor::Element &e, const QCborValue &value, ContainerDisposition disp);
    void replaceAt_internal(QtCbor::Element &e, const QCborValue &value, ContainerDisposition disp)
    {
        if (value.container)
            return replaceAt_complex(e, value, disp);

        e = { value.value_helper(), value.type() };
        if (value.isContainer())
            e.container = nullptr;
    }
    void replaceAt(qsizetype idx, const QCborValue &value, ContainerDisposition disp = CopyContainer)
    {
        QtCbor::Element &e = elements[idx];
        if (e.flags & QtCbor::Element::IsContainer) {
            e.container->deref();
            e.container = nullptr;
            e.flags = {};
        } else if (auto b = byteData(e)) {
            usedData -= b->len + sizeof(QtCbor::ByteData);
        }
        replaceAt_internal(e, value, disp);
    }
    void insertAt(qsizetype idx, const QCborValue &value, ContainerDisposition disp = CopyContainer)
    {
        replaceAt_internal(*elements.insert(idx, {}), value, disp);
    }

    void append(QtCbor::Undefined)
    {
        elements.append(QtCbor::Element());
    }
    void append(qint64 value)
    {
        elements.append(QtCbor::Element(value , QCborValue::Integer));
    }
    void append(QCborTag tag)
    {
        elements.append(QtCbor::Element(qint64(tag), QCborValue::Tag));
    }
    void appendByteData(const char *data, qsizetype len, QCborValue::Type type,
                        QtCbor::Element::ValueFlags extraFlags = {})
    {
        elements.append(QtCbor::Element(addByteData(data, len), type,
                                        QtCbor::Element::HasByteData | extraFlags));
    }
    void appendAsciiString(const QString &s);
    void appendAsciiString(const char *str, qsizetype len)
    {
        appendByteData(str, len, QCborValue::String, QtCbor::Element::StringIsAscii);
    }
    void appendUtf8String(const char *str, qsizetype len)
    {
        appendByteData(str, len, QCborValue::String);
    }
    void append(QLatin1StringView s)
    {
        if (!QtPrivate::isAscii(s))
            return appendNonAsciiString(QString(s));

        // US-ASCII is a subset of UTF-8, so we can keep in 8-bit
        appendByteData(s.latin1(), s.size(), QCborValue::String,
                       QtCbor::Element::StringIsAscii);
    }
    void appendAsciiString(QStringView s);
    void appendNonAsciiString(QStringView s);

    void append(const QString &s)
    {
        append(qToStringViewIgnoringNull(s));
    }

    void append(QStringView s)
    {
        if (QtPrivate::isAscii(s))
            appendAsciiString(s);
        else
            appendNonAsciiString(s);
    }
    void append(const QCborValue &v)
    {
        insertAt(elements.size(), v);
    }
    void append(QCborValue &&v)
    {
        insertAt(elements.size(), v, MoveContainer);
        v.container = nullptr;
        v.t = QCborValue::Undefined;
    }

    QByteArray byteArrayAt(qsizetype idx) const
    {
        const auto &e = elements.at(idx);
        const auto data = byteData(e);
        if (!data)
            return QByteArray();
        return data->toByteArray();
    }
    QString stringAt(qsizetype idx) const
    {
        const auto &e = elements.at(idx);
        const auto data = byteData(e);
        if (!data)
            return QString();
        if (e.flags & QtCbor::Element::StringIsUtf16)
            return data->toString();
        if (e.flags & QtCbor::Element::StringIsAscii)
            return data->asLatin1();
        return data->toUtf8String();
    }
    QAnyStringView anyStringViewAt(qsizetype idx) const
    {
        const auto &e = elements.at(idx);
        const auto data = byteData(e);
        if (!data)
            return nullptr;
        if (e.flags & QtCbor::Element::StringIsUtf16)
            return data->asStringView();
        if (e.flags & QtCbor::Element::StringIsAscii)
            return data->asLatin1();
        return data->asUtf8StringView();
    }

    static void resetValue(QCborValue &v)
    {
        v.container = nullptr;
    }

    static QCborValue makeValue(QCborValue::Type type, qint64 n, QCborContainerPrivate *d = nullptr,
                                ContainerDisposition disp = CopyContainer)
    {
        QCborValue result(type);
        result.n = n;
        result.container = d;
        if (d && disp == CopyContainer)
            d->ref.ref();
        return result;
    }

    QCborValue valueAt(qsizetype idx) const
    {
        const auto &e = elements.at(idx);

        if (e.flags & QtCbor::Element::IsContainer) {
            if (e.type == QCborValue::Tag && e.container->elements.size() != 2) {
                // invalid tags can be created due to incomplete parsing
                return makeValue(QCborValue::Invalid, 0, nullptr);
            }
            return makeValue(e.type, -1, e.container);
        } else if (e.flags & QtCbor::Element::HasByteData) {
            return makeValue(e.type, idx, const_cast<QCborContainerPrivate *>(this));
        }
        return makeValue(e.type, e.value);
    }
    QCborValue extractAt_complex(QtCbor::Element e);
    QCborValue extractAt(qsizetype idx)
    {
        QtCbor::Element e;
        qSwap(e, elements[idx]);

        if (e.flags & QtCbor::Element::IsContainer) {
            if (e.type == QCborValue::Tag && e.container->elements.size() != 2) {
                // invalid tags can be created due to incomplete parsing
                e.container->deref();
                return makeValue(QCborValue::Invalid, 0, nullptr);
            }
            return makeValue(e.type, -1, e.container, MoveContainer);
        } else if (e.flags & QtCbor::Element::HasByteData) {
            return extractAt_complex(e);
        }
        return makeValue(e.type, e.value);
    }

    static QtCbor::Element elementFromValue(const QCborValue &value)
    {
        if (value.n >= 0 && value.container)
            return value.container->elements.at(value.n);

        QtCbor::Element e;
        e.value = value.n;
        e.type = value.t;
        if (value.container) {
            e.container = value.container;
            e.flags = QtCbor::Element::IsContainer;
        }
        return e;
    }

    static int compareUtf8(const QtCbor::ByteData *b, QLatin1StringView s)
    {
        return QUtf8::compareUtf8(QByteArrayView(b->byte(), b->len), s);
    }

    static int compareUtf8(const QtCbor::ByteData *b, QStringView s)
    {
        return QUtf8::compareUtf8(QByteArrayView(b->byte(), b->len), s);
    }

    template<typename String>
    int stringCompareElement(const QtCbor::Element &e, String s, QtCbor::Comparison mode) const
    {
        if (e.type != QCborValue::String)
            return int(e.type) - int(QCborValue::String);

        const QtCbor::ByteData *b = byteData(e);
        if (!b)
            return s.isEmpty() ? 0 : -1;

        if (e.flags & QtCbor::Element::StringIsUtf16) {
            if (mode == QtCbor::Comparison::ForEquality)
                return b->asStringView() == s ? 0 : 1;
            return b->asStringView().compare(s);
        }
        return compareUtf8(b, s);
    }

    template<typename String>
    bool stringEqualsElement(const QtCbor::Element &e, String s) const
    {
        return stringCompareElement(e, s, QtCbor::Comparison::ForEquality) == 0;
    }

    template<typename String>
    bool stringEqualsElement(qsizetype idx, String s) const
    {
        return stringEqualsElement(elements.at(idx), s);
    }

    static int compareElement_helper(const QCborContainerPrivate *c1, QtCbor::Element e1,
                                     const QCborContainerPrivate *c2, QtCbor::Element e2,
                                     QtCbor::Comparison mode) noexcept;
    int compareElement(qsizetype idx, const QCborValue &value, QtCbor::Comparison mode) const
    {
        auto &e1 = elements.at(idx);
        auto e2 = elementFromValue(value);
        return compareElement_helper(this, e1, value.container, e2, mode);
    }

    void removeAt(qsizetype idx)
    {
        replaceAt(idx, {});
        elements.remove(idx);
    }

    // doesn't apply to JSON
    template <typename KeyType> QCborValueConstRef findCborMapKey(KeyType key)
    {
        qsizetype i = 0;
        for ( ; i < elements.size(); i += 2) {
            const auto &e = elements.at(i);
            bool equals;
            if constexpr (std::is_same_v<std::decay_t<KeyType>, QCborValue>) {
                equals = (compareElement(i, key, QtCbor::Comparison::ForEquality) == 0);
            } else if constexpr (std::is_integral_v<KeyType>) {
                equals = (e.type == QCborValue::Integer && e.value == key);
            } else {
                // assume it's a string
                equals = stringEqualsElement(i, key);
            }
            if (equals)
                break;
        }
        return { this, i + 1 };
    }
    template <typename KeyType> static QCborValue findCborMapKey(const QCborValue &self, KeyType key)
    {
        if (self.isMap() && self.container) {
            qsizetype idx = self.container->findCborMapKey(key).i;
            if (idx < self.container->elements.size())
                return self.container->valueAt(idx);
        }
        return QCborValue();
    }
    template <typename KeyType> static QCborValueRef
    findOrAddMapKey(QCborContainerPrivate *container, KeyType key)
    {
        qsizetype size = 0;
        qsizetype index = size + 1;
        if (container) {
            size = container->elements.size();
            index = container->findCborMapKey<KeyType>(key).i; // returns size + 1 if not found
        }
        Q_ASSERT(index & 1);
        Q_ASSERT((size & 1) == 0);

        container = detach(container, qMax(index + 1, size));
        Q_ASSERT(container);
        Q_ASSERT((container->elements.size() & 1) == 0);

        if (index >= size) {
            container->append(key);
            container->append(QCborValue());
        }
        Q_ASSERT(index < container->elements.size());
        return { container, index };
    }
    template <typename KeyType> static QCborValueRef findOrAddMapKey(QCborMap &map, KeyType key);
    template <typename KeyType> static QCborValueRef findOrAddMapKey(QCborValue &self, KeyType key);
    template <typename KeyType> static QCborValueRef findOrAddMapKey(QCborValueRef self, KeyType key);

#if QT_CONFIG(cborstreamreader)
    void decodeValueFromCbor(QCborStreamReader &reader, int remainingStackDepth);
    void decodeStringFromCbor(QCborStreamReader &reader);
    static inline void setErrorInReader(QCborStreamReader &reader, QCborError error);
#endif
};

QT_END_NAMESPACE

#endif // QCBORVALUE_P_H
