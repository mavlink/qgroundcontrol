// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLABEL_P_H
#define QLABEL_P_H

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
#include "qlabel.h"

#include "private/qtextdocumentlayout_p.h"
#include "private/qwidgettextcontrol_p.h"
#include "qtextdocumentfragment.h"
#include "qframe_p.h"
#include "qtextdocument.h"
#if QT_CONFIG(movie)
#include "qmovie.h"
#endif
#include "qpicture.h"
#include "qpixmap.h"
#if QT_CONFIG(menu)
#include "qmenu.h"
#endif

#include <QtCore/qpointer.h>

#include <array>
#include <optional>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QLabelPrivate : public QFramePrivate
{
    Q_DECLARE_PUBLIC(QLabel)
public:
    QLabelPrivate();
    ~QLabelPrivate();

    void init();
    void clearContents();
    void updateLabel();
    QSize sizeForWidth(int w) const;

#if QT_CONFIG(movie)
    void movieUpdated(const QRect &rect);
    void movieResized(const QSize &size);
#endif
#ifndef QT_NO_SHORTCUT
    void updateShortcut();
    void buddyDeleted();
#endif
    inline bool needTextControl() const {
        Q_Q(const QLabel);
        return isTextLabel
               && (effectiveTextFormat != Qt::PlainText
                   || (textInteractionFlags & (Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard))
                   || q->focusPolicy() != Qt::NoFocus);
    }

    void ensureTextPopulated() const;
    void ensureTextLayouted() const;
    void ensureTextControl() const;
    void sendControlEvent(QEvent *e);

    void linkHovered(const QString &link);

    QRectF layoutRect() const;
    QRect documentRect() const;
    QPoint layoutPoint(const QPoint& p) const;
    Qt::LayoutDirection textDirection() const;
#ifndef QT_NO_CONTEXTMENU
    QMenu *createStandardContextMenu(const QPoint &pos);
#endif

    mutable QSize sh;
    mutable QSize msh;
    QString text;
    std::optional<QIcon> icon;
    QSize pixmapSize;
#ifndef QT_NO_PICTURE
    std::optional<QPicture> picture;
#endif
#if QT_CONFIG(movie)
    QPointer<QMovie> movie;
    std::array<QMetaObject::Connection, 2> movieConnections;
#endif
    mutable QWidgetTextControl *control = nullptr;
    mutable QTextCursor shortcutCursor;
#ifndef QT_NO_CURSOR
    QCursor cursor;
#endif
#ifndef QT_NO_SHORTCUT
    QPointer<QWidget> buddy;
    int shortcutId = 0;
#endif
    Qt::TextFormat textformat = Qt::AutoText;
    Qt::TextFormat effectiveTextFormat = Qt::PlainText;
    Qt::TextInteractionFlags textInteractionFlags = Qt::LinksAccessibleByMouse;
    mutable QSizePolicy sizePolicy;
    int margin = 0;
    int align = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextExpandTabs;
    int indent = -1;
    mutable uint valid_hints : 1;
    uint scaledcontents : 1;
    mutable uint textLayoutDirty : 1;
    mutable uint textDirty : 1;
    mutable uint isTextLabel : 1;
    mutable uint hasShortcut : 1;
#ifndef QT_NO_CURSOR
    uint validCursor : 1;
    uint onAnchor : 1;
#endif
    uint openExternalLinks : 1;
    // <-- space for more bit field values here
    QTextDocument::ResourceProvider resourceProvider = nullptr;

    friend class QMessageBoxPrivate;
};

QT_END_NAMESPACE

#endif // QLABEL_P_H
