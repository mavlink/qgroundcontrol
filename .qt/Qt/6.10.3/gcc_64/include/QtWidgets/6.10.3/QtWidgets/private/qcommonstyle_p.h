// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCOMMONSTYLE_P_H
#define QCOMMONSTYLE_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include "qhash.h"
#include "qcommonstyle.h"
#include "qstyle_p.h"
#if QT_CONFIG(animation)
#include "qstyleanimation_p.h"
#endif
#include "qstyleoption.h"

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

class QTextOption;

// Private class
class Q_WIDGETS_EXPORT QCommonStylePrivate : public QStylePrivate
{
    Q_DECLARE_PUBLIC(QCommonStyle)
public:
    inline QCommonStylePrivate() :
#if QT_CONFIG(itemviews)
    cachedOption(nullptr),
#endif
    animationFps(30)
    { }

    ~QCommonStylePrivate()
    {
#if QT_CONFIG(animation)
        qDeleteAll(animations);
#endif
#if QT_CONFIG(itemviews)
        delete cachedOption;
#endif
    }

    QString calculateElidedText(const QString &text, const QTextOption &textOption,
                                const QFont &font, const QRect &textRect, const Qt::Alignment valign,
                                Qt::TextElideMode textElideMode, int flags,
                                bool lastVisibleLineShouldBeElided, QPointF *paintStartPosition) const;
#if QT_CONFIG(itemviews)
    void viewItemDrawText(QPainter *p, const QStyleOptionViewItem *option, const QRect &rect) const;
    void viewItemLayout(const QStyleOptionViewItem *opt,  QRect *checkRect,
                        QRect *pixmapRect, QRect *textRect, bool sizehint) const;
    QSize viewItemSize(const QStyleOptionViewItem *option, int role) const;

    mutable QRect decorationRect, displayRect, checkRect;
    mutable QStyleOptionViewItem *cachedOption;
    bool isViewItemCached(const QStyleOptionViewItem &option) const {
        return cachedOption && (option.widget == cachedOption->widget
               && option.index == cachedOption->index
               && option.state == cachedOption->state
               && option.rect == cachedOption->rect
               && option.text == cachedOption->text
               && option.direction == cachedOption->direction
               && option.displayAlignment == cachedOption->displayAlignment
               && option.decorationAlignment == cachedOption->decorationAlignment
               && option.decorationPosition == cachedOption->decorationPosition
               && option.decorationSize == cachedOption->decorationSize
               && option.features == cachedOption->features
               && option.icon.isNull() == cachedOption->icon.isNull()
               && option.font == cachedOption->font
               && option.viewItemPosition == cachedOption->viewItemPosition
               && option.showDecorationSelected == cachedOption->showDecorationSelected);
    }
#endif
#if QT_CONFIG(toolbutton)
    QString toolButtonElideText(const QStyleOptionToolButton *toolbutton,
                                const QRect &textRect, int flags) const;
#endif

    mutable QIcon tabBarcloseButtonIcon;
#if QT_CONFIG(tabbar)
    virtual void tabLayout(const QStyleOptionTab *opt, const QWidget *widget, QRect *textRect, QRect *pixmapRect) const;
#endif

    int animationFps;
#if QT_CONFIG(animation)
    QStyleAnimation* animation(const QObject *target) const;
    void startAnimation(QStyleAnimation *animation) const;
    void stopAnimation(const QObject *target) const;
    void removeAnimation(const QObject *target) const;
#endif

    QIcon iconFromWindowsTheme(QCommonStyle::StandardPixmap standardIcon,
                               const QStyleOption *option,
                               const QWidget *widget) const;
    QIcon iconFromMacTheme(QCommonStyle::StandardPixmap standardIcon,
                           const QStyleOption *option,
                           const QWidget *widget) const;
    QIcon iconFromApplicationTheme(QCommonStyle::StandardPixmap standardIcon,
                                   const QStyleOption *option,
                                   const QWidget *widget) const;
    QIcon iconFromResourceTheme(QCommonStyle::StandardPixmap standardIcon,
                                const QStyleOption *option,
                                const QWidget *widget) const;
    static bool inline rtl(const QStyleOption *option) {
        return (option && option->direction == Qt::RightToLeft) ||
            (!option && QGuiApplication::isRightToLeft());
    }
#if QT_CONFIG(animation)
private:
    mutable QHash<const QObject*, QStyleAnimation*> animations;
#endif // animation
};

QT_END_NAMESPACE

#endif //QCOMMONSTYLE_P_H
