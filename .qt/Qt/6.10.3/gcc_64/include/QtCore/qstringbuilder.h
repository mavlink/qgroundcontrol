// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <QtCore/qstring.h>

#ifndef QSTRINGBUILDER_H
#define QSTRINGBUILDER_H

#if 0
// syncqt can not handle the templates in this file, and it doesn't need to
// process them anyway because they are internal.
#pragma qt_class(QStringBuilder)
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qbytearray.h>

#include <string.h>

QT_BEGIN_NAMESPACE


struct Q_CORE_EXPORT QAbstractConcatenable
{
protected:
    static void convertFromUtf8(QByteArrayView in, QChar *&out) noexcept;
    static inline void convertFromAscii(char a, QChar *&out) noexcept
    {
        *out++ = QLatin1Char(a);
    }
    static void appendLatin1To(QLatin1StringView in, QChar *out) noexcept;
};

template <typename T> struct QConcatenable;

template <typename T>
using QConcatenableEx = QConcatenable<q20::remove_cvref_t<T>>;

namespace QtStringBuilder {
    template <typename A, typename B> struct ConvertToTypeHelper
    { typedef A ConvertTo; };
    template <typename T> struct ConvertToTypeHelper<T, QString>
    { typedef QString ConvertTo; };

    template <typename T> using HasIsNull = decltype(std::declval<const T &>().isNull());
    template <typename T> bool isNull(const T &t)
    {
        if constexpr (qxp::is_detected_v<HasIsNull, T>)
            return t.isNull();
        else
            return false;
    }
}

template<typename Builder, typename T>
struct QStringBuilderCommon
{
    T toUpper() const { return resolved().toUpper(); }
    T toLower() const { return resolved().toLower(); }

protected:
    T resolved() const { return *static_cast<const Builder*>(this); }
};

template<typename Builder, typename T>
struct QStringBuilderBase : public QStringBuilderCommon<Builder, T>
{
};

template<typename Builder>
struct QStringBuilderBase<Builder, QString> : public QStringBuilderCommon<Builder, QString>
{
    QByteArray toLatin1() const { return this->resolved().toLatin1(); }
    QByteArray toUtf8() const { return this->resolved().toUtf8(); }
    QByteArray toLocal8Bit() const { return this->resolved().toLocal8Bit(); }
};

template <typename A, typename B>
class QStringBuilder : public QStringBuilderBase<QStringBuilder<A, B>,
                                                 typename QtStringBuilder::ConvertToTypeHelper<
                                                         typename QConcatenableEx<A>::ConvertTo,
                                                         typename QConcatenableEx<B>::ConvertTo
                                                         >::ConvertTo
                                                >
{
public:
    QStringBuilder(A &&a_, B &&b_) : a(std::forward<A>(a_)), b(std::forward<B>(b_)) {}

    QStringBuilder(QStringBuilder &&) = default;
    QStringBuilder(const QStringBuilder &) = default;
    ~QStringBuilder() = default;

private:
    friend class QByteArray;
    friend class QString;
    template <typename T> T convertTo() const
    {
        if (isNull()) {
            // appending two null strings must give back a null string,
            // so we're special casing this one out, QTBUG-114206
            return T();
        }

        const qsizetype len = Concatenable::size(*this);
        T s(len, Qt::Uninitialized);

        // Using data_ptr() here (private API) so we can bypass the
        // isDetached() and the replacement of a null pointer with _empty in
        // both QString and QByteArray's data() and constData(). The result is
        // the same if len != 0.
        auto d = reinterpret_cast<typename T::iterator>(s.data_ptr().data());
        const auto start = d;
        Concatenable::appendTo(*this, d);

        if constexpr (Concatenable::ExactSize) {
            Q_UNUSED(start)
        } else {
            if (len != d - start) {
                // this resize is necessary since we allocate a bit too much
                // when dealing with variable sized 8-bit encodings
                s.resize(d - start);
            }
        }
        return s;
    }

    typedef QConcatenable<QStringBuilder<A, B> > Concatenable;
public:
    typedef typename Concatenable::ConvertTo ConvertTo;
    operator ConvertTo() const { return convertTo<ConvertTo>(); }

    qsizetype size() const { return Concatenable::size(*this); }

    bool isNull() const
    {
        return QtStringBuilder::isNull(a) && QtStringBuilder::isNull(b);
    }

    A a;
    B b;

private:
    QStringBuilder &operator=(QStringBuilder &&) = delete;
    QStringBuilder &operator=(const QStringBuilder &) = delete;
};

template <> struct QConcatenable<char> : private QAbstractConcatenable
{
    typedef char type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = true };
    static qsizetype size(const char) { return 1; }
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN static inline void appendTo(const char c, QChar *&out)
    {
        QAbstractConcatenable::convertFromAscii(c, out);
    }
#endif
    static inline void appendTo(const char c, char *&out)
    { *out++ = c; }
};

template <> struct QConcatenable<QByteArrayView> : private QAbstractConcatenable
{
    typedef QByteArrayView type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = true };
    static qsizetype size(QByteArrayView bav) { return bav.size(); }
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN static inline void appendTo(QByteArrayView bav, QChar *&out)
    {
        QAbstractConcatenable::convertFromUtf8(bav, out);
    }
#endif
    static inline void appendTo(QByteArrayView bav, char *&out)
    {
        qsizetype n = bav.size();
        if (n)
            memcpy(out, bav.data(), n);
        out += n;
    }
};

template <> struct QConcatenable<char16_t> : private QAbstractConcatenable
{
    typedef char16_t type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static constexpr qsizetype size(char16_t) { return 1; }
    static inline void appendTo(char16_t c, QChar *&out)
    { *out++ = c; }
};

template <> struct QConcatenable<QLatin1Char>
{
    typedef QLatin1Char type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static qsizetype size(const QLatin1Char) { return 1; }
    static inline void appendTo(const QLatin1Char c, QChar *&out)
    { *out++ = c; }
    static inline void appendTo(const QLatin1Char c, char *&out)
    { *out++ = c.toLatin1(); }
};

template <> struct QConcatenable<QChar> : private QAbstractConcatenable
{
    typedef QChar type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static qsizetype size(const QChar) { return 1; }
    static inline void appendTo(const QChar c, QChar *&out)
    { *out++ = c; }
};

template <> struct QConcatenable<QChar::SpecialCharacter> : private QAbstractConcatenable
{
    typedef QChar::SpecialCharacter type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static qsizetype size(const QChar::SpecialCharacter) { return 1; }
    static inline void appendTo(const QChar::SpecialCharacter c, QChar *&out)
    { *out++ = c; }
};

template <> struct QConcatenable<QLatin1StringView> : private QAbstractConcatenable
{
    typedef QLatin1StringView type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static qsizetype size(const QLatin1StringView a) { return a.size(); }
    static inline void appendTo(const QLatin1StringView a, QChar *&out)
    {
        appendLatin1To(a, out);
        out += a.size();
    }
    static inline void appendTo(const QLatin1StringView a, char *&out)
    {
        if (const char *data = a.data()) {
            memcpy(out, data, a.size());
            out += a.size();
        }
    }
};

template <> struct QConcatenable<QString> : private QAbstractConcatenable
{
    typedef QString type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static qsizetype size(const QString &a) { return a.size(); }
    static inline void appendTo(const QString &a, QChar *&out)
    {
        const qsizetype n = a.size();
        if (n)
            memcpy(out, a.data(), sizeof(QChar) * n);
        out += n;
    }
};

template <> struct QConcatenable<QStringView> : private QAbstractConcatenable
{
    typedef QStringView type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static qsizetype size(QStringView a) { return a.size(); }
    static inline void appendTo(QStringView a, QChar *&out)
    {
        const auto n = a.size();
        if (n)
            memcpy(out, a.data(), sizeof(QChar) * n);
        out += n;
    }
};

template <qsizetype N> struct QConcatenable<const char[N]> : private QAbstractConcatenable
{
    typedef const char type[N];
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static qsizetype size(const char[N]) { return N - 1; }
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN static inline void appendTo(const char a[N], QChar *&out)
    {
        QAbstractConcatenable::convertFromUtf8(QByteArrayView(a, N - 1), out);
    }
#endif
    static inline void appendTo(const char a[N], char *&out)
    {
        while (*a)
            *out++ = *a++;
    }
};

template <qsizetype N> struct QConcatenable<char[N]> : QConcatenable<const char[N]>
{
    typedef char type[N];
};

template <> struct QConcatenable<const char *> : private QAbstractConcatenable
{
    typedef const char *type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static qsizetype size(const char *a) { return qstrlen(a); }
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN static inline void appendTo(const char *a, QChar *&out)
    { QAbstractConcatenable::convertFromUtf8(QByteArrayView(a), out); }
#endif
    static inline void appendTo(const char *a, char *&out)
    {
        if (!a)
            return;
        while (*a)
            *out++ = *a++;
    }
};

template <> struct QConcatenable<char *> : QConcatenable<const char*>
{
    typedef char *type;
};

template <qsizetype N> struct QConcatenable<const char16_t[N]> : private QAbstractConcatenable
{
    using type = const char16_t[N];
    using ConvertTo = QString;
    enum { ExactSize = true };
    static qsizetype size(const char16_t[N]) { return N - 1; }
    static void appendTo(const char16_t a[N], QChar *&out)
    {
        memcpy(static_cast<void *>(out), a, (N - 1) * sizeof(char16_t));
        out += N - 1;
    }
};

template <qsizetype N> struct QConcatenable<char16_t[N]> : QConcatenable<const char16_t[N]>
{
    using type = char16_t[N];
};

template <> struct QConcatenable<const char16_t *> : private QAbstractConcatenable
{
    using type = const char16_t *;
    using ConvertTo = QString;
    enum { ExactSize = true };
    static qsizetype size(const char16_t *a) { return QStringView(a).size(); }
    QT_ASCII_CAST_WARN static inline void appendTo(const char16_t *a, QChar *&out)
    {
        if (!a)
            return;
        while (*a)
            *out++ = *a++;
    }
};

template <> struct QConcatenable<char16_t *> : QConcatenable<const char16_t*>
{
    typedef char16_t *type;
};

template <> struct QConcatenable<QByteArray> : private QAbstractConcatenable
{
    typedef QByteArray type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static qsizetype size(const QByteArray &ba) { return ba.size(); }
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN static inline void appendTo(const QByteArray &ba, QChar *&out)
    {
        QAbstractConcatenable::convertFromUtf8(ba, out);
    }
#endif
    static inline void appendTo(const QByteArray &ba, char *&out)
    {
        const qsizetype n = ba.size();
        if (n)
            memcpy(out, ba.begin(), n);
        out += n;
    }
};


template <typename A, typename B>
struct QConcatenable< QStringBuilder<A, B> >
{
    typedef QStringBuilder<A, B> type;
    using ConvertTo = typename QtStringBuilder::ConvertToTypeHelper<
                typename QConcatenableEx<A>::ConvertTo,
                typename QConcatenableEx<B>::ConvertTo
            >::ConvertTo;
    enum { ExactSize = QConcatenableEx<A>::ExactSize && QConcatenableEx<B>::ExactSize };
    static qsizetype size(const type &p)
    {
        return QConcatenableEx<A>::size(p.a) + QConcatenableEx<B>::size(p.b);
    }
    template<typename T> static inline void appendTo(const type &p, T *&out)
    {
        QConcatenableEx<A>::appendTo(p.a, out);
        QConcatenableEx<B>::appendTo(p.b, out);
    }
};

template <typename A, typename B,
         typename = std::void_t<typename QConcatenableEx<A>::type, typename QConcatenableEx<B>::type>>
auto operator%(A &&a, B &&b)
{
    return QStringBuilder<A, B>(std::forward<A>(a), std::forward<B>(b));
}

// QT_USE_FAST_OPERATOR_PLUS was introduced in 4.7, QT_USE_QSTRINGBUILDER is to be used from 4.8 onwards
// QT_USE_FAST_OPERATOR_PLUS does not remove the normal operator+ for QByteArray
#if defined(QT_USE_FAST_OPERATOR_PLUS) || defined(QT_USE_QSTRINGBUILDER)
template <typename A, typename B,
         typename = std::void_t<typename QConcatenableEx<A>::type, typename QConcatenableEx<B>::type>>
auto operator+(A &&a, B &&b)
{
    return std::forward<A>(a) % std::forward<B>(b);
}
#endif

namespace QtStringBuilder {
template <typename A, typename B>
QByteArray &appendToByteArray(QByteArray &a, const QStringBuilder<A, B> &b, char)
{
    // append 8-bit data to a byte array
    qsizetype len = a.size() + QConcatenable< QStringBuilder<A, B> >::size(b);
    a.detach(); // a detach() in a.data() could reset a.capacity() to a.size()
    if (len > a.data_ptr().freeSpaceAtEnd()) // capacity() was broken when prepend()-optimization landed
        a.reserve(qMax(len, 2 * a.capacity()));
    char *it = a.data() + a.size();
    QConcatenable< QStringBuilder<A, B> >::appendTo(b, it);
    a.resize(len); //we need to resize after the appendTo for the case str+=foo+str
    return a;
}

#ifndef QT_NO_CAST_TO_ASCII
template <typename A, typename B>
QByteArray &appendToByteArray(QByteArray &a, const QStringBuilder<A, B> &b, QChar)
{
    return a += QString(b).toUtf8();
}
#endif
}

template <typename A, typename B>
QByteArray &operator+=(QByteArray &a, const QStringBuilder<A, B> &b)
{
    return QtStringBuilder::appendToByteArray(a, b,
                                              typename QConcatenable< QStringBuilder<A, B> >::ConvertTo::value_type());
}

template <typename A, typename B>
QString &operator+=(QString &a, const QStringBuilder<A, B> &b)
{
    qsizetype len = a.size() + QConcatenable< QStringBuilder<A, B> >::size(b);
    a.detach(); // a detach() in a.data() could reset a.capacity() to a.size()
    if (len > a.data_ptr().freeSpaceAtEnd()) // capacity() was broken when prepend()-optimization landed
        a.reserve(qMax(len, 2 * a.capacity()));
    QChar *it = a.data() + a.size();
    QConcatenable< QStringBuilder<A, B> >::appendTo(b, it);
    // we need to resize after the appendTo for the case str+=foo+str
    a.resize(it - a.constData()); //may be smaller than len if there was conversion from utf8
    return a;
}

QT_END_NAMESPACE

#endif // QSTRINGBUILDER_H
