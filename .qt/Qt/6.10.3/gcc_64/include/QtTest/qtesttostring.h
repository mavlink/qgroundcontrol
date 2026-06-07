// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTTOSTRING_H
#define QTESTTOSTRING_H

#include <QtTest/qttestglobal.h>

#include <QtCore/qttypetraits.h>

#if QT_CONFIG(itemmodel)
#  include <QtCore/qabstractitemmodel.h>
#endif
#include <QtCore/qbitarray.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qcborarray.h>
#include <QtCore/qcborcommon.h>
#include <QtCore/qcbormap.h>
#include <QtCore/qcborvalue.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qurl.h>
#include <QtCore/quuid.h>
#include <QtCore/qvariant.h>

#include <cstdio>
#include <QtCore/q20memory.h>

QT_BEGIN_NAMESPACE

namespace QTest {
namespace Internal {

template<typename T> // Output registered enums
inline typename std::enable_if<QtPrivate::IsQEnumHelper<T>::Value, char*>::type toString(T e)
{
    QMetaEnum me = QMetaEnum::fromType<T>();
    return qstrdup(me.valueToKey(int(e))); // int cast is necessary to support enum classes
}

template <typename T>
inline typename std::enable_if<!QtPrivate::IsQEnumHelper<T>::Value && std::is_enum_v<T>, char*>::type toString(const T &e)
{
    return qstrdup(QByteArray::number(static_cast<std::underlying_type_t<T>>(e)).constData());
}

template <typename T> // Fallback; for built-in types debug streaming must be possible
inline typename std::enable_if<!QtPrivate::IsQEnumHelper<T>::Value && !std::is_enum_v<T>, char *>::type toString(const T &t)
{
    char *result = nullptr;
#ifndef QT_NO_DEBUG_STREAM
    if constexpr (QTypeTraits::has_ostream_operator_v<QDebug, T>) {
        result = qstrdup(QDebug::toBytes(t).constData());
    } else {
        static_assert(!QMetaTypeId2<T>::IsBuiltIn,
                "Built-in type must implement debug streaming operator "
                "or provide QTest::toString specialization");
    }
#endif
    return result;
}

template<typename F> // Output QFlags of registered enumerations
inline typename std::enable_if<QtPrivate::IsQEnumHelper<F>::Value, char*>::type toString(QFlags<F> f)
{
    const QMetaEnum me = QMetaEnum::fromType<F>();
    return qstrdup(me.valueToKeys(int(f.toInt())).constData());
}

template <typename F> // Fallback: Output hex value
inline typename std::enable_if<!QtPrivate::IsQEnumHelper<F>::Value, char*>::type toString(QFlags<F> f)
{
    const size_t space = 3 + 2 * sizeof(unsigned); // 2 for 0x, two hex digits per byte, 1 for '\0'
    char *msg = new char[space];
    std::snprintf(msg, space, "0x%x", unsigned(f.toInt()));
    return msg;
}

} // namespace Internal

Q_TESTLIB_EXPORT bool compare_string_helper(const char *t1, const char *t2, const char *actual,
                                            const char *expected, const char *file, int line);
Q_TESTLIB_EXPORT char *formatString(const char *prefix, const char *suffix, size_t numArguments, ...);
Q_TESTLIB_EXPORT char *toHexRepresentation(const char *ba, qsizetype length);
Q_TESTLIB_EXPORT char *toPrettyCString(const char *unicode, qsizetype length);
Q_TESTLIB_EXPORT char *toPrettyUnicode(QStringView string);

template <typename T>
inline char *toString(const T &t)
{
    return Internal::toString(t);
}

template <typename T1, typename T2>
inline char *toString(const std::pair<T1, T2> &pair);

template <class... Types>
inline char *toString(const std::tuple<Types...> &tuple);

template <typename Rep, typename Period>
inline char *toString(std::chrono::duration<Rep, Period> duration);

Q_TESTLIB_EXPORT char *toString(const char *);
Q_TESTLIB_EXPORT char *toString(const volatile void *);
Q_TESTLIB_EXPORT char *toString(const QObject *);
Q_TESTLIB_EXPORT char *toString(const volatile QObject *);

#define QTEST_COMPARE_DECL(KLASS)\
    template<> Q_TESTLIB_EXPORT char *toString<KLASS >(const KLASS &);
#ifndef Q_QDOC
QTEST_COMPARE_DECL(short)
QTEST_COMPARE_DECL(ushort)
QTEST_COMPARE_DECL(int)
QTEST_COMPARE_DECL(uint)
QTEST_COMPARE_DECL(long)
QTEST_COMPARE_DECL(ulong)
QTEST_COMPARE_DECL(qint64)
QTEST_COMPARE_DECL(quint64)

QTEST_COMPARE_DECL(float)
QTEST_COMPARE_DECL(double)
QTEST_COMPARE_DECL(qfloat16)
QTEST_COMPARE_DECL(char)
QTEST_COMPARE_DECL(signed char)
QTEST_COMPARE_DECL(unsigned char)
QTEST_COMPARE_DECL(bool)
#endif
#undef QTEST_COMPARE_DECL

template <> inline char *toString(const QStringView &str)
{
    return QTest::toPrettyUnicode(str);
}

template<> inline char *toString(const QString &str)
{
    return toString(QStringView(str));
}

template<> inline char *toString(const QLatin1StringView &str)
{
    return toString(QString(str));
}

template<> inline char *toString(const QByteArray &ba)
{
    return QTest::toPrettyCString(ba.constData(), ba.size());
}

template<> inline char *toString(const QBitArray &ba)
{
    qsizetype size = ba.size();
    char *str = new char[size + 1];
    for (qsizetype i = 0; i < size; ++i)
        str[i] = "01"[ba.testBit(i)];
    str[size] = '\0';
    return str;
}

#if QT_CONFIG(datestring)
template<> inline char *toString(const QTime &time)
{
    return time.isValid()
            ? qstrdup(qPrintable(time.toString(u"hh:mm:ss.zzz")))
            : qstrdup("Invalid QTime");
}

template<> inline char *toString(const QDate &date)
{
    return date.isValid()
            ? qstrdup(qPrintable(date.toString(u"yyyy/MM/dd")))
            : qstrdup("Invalid QDate");
}

template<> inline char *toString(const QDateTime &dateTime)
{
    return dateTime.isValid()
            ? qstrdup(qPrintable(dateTime.toString(u"yyyy/MM/dd hh:mm:ss.zzz[t]")))
            : qstrdup("Invalid QDateTime");
}
#endif // datestring

template<> inline char *toString(const QCborError &c)
{
    // use the Q_ENUM formatting
    return toString(c.c);
}

template<> inline char *toString(const QChar &c)
{
    const ushort uc = c.unicode();
    if (uc < 128) {
        char msg[32];
        std::snprintf(msg, sizeof(msg), "QChar: '%c' (0x%x)", char(uc), unsigned(uc));
        return qstrdup(msg);
    }
    return qstrdup(qPrintable(QString::fromLatin1("QChar: '%1' (0x%2)").arg(c).arg(QString::number(static_cast<int>(c.unicode()), 16))));
}

#if QT_CONFIG(itemmodel)
template<> inline char *toString(const QModelIndex &idx)
{
    char msg[128];
    std::snprintf(msg, sizeof(msg), "QModelIndex(%d,%d,%p,%p)",
                  idx.row(), idx.column(), idx.internalPointer(),
                  static_cast<const void*>(idx.model()));
    return qstrdup(msg);
}
#endif

template<> inline char *toString(const QPoint &p)
{
    char msg[128];
    std::snprintf(msg, sizeof(msg), "QPoint(%d,%d)", p.x(), p.y());
    return qstrdup(msg);
}

template<> inline char *toString(const QSize &s)
{
    char msg[128];
    std::snprintf(msg, sizeof(msg), "QSize(%dx%d)", s.width(), s.height());
    return qstrdup(msg);
}

template<> inline char *toString(const QRect &s)
{
    char msg[256];
    std::snprintf(msg, sizeof(msg), "QRect(%d,%d %dx%d) (bottomright %d,%d)",
              s.left(), s.top(), s.width(), s.height(), s.right(), s.bottom());
    return qstrdup(msg);
}

template<> inline char *toString(const QPointF &p)
{
    char msg[64];
    std::snprintf(msg, sizeof(msg), "QPointF(%g,%g)", p.x(), p.y());
    return qstrdup(msg);
}

template<> inline char *toString(const QSizeF &s)
{
    char msg[64];
    std::snprintf(msg, sizeof(msg), "QSizeF(%gx%g)", s.width(), s.height());
    return qstrdup(msg);
}

template<> inline char *toString(const QRectF &s)
{
    char msg[256];
    std::snprintf(msg, sizeof(msg), "QRectF(%g,%g %gx%g) (bottomright %g,%g)",
                  s.left(), s.top(), s.width(), s.height(), s.right(), s.bottom());
    return qstrdup(msg);
}

template<> inline char *toString(const QUrl &uri)
{
    if (!uri.isValid())
        return qstrdup(qPrintable(QLatin1StringView("Invalid URL: ") + uri.errorString()));
    return qstrdup(uri.toEncoded().constData());
}

template <> inline char *toString(const QUuid &uuid)
{
    return qstrdup(uuid.toByteArray().constData());
}

template<> inline char *toString(const QVariant &v)
{
    QByteArray vstring("QVariant(");
    if (v.isValid()) {
        QByteArray type(v.typeName());
        if (type.isEmpty()) {
            type = QByteArray::number(v.userType());
        }
        vstring.append(type);
        if (!v.isNull()) {
            vstring.append(',');
            if (v.canConvert<QString>()) {
                vstring.append(v.toString().toLocal8Bit());
            }
            else {
                vstring.append("<value not representable as string>");
            }
        }
    }
    vstring.append(')');

    return qstrdup(vstring.constData());
}

template<> inline char *toString(const QPartialOrdering &o)
{
    if (o == QPartialOrdering::Less)
        return qstrdup("Less");
    if (o == QPartialOrdering::Equivalent)
        return qstrdup("Equivalent");
    if (o == QPartialOrdering::Greater)
        return qstrdup("Greater");
    if (o == QPartialOrdering::Unordered)
        return qstrdup("Unordered");
    return qstrdup("<invalid>");
}

namespace Internal {
struct QCborValueFormatter
{
private:
    using UP = std::unique_ptr<char[]>;
    enum { BufferLen = 256 };

    static UP createBuffer() { return q20::make_unique_for_overwrite<char[]>(BufferLen); }

    static UP formatSimpleType(QCborSimpleType st)
    {
        auto buf = createBuffer();
        std::snprintf(buf.get(), BufferLen, "QCborValue(QCborSimpleType(%d))", int(st));
        return buf;
    }

    static UP formatTag(QCborTag tag, const QCborValue &taggedValue)
    {
        auto buf = createBuffer();
        const std::unique_ptr<char[]> hold(format(taggedValue));
        std::snprintf(buf.get(), BufferLen, "QCborValue(QCborTag(%llu), %s)",
                      qToUnderlying(tag), hold.get());
        return buf;
    }

    static UP innerFormat(QCborValue::Type t, const UP &str)
    {
        static const QMetaEnum typeEnum = []() {
            int idx = QCborValue::staticMetaObject.indexOfEnumerator("Type");
            return QCborValue::staticMetaObject.enumerator(idx);
        }();

        auto buf = createBuffer();
        const char *typeName = typeEnum.valueToKey(t);
        if (typeName)
            std::snprintf(buf.get(), BufferLen, "QCborValue(%s, %s)", typeName, str ? str.get() : "");
        else
            std::snprintf(buf.get(), BufferLen, "QCborValue(<unknown type 0x%02x>)", t);
        return buf;
    }

    template<typename T> static UP format(QCborValue::Type type, const T &t)
    {
        const std::unique_ptr<char[]> hold(QTest::toString(t));
        return innerFormat(type, hold);
    }

public:

    static UP format(const QCborValue &v)
    {
        switch (v.type()) {
        case QCborValue::Integer:
            return format(v.type(), v.toInteger());
        case QCborValue::ByteArray:
            return format(v.type(), v.toByteArray());
        case QCborValue::String:
            return format(v.type(), v.toString());
        case QCborValue::Array:
            return innerFormat(v.type(), format(v.toArray()));
        case QCborValue::Map:
            return innerFormat(v.type(), format(v.toMap()));
        case QCborValue::Tag:
            return formatTag(v.tag(), v.taggedValue());
        case QCborValue::SimpleType:
            break;
        case QCborValue::True:
            return UP{qstrdup("QCborValue(true)")};
        case QCborValue::False:
            return UP{qstrdup("QCborValue(false)")};
        case QCborValue::Null:
            return UP{qstrdup("QCborValue(nullptr)")};
        case QCborValue::Undefined:
            return UP{qstrdup("QCborValue()")};
        case QCborValue::Double:
            return format(v.type(), v.toDouble());
        case QCborValue::DateTime:
        case QCborValue::Url:
        case QCborValue::RegularExpression:
            return format(v.type(), v.taggedValue().toString());
        case QCborValue::Uuid:
            return format(v.type(), v.toUuid());
        case QCborValue::Invalid:
            return UP{qstrdup("QCborValue(<invalid>)")};
        }

        if (v.isSimpleType())
            return formatSimpleType(v.toSimpleType());
        return innerFormat(v.type(), nullptr);
    }

    static UP format(const QCborArray &a)
    {
        QByteArray out(1, '[');
        const char *comma = "";
        for (QCborValueConstRef v : a) {
            const std::unique_ptr<char[]> s(format(v));
            out += comma;
            out += s.get();
            comma = ", ";
        }
        out += ']';
        return UP{qstrdup(out.constData())};
    }

    static UP format(const QCborMap &m)
    {
        QByteArray out(1, '{');
        const char *comma = "";
        for (auto pair : m) {
            const std::unique_ptr<char[]> key(format(pair.first));
            const std::unique_ptr<char[]> value(format(pair.second));
            out += comma;
            out += key.get();
            out += ": ";
            out += value.get();
            comma = ", ";
        }
        out += '}';
        return UP{qstrdup(out.constData())};
    }
};
}

template<> inline char *toString(const QCborValue &v)
{
    return Internal::QCborValueFormatter::format(v).release();
}

template<> inline char *toString(const QCborValueRef &v)
{
    return toString(QCborValue(v));
}

template<> inline char *toString(const QCborArray &a)
{
    return Internal::QCborValueFormatter::format(a).release();
}

template<> inline char *toString(const QCborMap &m)
{
    return Internal::QCborValueFormatter::format(m).release();
}

template <typename Rep, typename Period> char *toString(std::chrono::duration<Rep, Period> dur)
{
    QString r;
    QDebug d(&r);
    d.nospace() << qSetRealNumberPrecision(9) << dur;
    if constexpr (Period::num != 1 || Period::den != 1) {
        // include the equivalent value in seconds, in parentheses
        using namespace std::chrono;
        d << " (" << duration_cast<duration<qreal>>(dur).count() << "s)";
    }
    return qstrdup(std::move(r).toUtf8().constData());
}

template <typename T1, typename T2>
inline char *toString(const std::pair<T1, T2> &pair)
{
    const std::unique_ptr<char[]> first(toString(pair.first));
    const std::unique_ptr<char[]> second(toString(pair.second));
    return formatString("std::pair(", ")", 2, first.get(), second.get());
}

template <typename Tuple, std::size_t... I>
inline char *tupleToString(const Tuple &tuple, std::index_sequence<I...>) {
    using UP = std::unique_ptr<char[]>;
    // Generate a table of N + 1 elements where N is the number of
    // elements in the tuple.
    // The last element is needed to support the empty tuple use case.
    const UP data[] = {
        UP(toString(std::get<I>(tuple)))..., UP{}
    };
    return formatString("std::tuple(", ")", sizeof...(I), data[I].get()...);
}

template <class... Types>
inline char *toString(const std::tuple<Types...> &tuple)
{
    return tupleToString(tuple, std::make_index_sequence<sizeof...(Types)>{});
}

inline char *toString(std::nullptr_t)
{
    return toString(QStringView(u"nullptr"));
}
} // namespace QTest

QT_END_NAMESPACE

#endif // QTESTTOSTRING_H
