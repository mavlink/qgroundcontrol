// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSTRINGCONVERTER_BASE_H
#define QSTRINGCONVERTER_BASE_H

#if 0
// QStringConverter(Base) class are handled in qstringconverter
#pragma qt_sync_stop_processing
#endif

#include <optional>

#include <QtCore/qglobal.h> // QT_{BEGIN,END}_NAMESPACE
#include <QtCore/qflags.h> // Q_DECLARE_FLAGS
#include <QtCore/qcontainerfwd.h>
#include <QtCore/qstringfwd.h>

#include <cstring>

QT_BEGIN_NAMESPACE

class QByteArrayView;
class QChar;
class QByteArrayView;
class QStringView;

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(Q_QDOC) && !defined(QT_BOOTSTRAPPED)
class QStringConverterBase
#else
class QStringConverter
#endif
{
public:
    enum class Flag {
        Default = 0,
        Stateless = 0x1,
        ConvertInvalidToNull = 0x2,
        WriteBom = 0x4,
        ConvertInitialBom = 0x8,
        UsesIcu = 0x10,
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    struct State {
        constexpr State(Flags f = Flag::Default) noexcept
            : flags(f), state_data{0, 0, 0, 0} {}
        ~State() { clear(); }

        State(State &&other) noexcept
            : flags(other.flags),
              remainingChars(other.remainingChars),
              invalidChars(other.invalidChars),
              state_data{other.state_data[0], other.state_data[1],
                         other.state_data[2], other.state_data[3]},
              clearFn(other.clearFn)
        { other.clearFn = nullptr; }
        State &operator=(State &&other) noexcept
        {
            clear();
            flags = other.flags;
            remainingChars = other.remainingChars;
            invalidChars = other.invalidChars;
            std::memmove(state_data, other.state_data, sizeof state_data); // self-assignment-safe
            clearFn = other.clearFn;
            other.clearFn = nullptr;
            return *this;
        }
        Q_CORE_EXPORT void clear() noexcept;
        Q_CORE_EXPORT void reset() noexcept;

        Flags flags;
        int internalState = 0;
        qsizetype remainingChars = 0;
        qsizetype invalidChars = 0;

        union {
            uint state_data[4];
            void *d[2];
        };
        using ClearDataFn = void (*)(State *) noexcept;
        ClearDataFn clearFn = nullptr;
    private:
        Q_DISABLE_COPY(State)
    };

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(Q_QDOC) && !defined(QT_BOOTSTRAPPED)
protected:
    QStringConverterBase() = default;
    ~QStringConverterBase() = default;
    QStringConverterBase(QStringConverterBase &&) = default;
    QStringConverterBase &operator=(QStringConverterBase &&) = default;
};

class QStringConverter : public QStringConverterBase
{
public:
#endif // Qt 6 compat for QStringConverterBase

    enum Encoding {
        Utf8,
#ifndef QT_BOOTSTRAPPED
        Utf16,
        Utf16LE,
        Utf16BE,
        Utf32,
        Utf32LE,
        Utf32BE,
#endif
        Latin1,
        System,
        LastEncoding = System
    };

protected:

    struct Interface
    {
        using DecoderFn = QChar * (*)(QChar *out, QByteArrayView in, State *state);
        using LengthFn = qsizetype (*)(qsizetype inLength);
        using EncoderFn = char * (*)(char *out, QStringView in, State *state);
        const char *name = nullptr;
        DecoderFn toUtf16 = nullptr;
        LengthFn toUtf16Len = nullptr;
        EncoderFn fromUtf16 = nullptr;
        LengthFn fromUtf16Len = nullptr;
    };

    constexpr QStringConverter() noexcept
        : iface(nullptr)
    {}
    constexpr explicit QStringConverter(Encoding encoding, Flags f)
        : iface(&encodingInterfaces[qsizetype(encoding)]), state(f)
    {}
    constexpr explicit QStringConverter(const Interface *i) noexcept
        : iface(i)
    {}
#if QT_CORE_REMOVED_SINCE(6, 8)
    Q_CORE_EXPORT explicit QStringConverter(const char *name, Flags f);
#endif
    Q_CORE_EXPORT explicit QStringConverter(QAnyStringView name, Flags f);


    ~QStringConverter() = default;

public:
    QStringConverter(QStringConverter &&) = default;
    QStringConverter &operator=(QStringConverter &&) = default;

    bool isValid() const noexcept { return iface != nullptr; }

    void resetState() noexcept
    {
        state.reset();
    }
    bool hasError() const noexcept { return state.invalidChars != 0; }

    Q_CORE_EXPORT const char *name() const noexcept;

#if QT_CORE_REMOVED_SINCE(6, 8)
    Q_CORE_EXPORT static std::optional<Encoding> encodingForName(const char *name) noexcept;
#endif
    Q_CORE_EXPORT static std::optional<Encoding> encodingForName(QAnyStringView name) noexcept;
    Q_DECL_PURE_FUNCTION Q_CORE_EXPORT static const char *nameForEncoding(Encoding e) noexcept;
    Q_CORE_EXPORT static std::optional<Encoding>
    encodingForData(QByteArrayView data, char16_t expectedFirstCharacter = 0) noexcept;
    Q_CORE_EXPORT static std::optional<Encoding> encodingForHtml(QByteArrayView data);

    Q_CORE_EXPORT static QStringList availableCodecs();

protected:
    const Interface *iface;
    State state;
private:
    Q_CORE_EXPORT static const Interface encodingInterfaces[Encoding::LastEncoding + 1];
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStringConverter::Flags)

QT_END_NAMESPACE

#endif
