// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPLAINTEXTEDIT_P_H
#define QPLAINTEXTEDIT_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qabstractscrollarea_p.h"
#include "QtGui/qtextdocumentfragment.h"
#if QT_CONFIG(scrollbar)
#include "QtWidgets/qscrollbar.h"
#endif
#include "QtGui/qtextcursor.h"
#include "QtGui/qtextformat.h"
#if QT_CONFIG(menu)
#include "QtWidgets/qmenu.h"
#endif
#include "QtGui/qabstracttextdocumentlayout.h"
#include "QtCore/qbasictimer.h"
#include "qplaintextedit.h"

#include "private/qwidgettextcontrol_p.h"

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(textedit);

QT_BEGIN_NAMESPACE

class QMimeData;

class QPlainTextEdit;
class ExtraArea;

class QPlainTextEditControl : public QWidgetTextControl
{
    Q_OBJECT
public:
    QPlainTextEditControl(QPlainTextEdit *parent);


    QMimeData *createMimeDataFromSelection() const override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    int hitTest(const QPointF &point, Qt::HitTestAccuracy = Qt::FuzzyHit) const override;
    QRectF blockBoundingRect(const QTextBlock &block) const override;
    QString anchorAt(const QPointF &pos) const override;
    inline QRectF cursorRect(const QTextCursor &cursor) const {
        QRectF r = QWidgetTextControl::cursorRect(cursor);
        r.setLeft(qMax(r.left(), (qreal) 0.));
        return r;
    }
    inline QRectF cursorRect() { return cursorRect(textCursor()); }
    void ensureCursorVisible() override {
        textEdit->ensureCursorVisible();
        emit microFocusChanged();
    }


    QPlainTextEdit *textEdit;
    int topBlock;
    QTextBlock firstVisibleBlock() const;

    QVariant loadResource(int type, const QUrl &name) override {
        return textEdit->loadResource(type, name);
    }

};


class QPlainTextEditPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QPlainTextEdit)
public:
    QPlainTextEditPrivate();

    void init(const QString &txt = QString());
    void repaintContents(const QRectF &contentsRect);
    void updatePlaceholderVisibility();

    inline QPoint mapToContents(const QPoint &point) const
        { return QPoint(point.x() + horizontalOffset(), point.y() + verticalOffset()); }

    void adjustScrollbars();
    void verticalScrollbarActionTriggered(int action);
    void ensureViewportLayouted();
    void relayoutDocument();

    void pageUpDown(QTextCursor::MoveOperation op, QTextCursor::MoveMode moveMode, bool moveCursor = true);

    inline int horizontalOffset() const
        { return (q_func()->isRightToLeft() ? (hbar->maximum() - hbar->value()) : hbar->value()); }
    qreal verticalOffset(int topBlock, int topLine) const;
    qreal verticalOffset() const;

    inline void sendControlEvent(QEvent *e)
        { control->processEvent(e, QPointF(horizontalOffset(), verticalOffset()), viewport); }

    void updateDefaultTextOption();

    QBasicTimer autoScrollTimer;
#ifdef QT_KEYPAD_NAVIGATION
    QBasicTimer deleteAllTimer;
#endif
    QPoint autoScrollDragPos;
    QString placeholderText;

    QPlainTextEditControl *control = nullptr;
    qreal topLineFracture = 0; // for non-int sized fonts
    qreal pageUpDownLastCursorY = 0;
    QPlainTextEdit::LineWrapMode lineWrap = QPlainTextEdit::WidgetWidth;
    QTextOption::WrapMode wordWrap = QTextOption::WrapAtWordBoundaryOrAnywhere;
    int originalOffsetY = 0;
    int topLine = 0;

    uint tabChangesFocus : 1;
    uint showCursorOnInitialShow : 1;
    uint backgroundVisible : 1;
    uint centerOnScroll : 1;
    uint inDrag : 1;
    uint clickCausedFocus : 1;
    uint pageUpDownLastCursorYIsValid : 1;
    uint placeholderTextShown : 1;

    void setTopLine(int visualTopLine, int dx = 0);
    void setTopBlock(int newTopBlock, int newTopLine, int dx = 0);

    void ensureVisible(int position, bool center, bool forceCenter = false);
    void ensureCursorVisible(bool center = false);
    void updateViewport();

    QPointer<QPlainTextDocumentLayout> documentLayoutPtr;

    void append(const QString &text, Qt::TextFormat format = Qt::AutoText);

    void cursorPositionChanged();
    void modificationChanged(bool);
    inline bool placeHolderTextToBeShown() const
    {
        Q_Q(const QPlainTextEdit);
        return q->document()->isEmpty() && !q->placeholderText().isEmpty();
    }
};

QT_END_NAMESPACE

#endif // QPLAINTEXTEDIT_P_H
