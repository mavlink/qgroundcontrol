// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2020 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSTRINGCONVERTER_P_H
#define QSTRINGCONVERTER_P_H

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
#include <QtCore/qendian.h>
#include <QtCore/qstringconverter.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

#ifndef __cpp_char8_t
enum qchar8_t : uchar {};
#else
using qchar8_t = char8_t;
#endif

struct QLatin1
{
    // Defined in qstring.cpp
    static char16_t *convertToUnicode(char16_t *dst, QLatin1StringView in) noexcept;

    static QChar *convertToUnicode(QChar *buffer, QLatin1StringView in) noexcept
    {
        char16_t *dst = reinterpret_cast<char16_t *>(buffer);
        dst = convertToUnicode(dst, in);
        return reinterpret_cast<QChar *>(dst);
    }

    static QChar *convertToUnicode(QChar *dst, QByteArrayView in,
                                   [[maybe_unused]] QStringConverter::State *state) noexcept
    {
        Q_ASSERT(state);

        return convertToUnicode(dst, QLatin1StringView(in.data(), in.size()));
    }

    static char *convertFromUnicode(char *out, QStringView in, QStringConverter::State *state) noexcept;

    // Defined in qstring.cpp
    Q_CORE_EXPORT
    static char *convertFromUnicode(char *out, QStringView in) noexcept;
};

struct QUtf8BaseTraits
{
    static const bool isTrusted = false;
    static const bool allowNonCharacters = true;
    static const bool skipAsciiHandling = false;
    static const int Error = -1;
    static const int EndOfString = -2;

    static void appendByte(uchar *&ptr, uchar b)
    { *ptr++ = b; }

    static void appendByte(qchar8_t *&ptr, qchar8_t b)
    { *ptr++ = b; }

    static uchar peekByte(const char *ptr, qsizetype n = 0)
    { return ptr[n]; }

    static uchar peekByte(const uchar *ptr, qsizetype n = 0)
    { return ptr[n]; }

    static uchar peekByte(const qchar8_t *ptr, qsizetype n = 0)
    { return ptr[n]; }

    static qptrdiff availableBytes(const char *ptr, const char *end)
    { return end - ptr; }

    static qptrdiff availableBytes(const uchar *ptr, const uchar *end)
    { return end - ptr; }

    static qptrdiff availableBytes(const qchar8_t *ptr, const qchar8_t *end)
    { return end - ptr; }

    static void advanceByte(const char *&ptr, qsizetype n = 1)
    { ptr += n; }

    static void advanceByte(const uchar *&ptr, qsizetype n = 1)
    { ptr += n; }

    static void advanceByte(const qchar8_t *&ptr, qsizetype n = 1)
    { ptr += n; }

    static void appendUtf16(char16_t *&ptr, char16_t uc)
    { *ptr++ = char16_t(uc); }

    static void appendUcs4(char16_t *&ptr, char32_t uc)
    {
        appendUtf16(ptr, QChar::highSurrogate(uc));
        appendUtf16(ptr, QChar::lowSurrogate(uc));
    }

    static char16_t peekUtf16(const char16_t *ptr, qsizetype n = 0) { return ptr[n]; }

    static qptrdiff availableUtf16(const char16_t *ptr, const char16_t *end)
    { return end - ptr; }

    static void advanceUtf16(const char16_t *&ptr, qsizetype n = 1) { ptr += n; }

    static void appendUtf16(char32_t *&ptr, char16_t uc)
    { *ptr++ = char32_t(uc); }

    static void appendUcs4(char32_t *&ptr, char32_t uc)
    { *ptr++ = uc; }
};

struct QUtf8BaseTraitsNoAscii : public QUtf8BaseTraits
{
    static const bool skipAsciiHandling = true;
};

namespace QUtf8Functions
{
    /// returns 0 on success; errors can only happen if \a u is a surrogate:
    /// Error if \a u is a low surrogate;
    /// if \a u is a high surrogate, Error if the next isn't a low one,
    /// EndOfString if we run into the end of the string.
    template <typename Traits, typename OutputPtr, typename InputPtr> inline
    int toUtf8(char16_t u, OutputPtr &dst, InputPtr &src, InputPtr end)
    {
        if (!Traits::skipAsciiHandling && u < 0x80) {
            // U+0000 to U+007F (US-ASCII) - one byte
            Traits::appendByte(dst, uchar(u));
            return 0;
        } else if (u < 0x0800) {
            // U+0080 to U+07FF - two bytes
            // first of two bytes
            Traits::appendByte(dst, 0xc0 | uchar(u >> 6));
        } else {
            if (!QChar::isSurrogate(u)) {
                // U+0800 to U+FFFF (except U+D800-U+DFFF) - three bytes
                if (!Traits::allowNonCharacters && QChar::isNonCharacter(u))
                    return Traits::Error;

                // first of three bytes
                Traits::appendByte(dst, 0xe0 | uchar(u >> 12));
            } else {
                // U+10000 to U+10FFFF - four bytes
                // need to get one extra codepoint
                if (Traits::availableUtf16(src, end) == 0)
                    return Traits::EndOfString;

                char16_t low = Traits::peekUtf16(src);
                if (!QChar::isHighSurrogate(u))
                    return Traits::Error;
                if (!QChar::isLowSurrogate(low))
                    return Traits::Error;

                Traits::advanceUtf16(src);
                char32_t ucs4 = QChar::surrogateToUcs4(u, low);

                if (!Traits::allowNonCharacters && QChar::isNonCharacter(ucs4))
                    return Traits::Error;

                // first byte
                Traits::appendByte(dst, 0xf0 | (uchar(ucs4 >> 18) & 0xf));

                // second of four bytes
                Traits::appendByte(dst, 0x80 | (uchar(ucs4 >> 12) & 0x3f));

                // for the rest of the bytes
                u = char16_t(ucs4);
            }

            // second to last byte
            Traits::appendByte(dst, 0x80 | (uchar(u >> 6) & 0x3f));
        }

        // last byte
        Traits::appendByte(dst, 0x80 | (u & 0x3f));
        return 0;
    }

    inline bool isContinuationByte(uchar b)
    {
        return (b & 0xc0) == 0x80;
    }

    /// returns the number of characters consumed (including \a b) in case of success;
    /// returns negative in case of error: Traits::Error or Traits::EndOfString
    template <typename Traits, typename OutputPtr, typename InputPtr> inline
    qsizetype fromUtf8(uchar b, OutputPtr &dst, InputPtr &src, InputPtr end)
    {
        qsizetype charsNeeded;
        char32_t min_uc;
        char32_t uc;

        if (!Traits::skipAsciiHandling && b < 0x80) {
            // US-ASCII
            Traits::appendUtf16(dst, b);
            return 1;
        }

        if (!Traits::isTrusted && Q_UNLIKELY(b <= 0xC1)) {
            // an UTF-8 first character must be at least 0xC0
            // however, all 0xC0 and 0xC1 first bytes can only produce overlong sequences
            return Traits::Error;
        } else if (b < 0xe0) {
            charsNeeded = 2;
            min_uc = 0x80;
            uc = b & 0x1f;
        } else if (b < 0xf0) {
            charsNeeded = 3;
            min_uc = 0x800;
            uc = b & 0x0f;
        } else if (b < 0xf5) {
            charsNeeded = 4;
            min_uc = 0x10000;
            uc = b & 0x07;
        } else {
            // the last Unicode character is U+10FFFF
            // it's encoded in UTF-8 as "\xF4\x8F\xBF\xBF"
            // therefore, a byte higher than 0xF4 is not the UTF-8 first byte
            return Traits::Error;
        }

        qptrdiff bytesAvailable = Traits::availableBytes(src, end);
        if (Q_UNLIKELY(bytesAvailable < charsNeeded - 1)) {
            // it's possible that we have an error instead of just unfinished bytes
            if (bytesAvailable > 0 && !isContinuationByte(Traits::peekByte(src, 0)))
                return Traits::Error;
            if (bytesAvailable > 1 && !isContinuationByte(Traits::peekByte(src, 1)))
                return Traits::Error;
            return Traits::EndOfString;
        }

        // first continuation character
        b = Traits::peekByte(src, 0);
        if (!isContinuationByte(b))
            return Traits::Error;
        uc <<= 6;
        uc |= b & 0x3f;

        if (charsNeeded > 2) {
            // second continuation character
            b = Traits::peekByte(src, 1);
            if (!isContinuationByte(b))
                return Traits::Error;
            uc <<= 6;
            uc |= b & 0x3f;

            if (charsNeeded > 3) {
                // third continuation character
                b = Traits::peekByte(src, 2);
                if (!isContinuationByte(b))
                    return Traits::Error;
                uc <<= 6;
                uc |= b & 0x3f;
            }
        }

        // we've decoded something; safety-check it
        if (!Traits::isTrusted) {
            if (uc < min_uc)
                return Traits::Error;
            if (QChar::isSurrogate(uc) || uc > QChar::LastValidCodePoint)
                return Traits::Error;
            if (!Traits::allowNonCharacters && QChar::isNonCharacter(uc))
                return Traits::Error;
        }

        // write the UTF-16 sequence
        if (!QChar::requiresSurrogates(uc)) {
            // UTF-8 decoded and no surrogates are required
            // detach if necessary
            Traits::appendUtf16(dst, char16_t(uc));
        } else {
            // UTF-8 decoded to something that requires a surrogate pair
            Traits::appendUcs4(dst, uc);
        }

        Traits::advanceByte(src, charsNeeded - 1);
        return charsNeeded;
    }

    /// wrapper around fromUtf8<Traits> to provide a simpler interface for a common case
    template <typename Traits = QUtf8BaseTraits>
    char32_t nextUcs4FromUtf8(const qchar8_t *&src, const qchar8_t *end,
                              char32_t errorChar = QChar::ReplacementCharacter)
    {
        auto ch = *src++;
        char32_t buffer[1];
        auto *output = buffer;
        if (QUtf8Functions::fromUtf8<Traits>(ch, output, src, end) < 0)
            return errorChar; // decoding error
        Q_ASSERT(output == buffer + 1);
        return buffer[0];
    }
}

enum DataEndianness
{
    DetectEndianness,
    BigEndianness,
    LittleEndianness
};

struct QUtf8
{
    static QChar *convertToUnicode(QChar *buffer, QByteArrayView in) noexcept
    {
        char16_t *dst = reinterpret_cast<char16_t *>(buffer);
        dst = QUtf8::convertToUnicode(dst, in);
        return reinterpret_cast<QChar *>(dst);
    }

    Q_CORE_EXPORT static char16_t* convertToUnicode(char16_t *dst, QByteArrayView in) noexcept;
    static QString convertToUnicode(QByteArrayView in);
    Q_CORE_EXPORT static QString convertToUnicode(QByteArrayView in, QStringConverter::State *state);

    static QChar *convertToUnicode(QChar *out, QByteArrayView in, QStringConverter::State *state)
    {
        char16_t *buffer = reinterpret_cast<char16_t *>(out);
        buffer = convertToUnicode(buffer, in, state);
        return reinterpret_cast<QChar *>(buffer);
    }

    static char16_t *convertToUnicode(char16_t *dst, QByteArrayView in, QStringConverter::State *state);

    Q_CORE_EXPORT
    static char *convertFromUnicode(char *dst, QStringView in) noexcept;
    Q_CORE_EXPORT static QByteArray convertFromUnicode(QStringView in);
    Q_CORE_EXPORT static QByteArray convertFromUnicode(QStringView in, QStringConverter::State *state);
    static char *convertFromUnicode(char *out, QStringView in, QStringConverter::State *state);
    Q_CORE_EXPORT static char *convertFromLatin1(char *out, QLatin1StringView in);
    struct ValidUtf8Result {
        bool isValidUtf8;
        bool isValidAscii;
    };
    static ValidUtf8Result isValidUtf8(QByteArrayView in);
    static int compareUtf8(QByteArrayView utf8, QStringView utf16,
                           Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
    static int compareUtf8(QByteArrayView utf8, QLatin1StringView s,
                           Qt::CaseSensitivity cs = Qt::CaseSensitive);
    static int compareUtf8(QByteArrayView lhs, QByteArrayView rhs,
                           Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

private:
    template <typename OnErrorLambda> static char *
    convertFromUnicode(char *out, QStringView in, OnErrorLambda &&onError) noexcept;
    template <typename OnErrorLambda> static char16_t *
    convertToUnicode(char16_t *dst, QByteArrayView in, OnErrorLambda &&onError) noexcept;
};

struct QUtf16
{
    Q_CORE_EXPORT static QString convertToUnicode(QByteArrayView, QStringConverter::State *, DataEndianness = DetectEndianness);
    static QChar *convertToUnicode(QChar *out, QByteArrayView, QStringConverter::State *state, DataEndianness endian);
    Q_CORE_EXPORT static QByteArray convertFromUnicode(QStringView, QStringConverter::State *, DataEndianness = DetectEndianness);
    static char *convertFromUnicode(char *out, QStringView in, QStringConverter::State *state, DataEndianness endian);
};

struct QUtf32
{
    static QChar *convertToUnicode(QChar *out, QByteArrayView, QStringConverter::State *state, DataEndianness endian);
    Q_CORE_EXPORT static QString convertToUnicode(QByteArrayView, QStringConverter::State *, DataEndianness = DetectEndianness);
    Q_CORE_EXPORT static QByteArray convertFromUnicode(QStringView, QStringConverter::State *, DataEndianness = DetectEndianness);
    static char *convertFromUnicode(char *out, QStringView in, QStringConverter::State *state, DataEndianness endian);
};

struct Q_CORE_EXPORT QLocal8Bit
{
#if !defined(Q_OS_WIN) || defined(QT_BOOTSTRAPPED)
    static QString convertToUnicode(QByteArrayView in, QStringConverter::State *state)
    { return QUtf8::convertToUnicode(in, state); }
    static QByteArray convertFromUnicode(QStringView in, QStringConverter::State *state)
    { return QUtf8::convertFromUnicode(in, state); }
#else
    static int checkUtf8();
    static bool isUtf8()
    {
        Q_CONSTINIT
        static QBasicAtomicInteger<qint8> result = { 0 };
        int r = result.loadRelaxed();
        if (r == 0) {
            r = checkUtf8();
            result.storeRelaxed(r);
        }
        return r > 0;
    }
    static QString convertToUnicode_sys(QByteArrayView, quint32, QStringConverter::State *);
    static QString convertToUnicode_sys(QByteArrayView, QStringConverter::State *);
    static QString convertToUnicode(QByteArrayView in, QStringConverter::State *state)
    {
        if (isUtf8())
            return QUtf8::convertToUnicode(in, state);
        return convertToUnicode_sys(in, state);
    }
    static QByteArray convertFromUnicode_sys(QStringView, quint32, QStringConverter::State *);
    static QByteArray convertFromUnicode_sys(QStringView, QStringConverter::State *);
    static QByteArray convertFromUnicode(QStringView in, QStringConverter::State *state)
    {
        if (isUtf8())
            return QUtf8::convertFromUnicode(in, state);
        return convertFromUnicode_sys(in, state);
    }
#endif
};

QT_END_NAMESPACE

#endif // QSTRINGCONVERTER_P_H
