// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXT_P_P_H
#define QQUICKTEXT_P_P_H

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

#include "qquicktext_p.h"
#include "qquickimplicitsizeitem_p_p.h"

#include <QtQml/qqml.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qtextlayout.h>
#include <private/qquickstyledtext_p.h>
#include <private/qlazilyallocated_p.h>
#include <private/qquicktextdocument_p.h>

QT_BEGIN_NAMESPACE

class QTextLayout;

class Q_QUICK_EXPORT QQuickTextPrivate : public QQuickImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickText)
public:
    QQuickTextPrivate();
    ~QQuickTextPrivate();
    void init();

    void updateBaseline(qreal baseline, qreal dy);
    void updateSize();
    void signalSizeChange(const QSizeF &previousSize);
    void updateLayout();
    bool determineHorizontalAlignment();
    bool setHAlign(QQuickText::HAlignment, bool forceAlign = false);
    void mirrorChange() override;
    bool isLineLaidOutConnected();
    void setLineGeometry(QTextLine &line, qreal lineWidth, qreal &height);

    int lineHeightOffset() const;
    QString elidedText(qreal lineWidth, const QTextLine &line, const QTextLine *nextLine = nullptr) const;
    void elideFormats(int start, int length, int offset, QVector<QTextLayout::FormatRange> *elidedFormats);
    void clearFormats();

    void processHoverEvent(QHoverEvent *event);
    bool transformChanged(QQuickItem *transformedItem) override;

    QRectF layedOutTextRect;
    QSizeF advance;

    struct ExtraData {
        ExtraData();

        qreal padding;
        qreal topPadding;
        qreal leftPadding;
        qreal rightPadding;
        qreal bottomPadding;
        bool explicitTopPadding : 1;
        bool explicitLeftPadding : 1;
        bool explicitRightPadding : 1;
        bool explicitBottomPadding : 1;
        qreal lineHeight;
        QTextDocument *doc;
        QQuickTextImageHandler *imageHandler = nullptr;
        QString activeLink;
        QString hoveredLink;
        int minimumPixelSize;
        int minimumPointSize;
        int maximumLineCount;
        int renderTypeQuality;
        bool lineHeightValid : 1;
        QQuickText::LineHeightMode lineHeightMode;
        QQuickText::FontSizeMode fontSizeMode;
        QList<QQuickStyledTextImgTag*> imgTags;
        QList<QQuickStyledTextImgTag*> visibleImgTags;
        QList<QQuickPixmap *> pixmapsInProgress;
        QUrl baseUrl;
    };
    QLazilyAllocated<ExtraData> extra;

    QString text;
    QFont font;
    QFont sourceFont;
    QFontInfo fontInfo;

    QTextLayout layout;
    QScopedPointer<QTextLayout> elideLayout;
    QScopedPointer<QQuickTextLine> textLine;

    qreal lineWidth;

    QRgb color;
    QRgb linkColor;
    QRgb styleColor;

    int lineCount;
    int multilengthEos;

    enum UpdateType {
        UpdateNone,
        UpdatePreprocess,
        UpdatePaintNode
    };

    QQuickText::TextElideMode elideMode;
    QQuickText::HAlignment hAlign;
    QQuickText::VAlignment vAlign;
    QQuickText::TextFormat format;
    QQuickText::WrapMode wrapMode;
    QQuickText::TextStyle style;
    QQuickText::RenderType renderType;
    UpdateType updateType;

    QString assignedFont;

    bool maximumLineCountValid:1;
    bool updateOnComponentComplete:1;
    bool richText:1;
    bool styledText:1;
    bool markdownText:1;
    bool widthExceeded:1;
    bool heightExceeded:1;
    bool internalWidthUpdate:1;
    bool requireImplicitSize:1;
    bool implicitWidthValid:1;
    bool implicitHeightValid:1;
    bool truncated:1;
    bool hAlignImplicit:1;
    bool rightToLeftText:1;
    bool layoutTextElided:1;
    bool textHasChanged:1;
    bool needToUpdateLayout:1;
    bool formatModifiesFontSize:1;
    bool polishSize:1; // Workaround for problem with polish called after updateSize (QTBUG-42636)
    bool updateSizeRecursionGuard:1;
    bool containsUnscalableGlyphs:1;

    static const QChar elideChar;
    static const int largeTextSizeThreshold;

    qreal getImplicitWidth() const override;
    qreal getImplicitHeight() const override;

    qreal availableWidth() const;
    qreal availableHeight() const;

    inline qreal padding() const { return extra.isAllocated() ? extra->padding : 0.0; }
    void setTopPadding(qreal value, bool reset = false);
    void setLeftPadding(qreal value, bool reset = false);
    void setRightPadding(qreal value, bool reset = false);
    void setBottomPadding(qreal value, bool reset = false);

    void ensureDoc();
    void updateDocumentText();

    QRectF setupTextLayout(qreal * const baseline);
    void setupCustomLineGeometry(QTextLine &line, qreal &height, int fullLayoutTextLength, int lineOffset = 0);
    bool isLinkActivatedConnected();
    bool isLinkHoveredConnected();
    QStringList links() const;

    struct LinkDesc {
        QString m_anchor;
        QString m_anchorTarget;
        int m_startIndex;
        int m_endIndex;
        QRect rect;
    };

    QVector<LinkDesc> getLinks() const;

    static QString anchorAt(const QTextLayout *layout, const QPointF &mousePos);
    QString anchorAt(const QPointF &pos) const;

    inline qreal lineHeight() const { return extra.isAllocated() ? extra->lineHeight : 1.0; }
    inline int maximumLineCount() const { return extra.isAllocated() ? extra->maximumLineCount : INT_MAX; }
    inline int renderTypeQuality() const { return extra.isAllocated() ? extra->renderTypeQuality : QQuickText::DefaultRenderTypeQuality; }
    inline QQuickText::LineHeightMode lineHeightMode() const { return extra.isAllocated() ? extra->lineHeightMode : QQuickText::ProportionalHeight; }
    inline QQuickText::FontSizeMode fontSizeMode() const { return extra.isAllocated() ? extra->fontSizeMode : QQuickText::FixedSize; }
    inline int minimumPixelSize() const { return extra.isAllocated() ? extra->minimumPixelSize : 12; }
    inline int minimumPointSize() const { return extra.isAllocated() ? extra->minimumPointSize : 12; }
    static inline QQuickTextPrivate *get(QQuickText *t) { return t->d_func(); }
};

QT_END_NAMESPACE

#endif // QQUICKTEXT_P_P_H
