// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGTEXTNODE_H
#define QSGTEXTNODE_H

#include <QtGui/qtextlayout.h>
#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexture.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGTextNode : public QSGTransformNode
{
public:
    ~QSGTextNode() override;

    // Should match the TextStyle in qquicktext_p.h
    enum TextStyle : quint8
    {
        Normal,
        Outline,
        Raised,
        Sunken
    };

    // Should match the RenderType in qquicktext_p.h
    enum RenderType: quint8
    {
        QtRendering,
        NativeRendering,
        CurveRendering
    };

    virtual void setColor(QColor color) = 0;
    virtual QColor color() const = 0;

    virtual void setTextStyle(TextStyle textStyle) = 0;
    virtual TextStyle textStyle() = 0;

    virtual void setStyleColor(QColor styleColor) = 0;
    virtual QColor styleColor() const = 0;

    virtual void setLinkColor(QColor linkColor) = 0;
    virtual QColor linkColor() const = 0;

    virtual void setSelectionColor(QColor selectionColor) = 0;
    virtual QColor selectionColor() const = 0;

    virtual void setSelectionTextColor(QColor selectionTextColor) = 0;
    virtual QColor selectionTextColor() const = 0;

    virtual void setRenderType(RenderType renderType) = 0;
    virtual RenderType renderType() const = 0;

    virtual void setRenderTypeQuality(int renderTypeQuality) = 0;
    virtual int renderTypeQuality() const = 0;

    virtual void setFiltering(QSGTexture::Filtering) = 0;
    virtual QSGTexture::Filtering filtering() const = 0;

    virtual void clear() = 0;

    virtual void setViewport(const QRectF &viewport) = 0;
    virtual QRectF viewport() const = 0;

    void addTextLayout(QPointF position,
                       QTextLayout *layout,
                       int selectionStart = -1,
                       int selectionCount = -1,
                       int lineStart = 0,
                       int lineCount = -1)
    {
        doAddTextLayout(position, layout, selectionStart, selectionCount, lineStart, lineCount);
    }

    void addTextDocument(QPointF position,
                         QTextDocument *document,
                         int selectionStart = -1,
                         int selectionCount = -1)
    {
        doAddTextDocument(position, document, selectionStart, selectionCount);
    }

private:
    virtual void doAddTextLayout(QPointF position,
                                 QTextLayout *layout,
                                 int selectionStart,
                                 int selectionCount,
                                 int lineStart,
                                 int lineCount) = 0;
    virtual void doAddTextDocument(QPointF position,
                                   QTextDocument *document,
                                   int selectionStart,
                                   int selectionCount) = 0;

};

QT_END_NAMESPACE

#endif // QSGTEXTNODE_H
