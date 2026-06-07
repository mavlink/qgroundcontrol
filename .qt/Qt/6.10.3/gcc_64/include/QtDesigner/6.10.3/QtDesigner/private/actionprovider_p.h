// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ACTIONPROVIDER_H
#define ACTIONPROVIDER_H

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

#include <QtDesigner/extension.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtWidgets/qapplication.h>

QT_BEGIN_NAMESPACE

class QAction;

class QDesignerActionProviderExtension
{
public:
    virtual ~QDesignerActionProviderExtension() = default;

    virtual QRect actionGeometry(QAction *action) const = 0;
    virtual QAction *actionAt(const QPoint &pos) const = 0;

    virtual void adjustIndicator(const QPoint &pos) = 0;
};

// Find action at the given position for a widget that has actionGeometry() (QToolBar,
// QMenuBar, QMenu). They usually have actionAt(), but that fails since Designer usually sets
// WA_TransparentForMouseEvents on the widgets.
template <class Widget>
    int actionIndexAt(const Widget *w, const QPoint &pos, Qt::Orientation orientation)
{
    const auto actions = w->actions();
    if (actions.isEmpty())
        return -1;
    // actionGeometry() can be wrong sometimes; it returns a geometry that
    // stretches to the end of the toolbar/menu bar. So, check from the beginning
    // in the case of a horizontal right-to-left orientation.
    const bool checkTopRight = orientation == Qt::Horizontal && w->layoutDirection() == Qt::RightToLeft;
    const QPoint topRight = QPoint(w->rect().width(), 0);
    for (qsizetype index = 0, actionCount = actions.size(); index < actionCount; ++index) {
        QRect g = w->actionGeometry(actions.at(index));
        if (checkTopRight)
            g.setTopRight(topRight);
        else
            g.setTopLeft(QPoint(0, 0));

        if (g.contains(pos))
            return int(index);
    }
    return -1;
}

Q_DECLARE_EXTENSION_INTERFACE(QDesignerActionProviderExtension, "org.qt-project.Qt.Designer.ActionProvider")

QT_END_NAMESPACE

#endif // ACTIONPROVIDER_H
