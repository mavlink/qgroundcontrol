// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTDOCUMENTLAYOUT_P_H
#define QTEXTDOCUMENTLAYOUT_P_H

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
#include "QtGui/qabstracttextdocumentlayout.h"
#include "QtGui/qtextoption.h"
#include "QtGui/qtextobject.h"
#include "QtCore/qloggingcategory.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcLayout)

class QTextListFormat;
class QTextTableCell;
class QTextDocumentLayoutPrivate;

class Q_GUI_EXPORT QTextDocumentLayout : public QAbstractTextDocumentLayout
{
    Q_DECLARE_PRIVATE(QTextDocumentLayout)
    Q_OBJECT
    Q_PROPERTY(int cursorWidth READ cursorWidth WRITE setCursorWidth)
    Q_PROPERTY(qreal idealWidth READ idealWidth)
    Q_PROPERTY(bool contentHasAlignment READ contentHasAlignment)
public:
    explicit QTextDocumentLayout(QTextDocument *doc);

    // from the abstract layout
    void draw(QPainter *painter, const PaintContext &context) override;
    int hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const override;

    int pageCount() const override;
    QSizeF documentSize() const override;

    void setCursorWidth(int width);
    int cursorWidth() const;

    // internal, to support the ugly FixedColumnWidth wordwrap mode in QTextEdit
    void setFixedColumnWidth(int width);

    // internal for QTextEdit's NoWrap mode
    void setViewport(const QRectF &viewport);

    virtual QRectF frameBoundingRect(QTextFrame *frame) const override;
    virtual QRectF blockBoundingRect(const QTextBlock &block) const override;
    QRectF tableBoundingRect(QTextTable *table) const;
    QRectF tableCellBoundingRect(QTextTable *table, const QTextTableCell &cell) const;

    // ####
    int layoutStatus() const;
    int dynamicPageCount() const;
    QSizeF dynamicDocumentSize() const;
    void ensureLayouted(qreal);

    qreal idealWidth() const;

    bool contentHasAlignment() const;

protected:
    void documentChanged(int from, int oldLength, int length) override;
    void resizeInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format) override;
    void positionInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format) override;
    void drawInlineObject(QPainter *p, const QRectF &rect, QTextInlineObject item,
                          int posInDocument, const QTextFormat &format) override;
    virtual void timerEvent(QTimerEvent *e) override;
private:
    QRectF doLayout(int from, int oldLength, int length);
    void layoutFinished();
};

QT_END_NAMESPACE

#endif // QTEXTDOCUMENTLAYOUT_P_H
