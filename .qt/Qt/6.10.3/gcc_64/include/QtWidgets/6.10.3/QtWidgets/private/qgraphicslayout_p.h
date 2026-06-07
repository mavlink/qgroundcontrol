// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGRAPHICSLAYOUT_P_H
#define QGRAPHICSLAYOUT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>

#include "qgraphicslayout.h"
#include "qgraphicslayoutitem_p.h"
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qstyleoption.h>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsLayoutItem;
class QGraphicsWidget;

#ifdef QT_DEBUG
inline bool qt_graphicsLayoutDebug()
{
    static int checked_env = -1;
    if (checked_env == -1)
        checked_env = !!qEnvironmentVariableIntValue("QT_GRAPHICSLAYOUT_DEBUG");
    return checked_env;
}
#endif


class QLayoutStyleInfo
{
public:
    inline QLayoutStyleInfo() { invalidate(); }
    inline QLayoutStyleInfo(QStyle *style, QWidget *widget)
        : m_valid(true), m_style(style), m_widget(widget)
    {
        Q_ASSERT(style);
        if (widget) //###
            m_styleOption.initFrom(widget);
        m_defaultSpacing[0] = style->pixelMetric(QStyle::PM_LayoutHorizontalSpacing, &m_styleOption, widget);
        m_defaultSpacing[1] = style->pixelMetric(QStyle::PM_LayoutVerticalSpacing, &m_styleOption, widget);
    }

    inline void invalidate() { m_valid = false; m_style = nullptr; m_widget = nullptr; }

    inline QStyle *style() const { return m_style; }
    inline QWidget *widget() const { return m_widget; }

    inline bool operator==(const QLayoutStyleInfo &other) const
        { return m_style == other.m_style && m_widget == other.m_widget; }
    inline bool operator!=(const QLayoutStyleInfo &other) const
        { return !(*this == other); }

    inline void setDefaultSpacing(Qt::Orientation o, qreal spacing){
        if (spacing >= 0)
            m_defaultSpacing[int(o) - 1] = spacing;
    }

    inline qreal defaultSpacing(Qt::Orientation o) const {
        return m_defaultSpacing[int(o) - 1];
    }

    inline qreal perItemSpacing(QSizePolicy::ControlType control1,
                                  QSizePolicy::ControlType control2,
                                  Qt::Orientation orientation) const
    {
        Q_ASSERT(style());
        return style()->layoutSpacing(control1, control2, orientation, &m_styleOption, widget());
    }
private:
    bool m_valid;
    QStyle *m_style;
    QWidget *m_widget;
    QStyleOption m_styleOption;
    qreal m_defaultSpacing[2];
};

class Q_AUTOTEST_EXPORT QGraphicsLayoutPrivate : public QGraphicsLayoutItemPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsLayout)

public:
    QGraphicsLayoutPrivate() : QGraphicsLayoutItemPrivate(nullptr, true), left(-1.0), top(-1.0), right(-1.0), bottom(-1.0),
        activated(true) { }

    void reparentChildItems(QGraphicsItem *newParent);
    void getMargin(qreal *result, qreal userMargin, QStyle::PixelMetric pm) const;
    Qt::LayoutDirection visualDirection() const;

    void addChildLayoutItem(QGraphicsLayoutItem *item);
    void activateRecursive(QGraphicsLayoutItem *item);

    qreal left, top, right, bottom;
    bool activated;
};

QT_END_NAMESPACE

#endif
