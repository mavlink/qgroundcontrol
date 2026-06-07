// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSGINTERNALTEXTNODE_P_H
#define QSGINTERNALTEXTNODE_P_H

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

#include "qsgtextnode.h"
#include "qquicktext_p.h"
#include <qglyphrun.h>

#include <QtGui/qcolor.h>
#include <QtGui/qtextlayout.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QSGGlyphNode;
class QTextBlock;
class QColor;
class QTextDocument;
class QSGContext;
class QRawFont;
class QSGInternalRectangleNode;
class QSGClipNode;
class QSGTexture;
class QSGRenderContext;

class QQuickTextNodeEngine;

class Q_QUICK_EXPORT QSGInternalTextNode : public QSGTextNode
{
public:
    QSGInternalTextNode(QSGRenderContext *renderContext);
    ~QSGInternalTextNode();

    static bool isComplexRichText(QTextDocument *);

    void setColor(QColor color) override
    {
        m_color = color;
    }

    QColor color() const override
    {
        return m_color;
    }

    void setTextStyle(TextStyle textStyle) override
    {
        m_textStyle = textStyle;
    }

    TextStyle textStyle() override
    {
        return m_textStyle;
    }

    void setStyleColor(QColor styleColor) override
    {
        m_styleColor = styleColor;
    }

    QColor styleColor() const override
    {
        return m_styleColor;
    }

    void setLinkColor(QColor linkColor) override
    {
        m_linkColor = linkColor;
    }

    QColor linkColor() const override
    {
        return m_linkColor;
    }

    void setSelectionColor(QColor selectionColor) override
    {
        m_selectionColor = selectionColor;
    }

    QColor selectionColor() const override
    {
        return m_selectionColor;
    }

    void setSelectionTextColor(QColor selectionTextColor) override
    {
        m_selectionTextColor = selectionTextColor;
    }

    QColor selectionTextColor() const override
    {
        return m_selectionTextColor;
    }

    void setRenderTypeQuality(int renderTypeQuality) override
    {
        m_renderTypeQuality = renderTypeQuality;
    }
    int renderTypeQuality() const override
    {
        return m_renderTypeQuality;
    }

    void setRenderType(RenderType renderType) override
    {
        m_renderType = renderType;
    }

    RenderType renderType() const override
    {
        return m_renderType;
    }

    bool containsUnscalableGlyphs() const
    {
        return m_containsUnscalableGlyphs;
    }

    void setFiltering(QSGTexture::Filtering filtering) override
    {
        m_filtering = filtering;
    }

    QSGTexture::Filtering filtering() const override
    {
        return m_filtering;
    }

    void setViewport(const QRectF &viewport) override
    {
        m_viewport = viewport;
    }

    QRectF viewport() const override
    {
        return m_viewport;
    }

    void setDevicePixelRatio(qreal dpr)
    {
        m_devicePixelRatio = dpr;
    }

    void setCursor(const QRectF &rect, const QColor &color);
    void clearCursor();

    void addRectangleNode(const QRectF &rect, const QColor &color);
    virtual void addDecorationNode(const QRectF &rect, const QColor &color);
    void addImage(const QRectF &rect, const QImage &image);
    void clear() override;
    QSGGlyphNode *addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                            QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                            QSGNode *parentNode = 0);

    QSGInternalRectangleNode *cursorNode() const { return m_cursorNode; }
    std::pair<int, int> renderedLineRange() const { return { m_firstLineInViewport, m_firstLinePastViewport }; }

protected:
    void doAddTextLayout(QPointF position,
                         QTextLayout *textLayout,
                         int selectionStart,
                         int selectionEnd,
                         int lineStart,
                         int lineCount) override;

    void doAddTextDocument(QPointF position,
                           QTextDocument *textDocument,
                           int selectionStart,
                           int selectionEnd) override;

private:
    QSGInternalRectangleNode *m_cursorNode = nullptr;
    QList<QSGTexture *> m_textures;
    QSGRenderContext *m_renderContext = nullptr;
    RenderType m_renderType = QtRendering;
    TextStyle m_textStyle = Normal;
    QRectF m_viewport;
    QColor m_color = QColor(0, 0, 0);
    QColor m_styleColor = QColor(0, 0, 0);
    QColor m_linkColor = QColor(0, 0, 255);
    QColor m_selectionColor = QColor(0, 0, 128);
    QColor m_selectionTextColor = QColor(255, 255, 255);
    QSGTexture::Filtering m_filtering = QSGTexture::Nearest;
    int m_renderTypeQuality = -1;
    int m_firstLineInViewport = -1;
    int m_firstLinePastViewport = -1;
    bool m_containsUnscalableGlyphs = false;
    qreal m_devicePixelRatio = 1.0;

    friend class QQuickTextEdit;
    friend class QQuickTextEditPrivate;
};

QT_END_NAMESPACE

#endif // QSGINTERNALTEXTNODE_P_H
