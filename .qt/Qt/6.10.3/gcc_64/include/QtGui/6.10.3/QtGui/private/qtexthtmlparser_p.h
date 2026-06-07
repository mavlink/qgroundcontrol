// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTHTMLPARSER_P_H
#define QTEXTHTMLPARSER_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qbrush.h"
#include "QtGui/qcolor.h"
#include "QtGui/qfont.h"
#include "QtGui/qtextdocument.h"
#include "QtGui/qtextcursor.h"

#include "QtCore/qlist.h"

#include "private/qtextformat_p.h"
#include "private/qtextdocument_p.h"
#if QT_CONFIG(cssparser)
#include "private/qcssparser_p.h"
#endif

#ifndef QT_NO_TEXTHTMLPARSER

QT_BEGIN_NAMESPACE

enum QTextHTMLElements {
    Html_unknown = -1,
    Html_qt = 0,
    Html_body,

    Html_a,
    Html_em,
    Html_i,
    Html_big,
    Html_small,
    Html_strong,
    Html_b,
    Html_cite,
    Html_address,
    Html_var,
    Html_dfn,

    Html_h1,
    Html_h2,
    Html_h3,
    Html_h4,
    Html_h5,
    Html_h6,
    Html_p,
    Html_center,

    Html_font,

    Html_ul,
    Html_ol,
    Html_li,

    Html_code,
    Html_tt,
    Html_kbd,
    Html_samp,

    Html_img,
    Html_br,
    Html_hr,

    Html_sub,
    Html_sup,

    Html_pre,
    Html_blockquote,
    Html_head,
    Html_div,
    Html_span,
    Html_dl,
    Html_dt,
    Html_dd,
    Html_u,
    Html_s,
    Html_nobr,

    // tables
    Html_table,
    Html_tr,
    Html_td,
    Html_th,
    Html_thead,
    Html_tbody,
    Html_tfoot,
    Html_caption,

    // misc...
    Html_html,
    Html_style,
    Html_title,
    Html_meta,
    Html_link,
    Html_script,

    Html_NumElements
};

struct QTextHtmlElement
{
    const char name[11];
    QTextHTMLElements id;
    enum DisplayMode { DisplayBlock, DisplayInline, DisplayTable, DisplayNone } displayMode;
};

class QTextHtmlParser;

struct QTextHtmlParserNode {
    enum WhiteSpaceMode {
        WhiteSpaceNormal,
        WhiteSpacePre,
        WhiteSpaceNoWrap,
        WhiteSpacePreWrap,
        WhiteSpacePreLine,
        WhiteSpaceModeUndefined = -1
    };

    QTextHtmlParserNode();
    QString tag;
    QString text;
    QStringList attributes;
    int parent;
    QList<int> children;
    QTextHTMLElements id;
    QTextCharFormat charFormat;
    QTextBlockFormat blockFormat;
    uint cssFloat : 2;
    uint hasOwnListStyle : 1;
    uint hasOwnLineHeightType : 1;
    uint hasLineHeightMultiplier : 1;
    uint hasCssListIndent : 1;
    uint isEmptyParagraph : 1;
    uint isTextFrame : 1;
    uint isRootFrame : 1;
    uint displayMode : 3; // QTextHtmlElement::DisplayMode
    uint hasHref : 1;
    QTextListFormat::Style listStyle;
    int listStart = 1;
    QString textListNumberPrefix;
    QString textListNumberSuffix;
    QString imageName;
    QString imageAlt;
    qreal imageWidth;
    qreal imageHeight;
    QTextLength width;
    QTextLength height;
    qreal tableBorder;
    int tableCellRowSpan;
    int tableCellColSpan;
    qreal tableCellSpacing;
    qreal tableCellPadding;
    qreal tableCellBorder[4];
    QBrush tableCellBorderBrush[4];
    QTextFrameFormat::BorderStyle tableCellBorderStyle[4];
    QBrush borderBrush;
    QTextFrameFormat::BorderStyle borderStyle;
    bool borderCollapse;
    int userState;

    int cssListIndent;

    WhiteSpaceMode wsm;

    inline bool isListStart() const
    { return id == Html_ol || id == Html_ul; }
    inline bool isTableCell() const
    { return id == Html_td || id == Html_th; }
    inline bool isBlock() const
    { return displayMode == QTextHtmlElement::DisplayBlock; }

    inline bool isNotSelfNesting() const
    { return id == Html_p || id == Html_li; }

    inline bool allowedInContext(int parentId) const
    {
        switch (id) {
            case Html_dd:
            case Html_dt: return (parentId == Html_dl);
            case Html_tr: return (parentId == Html_table
                                  || parentId == Html_thead
                                  || parentId == Html_tbody
                                  || parentId == Html_tfoot
                                 );
            case Html_th:
            case Html_td: return (parentId == Html_tr);
            case Html_thead:
            case Html_tbody:
            case Html_tfoot: return (parentId == Html_table);
            case Html_caption: return (parentId == Html_table);
            case Html_body: return parentId != Html_head;
            default: break;
        }
        return true;
    }

    inline bool mayNotHaveChildren() const
    { return id == Html_img || id == Html_hr || id == Html_br || id == Html_meta; }

    void initializeProperties(const QTextHtmlParserNode *parent, const QTextHtmlParser *parser);

    inline int uncollapsedMargin(int mar) const { return margin[mar]; }

    bool isNestedList(const QTextHtmlParser *parser) const;

    void parseStyleAttribute(const QString &value, const QTextDocument *resourceProvider);

#if QT_CONFIG(cssparser)
    void applyCssDeclarations(const QList<QCss::Declaration> &declarations,
                              const QTextDocument *resourceProvider);

    void setListStyle(const QList<QCss::Value> &cssValues);
#    endif

    void applyForegroundImage(qint64 cacheKey, const QTextDocument *resourceProvider);
    void applyBackgroundImage(const QString &url, const QTextDocument *resourceProvider);

    bool hasOnlyWhitespace() const;

    int margin[4];
    int padding[4];

    friend class QTextHtmlParser;
};
Q_DECLARE_TYPEINFO(QTextHtmlParserNode, Q_RELOCATABLE_TYPE);


class QTextHtmlParser
{
public:
    enum Margin {
        MarginTop,
        MarginRight,
        MarginBottom,
        MarginLeft
    };
    ~QTextHtmlParser()
    {
        qDeleteAll(nodes);
    }

    inline const QTextHtmlParserNode &at(int i) const { return *nodes.at(i); }
    inline QTextHtmlParserNode &operator[](int i) { return *nodes[i]; }
    inline int count() const { return nodes.size(); }
    inline int last() const { return nodes.size()-1; }
    int depth(int i) const;
    int topMargin(int i) const;
    int bottomMargin(int i) const;
    inline int leftMargin(int i) const { return margin(i, MarginLeft); }
    inline int rightMargin(int i) const { return margin(i, MarginRight); }

    inline int topPadding(int i) const { return at(i).padding[MarginTop]; }
    inline int bottomPadding(int i) const { return at(i).padding[MarginBottom]; }
    inline int leftPadding(int i) const { return at(i).padding[MarginLeft]; }
    inline int rightPadding(int i) const { return at(i).padding[MarginRight]; }

    inline qreal tableCellBorder(int i, int edge) const { return at(i).tableCellBorder[edge]; }
    inline QTextFrameFormat::BorderStyle tableCellBorderStyle(int i, int edge) const { return at(i).tableCellBorderStyle[edge]; }
    inline QBrush tableCellBorderBrush(int i, int edge) const { return at(i).tableCellBorderBrush[edge]; }

    void dumpHtml();

    void parse(const QString &text, const QTextDocument *resourceProvider);

    static int lookupElement(QStringView element);

    Q_GUI_EXPORT static QString parseEntity(QStringView entity);

protected:
    QTextHtmlParserNode *newNode(int parent);
    QList<QTextHtmlParserNode *> nodes;
    QString txt;
    int pos, len;

    bool textEditMode;

    void parse();
    void parseTag();
    void parseCloseTag();
    void parseExclamationTag();
    QString parseEntity();
    QString parseWord();
    QTextHtmlParserNode *resolveParent();
    void resolveNode();
    QStringList parseAttributes();
    void applyAttributes(const QStringList &attributes);
    void eatSpace();
    inline bool hasPrefix(QChar c, int lookahead = 0) const
    {
        return pos + lookahead < len && txt.at(pos + lookahead) == c;
    }
    int margin(int i, int mar) const;

    bool nodeIsChildOf(int i, QTextHTMLElements id) const;


#if QT_CONFIG(cssparser)
    QList<QCss::Declaration> declarationsForNode(int node) const;
    void resolveStyleSheetImports(const QCss::StyleSheet &sheet);
    void importStyleSheet(const QString &href);

    struct ExternalStyleSheet
    {
        inline ExternalStyleSheet() {}
        inline ExternalStyleSheet(const QString &_url, const QCss::StyleSheet &_sheet)
            : url(_url), sheet(_sheet) {}
        QString url;
        QCss::StyleSheet sheet;
    };
    friend class QTypeInfo<ExternalStyleSheet>;
    QList<ExternalStyleSheet> externalStyleSheets;
    QList<QCss::StyleSheet> inlineStyleSheets;
#    endif

    const QTextDocument *resourceProvider;
};
#if QT_CONFIG(cssparser)
Q_DECLARE_TYPEINFO(QTextHtmlParser::ExternalStyleSheet, Q_RELOCATABLE_TYPE);
#endif

QT_END_NAMESPACE

#endif // QT_NO_TEXTHTMLPARSER

#endif // QTEXTHTMLPARSER_P_H
