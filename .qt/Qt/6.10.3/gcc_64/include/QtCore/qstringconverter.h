// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#if 0
// keep existing syncqt header working after the move of the class
// into qstringconverter_base
#pragma qt_class(QStringConverter)
#pragma qt_class(QStringConverterBase)
#endif

#ifndef QSTRINGCONVERTER_H
#define QSTRINGCONVERTER_H

#include <QtCore/qstringconverter_base.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringbuilder.h>

QT_BEGIN_NAMESPACE

class QStringEncoder : public QStringConverter
{
protected:
    constexpr explicit QStringEncoder(const Interface *i) noexcept
        : QStringConverter(i)
    {}
public:
    constexpr QStringEncoder() noexcept
        : QStringConverter()
    {}
    constexpr explicit QStringEncoder(Encoding encoding, Flags flags = Flag::Default)
        : QStringConverter(encoding, flags)
    {}
    explicit QStringEncoder(QAnyStringView name, Flags flags = Flag::Default)
        : QStringConverter(name, flags)
    {}

    template<typename T>
    struct DecodedData
    {
        QStringEncoder *encoder;
        T data;
        operator QByteArray() const { return encoder->encodeAsByteArray(data); }
    };
    Q_WEAK_OVERLOAD
    DecodedData<const QString &> operator()(const QString &str)
    { return DecodedData<const QString &>{this, str}; }
    DecodedData<QStringView> operator()(QStringView in)
    { return DecodedData<QStringView>{this, in}; }
    Q_WEAK_OVERLOAD
    DecodedData<const QString &> encode(const QString &str)
    { return DecodedData<const QString &>{this, str}; }
    DecodedData<QStringView> encode(QStringView in)
    { return DecodedData<QStringView>{this, in}; }

    qsizetype requiredSpace(qsizetype inputLength) const
    { return iface ? iface->fromUtf16Len(inputLength) : 0; }
    char *appendToBuffer(char *out, QStringView in)
    {
        if (!iface) {
            state.invalidChars = 1;
            return out;
        }
        return iface->fromUtf16(out, in, &state);
    }
private:
    QByteArray encodeAsByteArray(QStringView in)
    {
        if (!iface) {
            // ensure that hasError returns true
            state.invalidChars = 1;
            return {};
        }
        QByteArray result(iface->fromUtf16Len(in.size()), Qt::Uninitialized);
        char *out = result.data();
        out = iface->fromUtf16(out, in, &state);
        result.truncate(out - result.constData());
        return result;
    }

};

class QStringDecoder : public QStringConverter
{
protected:
    constexpr explicit QStringDecoder(const Interface *i) noexcept
        : QStringConverter(i)
    {}
public:
    constexpr explicit QStringDecoder(Encoding encoding, Flags flags = Flag::Default)
        : QStringConverter(encoding, flags)
    {}
    constexpr QStringDecoder() noexcept
        : QStringConverter()
    {}
    explicit QStringDecoder(QAnyStringView name, Flags f = Flag::Default)
        : QStringConverter(name, f)
    {}

    template<typename T>
    struct EncodedData
    {
        QStringDecoder *decoder;
        T data;
        operator QString() const { return decoder->decodeAsString(data); }
    };
    Q_WEAK_OVERLOAD
    EncodedData<const QByteArray &> operator()(const QByteArray &ba)
    { return EncodedData<const QByteArray &>{this, ba}; }
    EncodedData<QByteArrayView> operator()(QByteArrayView ba)
    { return EncodedData<QByteArrayView>{this, ba}; }
    Q_WEAK_OVERLOAD
    EncodedData<const QByteArray &> decode(const QByteArray &ba)
    { return EncodedData<const QByteArray &>{this, ba}; }
    EncodedData<QByteArrayView> decode(QByteArrayView ba)
    { return EncodedData<QByteArrayView>{this, ba}; }

    qsizetype requiredSpace(qsizetype inputLength) const
    { return iface ? iface->toUtf16Len(inputLength) : 0; }
    QChar *appendToBuffer(QChar *out, QByteArrayView ba)
    {
        if (!iface) {
            state.invalidChars = 1;
            return out;
        }
        return iface->toUtf16(out, ba, &state);
    }
    char16_t *appendToBuffer(char16_t *out, QByteArrayView ba)
    { return reinterpret_cast<char16_t *>(appendToBuffer(reinterpret_cast<QChar *>(out), ba)); }

    Q_CORE_EXPORT static QStringDecoder decoderForHtml(QByteArrayView data);

private:
    QString decodeAsString(QByteArrayView in)
    {
        if (!iface) {
            // ensure that hasError returns true
            state.invalidChars = 1;
            return {};
        }
        QString result(iface->toUtf16Len(in.size()), Qt::Uninitialized);
        const QChar *out = iface->toUtf16(result.data(), in, &state);
        result.truncate(out - result.constData());
        return result;
    }
};


#if defined(QT_USE_FAST_OPERATOR_PLUS) || defined(QT_USE_QSTRINGBUILDER)
template <typename T>
struct QConcatenable<QStringDecoder::EncodedData<T>>
        : private QAbstractConcatenable
{
    typedef QChar type;
    typedef QString ConvertTo;
    enum { ExactSize = false };
    static qsizetype size(const QStringDecoder::EncodedData<T> &s) { return s.decoder->requiredSpace(s.data.size()); }
    static inline void appendTo(const QStringDecoder::EncodedData<T> &s, QChar *&out)
    {
        out = s.decoder->appendToBuffer(out, s.data);
    }
};

template <typename T>
struct QConcatenable<QStringEncoder::DecodedData<T>>
        : private QAbstractConcatenable
{
    typedef char type;
    typedef QByteArray ConvertTo;
    enum { ExactSize = false };
    static qsizetype size(const QStringEncoder::DecodedData<T> &s) { return s.encoder->requiredSpace(s.data.size()); }
    static inline void appendTo(const QStringEncoder::DecodedData<T> &s, char *&out)
    {
        out = s.encoder->appendToBuffer(out, s.data);
    }
};

template <typename T>
QString &operator+=(QString &a, const QStringDecoder::EncodedData<T> &b)
{
    qsizetype len = a.size() + QConcatenable<QStringDecoder::EncodedData<T>>::size(b);
    a.reserve(len);
    QChar *it = a.data() + a.size();
    QConcatenable<QStringDecoder::EncodedData<T>>::appendTo(b, it);
    a.resize(qsizetype(it - a.constData())); //may be smaller than len
    return a;
}

template <typename T>
QByteArray &operator+=(QByteArray &a, const QStringEncoder::DecodedData<T> &b)
{
    qsizetype len = a.size() + QConcatenable<QStringEncoder::DecodedData<T>>::size(b);
    a.reserve(len);
    char *it = a.data() + a.size();
    QConcatenable<QStringEncoder::DecodedData<T>>::appendTo(b, it);
    a.resize(qsizetype(it - a.constData())); //may be smaller than len
    return a;
}
#endif

template <typename InputIterator>
void QString::assign_helper_char8(InputIterator first, InputIterator last)
{
    static_assert(!QString::is_contiguous_iterator_v<InputIterator>,
        "Internal error: Should have been handed over to the QAnyStringView overload."
    );

    using ValueType = typename std::iterator_traits<InputIterator>::value_type;
    constexpr bool IsFwdIt = std::is_convertible_v<
        typename std::iterator_traits<InputIterator>::iterator_category,
        std::forward_iterator_tag
    >;

    resize(0);
    // In case of not being shared, there is the possibility of having free space at begin
    // even after the resize to zero.
    if (const auto offset = d.freeSpaceAtBegin())
        d.setBegin(d.begin() - offset);

    if constexpr (IsFwdIt)
        reserve(static_cast<qsizetype>(std::distance(first, last)));

    auto toUtf16 = QStringDecoder(QStringDecoder::Utf8);
    auto availableCapacity = d.constAllocatedCapacity();
    auto *dst = d.data();
    auto *dend = d.data() + availableCapacity;

    while (true) {
        if (first == last) {                                    // ran out of input elements
            Q_ASSERT(!std::less<>{}(dend, dst));
            d.size = dst - d.begin();
            return;
        }
        const ValueType next = *first; // decays proxies, if any
        const auto chunk = QUtf8StringView(&next, 1);
        // UTF-8 characters can have a maximum size of 4 bytes and may result in a surrogate
        // pair of UTF-16 code units. In the input-iterator case, we don't know the size
        // and would need to always reserve space for 2 code units. To keep our promise
        // of 'not allocating if it fits', we have to pre-check this condition.
        //          We know that it fits in the forward-iterator case.
        if constexpr (!IsFwdIt) {
            constexpr qsizetype Pair = 2;
            char16_t buf[Pair];
            const qptrdiff n = toUtf16.appendToBuffer(buf, chunk) - buf;
            if (dend - dst < n) {                               // ran out of allocated memory
                const auto offset = dst - d.begin();
                reallocData(d.constAllocatedCapacity() + Pair, QArrayData::Grow);
                // update the pointers since we've re-allocated
                availableCapacity = d.constAllocatedCapacity();
                dst = d.data() + offset;
                dend = d.data() + availableCapacity;
            }
            dst = std::copy_n(buf, n, dst);
        } else {                                                // take the fast path
            dst = toUtf16.appendToBuffer(dst, chunk);
        }
        ++first;
    }
}

QT_END_NAMESPACE

#endif
