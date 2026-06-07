// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QCHAR_H
#define QCHAR_H

#include <QtCore/qglobal.h>
#include <QtCore/qcompare.h>

#include <functional> // for std::hash

QT_BEGIN_NAMESPACE

class QString;

struct QLatin1Char
{
public:
    constexpr inline explicit QLatin1Char(char c) noexcept : ch(c) {}
    constexpr inline char toLatin1() const noexcept { return ch; }
    constexpr inline char16_t unicode() const noexcept { return char16_t(uchar(ch)); }

    friend constexpr bool
    comparesEqual(const QLatin1Char &lhs, const QLatin1Char &rhs) noexcept
    { return lhs.ch == rhs.ch; }
    friend constexpr Qt::strong_ordering
    compareThreeWay(const QLatin1Char &lhs, const QLatin1Char &rhs) noexcept
    { return Qt::compareThreeWay(uchar(lhs.ch), uchar(rhs.ch)); }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QLatin1Char)

    friend constexpr bool comparesEqual(const QLatin1Char &lhs, char rhs) noexcept
    { return lhs.toLatin1() == rhs; }
    friend constexpr Qt::strong_ordering
    compareThreeWay(const QLatin1Char &lhs, char rhs) noexcept
    { return Qt::compareThreeWay(uchar(lhs.toLatin1()), uchar(rhs)); }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QLatin1Char, char)

private:
    friend class QChar;
    // this is for QChar's ctor only:
    explicit constexpr operator char16_t() const noexcept { return unicode(); }

    char ch;
};

#define QT_CHAR_FASTCALL QT7_ONLY(Q_CORE_EXPORT) QT_FASTCALL
class QT6_ONLY(Q_CORE_EXPORT) QChar {
public:
    enum SpecialCharacter {
        Null = 0x0000,
        Tabulation = 0x0009,
        LineFeed = 0x000a,
        FormFeed = 0x000c,
        CarriageReturn = 0x000d,
        Space = 0x0020,
        Nbsp = 0x00a0,
        SoftHyphen = 0x00ad,
        ReplacementCharacter = 0xfffd,
        ObjectReplacementCharacter = 0xfffc,
        ByteOrderMark = 0xfeff,
        ByteOrderSwapped = 0xfffe,
        ParagraphSeparator = 0x2029,
        LineSeparator = 0x2028,
        VisualTabCharacter = 0x2192,
        LastValidCodePoint = 0x10ffff
    };

#ifdef QT_IMPLICIT_QCHAR_CONSTRUCTION
#error This macro has been removed in Qt 6.8.
#endif
private:
    using is_wide_wchar_t = std::bool_constant<(sizeof(wchar_t) > 2)>;
    template <typename Char>
    using is_implicit_conversion_char = std::disjunction<
            std::is_same<Char, ushort>,
            std::is_same<Char, short>,
            std::is_same<Char, SpecialCharacter>,
            std::is_same<Char, QLatin1Char>,
            std::conjunction<std::is_same<Char, wchar_t>, std::negation<is_wide_wchar_t>>,
            std::is_same<Char, char16_t>
        >;
    template <typename Char>
    using is_explicit_conversion_char = std::disjunction<
            std::is_same<Char, char32_t>, // implicit conversion to uint(?) before 6.8
            std::conjunction<std::is_same<Char, wchar_t>, is_wide_wchar_t>,
            std::is_same<Char, int>,
            std::is_same<Char, uint>
        >;
    template <typename Char>
    using is_conversion_char = std::disjunction<
            is_implicit_conversion_char<Char>,
            is_explicit_conversion_char<Char>
        >;
    template <typename Char>
    using is_implicit_ascii_warn_char = std::is_same<Char, char>;
    template <typename Char>
    using is_explicit_ascii_warn_char = std::is_same<Char, uchar>;
    template <typename Char>
    using if_compatible_char = std::enable_if_t<is_conversion_char<Char>::value, bool>;
    template <typename Char>
    using if_implicit_conversion_char = std::enable_if_t<is_implicit_conversion_char<Char>::value, bool>;
    template <typename Char>
    using if_explicit_conversion_char = std::enable_if_t<is_explicit_conversion_char<Char>::value, bool>;
    template <typename Char>
    using if_implicit_ascii_warn_char = std::enable_if_t<is_implicit_ascii_warn_char<Char>::value, bool>;
    template <typename Char>
    using if_explicit_ascii_warn_char = std::enable_if_t<is_explicit_ascii_warn_char<Char>::value, bool>;
    template <typename Char>
    [[maybe_unused]] static constexpr bool is_explicit_char_v = std::disjunction_v<
            is_explicit_conversion_char<Char>,
            is_explicit_ascii_warn_char<Char>
        >;
public:
    constexpr Q_IMPLICIT QChar() noexcept : ucs(0) {}
#if QT_CORE_REMOVED_SINCE(6, 9) || defined(Q_QDOC)
    constexpr Q_IMPLICIT QChar(ushort rc) noexcept : ucs(rc) {}
#endif
    constexpr explicit QChar(uchar c, uchar r) noexcept : ucs(char16_t((r << 8) | c)) {}
#if QT_CORE_REMOVED_SINCE(6, 9) || defined(Q_QDOC)
    constexpr Q_IMPLICIT QChar(short rc) noexcept : ucs(char16_t(rc)) {}
    constexpr explicit QChar(uint rc) noexcept : ucs((Q_ASSERT(rc <= 0xffff), char16_t(rc))) {}
    constexpr explicit QChar(int rc) noexcept : QChar(uint(rc)) {}
    constexpr Q_IMPLICIT QChar(SpecialCharacter s) noexcept : ucs(char16_t(s)) {}
    constexpr Q_IMPLICIT QChar(QLatin1Char ch) noexcept : ucs(ch.unicode()) {}
    constexpr Q_IMPLICIT QChar(char16_t ch) noexcept : ucs(ch) {}
#if defined(Q_OS_WIN) || defined(Q_QDOC)
    constexpr Q_IMPLICIT QChar(wchar_t ch) noexcept : ucs(char16_t(ch)) {}
#endif
#endif // QT_CORE_REMOVED_SINCE(6, 9)
    template <typename Char, if_implicit_conversion_char<Char> = true>
    constexpr Q_IMPLICIT QChar(const Char ch) noexcept : ucs(char16_t(ch)) {}
    template <typename Char, if_explicit_conversion_char<Char> = true>
    constexpr explicit QChar(const Char ch) noexcept
        : ucs((Q_ASSERT(char32_t(ch) <= 0xffff), char16_t(ch))) {}

#ifndef QT_NO_CAST_FROM_ASCII
    // Always implicit -- allow for 'x' => QChar conversions
#if QT_CORE_REMOVED_SINCE(6, 9) || defined(Q_QDOC)
    QT_ASCII_CAST_WARN constexpr Q_IMPLICIT QChar(char c) noexcept : ucs(uchar(c)) { }
#endif
    template <typename Char, if_implicit_ascii_warn_char<Char> = true>
    QT_ASCII_CAST_WARN constexpr Q_IMPLICIT QChar(const Char ch) noexcept : ucs(uchar(ch)) {}
#ifndef QT_RESTRICTED_CAST_FROM_ASCII
#if QT_CORE_REMOVED_SINCE(6, 9) || defined(Q_QDOC)
    QT_ASCII_CAST_WARN constexpr explicit QChar(uchar c) noexcept : ucs(c) { }
#endif
    template <typename Char, if_explicit_ascii_warn_char<Char> = true>
    QT_ASCII_CAST_WARN constexpr explicit QChar(const Char c) noexcept : ucs(c) { }
#endif
#endif

    static constexpr QChar fromUcs2(char16_t c) noexcept { return QChar{c}; }
    static constexpr inline auto fromUcs4(char32_t c) noexcept;

    // Unicode information

    enum Category
    {
        Mark_NonSpacing,          //   Mn
        Mark_SpacingCombining,    //   Mc
        Mark_Enclosing,           //   Me

        Number_DecimalDigit,      //   Nd
        Number_Letter,            //   Nl
        Number_Other,             //   No

        Separator_Space,          //   Zs
        Separator_Line,           //   Zl
        Separator_Paragraph,      //   Zp

        Other_Control,            //   Cc
        Other_Format,             //   Cf
        Other_Surrogate,          //   Cs
        Other_PrivateUse,         //   Co
        Other_NotAssigned,        //   Cn

        Letter_Uppercase,         //   Lu
        Letter_Lowercase,         //   Ll
        Letter_Titlecase,         //   Lt
        Letter_Modifier,          //   Lm
        Letter_Other,             //   Lo

        Punctuation_Connector,    //   Pc
        Punctuation_Dash,         //   Pd
        Punctuation_Open,         //   Ps
        Punctuation_Close,        //   Pe
        Punctuation_InitialQuote, //   Pi
        Punctuation_FinalQuote,   //   Pf
        Punctuation_Other,        //   Po

        Symbol_Math,              //   Sm
        Symbol_Currency,          //   Sc
        Symbol_Modifier,          //   Sk
        Symbol_Other              //   So
    };

    enum Script
    {
        Script_Unknown,
        Script_Inherited,
        Script_Common,

        Script_Latin,
        Script_Greek,
        Script_Cyrillic,
        Script_Armenian,
        Script_Hebrew,
        Script_Arabic,
        Script_Syriac,
        Script_Thaana,
        Script_Devanagari,
        Script_Bengali,
        Script_Gurmukhi,
        Script_Gujarati,
        Script_Oriya,
        Script_Tamil,
        Script_Telugu,
        Script_Kannada,
        Script_Malayalam,
        Script_Sinhala,
        Script_Thai,
        Script_Lao,
        Script_Tibetan,
        Script_Myanmar,
        Script_Georgian,
        Script_Hangul,
        Script_Ethiopic,
        Script_Cherokee,
        Script_CanadianAboriginal,
        Script_Ogham,
        Script_Runic,
        Script_Khmer,
        Script_Mongolian,
        Script_Hiragana,
        Script_Katakana,
        Script_Bopomofo,
        Script_Han,
        Script_Yi,
        Script_OldItalic,
        Script_Gothic,
        Script_Deseret,
        Script_Tagalog,
        Script_Hanunoo,
        Script_Buhid,
        Script_Tagbanwa,
        Script_Coptic,

        // Unicode 4.0 additions
        Script_Limbu,
        Script_TaiLe,
        Script_LinearB,
        Script_Ugaritic,
        Script_Shavian,
        Script_Osmanya,
        Script_Cypriot,
        Script_Braille,

        // Unicode 4.1 additions
        Script_Buginese,
        Script_NewTaiLue,
        Script_Glagolitic,
        Script_Tifinagh,
        Script_SylotiNagri,
        Script_OldPersian,
        Script_Kharoshthi,

        // Unicode 5.0 additions
        Script_Balinese,
        Script_Cuneiform,
        Script_Phoenician,
        Script_PhagsPa,
        Script_Nko,

        // Unicode 5.1 additions
        Script_Sundanese,
        Script_Lepcha,
        Script_OlChiki,
        Script_Vai,
        Script_Saurashtra,
        Script_KayahLi,
        Script_Rejang,
        Script_Lycian,
        Script_Carian,
        Script_Lydian,
        Script_Cham,

        // Unicode 5.2 additions
        Script_TaiTham,
        Script_TaiViet,
        Script_Avestan,
        Script_EgyptianHieroglyphs,
        Script_Samaritan,
        Script_Lisu,
        Script_Bamum,
        Script_Javanese,
        Script_MeeteiMayek,
        Script_ImperialAramaic,
        Script_OldSouthArabian,
        Script_InscriptionalParthian,
        Script_InscriptionalPahlavi,
        Script_OldTurkic,
        Script_Kaithi,

        // Unicode 6.0 additions
        Script_Batak,
        Script_Brahmi,
        Script_Mandaic,

        // Unicode 6.1 additions
        Script_Chakma,
        Script_MeroiticCursive,
        Script_MeroiticHieroglyphs,
        Script_Miao,
        Script_Sharada,
        Script_SoraSompeng,
        Script_Takri,

        // Unicode 7.0 additions
        Script_CaucasianAlbanian,
        Script_BassaVah,
        Script_Duployan,
        Script_Elbasan,
        Script_Grantha,
        Script_PahawhHmong,
        Script_Khojki,
        Script_LinearA,
        Script_Mahajani,
        Script_Manichaean,
        Script_MendeKikakui,
        Script_Modi,
        Script_Mro,
        Script_OldNorthArabian,
        Script_Nabataean,
        Script_Palmyrene,
        Script_PauCinHau,
        Script_OldPermic,
        Script_PsalterPahlavi,
        Script_Siddham,
        Script_Khudawadi,
        Script_Tirhuta,
        Script_WarangCiti,

        // Unicode 8.0 additions
        Script_Ahom,
        Script_AnatolianHieroglyphs,
        Script_Hatran,
        Script_Multani,
        Script_OldHungarian,
        Script_SignWriting,

        // Unicode 9.0 additions
        Script_Adlam,
        Script_Bhaiksuki,
        Script_Marchen,
        Script_Newa,
        Script_Osage,
        Script_Tangut,

        // Unicode 10.0 additions
        Script_MasaramGondi,
        Script_Nushu,
        Script_Soyombo,
        Script_ZanabazarSquare,

        // Unicode 12.1 additions
        Script_Dogra,
        Script_GunjalaGondi,
        Script_HanifiRohingya,
        Script_Makasar,
        Script_Medefaidrin,
        Script_OldSogdian,
        Script_Sogdian,
        Script_Elymaic,
        Script_Nandinagari,
        Script_NyiakengPuachueHmong,
        Script_Wancho,

        // Unicode 13.0 additions
        Script_Chorasmian,
        Script_DivesAkuru,
        Script_KhitanSmallScript,
        Script_Yezidi,

        // Unicode 14.0 additions
        Script_CyproMinoan,
        Script_OldUyghur,
        Script_Tangsa,
        Script_Toto,
        Script_Vithkuqi,

        // Unicode 15.0 additions
        Script_Kawi,
        Script_NagMundari,

        // Unicode 16.0 additions
        Script_Garay,
        Script_GurungKhema,
        Script_KiratRai,
        Script_OlOnal,
        Script_Sunuwar,
        Script_Todhri,
        Script_TuluTigalari,

        // Unicode 17.0 additions
        Script_Sidetic,
        Script_TaiYo,
        Script_TolongSiki,
        Script_BeriaErfe,

        ScriptCount
    };

    enum Direction
    {
        DirL, DirR, DirEN, DirES, DirET, DirAN, DirCS, DirB, DirS, DirWS, DirON,
        DirLRE, DirLRO, DirAL, DirRLE, DirRLO, DirPDF, DirNSM, DirBN,
        DirLRI, DirRLI, DirFSI, DirPDI
    };

    enum Decomposition
    {
        NoDecomposition,
        Canonical,
        Font,
        NoBreak,
        Initial,
        Medial,
        Final,
        Isolated,
        Circle,
        Super,
        Sub,
        Vertical,
        Wide,
        Narrow,
        Small,
        Square,
        Compat,
        Fraction
    };

    enum JoiningType {
        Joining_None,
        Joining_Causing,
        Joining_Dual,
        Joining_Right,
        Joining_Left,
        Joining_Transparent
    };

    enum CombiningClass
    {
        Combining_BelowLeftAttached       = 200,
        Combining_BelowAttached           = 202,
        Combining_BelowRightAttached      = 204,
        Combining_LeftAttached            = 208,
        Combining_RightAttached           = 210,
        Combining_AboveLeftAttached       = 212,
        Combining_AboveAttached           = 214,
        Combining_AboveRightAttached      = 216,

        Combining_BelowLeft               = 218,
        Combining_Below                   = 220,
        Combining_BelowRight              = 222,
        Combining_Left                    = 224,
        Combining_Right                   = 226,
        Combining_AboveLeft               = 228,
        Combining_Above                   = 230,
        Combining_AboveRight              = 232,

        Combining_DoubleBelow             = 233,
        Combining_DoubleAbove             = 234,
        Combining_IotaSubscript           = 240
    };

    enum UnicodeVersion {
        Unicode_Unassigned,
        Unicode_1_1,
        Unicode_2_0,
        Unicode_2_1_2,
        Unicode_3_0,
        Unicode_3_1,
        Unicode_3_2,
        Unicode_4_0,
        Unicode_4_1,
        Unicode_5_0,
        Unicode_5_1,
        Unicode_5_2,
        Unicode_6_0,
        Unicode_6_1,
        Unicode_6_2,
        Unicode_6_3,
        Unicode_7_0,
        Unicode_8_0,
        Unicode_9_0,
        Unicode_10_0,
        Unicode_11_0,
        Unicode_12_0,
        Unicode_12_1,
        Unicode_13_0,
        Unicode_14_0,
        Unicode_15_0,
        Unicode_15_1,
        Unicode_16_0,
        Unicode_17_0,
    };

    Category category() const noexcept { return QChar::category(char32_t(ucs)); }
    Direction direction() const noexcept { return QChar::direction(char32_t(ucs)); }
    JoiningType joiningType() const noexcept { return QChar::joiningType(char32_t(ucs)); }
    unsigned char combiningClass() const noexcept { return QChar::combiningClass(char32_t(ucs)); }

    QChar mirroredChar() const noexcept { return QChar(QChar::mirroredChar(char32_t(ucs))); }
    bool hasMirrored() const noexcept { return QChar::hasMirrored(char32_t(ucs)); }

    QString decomposition() const;
    Decomposition decompositionTag() const noexcept { return QChar::decompositionTag(char32_t(ucs)); }

    int digitValue() const noexcept { return QChar::digitValue(char32_t(ucs)); }
    QChar toLower() const noexcept { return QChar(QChar::toLower(char32_t(ucs))); }
    QChar toUpper() const noexcept { return QChar(QChar::toUpper(char32_t(ucs))); }
    QChar toTitleCase() const noexcept { return QChar(QChar::toTitleCase(char32_t(ucs))); }
    QChar toCaseFolded() const noexcept { return QChar(QChar::toCaseFolded(char32_t(ucs))); }

    Script script() const noexcept { return QChar::script(char32_t(ucs)); }

    UnicodeVersion unicodeVersion() const noexcept { return QChar::unicodeVersion(char32_t(ucs)); }

    constexpr inline char toLatin1() const noexcept { return ucs > 0xff ? '\0' : char(ucs); }
    constexpr inline char16_t unicode() const noexcept { return ucs; }
    constexpr inline char16_t &unicode() noexcept { return ucs; }

    static constexpr QChar fromLatin1(char c) noexcept { return QLatin1Char(c); }

    constexpr inline bool isNull() const noexcept { return ucs == 0; }

    bool isPrint() const noexcept { return QChar::isPrint(char32_t(ucs)); }
    constexpr bool isSpace() const noexcept { return QChar::isSpace(char32_t(ucs)); }
    bool isMark() const noexcept { return QChar::isMark(char32_t(ucs)); }
    bool isPunct() const noexcept { return QChar::isPunct(char32_t(ucs)); }
    bool isSymbol() const noexcept { return QChar::isSymbol(char32_t(ucs)); }
    constexpr bool isLetter() const noexcept { return QChar::isLetter(char32_t(ucs)); }
    constexpr bool isNumber() const noexcept { return QChar::isNumber(char32_t(ucs)); }
    constexpr bool isLetterOrNumber() const noexcept { return QChar::isLetterOrNumber(char32_t(ucs)); }
    constexpr bool isDigit() const noexcept { return QChar::isDigit(char32_t(ucs)); }
    constexpr bool isLower() const noexcept { return QChar::isLower(char32_t(ucs)); }
    constexpr bool isUpper() const noexcept { return QChar::isUpper(char32_t(ucs)); }
    constexpr bool isTitleCase() const noexcept { return QChar::isTitleCase(char32_t(ucs)); }

    constexpr bool isNonCharacter() const noexcept { return QChar::isNonCharacter(char32_t(ucs)); }
    constexpr bool isHighSurrogate() const noexcept { return QChar::isHighSurrogate(char32_t(ucs)); }
    constexpr bool isLowSurrogate() const noexcept { return QChar::isLowSurrogate(char32_t(ucs)); }
    constexpr bool isSurrogate() const noexcept { return QChar::isSurrogate(char32_t(ucs)); }

    constexpr inline uchar cell() const noexcept { return uchar(ucs & 0xff); }
    constexpr inline uchar row() const noexcept { return uchar((ucs>>8)&0xff); }
    constexpr inline void setCell(uchar acell) noexcept { ucs = char16_t((ucs & 0xff00) + acell); }
    constexpr inline void setRow(uchar arow) noexcept { ucs = char16_t((char16_t(arow)<<8) + (ucs&0xff)); }

    static constexpr inline bool isNonCharacter(char32_t ucs4) noexcept
    {
        return ucs4 >= 0xfdd0 && (ucs4 <= 0xfdef || (ucs4 & 0xfffe) == 0xfffe);
    }
    static constexpr inline bool isHighSurrogate(char32_t ucs4) noexcept
    {
        return (ucs4 & 0xfffffc00) == 0xd800; // 0xd800 + up to 1023 (0x3ff)
    }
    static constexpr inline bool isLowSurrogate(char32_t ucs4) noexcept
    {
        return (ucs4 & 0xfffffc00) == 0xdc00; // 0xdc00 + up to 1023 (0x3ff)
    }
    static constexpr inline bool isSurrogate(char32_t ucs4) noexcept
    {
        return (ucs4 - 0xd800u < 2048u);
    }
    static constexpr inline bool requiresSurrogates(char32_t ucs4) noexcept
    {
        return (ucs4 >= 0x10000);
    }
    static constexpr inline char32_t surrogateToUcs4(char16_t high, char16_t low) noexcept
    {
        // 0x010000 through 0x10ffff, provided params are actual high, low surrogates.
        // 0x010000 + ((high - 0xd800) << 10) + (low - 0xdc00), optimized:
        return (char32_t(high)<<10) + low - 0x35fdc00;
    }
    static constexpr inline char32_t surrogateToUcs4(QChar high, QChar low) noexcept
    {
        return surrogateToUcs4(high.ucs, low.ucs);
    }
    static constexpr inline char16_t highSurrogate(char32_t ucs4) noexcept
    {
        return char16_t((ucs4>>10) + 0xd7c0);
    }
    static constexpr inline char16_t lowSurrogate(char32_t ucs4) noexcept
    {
        return char16_t(ucs4%0x400 + 0xdc00);
    }

    static Category QT_CHAR_FASTCALL category(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static Direction QT_CHAR_FASTCALL direction(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static JoiningType QT_CHAR_FASTCALL joiningType(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static unsigned char QT_CHAR_FASTCALL combiningClass(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;

    static char32_t QT_CHAR_FASTCALL mirroredChar(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static bool QT_CHAR_FASTCALL hasMirrored(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;

    static QString QT_CHAR_FASTCALL decomposition(char32_t ucs4);
    static Decomposition QT_CHAR_FASTCALL decompositionTag(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;

    static int QT_CHAR_FASTCALL digitValue(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static char32_t QT_CHAR_FASTCALL toLower(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static char32_t QT_CHAR_FASTCALL toUpper(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static char32_t QT_CHAR_FASTCALL toTitleCase(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static char32_t QT_CHAR_FASTCALL toCaseFolded(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;

    static Script QT_CHAR_FASTCALL script(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;

    static UnicodeVersion QT_CHAR_FASTCALL unicodeVersion(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;

    static UnicodeVersion QT_CHAR_FASTCALL currentUnicodeVersion() noexcept Q_DECL_CONST_FUNCTION;

    static bool QT_CHAR_FASTCALL isPrint(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static constexpr inline bool isSpace(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION
    {
        // note that [0x09..0x0d] + 0x85 are exceptional Cc-s and must be handled explicitly
        return ucs4 == 0x20 || (ucs4 <= 0x0d && ucs4 >= 0x09)
                || (ucs4 > 127 && (ucs4 == 0x85 || ucs4 == 0xa0 || QChar::isSpace_helper(ucs4)));
    }
    static bool QT_CHAR_FASTCALL isMark(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static bool QT_CHAR_FASTCALL isPunct(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static bool QT_CHAR_FASTCALL isSymbol(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static constexpr inline bool isLetter(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION
    {
        return (ucs4 >= 'A' && ucs4 <= 'z' && (ucs4 >= 'a' || ucs4 <= 'Z'))
                || (ucs4 > 127 && QChar::isLetter_helper(ucs4));
    }
    static constexpr inline bool isNumber(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION
    { return (ucs4 <= '9' && ucs4 >= '0') || (ucs4 > 127 && QChar::isNumber_helper(ucs4)); }
    static constexpr inline bool isLetterOrNumber(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION
    {
        return (ucs4 >= 'A' && ucs4 <= 'z' && (ucs4 >= 'a' || ucs4 <= 'Z'))
                || (ucs4 >= '0' && ucs4 <= '9')
                || (ucs4 > 127 && QChar::isLetterOrNumber_helper(ucs4));
    }
    static constexpr inline bool isDigit(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION
    { return (ucs4 <= '9' && ucs4 >= '0') || (ucs4 > 127 && QChar::category(ucs4) == Number_DecimalDigit); }
    static constexpr inline bool isLower(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION
    { return (ucs4 <= 'z' && ucs4 >= 'a') || (ucs4 > 127 && QChar::category(ucs4) == Letter_Lowercase); }
    static constexpr inline bool isUpper(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION
    { return (ucs4 <= 'Z' && ucs4 >= 'A') || (ucs4 > 127 && QChar::category(ucs4) == Letter_Uppercase); }
    static constexpr inline bool isTitleCase(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION
    { return ucs4 > 127 && QChar::category(ucs4) == Letter_Titlecase; }

    friend constexpr bool comparesEqual(const QChar &lhs, const QChar &rhs) noexcept
    { return lhs.ucs == rhs.ucs; }
    friend constexpr Qt::strong_ordering
    compareThreeWay(const QChar &lhs, const QChar &rhs) noexcept
    { return Qt::compareThreeWay(lhs.ucs, rhs.ucs); }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QChar)

    friend constexpr bool comparesEqual(const QChar &lhs, std::nullptr_t) noexcept
    { return lhs.isNull(); }
    friend constexpr Qt::strong_ordering
    compareThreeWay(const QChar &lhs, std::nullptr_t) noexcept
    { return lhs.isNull() ? Qt::strong_ordering::equivalent : Qt::strong_ordering::greater; }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QChar, std::nullptr_t)

private:
    static bool QT_CHAR_FASTCALL isSpace_helper(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static bool QT_CHAR_FASTCALL isLetter_helper(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static bool QT_CHAR_FASTCALL isNumber_helper(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;
    static bool QT_CHAR_FASTCALL isLetterOrNumber_helper(char32_t ucs4) noexcept Q_DECL_CONST_FUNCTION;

    // defined in qstring.cpp, because we need to go via QUtf8StringView
    static bool QT_CHAR_FASTCALL
    equal_helper(QChar lhs, const char *rhs) noexcept Q_DECL_CONST_FUNCTION;
    static int QT_CHAR_FASTCALL
    compare_helper(QChar lhs, const char *rhs) noexcept Q_DECL_CONST_FUNCTION;

#if !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)
    Q_WEAK_OVERLOAD
    friend bool comparesEqual(const QChar &lhs, const char *rhs) noexcept
    { return equal_helper(lhs, rhs); }
    Q_WEAK_OVERLOAD
    friend Qt::strong_ordering compareThreeWay(const QChar &lhs, const char *rhs) noexcept
    {
        const int res = compare_helper(lhs, rhs);
        return Qt::compareThreeWay(res, 0);
    }
    Q_DECLARE_STRONGLY_ORDERED(QChar, const char *, Q_WEAK_OVERLOAD QT_ASCII_CAST_WARN)
#endif // !defined(QT_NO_CAST_FROM_ASCII) && !defined(QT_RESTRICTED_CAST_FROM_ASCII)

    char16_t ucs;
};
#undef QT_CHAR_FASTCALL

Q_DECLARE_TYPEINFO(QChar, Q_PRIMITIVE_TYPE);

namespace Qt {
inline namespace Literals {
inline namespace StringLiterals {

constexpr inline QLatin1Char operator""_L1(char ch) noexcept
{
    return QLatin1Char(ch);
}

} // StringLiterals
} // Literals
} // Qt

QT_END_NAMESPACE

namespace std {
template <>
struct hash<QT_PREPEND_NAMESPACE(QChar)>
{
    template <typename = void> // for transparent constexpr tracking
    constexpr size_t operator()(QT_PREPEND_NAMESPACE(QChar) c) const
        noexcept(noexcept(std::hash<char16_t>{}(u' ')))
    {
        return std::hash<char16_t>{}(c.unicode());
    }
};
} // namespace std

#endif // QCHAR_H

#include <QtCore/qstringview.h> // for QChar::fromUcs4() definition
