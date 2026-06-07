// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLAYOUT_P_H
#define QLAYOUT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qlayout*.cpp, and qabstractlayout.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qobject_p.h"
#include "qstyle.h"
#include "qsizepolicy.h"

QT_BEGIN_NAMESPACE

class QWidgetItem;
class QSpacerItem;
class QLayoutItem;

class Q_WIDGETS_EXPORT QLayoutPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QLayout)

public:
    typedef QWidgetItem * (*QWidgetItemFactoryMethod)(const QLayout *layout, QWidget *widget);
    typedef QSpacerItem * (*QSpacerItemFactoryMethod)(const QLayout *layout, int w, int h, QSizePolicy::Policy hPolicy, QSizePolicy::Policy);

    QLayoutPrivate();

    void getMargin(int *result, int userMargin, QStyle::PixelMetric pm) const;
    void doResize();
    void reparentChildWidgets(QWidget *mw);
    bool checkWidget(QWidget *widget) const;
    bool checkLayout(QLayout *otherLayout) const;

    static QWidgetItem *createWidgetItem(const QLayout *layout, QWidget *widget);
    static QSpacerItem *createSpacerItem(const QLayout *layout, int w, int h, QSizePolicy::Policy hPolicy = QSizePolicy::Minimum, QSizePolicy::Policy vPolicy = QSizePolicy::Minimum);
    virtual QLayoutItem* replaceAt(int, QLayoutItem *) { return nullptr; }

    static QWidgetItemFactoryMethod widgetItemFactoryMethod;
    static QSpacerItemFactoryMethod spacerItemFactoryMethod;

    int insideSpacing;
    int userLeftMargin;
    int userTopMargin;
    int userRightMargin;
    int userBottomMargin;
    uint topLevel : 1;
    uint enabled : 1;
    uint activated : 1;
    uint autoNewChild : 1;
    QLayout::SizeConstraint horizontalConstraint;
    QLayout::SizeConstraint verticalConstraint;
    QRect rect;
    QWidget *menubar;
};

QT_END_NAMESPACE

#endif // QLAYOUT_P_H
