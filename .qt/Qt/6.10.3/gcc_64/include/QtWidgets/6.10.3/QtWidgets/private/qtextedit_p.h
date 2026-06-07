// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEXTEDIT_P_H
#define QTEXTEDIT_P_H

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
#include "QtCore/qurl.h"
#include "qtextedit.h"

#include "private/qwidgettextcontrol_p.h"

#include <array>

QT_REQUIRE_CONFIG(textedit);

QT_BEGIN_NAMESPACE

class QMimeData;
class QTextEditPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QTextEdit)
public:
    QTextEditPrivate();
    ~QTextEditPrivate();

    void init(const QString &html = QString());
    void paint(QPainter *p, QPaintEvent *e);
    void repaintContents(const QRectF &contentsRect);

    inline QPoint mapToContents(const QPoint &point) const
    { return QPoint(point.x() + horizontalOffset(), point.y() + verticalOffset()); }

    void adjustScrollbars();
    void ensureVisible(const QRectF &rect);
    void relayoutDocument();

    void createAutoBulletList();
    void pageUpDown(QTextCursor::MoveOperation op, QTextCursor::MoveMode moveMode);

    inline int horizontalOffset() const
    { return q_func()->isRightToLeft() ? (hbar->maximum() - hbar->value()) : hbar->value(); }
    inline int verticalOffset() const
    { return vbar->value(); }

    inline void sendControlEvent(QEvent *e)
    { control->processEvent(e, QPointF(horizontalOffset(), verticalOffset()), viewport); }

    void cursorPositionChanged();
    void hoveredBlockWithMarkerChanged(const QTextBlock &block);

    void updateDefaultTextOption();

    // re-implemented by QTextBrowser, called by QTextDocument::loadResource
    virtual QUrl resolveUrl(const QUrl &url) const
    { return url; }

    QWidgetTextControl *control;

    QTextEdit::AutoFormatting autoFormatting;
    bool tabChangesFocus;

    QBasicTimer autoScrollTimer;
    QPoint autoScrollDragPos;

    QTextEdit::LineWrapMode lineWrap;
    int lineWrapColumnOrWidth;
    QTextOption::WrapMode wordWrap;

    uint ignoreAutomaticScrollbarAdjustment : 1;
    uint preferRichText : 1;
    uint showCursorOnInitialShow : 1;
    uint inDrag : 1;
    uint clickCausedFocus : 1;

    QString anchorToScrollToWhenVisible;

    QString placeholderText;

    Qt::CursorShape cursorToRestoreAfterHover = Qt::IBeamCursor;

    std::array<QMetaObject::Connection, 13> connections;

#ifdef QT_KEYPAD_NAVIGATION
    QBasicTimer deleteAllTimer;
#endif
};

QT_END_NAMESPACE

#endif // QTEXTEDIT_P_H
