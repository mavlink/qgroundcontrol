// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QLOCALE_P_H
#define QLOCALE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qlocale.h"

#include <QtCore/qcalendar.h>
#include <QtCore/qlist.h>
#include <QtCore/qnumeric.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvarlengtharray.h>
#ifdef Q_OS_WASM
#include <private/qstdweb_p.h>
#endif

#include <limits>
#include <cmath>
#include <string_view>

QT_BEGIN_NAMESPACE

template <typename T> struct QSimpleParsedNumber
{
    T result;
    // When used < 0, -used is how much was used, but it was an error.
    qsizetype used;
    bool ok() const { return used > 0; }
};

template <int Extent, uchar Lowest> struct QCharacterSetMatch
{
    using Word = qregisteruint;
    static constexpr int WordBits = std::numeric_limits<Word>::digits;
    static constexpr int MaxRange = WordBits * Extent;
    qregisteruint mask[Extent];

    constexpr QCharacterSetMatch(std::string_view set) noexcept
        : mask{}
    {
        for (char c : set) {
            auto [offset, shift] = maskLocation(c);
            mask[offset] |= Word(1) << shift;
        }
    }

    constexpr bool matches(uchar c) const noexcept
    {
        auto [offset, shift] = maskLocation(c);
        if (offset < 0)
            return false;
        Word m = 0;
        if constexpr (Extent == 2) {
            // special case for faster code (with GCC, at least)
            m = (c - Lowest < WordBits) ? mask[0] : mask[1];
        } else {
            m = mask[offset];
        }
        return (m >> shift) & 1;
    }

    constexpr auto maskLocation(uchar c) const noexcept
    {
        struct { int offset; int shift; } r = { -1, -1 };
        unsigned idx = c - Lowest;
        if (idx < MaxRange) {
            r.offset = idx / WordBits;
            r.shift = idx % WordBits;
        }
        return r;
    }
};

namespace QtPrivate {
inline constexpr char ascii_space_chars[] =
        "\t"    // 9: HT - horizontal tab
        "\n"    // 10: LF - line feed
        "\v"    // 11: VT - vertical tab
        "\f"    // 12: FF - form feed
        "\r"    // 13: CR - carriage return
        " ";    // 32: space

template <const char *Set, int ForcedLowest = -1>
inline constexpr auto makeCharacterSetMatch() noexcept
{
    constexpr int BitsPerWord = std::numeric_limits<qregisteruint>::digits;
    constexpr auto view = std::string_view(Set);
    constexpr uchar MinElement = *std::min_element(view.begin(), view.end());
    constexpr uchar MaxElement = *std::max_element(view.begin(), view.end());
    constexpr int Range = MaxElement - MinElement;
    constexpr int Extent = (Range + BitsPerWord - 1) / BitsPerWord;
    constexpr int TotalBits = BitsPerWord * Extent;

    if constexpr (ForcedLowest >= 0) {
        // use the force
        static_assert(ForcedLowest <= int(MinElement), "The force is not with you");
        static_assert(ForcedLowest + TotalBits >= MaxElement, "The force is not with you");
        return QCharacterSetMatch<Extent, ForcedLowest>(view);
    } else if constexpr (MaxElement < TotalBits) {
        // if we can use a Lowest of zero, we can remove a subtraction
        // from the matches() code at runtime
        return QCharacterSetMatch<Extent, 0>(view);
    } else {
        return QCharacterSetMatch<Extent, MinElement>(view);
    }
}
} // QtPrivate

// Subclassed by Android platform plugin:
class Q_CORE_EXPORT QSystemLocale
{
    Q_DISABLE_COPY_MOVE(QSystemLocale)
    QSystemLocale *next = nullptr; // Maintains a stack.

public:
    QSystemLocale();
    virtual ~QSystemLocale();

    struct CurrencyToStringArgument
    {
        CurrencyToStringArgument() { }
        CurrencyToStringArgument(const QVariant &v, const QString &s)
            : value(v), symbol(s) { }
        QVariant value;
        QString symbol;
    };

    enum QueryType {
        LanguageId, // uint
        TerritoryId, // uint
        DecimalPoint, // QString
        Grouping, // QLocaleData::GroupSizes
        GroupSeparator, // QString (empty QString means: don't group digits)
        ZeroDigit, // QString
        NegativeSign, // QString
        DateFormatLong, // QString
        DateFormatShort, // QString
        TimeFormatLong, // QString
        TimeFormatShort, // QString
        DayNameLong, // QString, in: int
        DayNameShort, // QString, in: int
        DayNameNarrow, // QString, in: int
        MonthNameLong, // QString, in: int
        MonthNameShort, // QString, in: int
        MonthNameNarrow, // QString, in: int
        DateToStringLong, // QString, in: QDate
        DateToStringShort, // QString in: QDate
        TimeToStringLong, // QString in: QTime
        TimeToStringShort, // QString in: QTime
        DateTimeFormatLong, // QString
        DateTimeFormatShort, // QString
        DateTimeToStringLong, // QString in: QDateTime
        DateTimeToStringShort, // QString in: QDateTime
        MeasurementSystem, // uint
        PositiveSign, // QString
        AMText, // QString
        PMText, // QString
        FirstDayOfWeek, // Qt::DayOfWeek
        Weekdays, // QList<Qt::DayOfWeek>
        CurrencySymbol, // QString in: CurrencyToStringArgument
        CurrencyToString, // QString in: qlonglong, qulonglong or double
        Collation, // QString
        UILanguages, // QStringList
        StringToStandardQuotation, // QString in: QStringView to quote
        StringToAlternateQuotation, // QString in: QStringView to quote
        ScriptId, // uint
        ListToSeparatedString, // QString
        LocaleChanged, // system locale changed
        NativeLanguageName, // QString
        NativeTerritoryName, // QString
        StandaloneMonthNameLong, // QString, in: int
        StandaloneMonthNameShort, // QString, in: int
        StandaloneMonthNameNarrow, // QString, in: int
        StandaloneDayNameLong, // QString, in: int
        StandaloneDayNameShort, // QString, in: int
        StandaloneDayNameNarrow // QString, in: int
    };
    virtual QVariant query(QueryType type, QVariant &&in = QVariant()) const;

    virtual QLocale fallbackLocale() const;
    inline qsizetype fallbackLocaleIndex() const;

protected:
    inline const QSharedDataPointer<QLocalePrivate> localeData(const QLocale &locale) const
    {
        return locale.d;
    }
};
Q_DECLARE_TYPEINFO(QSystemLocale::QueryType, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(QSystemLocale::CurrencyToStringArgument, Q_RELOCATABLE_TYPE);

struct QLocaleId
{
    [[nodiscard]] Q_AUTOTEST_EXPORT static QLocaleId fromName(QStringView name) noexcept;
    [[nodiscard]] inline bool operator==(QLocaleId other) const noexcept
    { return language_id == other.language_id && script_id == other.script_id && territory_id == other.territory_id; }
    [[nodiscard]] inline bool operator!=(QLocaleId other) const noexcept
    { return !operator==(other); }
    [[nodiscard]] inline bool isValid() const noexcept
    {
        return language_id <= QLocale::LastLanguage && script_id <= QLocale::LastScript
                && territory_id <= QLocale::LastTerritory;
    }
    [[nodiscard]] inline bool matchesAll() const noexcept
    {
        return !language_id && !script_id && !territory_id;
    }
    // Use as: filter.accept...(candidate)
    [[nodiscard]] inline bool acceptLanguage(quint16 lang) const noexcept
    {
        // Always reject AnyLanguage (only used for last entry in locale_data array).
        // So, when searching for AnyLanguage, accept everything *but* AnyLanguage.
        return language_id ? lang == language_id : lang;
    }
    [[nodiscard]] inline bool acceptScriptTerritory(QLocaleId other) const noexcept
    {
        return (!territory_id || other.territory_id == territory_id)
                && (!script_id || other.script_id == script_id);
    }

    [[nodiscard]] QLocaleId withLikelySubtagsAdded() const noexcept;
    [[nodiscard]] QLocaleId withLikelySubtagsRemoved() const noexcept;

    [[nodiscard]] QByteArray name(char separator = '-') const;

    ushort language_id = 0, script_id = 0, territory_id = 0;
};
Q_DECLARE_TYPEINFO(QLocaleId, Q_PRIMITIVE_TYPE);

struct QLocaleData
{
public:
    // Having an index for each locale enables us to have diverse sources of
    // data, e.g. calendar locales, as well as the main CLDR-derived data.
    [[nodiscard]] static qsizetype findLocaleIndex(QLocaleId localeId) noexcept;
    [[nodiscard]] static const QLocaleData *c() noexcept;
    [[nodiscard]] Q_AUTOTEST_EXPORT
    static bool allLocaleDataRows(bool (*check)(qsizetype, const QLocaleData &));

    enum DoubleForm {
        DFExponent = 0,
        DFDecimal,
        DFSignificantDigits,
        _DFMax = DFSignificantDigits
    };

    enum Flags {
        NoFlags             = 0,
        AddTrailingZeroes   = 0x01,
        ZeroPadded          = 0x02,
        LeftAdjusted        = 0x04,
        BlankBeforePositive = 0x08,
        AlwaysShowSign      = 0x10,
        GroupDigits         = 0x20,
        CapitalEorX         = 0x40,

        ShowBase            = 0x80,
        UppercaseBase       = 0x100,
        ZeroPadExponent     = 0x200,
        ForcePoint          = 0x400
    };

    enum NumberMode { IntegerMode, DoubleStandardMode, DoubleScientificMode };

    struct GroupSizes // Numbers of digits in various groups:
    {
        int first = 0; // Min needed before the separator, when there's only one.
        int higher = 0; // Each group between separators.
        int least = 0; // Least significant, when any separators appear.
        bool isValid() const { return least > 0 && higher > first && first > 0; }
    };

    using CharBuff = QVarLengthArray<char, 256>;

    struct ParsingResult
    {
        enum State { // A duplicate of QValidator::State
            Invalid,
            Intermediate,
            Acceptable,
        };

        State state = Invalid;
        CharBuff buff;
    };

private:
    enum PrecisionMode {
        PMDecimalDigits =       0x01,
        PMSignificantDigits =   0x02,
        PMChopTrailingZeros =   0x03
    };

    [[nodiscard]] QString decimalForm(QString &&digits, int decpt, int precision,
                                      PrecisionMode pm, bool mustMarkDecimal,
                                      bool groupDigits) const;
    [[nodiscard]] QString exponentForm(QString &&digits, int decpt, int precision,
                                       PrecisionMode pm, bool mustMarkDecimal,
                                       int minExponentDigits) const;
    [[nodiscard]] QString signPrefix(bool negative, unsigned flags) const;
    [[nodiscard]] QString applyIntegerFormatting(QString &&numStr, bool negative, int precision,
                                                 int base, int width, unsigned flags) const;

public:
    [[nodiscard]] QString doubleToString(double d,
                                         int precision = -1,
                                         DoubleForm form = DFSignificantDigits,
                                         int width = -1,
                                         unsigned flags = NoFlags) const;
    [[nodiscard]] QString longLongToString(qint64 l, int precision = -1,
                                           int base = 10,
                                           int width = -1,
                                           unsigned flags = NoFlags) const;
    [[nodiscard]] QString unsLongLongToString(quint64 l, int precision = -1,
                                              int base = 10,
                                              int width = -1,
                                              unsigned flags = NoFlags) const;

    // this function is meant to be called with the result of stringToDouble or bytearrayToDouble
    // so *ok must have been properly set (if not null)
    [[nodiscard]] static float convertDoubleToFloat(double d, bool *ok)
    {
        float result;
        bool b = convertDoubleTo<float>(d, &result);
        if (ok && *ok)
            *ok = b;
        return result;
    }

    [[nodiscard]] double stringToDouble(QStringView str, bool *ok,
                                        QLocale::NumberOptions options) const;
    [[nodiscard]] QSimpleParsedNumber<qint64>
    stringToLongLong(QStringView str, int base, QLocale::NumberOptions options) const;
    [[nodiscard]] QSimpleParsedNumber<quint64>
    stringToUnsLongLong(QStringView str, int base, QLocale::NumberOptions options) const;

    // this function is used in QIntValidator (QtGui)
    [[nodiscard]] Q_CORE_EXPORT
    static QSimpleParsedNumber<qint64> bytearrayToLongLong(QByteArrayView num, int base);
    [[nodiscard]] static QSimpleParsedNumber<quint64>
    bytearrayToUnsLongLong(QByteArrayView num, int base);

    [[nodiscard]] bool numberToCLocale(QStringView s, QLocale::NumberOptions number_options,
                                       NumberMode mode, CharBuff *result) const;

    struct NumericData
    {
#ifndef QT_NO_SYSTEMLOCALE
        // Only used for the system locale, to store data for the view to look at:
        QString sysDecimal, sysGroup, sysMinus, sysPlus;
#endif
        QStringView decimal, group, minus, plus, exponent;
        char32_t zeroUcs = 0;
        qint8 zeroLen = 0;
        bool isC = false; // C locale sets this and nothing else.
        bool exponentCyrillic = false; // True only for floating-point parsing of Cyrillic.
        void setZero(QStringView zero)
        {
            // No known locale has digits that are more than one Unicode
            // code-point, so we can safely deal with digits as plain char32_t.
            switch (zero.size()) {
            case 1:
                Q_ASSERT(!zero.at(0).isSurrogate());
                zeroUcs = zero.at(0).unicode();
                zeroLen = 1;
                break;
            case 2:
                Q_ASSERT(zero.at(0).isHighSurrogate());
                Q_ASSERT(zero.at(1).isLowSurrogate());
                zeroUcs = QChar::surrogateToUcs4(zero.at(0), zero.at(1));
                zeroLen = 2;
                break;
            default:
                Q_ASSERT(zero.size() == 0); // i.e. we got no value to use
                break;
            }
        }
        [[nodiscard]] bool isValid(NumberMode mode) const // Asserted as a sanity check.
        {
            if (isC)
                return true;
            if (exponentCyrillic && exponent != u"E" && exponent != u"\u0415")
                return false;
            return (zeroLen == 1 || zeroLen == 2) && zeroUcs > 0
                && (mode == IntegerMode || !decimal.isEmpty())
                // group may be empty (user config in system locale)
                && !minus.isEmpty() && !plus.isEmpty()
                && (mode != DoubleScientificMode || !exponent.isEmpty());
        }
    };
    [[nodiscard]] inline NumericData numericData(NumberMode mode) const;

    // this function is used in QIntValidator (QtGui)
    [[nodiscard]] Q_CORE_EXPORT ParsingResult
    validateChars(QStringView str, NumberMode numMode, int decDigits = -1,
                  QLocale::NumberOptions number_options = QLocale::DefaultNumberOptions) const;

    // Access to assorted data members:
    [[nodiscard]] QLocaleId id() const
    { return QLocaleId { m_language_id, m_script_id, m_territory_id }; }

    [[nodiscard]] QString decimalPoint() const;
    [[nodiscard]] QString groupSeparator() const;
    [[nodiscard]] QString listSeparator() const;
    [[nodiscard]] QString percentSign() const;
    [[nodiscard]] QString zeroDigit() const;
    [[nodiscard]] char32_t zeroUcs() const;
    [[nodiscard]] QString positiveSign() const;
    [[nodiscard]] QString negativeSign() const;
    [[nodiscard]] QString exponentSeparator() const;
    [[nodiscard]] Q_CORE_EXPORT GroupSizes groupSizes() const;

    struct DataRange
    {
        using Index = quint32;
        Index offset; // Some zone data tables are big.
        Index size; // (for consistency and to avoid struct-padding)
        [[nodiscard]] QString getData(const char16_t *table) const
        {
            return size > 0
                ? QString::fromRawData(stringStart(table), stringSize())
                : QString();
        }
        [[nodiscard]] QStringView viewData(const char16_t *table) const
        {
            return { stringStart(table), stringSize() };
        }
        [[nodiscard]] QString getListEntry(const char16_t *table, qsizetype index) const
        {
            return listEntry(table, index).getData(table);
        }
        [[nodiscard]] QStringView viewListEntry(const char16_t *table, qsizetype index) const
        {
            return listEntry(table, index).viewData(table);
        }
        [[nodiscard]] char32_t ucsFirst(const char16_t *table) const
        {
            if (size && !QChar::isSurrogate(table[offset]))
                return table[offset];
            if (size > 1 && QChar::isHighSurrogate(table[offset]))
                return QChar::surrogateToUcs4(table[offset], table[offset + 1]);
            return 0;
        }
    private:
        [[nodiscard]] const QChar *stringStart(const char16_t *table) const
        {
            return reinterpret_cast<const QChar *>(table + offset);
        }
        [[nodiscard]] qsizetype stringSize() const
        {
            // On 32-bit platforms, this is a narrowing cast, but the size has
            // always come from an 8-bit or 16-bit table value so can't actually
            // have a problem with that.
            qsizetype result = static_cast<qsizetype>(size);
            Q_ASSERT(result >= 0);
            return result;
        }
        [[nodiscard]] DataRange listEntry(const char16_t *table, qsizetype index) const
        {
            const char16_t separator = ';';
            Index i = 0;
            while (index > 0 && i < size) {
                if (table[offset + i] == separator)
                    index--;
                i++;
            }
            Index end = i;
            while (end < size && table[offset + end] != separator)
                end++;
            return { offset + i, end - i };
        }
    };

#define ForEachQLocaleRange(X) \
    X(startListPattern) X(midListPattern) X(endListPattern) X(pairListPattern) X(listDelimit) \
    X(decimalSeparator) X(groupDelim) X(percent) X(zero) X(minus) X(plus) X(exponential) \
    X(quoteStart) X(quoteEnd) X(quoteStartAlternate) X(quoteEndAlternate) \
    X(longDateFormat) X(shortDateFormat) X(longTimeFormat) X(shortTimeFormat) \
    X(longDayNamesStandalone) X(longDayNames) \
    X(shortDayNamesStandalone) X(shortDayNames) \
    X(narrowDayNamesStandalone) X(narrowDayNames) \
    X(anteMeridiem) X(postMeridiem) \
    X(byteCount) X(byteAmountSI) X(byteAmountIEC) \
    X(currencySymbol) X(currencyDisplayName) \
    X(currencyFormat) X(currencyFormatNegative) \
    X(endonymLanguage) X(endonymTerritory)

#define rangeGetter(name) \
    [[nodiscard]] DataRange name() const { return { m_ ## name ## _idx, m_ ## name ## _size }; }
    ForEachQLocaleRange(rangeGetter)
#undef rangeGetter

public:
    quint16 m_language_id, m_script_id, m_territory_id;

    // Offsets, then sizes, for each range:
#define rangeIndex(name) quint16 m_ ## name ## _idx;
    ForEachQLocaleRange(rangeIndex)
#undef rangeIndex
#define Size(name) quint8 m_ ## name ## _size;
    ForEachQLocaleRange(Size)
#undef Size

#undef ForEachQLocaleRange

    // Strays:
    char m_currency_iso_code[3];
    quint8 m_currency_digits : 2;
    quint8 m_currency_rounding : 3; // (not yet used !)
    quint8 m_first_day_of_week : 3;
    quint8 m_weekend_start : 3;
    quint8 m_weekend_end : 3;
    quint8 m_grouping_first : 2; // Don't group until more significant group has this many digits.
    quint8 m_grouping_higher : 3; // Number of digits between grouping separators
    quint8 m_grouping_least : 3; // Number of digits after last grouping separator (before decimal).
};

Q_DECLARE_TYPEINFO(QLocaleData::GroupSizes, Q_PRIMITIVE_TYPE);

class QLocalePrivate
{
public:
    constexpr QLocalePrivate(const QLocaleData *data, qsizetype index,
                             QLocale::NumberOptions numberOptions = QLocale::DefaultNumberOptions,
                             int refs = 0)
        : m_data(data), ref Q_BASIC_ATOMIC_INITIALIZER(refs),
          m_index(index), m_numberOptions(numberOptions) {}

    [[nodiscard]] quint16 languageId() const { return m_data->m_language_id; }
    [[nodiscard]] quint16 territoryId() const { return m_data->m_territory_id; }

    [[nodiscard]] QByteArray bcp47Name(char separator = '-') const;

    [[nodiscard]] inline std::array<char, 4>
    languageCode(QLocale::LanguageCodeTypes codeTypes = QLocale::AnyLanguageCode) const
    {
        return languageToCode(QLocale::Language(m_data->m_language_id), codeTypes);
    }
    [[nodiscard]] inline QLatin1StringView scriptCode() const
    { return scriptToCode(QLocale::Script(m_data->m_script_id)); }
    [[nodiscard]] inline QLatin1StringView territoryCode() const
    { return territoryToCode(QLocale::Territory(m_data->m_territory_id)); }

    [[nodiscard]] static const QLocalePrivate *get(const QLocale &l) { return l.d; }
    [[nodiscard]] static std::array<char, 4>
    languageToCode(QLocale::Language language,
                   QLocale::LanguageCodeTypes codeTypes = QLocale::AnyLanguageCode);
    [[nodiscard]] static QLatin1StringView scriptToCode(QLocale::Script script);
    [[nodiscard]] static QLatin1StringView territoryToCode(QLocale::Territory territory);
    [[nodiscard]] static QLocale::Language
    codeToLanguage(QStringView code,
                   QLocale::LanguageCodeTypes codeTypes = QLocale::AnyLanguageCode) noexcept;
    [[nodiscard]] static QLocale::Script codeToScript(QStringView code) noexcept;
    [[nodiscard]] static QLocale::Territory codeToTerritory(QStringView code) noexcept;

    [[nodiscard]] QLocale::MeasurementSystem measurementSystem() const;

    [[nodiscard]] QString toUpper(const QString &str, bool *ok) const;
    [[nodiscard]] QString toLower(const QString &str, bool *ok) const;

    // System locale has an m_data all its own; all others have m_data = locale_data + m_index
    const QLocaleData *const m_data;
    QBasicAtomicInt ref;
    qsizetype m_index; // System locale needs this updated when m_data->id() changes.
    QLocale::NumberOptions m_numberOptions;

    static QBasicAtomicInt s_generation;
};

#ifndef QT_NO_SYSTEMLOCALE
qsizetype QSystemLocale::fallbackLocaleIndex() const { return fallbackLocale().d->m_index; }
#endif

template <>
inline QLocalePrivate *QSharedDataPointer<QLocalePrivate>::clone()
{
    // cannot use QLocalePrivate's copy constructor
    // since it is deleted in C++11
    return new QLocalePrivate(d->m_data, d->m_index, d->m_numberOptions);
}

// Also used to merely skip over an escape in a format string, advancint idx to
// point after it (so not [[nodiscard]]):
QString qt_readEscapedFormatString(QStringView format, qsizetype *idx);
[[nodiscard]] bool qt_splitLocaleName(QStringView name, QStringView *lang = nullptr,
                                      QStringView *script = nullptr,
                                      QStringView *cntry = nullptr) noexcept;
[[nodiscard]] qsizetype qt_repeatCount(QStringView s) noexcept;

[[nodiscard]] constexpr inline bool ascii_isspace(uchar c) noexcept
{
    constexpr auto matcher = QtPrivate::makeCharacterSetMatch<QtPrivate::ascii_space_chars>();
    return matcher.matches(c);
}

QT_END_NAMESPACE

// ### move to qnamespace.h
QT_DECL_METATYPE_EXTERN_TAGGED(QList<Qt::DayOfWeek>, QList_Qt__DayOfWeek, Q_CORE_EXPORT)
#ifndef QT_NO_SYSTEMLOCALE
QT_DECL_METATYPE_EXTERN_TAGGED(QSystemLocale::CurrencyToStringArgument,
                               QSystemLocale__CurrencyToStringArgument, Q_CORE_EXPORT)
#endif

#endif // QLOCALE_P_H
