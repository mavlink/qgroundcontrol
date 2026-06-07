// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCSSPARSER_P_H
#define QCSSPARSER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtCore/QSize>
#include <QtCore/QMultiHash>
#include <QtGui/QFont>
#include <QtGui/QPalette>
#include <QtCore/QSharedData>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

#ifndef QT_NO_CSSPARSER

// VxWorks defines NONE as (-1) "for times when NULL won't do"
#if defined(Q_OS_VXWORKS) && defined(NONE)
#  undef NONE
#endif
#if defined(Q_OS_INTEGRITY)
#  undef Value
#endif
// Hurd has #define TILDE 0x00080000 from <sys/ioctl.h>
#if defined(TILDE)
#  undef TILDE
#endif

#define QT_CSS_DECLARE_TYPEINFO(Class, Type) \
    } /* namespace QCss */ \
    Q_DECLARE_TYPEINFO(QCss:: Class, Type); \
    namespace QCss {

QT_BEGIN_NAMESPACE

namespace QCss
{

enum Property {
    UnknownProperty,
    BackgroundColor,
    Color,
    Float,
    Font,
    FontFamily,
    FontSize,
    FontStyle,
    FontWeight,
    Margin,
    MarginBottom,
    MarginLeft,
    MarginRight,
    MarginTop,
    QtBlockIndent,
    QtListIndent,
    QtParagraphType,
    QtTableType,
    QtUserState,
    TextDecoration,
    TextIndent,
    TextUnderlineStyle,
    VerticalAlignment,
    Whitespace,
    QtSelectionForeground,
    QtSelectionBackground,
    Border,
    BorderLeft,
    BorderRight,
    BorderTop,
    BorderBottom,
    BorderCollapse,
    Padding,
    PaddingLeft,
    PaddingRight,
    PaddingTop,
    PaddingBottom,
    PageBreakBefore,
    PageBreakAfter,
    QtAlternateBackground,
    BorderLeftStyle,
    BorderRightStyle,
    BorderTopStyle,
    BorderBottomStyle,
    BorderStyles,
    BorderLeftColor,
    BorderRightColor,
    BorderTopColor,
    BorderBottomColor,
    BorderColor,
    BorderLeftWidth,
    BorderRightWidth,
    BorderTopWidth,
    BorderBottomWidth,
    BorderWidth,
    BorderTopLeftRadius,
    BorderTopRightRadius,
    BorderBottomLeftRadius,
    BorderBottomRightRadius,
    BorderRadius,
    Background,
    BackgroundOrigin,
    BackgroundClip,
    BackgroundRepeat,
    BackgroundPosition,
    BackgroundAttachment,
    BackgroundImage,
    BorderImage,
    QtSpacing,
    Width,
    Height,
    MinimumWidth,
    MinimumHeight,
    MaximumWidth,
    MaximumHeight,
    QtImage,
    Left,
    Right,
    Top,
    Bottom,
    QtOrigin,
    QtPosition,
    Position,
    QtStyleFeatures,
    QtBackgroundRole,
    ListStyleType,
    ListStyle,
    QtImageAlignment,
    TextAlignment,
    Outline,
    OutlineOffset,
    OutlineWidth,
    OutlineColor,
    OutlineStyle,
    OutlineRadius,
    OutlineTopLeftRadius,
    OutlineTopRightRadius,
    OutlineBottomLeftRadius,
    OutlineBottomRightRadius,
    FontVariant,
    TextTransform,
    QtListNumberPrefix,
    QtListNumberSuffix,
    LineHeight,
    QtLineHeightType,
    FontKerning,
    QtForegroundTextureCacheKey,
    QtIcon,
    LetterSpacing,
    WordSpacing,
    TextDecorationColor,
    QtPlaceHolderTextColor,
    QtAccent,
    QtStrokeWidth,
    QtStrokeColor,
    QtStrokeLineCap,
    QtStrokeLineJoin,
    QtStrokeMiterLimit,
    QtStrokeDashArray,
    QtStrokeDashOffset,
    QtForeground,
    NumProperties
};

enum KnownValue {
    UnknownValue,
    Value_Normal,
    Value_Pre,
    Value_NoWrap,
    Value_PreLine,
    Value_PreWrap,
    Value_Small,
    Value_Medium,
    Value_Large,
    Value_XLarge,
    Value_XXLarge,
    Value_Italic,
    Value_Oblique,
    Value_Bold,
    Value_Underline,
    Value_Overline,
    Value_LineThrough,
    Value_Sub,
    Value_Super,
    Value_Left,
    Value_Right,
    Value_Top,
    Value_Bottom,
    Value_Center,
    Value_Native,
    Value_Solid,
    Value_Dotted,
    Value_Dashed,
    Value_DotDash,
    Value_DotDotDash,
    Value_Double,
    Value_Groove,
    Value_Ridge,
    Value_Inset,
    Value_Outset,
    Value_Wave,
    Value_Middle,
    Value_Auto,
    Value_Always,
    Value_None,
    Value_Transparent,
    Value_Disc,
    Value_Circle,
    Value_Square,
    Value_Decimal,
    Value_LowerAlpha,
    Value_UpperAlpha,
    Value_LowerRoman,
    Value_UpperRoman,
    Value_SmallCaps,
    Value_Uppercase,
    Value_Lowercase,
    Value_SquareCap,
    Value_FlatCap,
    Value_RoundCap,
    Value_MiterJoin,
    Value_BevelJoin,
    Value_RoundJoin,
    Value_SvgMiterJoin,

    /* keep these in same order as QPalette::ColorRole */
    Value_FirstColorRole,
    Value_WindowText = Value_FirstColorRole,
    Value_Button,
    Value_Light,
    Value_Midlight,
    Value_Dark,
    Value_Mid,
    Value_Text,
    Value_BrightText,
    Value_ButtonText,
    Value_Base,
    Value_Window,
    Value_Shadow,
    Value_Highlight,
    Value_HighlightedText,
    Value_Link,
    Value_LinkVisited,
    Value_AlternateBase,
    Value_NoRole,
    Value_ToolTipBase,
    Value_ToolTipText,
    Value_PlaceholderText,
    Value_Accent,
    Value_LastColorRole = Value_Accent,

    Value_Disabled,
    Value_Active,
    Value_Selected,
    Value_On,
    Value_Off,

    NumKnownValues
};

enum BorderStyle {
    BorderStyle_Unknown,
    BorderStyle_None,
    BorderStyle_Dotted,
    BorderStyle_Dashed,
    BorderStyle_Solid,
    BorderStyle_Double,
    BorderStyle_DotDash,
    BorderStyle_DotDotDash,
    BorderStyle_Groove,
    BorderStyle_Ridge,
    BorderStyle_Inset,
    BorderStyle_Outset,
    BorderStyle_Native,
    NumKnownBorderStyles
};

enum Edge {
    TopEdge,
    RightEdge,
    BottomEdge,
    LeftEdge,
    NumEdges
};

enum Corner {
    TopLeftCorner,
    TopRightCorner,
    BottomLeftCorner,
    BottomRightCorner
};

enum TileMode {
    TileMode_Unknown,
    TileMode_Round,
    TileMode_Stretch,
    TileMode_Repeat,
    NumKnownTileModes
};

enum Repeat {
    Repeat_Unknown,
    Repeat_None,
    Repeat_X,
    Repeat_Y,
    Repeat_XY,
    NumKnownRepeats
};

enum Origin {
    Origin_Unknown,
    Origin_Padding,
    Origin_Border,
    Origin_Content,
    Origin_Margin,
    NumKnownOrigins
};

enum PositionMode {
    PositionMode_Unknown,
    PositionMode_Static,
    PositionMode_Relative,
    PositionMode_Absolute,
    PositionMode_Fixed,
    NumKnownPositionModes
};

enum Attachment {
    Attachment_Unknown,
    Attachment_Fixed,
    Attachment_Scroll,
    NumKnownAttachments
};

enum StyleFeature {
    StyleFeature_None = 0,
    StyleFeature_BackgroundColor = 1,
    StyleFeature_BackgroundGradient = 2,
    NumKnownStyleFeatures = 4
};

struct Value
{
    enum Type {
        Unknown,
        Number,
        Percentage,
        Length,
        String,
        Identifier,
        KnownIdentifier,
        Uri,
        Color,
        Function,
        TermOperatorSlash,
        TermOperatorComma
    };
    inline Value() : type(Unknown) { }
    Type type;
    QVariant variant;

    Q_GUI_EXPORT QString toString() const;
};
QT_CSS_DECLARE_TYPEINFO(Value, Q_RELOCATABLE_TYPE)

struct ColorData {
    ColorData() : role(QPalette::NoRole), type(Invalid) {}
    ColorData(const QColor &col) : color(col), role(QPalette::NoRole), type(Color) {}
    ColorData(QPalette::ColorRole r) : role(r), type(Role) {}
    QColor color;
    QPalette::ColorRole role;
    enum { Invalid, Color, Role} type;
};
QT_CSS_DECLARE_TYPEINFO(ColorData, Q_RELOCATABLE_TYPE)

struct BrushData {
    BrushData() : role(QPalette::NoRole), type(Invalid) {}
    BrushData(const QBrush &br) : brush(br), role(QPalette::NoRole), type(Brush) {}
    BrushData(QPalette::ColorRole r) : role(r), type(Role) {}
    QBrush brush;
    QPalette::ColorRole role;
    enum { Invalid, Brush, Role, DependsOnThePalette } type;
};
QT_CSS_DECLARE_TYPEINFO(BrushData, Q_RELOCATABLE_TYPE)

struct BackgroundData {
    BrushData brush;
    QString image;
    Repeat repeat;
    Qt::Alignment alignment;
};
QT_CSS_DECLARE_TYPEINFO(BackgroundData, Q_RELOCATABLE_TYPE)

struct LengthData {
    qreal number;
    enum { None, Px, Ex, Em, Percent } unit;
};
QT_CSS_DECLARE_TYPEINFO(LengthData, Q_PRIMITIVE_TYPE)

struct BorderData {
    LengthData width;
    BorderStyle style;
    BrushData color;
};
QT_CSS_DECLARE_TYPEINFO(BorderData, Q_RELOCATABLE_TYPE)

// 1. StyleRule - x:hover, y:clicked > z:checked { prop1: value1; prop2: value2; }
// 2. QList<Selector> - x:hover, y:clicked z:checked
// 3. QList<BasicSelector> - y:clicked z:checked
// 4. QList<Declaration> - { prop1: value1; prop2: value2; }
// 5. Declaration - prop1: value1;

struct Q_GUI_EXPORT Declaration
{
    struct DeclarationData : public QSharedData
    {
        inline DeclarationData() : propertyId(UnknownProperty), important(false), inheritable(false) {}
        QString property;
        Property propertyId;
        QList<Value> values;
        QVariant parsed;
        bool important:1;
        bool inheritable:1;
    };
    QExplicitlySharedDataPointer<DeclarationData> d;
    inline Declaration() : d(new DeclarationData()) {}
    inline bool isEmpty() const { return d->property.isEmpty() && d->propertyId == UnknownProperty; }

    // helper functions
    QColor colorValue(const QPalette & = QPalette()) const;
    void colorValues(QColor *c, const QPalette & = QPalette()) const;
    QBrush brushValue(const QPalette & = QPalette()) const;
    void brushValues(QBrush *c, const QPalette & = QPalette()) const;

    BorderStyle styleValue() const;
    void styleValues(BorderStyle *s) const;

    Origin originValue() const;
    Repeat repeatValue() const;
    Qt::Alignment alignmentValue() const;
    PositionMode positionValue() const;
    Attachment attachmentValue() const;
    int  styleFeaturesValue() const;

    bool intValue(int *i, const char *unit = nullptr) const;
    bool realValue(qreal *r, const char *unit = nullptr) const;

    QSize sizeValue() const;
    QRect rectValue() const;
    QString uriValue() const;
    QIcon iconValue() const;

    void borderImageValue(QString *image, int *cuts, TileMode *h, TileMode *v) const;
    bool borderCollapseValue() const;

    QList<qreal> dashArray() const;
};
QT_CSS_DECLARE_TYPEINFO(Declaration, Q_RELOCATABLE_TYPE)

const quint64 PseudoClass_Unknown          = Q_UINT64_C(0x0000000000000000);
const quint64 PseudoClass_Enabled          = Q_UINT64_C(0x0000000000000001);
const quint64 PseudoClass_Disabled         = Q_UINT64_C(0x0000000000000002);
const quint64 PseudoClass_Pressed          = Q_UINT64_C(0x0000000000000004);
const quint64 PseudoClass_Focus            = Q_UINT64_C(0x0000000000000008);
const quint64 PseudoClass_Hover            = Q_UINT64_C(0x0000000000000010);
const quint64 PseudoClass_Checked          = Q_UINT64_C(0x0000000000000020);
const quint64 PseudoClass_Unchecked        = Q_UINT64_C(0x0000000000000040);
const quint64 PseudoClass_Indeterminate    = Q_UINT64_C(0x0000000000000080);
const quint64 PseudoClass_Unspecified      = Q_UINT64_C(0x0000000000000100);
const quint64 PseudoClass_Selected         = Q_UINT64_C(0x0000000000000200);
const quint64 PseudoClass_Horizontal       = Q_UINT64_C(0x0000000000000400);
const quint64 PseudoClass_Vertical         = Q_UINT64_C(0x0000000000000800);
const quint64 PseudoClass_Window           = Q_UINT64_C(0x0000000000001000);
const quint64 PseudoClass_Children         = Q_UINT64_C(0x0000000000002000);
const quint64 PseudoClass_Sibling          = Q_UINT64_C(0x0000000000004000);
const quint64 PseudoClass_Default          = Q_UINT64_C(0x0000000000008000);
const quint64 PseudoClass_First            = Q_UINT64_C(0x0000000000010000);
const quint64 PseudoClass_Last             = Q_UINT64_C(0x0000000000020000);
const quint64 PseudoClass_Middle           = Q_UINT64_C(0x0000000000040000);
const quint64 PseudoClass_OnlyOne          = Q_UINT64_C(0x0000000000080000);
const quint64 PseudoClass_PreviousSelected = Q_UINT64_C(0x0000000000100000);
const quint64 PseudoClass_NextSelected     = Q_UINT64_C(0x0000000000200000);
const quint64 PseudoClass_Flat             = Q_UINT64_C(0x0000000000400000);
const quint64 PseudoClass_Left             = Q_UINT64_C(0x0000000000800000);
const quint64 PseudoClass_Right            = Q_UINT64_C(0x0000000001000000);
const quint64 PseudoClass_Top              = Q_UINT64_C(0x0000000002000000);
const quint64 PseudoClass_Bottom           = Q_UINT64_C(0x0000000004000000);
const quint64 PseudoClass_Exclusive        = Q_UINT64_C(0x0000000008000000);
const quint64 PseudoClass_NonExclusive     = Q_UINT64_C(0x0000000010000000);
const quint64 PseudoClass_Frameless        = Q_UINT64_C(0x0000000020000000);
const quint64 PseudoClass_ReadOnly         = Q_UINT64_C(0x0000000040000000);
const quint64 PseudoClass_Active           = Q_UINT64_C(0x0000000080000000);
const quint64 PseudoClass_Closable         = Q_UINT64_C(0x0000000100000000);
const quint64 PseudoClass_Movable          = Q_UINT64_C(0x0000000200000000);
const quint64 PseudoClass_Floatable        = Q_UINT64_C(0x0000000400000000);
const quint64 PseudoClass_Minimized        = Q_UINT64_C(0x0000000800000000);
const quint64 PseudoClass_Maximized        = Q_UINT64_C(0x0000001000000000);
const quint64 PseudoClass_On               = Q_UINT64_C(0x0000002000000000);
const quint64 PseudoClass_Off              = Q_UINT64_C(0x0000004000000000);
const quint64 PseudoClass_Editable         = Q_UINT64_C(0x0000008000000000);
const quint64 PseudoClass_Item             = Q_UINT64_C(0x0000010000000000);
const quint64 PseudoClass_Closed           = Q_UINT64_C(0x0000020000000000);
const quint64 PseudoClass_Open             = Q_UINT64_C(0x0000040000000000);
const quint64 PseudoClass_EditFocus        = Q_UINT64_C(0x0000080000000000);
const quint64 PseudoClass_Alternate        = Q_UINT64_C(0x0000100000000000);
// The Any specifier is never generated, but can be used as a wildcard in searches.
const quint64 PseudoClass_Any              = Q_UINT64_C(0x0000ffffffffffff);
const int NumPseudos = 45;

struct Pseudo
{
    Pseudo() : type(0), negated(false) { }
    quint64 type;
    QString name;
    QString function;
    bool negated;
};
QT_CSS_DECLARE_TYPEINFO(Pseudo, Q_RELOCATABLE_TYPE)

struct AttributeSelector
{
    enum ValueMatchType {
        NoMatch,
        MatchEqual,
        MatchIncludes,
        MatchDashMatch,
        MatchBeginsWith,
        MatchEndsWith,
        MatchContains
    };

    QString name;
    QString value;
    ValueMatchType valueMatchCriterium = NoMatch;
};
QT_CSS_DECLARE_TYPEINFO(AttributeSelector, Q_RELOCATABLE_TYPE)

struct BasicSelector
{
    inline BasicSelector() : relationToNext(NoRelation) {}

    enum Relation {
        NoRelation,
        MatchNextSelectorIfAncestor,
        MatchNextSelectorIfParent,
        MatchNextSelectorIfDirectAdjecent,
        MatchNextSelectorIfIndirectAdjecent,
    };

    QString elementName;

    QStringList ids;
    QList<Pseudo> pseudos;
    QList<AttributeSelector> attributeSelectors;

    Relation relationToNext;
};
QT_CSS_DECLARE_TYPEINFO(BasicSelector, Q_RELOCATABLE_TYPE)

struct Q_GUI_EXPORT Selector
{
    QList<BasicSelector> basicSelectors;
    int specificity() const;
    quint64 pseudoClass(quint64 *negated = nullptr) const;
    QString pseudoElement() const;
};
QT_CSS_DECLARE_TYPEINFO(Selector, Q_RELOCATABLE_TYPE)

struct StyleRule
{
    StyleRule() : order(0) { }
    QList<Selector> selectors;
    QList<Declaration> declarations;
    int order;
};
QT_CSS_DECLARE_TYPEINFO(StyleRule, Q_RELOCATABLE_TYPE)

struct MediaRule
{
    QStringList media;
    QList<StyleRule> styleRules;
};
QT_CSS_DECLARE_TYPEINFO(MediaRule, Q_RELOCATABLE_TYPE)

struct PageRule
{
    QString selector;
    QList<Declaration> declarations;
};
QT_CSS_DECLARE_TYPEINFO(PageRule, Q_RELOCATABLE_TYPE)

struct AnimationRule
{
    struct AnimationRuleSet
    {
        float keyFrame;
        QList<Declaration> declarations;
    };

    QString animName;
    QList<AnimationRuleSet> ruleSets;
};
QT_CSS_DECLARE_TYPEINFO(AnimationRule, Q_RELOCATABLE_TYPE)

struct ImportRule
{
    QString href;
    QStringList media;
};
QT_CSS_DECLARE_TYPEINFO(ImportRule, Q_RELOCATABLE_TYPE)

enum StyleSheetOrigin {
    StyleSheetOrigin_Unspecified,
    StyleSheetOrigin_UserAgent,
    StyleSheetOrigin_User,
    StyleSheetOrigin_Author,
    StyleSheetOrigin_Inline
};

struct StyleSheet
{
    StyleSheet() : origin(StyleSheetOrigin_Unspecified), depth(0) { }
    QList<StyleRule> styleRules; // only contains rules that are not indexed
    QList<MediaRule> mediaRules;
    QList<PageRule> pageRules;
    QList<AnimationRule> animationRules;
    QList<ImportRule> importRules;
    StyleSheetOrigin origin;
    int depth; // applicable only for inline style sheets
    QMultiHash<QString, StyleRule> nameIndex;
    QMultiHash<QString, StyleRule> idIndex;

    Q_GUI_EXPORT void buildIndexes(Qt::CaseSensitivity nameCaseSensitivity = Qt::CaseSensitive);
};
QT_CSS_DECLARE_TYPEINFO(StyleSheet, Q_RELOCATABLE_TYPE)


class Q_GUI_EXPORT StyleSelector
{
public:
    StyleSelector() : nameCaseSensitivity(Qt::CaseSensitive)  {}
    virtual ~StyleSelector();

    union NodePtr {
        void *ptr;
        int id;
    };

    QList<StyleRule> styleRulesForNode(NodePtr node);
    QList<Declaration> declarationsForNode(NodePtr node, const char *extraPseudo = nullptr);

    virtual bool nodeNameEquals(NodePtr node, const QString& nodeName) const;
    virtual QString attributeValue(NodePtr node, const QCss::AttributeSelector &aSelector) const = 0;
    virtual bool hasAttributes(NodePtr node) const = 0;
    virtual QStringList nodeIds(NodePtr node) const;
    virtual QStringList nodeNames(NodePtr node) const = 0;
    virtual bool isNullNode(NodePtr node) const = 0;
    virtual NodePtr parentNode(NodePtr node) const = 0;
    virtual NodePtr previousSiblingNode(NodePtr node) const = 0;
    virtual NodePtr duplicateNode(NodePtr node) const = 0;
    virtual void freeNode(NodePtr node) const = 0;

    QList<StyleSheet> styleSheets;
    QString medium;
    Qt::CaseSensitivity nameCaseSensitivity;
private:
    void matchRule(NodePtr node, const StyleRule &rules, StyleSheetOrigin origin,
                    int depth, std::multimap<uint64_t, StyleRule> *weightedRules) const;
    bool selectorMatches(const Selector &rule, NodePtr node) const;
    bool basicSelectorMatches(const BasicSelector &rule, NodePtr node) const;
};

enum TokenType {
    NONE,

    S,

    CDO,
    CDC,
    INCLUDES,
    DASHMATCH,
    BEGINSWITH,
    ENDSWITH,
    CONTAINS,

    LBRACE,
    PLUS,
    GREATER,
    COMMA,
    TILDE,

    STRING,
    INVALID,

    IDENT,

    HASH,

    ATKEYWORD_SYM,

    EXCLAMATION_SYM,

    LENGTH,

    PERCENTAGE,
    NUMBER,

    FUNCTION,

    COLON,
    SEMICOLON,
    RBRACE,
    SLASH,
    MINUS,
    DOT,
    STAR,
    LBRACKET,
    RBRACKET,
    EQUAL,
    LPAREN,
    RPAREN,
    OR
};

struct Symbol
{
    inline Symbol() : token(NONE), start(0), len(-1) {}
    TokenType token;
    QString text;
    int start, len;
    Q_GUI_EXPORT QString lexem() const;
};
QT_CSS_DECLARE_TYPEINFO(Symbol, Q_RELOCATABLE_TYPE)

class Q_GUI_EXPORT Scanner
{
public:
    static QString preprocess(const QString &input, bool *hasEscapeSequences = nullptr);
    static void scan(const QString &preprocessedInput, QList<Symbol> *symbols);
};

class Q_GUI_EXPORT Parser
{
public:
    Parser();
    explicit Parser(const QString &css, bool file = false);

    void init(const QString &css, bool file = false);
    bool parse(StyleSheet *styleSheet, Qt::CaseSensitivity nameCaseSensitivity = Qt::CaseSensitive);
    Symbol errorSymbol();

    bool parseImport(ImportRule *importRule);
    bool parseMedia(MediaRule *mediaRule);
    bool parseMedium(QStringList *media);
    bool parsePage(PageRule *pageRule);
    bool parsePseudoPage(QString *selector);
    bool parseNextOperator(Value *value);
    bool parseAnimation(AnimationRule *animationRule);
    bool parseCombinator(BasicSelector::Relation *relation);
    bool parseProperty(Declaration *decl);
    bool parseRuleset(StyleRule *styleRule);
    bool parseSelector(Selector *sel);
    bool parseSimpleSelector(BasicSelector *basicSel);
    bool parseClass(QString *name);
    bool parseElementName(QString *name);
    bool parseAttrib(AttributeSelector *attr);
    bool parsePseudo(Pseudo *pseudo);
    bool parseNextDeclaration(Declaration *declaration);
    bool parsePrio(Declaration *declaration);
    bool parseExpr(QList<Value> *values);
    bool parseTerm(Value *value);
    bool parseFunction(QString *name, QString *args);
    bool parseHexColor(QColor *col);
    bool testAndParseUri(QString *uri);

    inline bool testRuleset() { return testSelector(); }
    inline bool testSelector() { return testSimpleSelector(); }
    inline bool parseNextSelector(Selector *sel) { if (!testSelector()) return recordError(); return parseSelector(sel); }
    bool testSimpleSelector();
    inline bool parseNextSimpleSelector(BasicSelector *basicSel) { if (!testSimpleSelector()) return recordError(); return parseSimpleSelector(basicSel); }
    inline bool testElementName() { return test(IDENT) || test(STAR); }
    inline bool testClass() { return test(DOT); }
    inline bool testAttrib() { return test(LBRACKET); }
    inline bool testPseudo() { return test(COLON); }
    inline bool testMedium() { return test(IDENT); }
    inline bool parseNextMedium(QStringList *media) { if (!testMedium()) return recordError(); return parseMedium(media); }
    inline bool testPseudoPage() { return test(COLON); }
    inline bool testImport() { return testTokenAndEndsWith(ATKEYWORD_SYM, QLatin1StringView("import")); }
    inline bool testMedia() { return testTokenAndEndsWith(ATKEYWORD_SYM, QLatin1StringView("media")); }
    inline bool testPage() { return testTokenAndEndsWith(ATKEYWORD_SYM, QLatin1StringView("page")); }
    inline bool testAnimation() { return testTokenAndEndsWith(ATKEYWORD_SYM, QLatin1StringView("keyframes")); }
    inline bool testCombinator() { return test(PLUS) || test(GREATER) || test(TILDE) || test(S); }
    inline bool testProperty() { return test(IDENT); }
    bool testTerm();
    inline bool testExpr() { return testTerm(); }
    inline bool parseNextExpr(QList<Value> *values)
    {
        if (!testExpr())
            return recordError();
        return parseExpr(values);
    }
    bool testPrio();
    inline bool testHexColor() { return test(HASH); }
    inline bool testFunction() { return test(FUNCTION); }
    inline bool parseNextFunction(QString *name, QString *args) { if (!testFunction()) return recordError(); return parseFunction(name, args); }

    inline bool lookupElementName() const { return lookup() == IDENT || lookup() == STAR; }

    inline void skipSpace() { while (test(S)) {}; }

    inline bool hasNext() const { return index < symbols.size(); }
    inline TokenType next() { return symbols.at(index++).token; }
    bool next(TokenType t);
    bool test(TokenType t);
    inline void prev() { index--; }
    inline const Symbol &symbol() const { return symbols.at(index - 1); }
    inline QString lexem() const { return symbol().lexem(); }
    QString unquotedLexem() const;
    QString lexemUntil(TokenType t);
    bool until(TokenType target, TokenType target2 = NONE);
    inline TokenType lookup() const {
        return (index - 1) < symbols.size() ? symbols.at(index - 1).token : NONE;
    }

    bool testTokenAndEndsWith(TokenType t, QLatin1StringView str);

    inline bool recordError() { errorIndex = index; return false; }

    QList<Symbol> symbols;
    int index;
    int errorIndex;
    bool hasEscapeSequences;
    QString sourcePath;
};

struct Q_GUI_EXPORT ValueExtractor
{
    ValueExtractor(const QList<Declaration> &declarations, const QPalette & = QPalette());

    bool extractFont(QFont *font, int *fontSizeAdjustment);
    bool extractBackground(QBrush *, QString *, Repeat *, Qt::Alignment *, QCss::Origin *, QCss::Attachment *,
                           QCss::Origin *) const;
    bool extractGeometry(int *w, int *h, int *minw, int *minh, int *maxw, int *maxh);
    bool extractPosition(int *l, int *t, int *r, int *b, QCss::Origin *, Qt::Alignment *,
                         QCss::PositionMode *, Qt::Alignment *);
    bool extractBox(int *margins, int *paddings, int *spacing = nullptr);
    bool extractBorder(int *borders, QBrush *colors, BorderStyle *Styles, QSize *radii);
    bool extractOutline(int *borders, QBrush *colors, BorderStyle *Styles, QSize *radii, int *offsets);
    bool extractPalette(QBrush *foreground, QBrush *selectedForeground, QBrush *selectedBackground,
                        QBrush *alternateBackground, QBrush *placeHolderTextForeground,
                        QBrush *accent) const;
    int  extractStyleFeatures() const;
    bool extractImage(QIcon *icon, Qt::Alignment *a, QSize *size) const;
    bool extractIcon(QIcon *icon, QSize *size) const;

    void lengthValues(const Declaration &decl, int *m);
    QTextLength textLength(const Declaration &decl);

private:
    void extractFont();
    void borderValue(const Declaration &decl, int *width, QCss::BorderStyle *style, QBrush *color);
    LengthData lengthValue(const Value& v);
    int lengthValue(const Declaration &decl);
    QSize sizeValue(const Declaration &decl);
    void sizeValues(const Declaration &decl, QSize *radii);

    QList<Declaration> declarations;
    QFont f;
    int adjustment;
    int fontExtracted;
    QPalette pal;
};

} // namespace QCss

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN_TAGGED(QCss::BackgroundData, QCss__BackgroundData, Q_GUI_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QCss::LengthData, QCss__LengthData, Q_GUI_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QCss::BorderData, QCss__BorderData, Q_GUI_EXPORT)

#undef QT_CSS_DECLARE_TYPEINFO

#endif // QT_NO_CSSPARSER

#endif
